// mousse: CFD toolbox
// Copyright (C) 2011-2013 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "mesh_refinement.hpp"
#include "combine_faces.hpp"
#include "poly_topo_change.hpp"
#include "remove_points.hpp"
#include "face_set.hpp"
#include "time.hpp"
#include "motion_smoother.hpp"
#include "sync_tools.hpp"


// Member Functions 
mousse::label mousse::meshRefinement::mergePatchFacesUndo
(
  const scalar minCos,
  const scalar concaveCos,
  const labelList& patchIDs,
  const dictionary& motionDict,
  const labelList& preserveFaces
)
{
  // Patch face merging engine
  combineFaces faceCombiner{mesh_, true};
  // Pick up all candidate cells on boundary
  labelHashSet boundaryCells{mesh_.nFaces() - mesh_.nInternalFaces()};

  {
    const polyBoundaryMesh& patches = mesh_.boundaryMesh();
    FOR_ALL(patchIDs, i) {
      label patchI = patchIDs[i];
      const polyPatch& patch = patches[patchI];
      if (patch.coupled())
        continue;
      FOR_ALL(patch, i) {
        boundaryCells.insert(mesh_.faceOwner()[patch.start()+i]);
      }
    }
  }
  // Get all sets of faces that can be merged. Since only faces on the same
  // patch get merged there is no risk of e.g. patchID faces getting merged
  // with original patches (or even processor patches). There is a risk
  // though of original patch faces getting merged if they make a small
  // angle. Would be pretty weird starting mesh though.
  labelListList allFaceSets
  {
    faceCombiner.getMergeSets(minCos, concaveCos, boundaryCells)
  };
  // Filter out any set that contains any preserveFace
  label compactI = 0;
  FOR_ALL(allFaceSets, i) {
    const labelList& set = allFaceSets[i];
    bool keep = true;
    FOR_ALL(set, j) {
      if (preserveFaces[set[j]] != -1) {
        keep = false;
        break;
      }
    }
    if (keep) {
      if (compactI != i) {
        allFaceSets[compactI] = set;
      }
      compactI++;
    }
  }
  allFaceSets.setSize(compactI);
  label nFaceSets = returnReduce(allFaceSets.size(), sumOp<label>());
  Info << "Merging " << nFaceSets << " sets of faces." << nl << endl;
  if (nFaceSets > 0) {
    if (debug&meshRefinement::MESH) {
      faceSet allSets{mesh_, "allFaceSets", allFaceSets.size()};
      FOR_ALL(allFaceSets, setI) {
        FOR_ALL(allFaceSets[setI], i) {
          allSets.insert(allFaceSets[setI][i]);
        }
      }
      Pout << "Writing all faces to be merged to set "
        << allSets.objectPath() << endl;
      allSets.instance() = timeName();
      allSets.write();
      const_cast<Time&>(mesh_.time())++;
    }
    // Topology changes container
    polyTopoChange meshMod{mesh_};
    // Merge all faces of a set into the first face of the set.
    faceCombiner.setRefinement(allFaceSets, meshMod);
    // Experimental: store data for all the points that have been deleted
    storeData
      (
        faceCombiner.savedPointLabels(),    // points to store
        labelList(0),                       // faces to store
        labelList(0)                        // cells to store
      );
    // Change the mesh (no inflation)
    autoPtr<mapPolyMesh> map = meshMod.changeMesh(mesh_, false, true);
    // Update fields
    mesh_.updateMesh(map);
    // Move mesh (since morphing does not do this)
    if (map().hasMotionPoints()) {
      mesh_.movePoints(map().preMotionPoints());
    } else {
      // Delete mesh volumes.
      mesh_.clearOut();
    }
    // Reset the instance for if in overwrite mode
    mesh_.setInstance(timeName());
    faceCombiner.updateMesh(map);
    // Get the kept faces that need to be recalculated.
    // Merging two boundary faces might shift the cell centre
    // (unless the faces are absolutely planar)
    labelHashSet retestFaces{2*allFaceSets.size()};
    FOR_ALL(allFaceSets, setI) {
      label oldMasterI = allFaceSets[setI][0];
      retestFaces.insert(map().reverseFaceMap()[oldMasterI]);
    }
    updateMesh(map, growFaceCellFace(retestFaces));
    if (debug & meshRefinement::MESH) {
      // Check sync
      Pout << "Checking sync after initial merging " << nFaceSets
        << " faces." << endl;
      checkData();
      Pout << "Writing initial merged-faces mesh to time "
        << timeName() << nl << endl;
      write();
    }
    for (label iteration = 0; iteration < 100; iteration++) {
      Info << nl
        << "Undo iteration " << iteration << nl
        << "----------------" << endl;
      // Check mesh for errors
      // ~~~~~~~~~~~~~~~~~~~~~
      faceSet errorFaces
      {
        mesh_,
        "errorFaces",
        mesh_.nFaces() - mesh_.nInternalFaces()
      };
      bool hasErrors =
        motionSmoother::checkMesh
        (
          false,  // report
          mesh_,
          motionDict,
          errorFaces
        );
      if (!hasErrors) {
        break;
      }
      if (debug&meshRefinement::MESH) {
        errorFaces.instance() = timeName();
        Pout << "Writing all faces in error to faceSet "
          << errorFaces.objectPath() << nl << endl;
        errorFaces.write();
      }
      // Check any master cells for using any of the error faces
      DynamicList<label> mastersToRestore{allFaceSets.size()};
      FOR_ALL(allFaceSets, setI) {
        label masterFaceI = faceCombiner.masterFace()[setI];
        if (masterFaceI != -1) {
          label masterCellII = mesh_.faceOwner()[masterFaceI];
          const cell& cFaces = mesh_.cells()[masterCellII];
          FOR_ALL(cFaces, i) {
            if (errorFaces.found(cFaces[i])) {
              mastersToRestore.append(masterFaceI);
              break;
            }
          }
        }
      }
      mastersToRestore.shrink();
      label nRestore =
        returnReduce(mastersToRestore.size(), sumOp<label>());
      Info << "Masters that need to be restored:" << nRestore << endl;
      if (debug & meshRefinement::MESH) {
        faceSet restoreSet{mesh_, "mastersToRestore", mastersToRestore};
        restoreSet.instance() = timeName();
        Pout << "Writing all " << mastersToRestore.size()
          << " masterfaces to be restored to set "
          << restoreSet.objectPath() << endl;
        restoreSet.write();
      }
      if (nRestore == 0) {
        break;
      }
      // Undo
      if (debug) {
        const_cast<Time&>(mesh_.time())++;
      }
      // Topology changes container
      polyTopoChange meshMod{mesh_};
      // Merge all faces of a set into the first face of the set.
      // Experimental:mark all points/faces/cells that have been restored.
      Map<label> restoredPoints{0};
      Map<label> restoredFaces{0};
      Map<label> restoredCells{0};
      faceCombiner.setUnrefinement
        (
          mastersToRestore,
          meshMod,
          restoredPoints,
          restoredFaces,
          restoredCells
        );
      // Change the mesh (no inflation)
      autoPtr<mapPolyMesh> map = meshMod.changeMesh(mesh_, false, true);
      // Update fields
      mesh_.updateMesh(map);
      // Move mesh (since morphing does not do this)
      if (map().hasMotionPoints()) {
        mesh_.movePoints(map().preMotionPoints());
      } else {
        // Delete mesh volumes.
        mesh_.clearOut();
      }
      // Reset the instance for if in overwrite mode
      mesh_.setInstance(timeName());
      faceCombiner.updateMesh(map);
      // Renumber restore maps
      inplaceMapKey(map().reversePointMap(), restoredPoints);
      inplaceMapKey(map().reverseFaceMap(), restoredFaces);
      inplaceMapKey(map().reverseCellMap(), restoredCells);
      // Get the kept faces that need to be recalculated.
      // Merging two boundary faces might shift the cell centre
      // (unless the faces are absolutely planar)
      labelHashSet retestFaces{2*restoredFaces.size()};
      FOR_ALL_CONST_ITER(Map<label>, restoredFaces, iter) {
        retestFaces.insert(iter.key());
      }
      // Experimental:restore all points/face/cells in maps
      updateMesh
      (
        map,
        growFaceCellFace(retestFaces),
        restoredPoints,
        restoredFaces,
        restoredCells
      );
      if (debug&meshRefinement::MESH) {
        // Check sync
        Pout << "Checking sync after restoring " << retestFaces.size()
          << " faces." << endl;
        checkData();
        Pout << "Writing merged-faces mesh to time "
          << timeName() << nl << endl;
        write();
      }
      Info << endl;
    }
  } else {
    Info << "No faces merged ..." << endl;
  }
  return nFaceSets;
}


// Remove points. pointCanBeDeleted is parallel synchronised.
mousse::autoPtr<mousse::mapPolyMesh> mousse::meshRefinement::doRemovePoints
(
  removePoints& pointRemover,
  const boolList& pointCanBeDeleted
)
{
  // Topology changes container
  polyTopoChange meshMod{mesh_};
  pointRemover.setRefinement(pointCanBeDeleted, meshMod);
  // Change the mesh (no inflation)
  autoPtr<mapPolyMesh> map = meshMod.changeMesh(mesh_, false, true);
  // Update fields
  mesh_.updateMesh(map);
  // Move mesh (since morphing does not do this)
  if (map().hasMotionPoints()) {
    mesh_.movePoints(map().preMotionPoints());
  } else {
    // Delete mesh volumes.
    mesh_.clearOut();
  }
  // Reset the instance for if in overwrite mode
  mesh_.setInstance(timeName());
  pointRemover.updateMesh(map);
  // Retest all affected faces and all the cells using them
  labelHashSet retestFaces{pointRemover.savedFaceLabels().size()};
  FOR_ALL(pointRemover.savedFaceLabels(), i) {
    label faceI = pointRemover.savedFaceLabels()[i];
    if (faceI >= 0) {
      retestFaces.insert(faceI);
    }
  }
  updateMesh(map, growFaceCellFace(retestFaces));
  if (debug) {
    // Check sync
    Pout << "Checking sync after removing points." << endl;
    checkData();
  }
  return map;
}


// Restore faces (which contain removed points)
mousse::autoPtr<mousse::mapPolyMesh> mousse::meshRefinement::doRestorePoints
(
  removePoints& pointRemover,
  const labelList& facesToRestore
)
{
  // Topology changes container
  polyTopoChange meshMod{mesh_};
  // Determine sets of points and faces to restore
  labelList localFaces, localPoints;
  pointRemover.getUnrefimentSet
    (
      facesToRestore,
      localFaces,
      localPoints
    );
  // Undo the changes on the faces that are in error.
  pointRemover.setUnrefinement
    (
      localFaces,
      localPoints,
      meshMod
    );
  // Change the mesh (no inflation)
  autoPtr<mapPolyMesh> map = meshMod.changeMesh(mesh_, false, true);
  // Update fields
  mesh_.updateMesh(map);
  // Move mesh (since morphing does not do this)
  if (map().hasMotionPoints()) {
    mesh_.movePoints(map().preMotionPoints());
  } else {
    // Delete mesh volumes.
    mesh_.clearOut();
  }
  // Reset the instance for if in overwrite mode
  mesh_.setInstance(timeName());
  pointRemover.updateMesh(map);
  labelHashSet retestFaces{2*facesToRestore.size()};
  FOR_ALL(facesToRestore, i) {
    label faceI = map().reverseFaceMap()[facesToRestore[i]];
    if (faceI >= 0) {
      retestFaces.insert(faceI);
    }
  }
  updateMesh(map, growFaceCellFace(retestFaces));
  if (debug) {
    // Check sync
    Pout << "Checking sync after restoring points on "
      << facesToRestore.size() << " faces." << endl;
    checkData();
  }
  return map;
}


// Collect all faces that are both in candidateFaces and in the set.
// If coupled face also collects the coupled face.
mousse::labelList mousse::meshRefinement::collectFaces
(
  const labelList& candidateFaces,
  const labelHashSet& set
) const
{
  // Has face been selected?
  boolList selected{mesh_.nFaces(), false};
  FOR_ALL(candidateFaces, i) {
    label faceI = candidateFaces[i];
    if (set.found(faceI)) {
      selected[faceI] = true;
    }
  }
  syncTools::syncFaceList
    (
      mesh_,
      selected,
      orEqOp<bool>()      // combine operator
    );
  labelList selectedFaces{findIndices(selected, true)};
  return selectedFaces;
}


// Pick up faces of cells of faces in set.
mousse::labelList mousse::meshRefinement::growFaceCellFace
(
  const labelHashSet& set
) const
{
  boolList selected{mesh_.nFaces(), false};
  FOR_ALL_CONST_ITER(faceSet, set, iter) {
    label faceI = iter.key();
    label own = mesh_.faceOwner()[faceI];
    const cell& ownFaces = mesh_.cells()[own];
    FOR_ALL(ownFaces, ownFaceI) {
      selected[ownFaces[ownFaceI]] = true;
    }
    if (mesh_.isInternalFace(faceI)) {
      label nbr = mesh_.faceNeighbour()[faceI];
      const cell& nbrFaces = mesh_.cells()[nbr];
      FOR_ALL(nbrFaces, nbrFaceI) {
        selected[nbrFaces[nbrFaceI]] = true;
      }
    }
  }
  syncTools::syncFaceList
    (
      mesh_,
      selected,
      orEqOp<bool>()      // combine operator
    );
  return findIndices(selected, true);
}


// Remove points not used by any face or points used by only two faces where
// the edges are in line
mousse::label mousse::meshRefinement::mergeEdgesUndo
(
  const scalar minCos,
  const dictionary& motionDict
)
{
  Info << nl
    << "Merging all points on surface that" << nl
    << "- are used by only two boundary faces and" << nl
    << "- make an angle with a cosine of more than " << minCos
    << "." << nl << endl;
  // Point removal analysis engine with undo
  removePoints pointRemover{mesh_, true};
  // Count usage of points
  boolList pointCanBeDeleted;
  label nRemove = pointRemover.countPointUsage(minCos, pointCanBeDeleted);
  if (nRemove > 0) {
    Info << "Removing " << nRemove
      << " straight edge points ..." << nl << endl;
    // Remove points
    // ~~~~~~~~~~~~~
    doRemovePoints(pointRemover, pointCanBeDeleted);
    for (label iteration = 0; iteration < 100; iteration++) {
      Info << nl
        << "Undo iteration " << iteration << nl
        << "----------------" << endl;
      // Check mesh for errors
      // ~~~~~~~~~~~~~~~~~~~~~
      faceSet errorFaces
      {
        mesh_,
        "errorFaces",
        mesh_.nFaces()-mesh_.nInternalFaces()
      };
      bool hasErrors =
        motionSmoother::checkMesh
        (
          false,  // report
          mesh_,
          motionDict,
          errorFaces
        );
      if (!hasErrors) {
        break;
      }
      if (debug&meshRefinement::MESH) {
        errorFaces.instance() = timeName();
        Pout << "**Writing all faces in error to faceSet "
          << errorFaces.objectPath() << nl << endl;
        errorFaces.write();
      }
      labelList masterErrorFaces
      {
        collectFaces
        (
          pointRemover.savedFaceLabels(),
          errorFaces
        )
      };
      label n = returnReduce(masterErrorFaces.size(), sumOp<label>());
      Info << "Detected " << n
        << " error faces on boundaries that have been merged."
        << " These will be restored to their original faces." << nl
        << endl;
      if (n == 0) {
        if (hasErrors) {
          Info << "Detected "
            << returnReduce(errorFaces.size(), sumOp<label>())
            << " error faces in mesh."
            << " Restoring neighbours of faces in error." << nl
            << endl;
          labelList expandedErrorFaces
          {
            growFaceCellFace(errorFaces)
          };
          doRestorePoints(pointRemover, expandedErrorFaces);
        }
        break;
      }
      doRestorePoints(pointRemover, masterErrorFaces);
    }
    if (debug&meshRefinement::MESH) {
      const_cast<Time&>(mesh_.time())++;
      Pout << "Writing merged-edges mesh to time " << timeName() << nl << endl;
      write();
    }
  } else {
    Info << "No straight edges simplified and no points removed ..." << endl;
  }
  return nRemove;
}

