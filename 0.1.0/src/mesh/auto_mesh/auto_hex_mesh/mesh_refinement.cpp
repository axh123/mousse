// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "mesh_refinement.hpp"
#include "vol_mesh.hpp"
#include "vol_fields.hpp"
#include "surface_mesh.hpp"
#include "sync_tools.hpp"
#include "time.hpp"
#include "refinement_history.hpp"
#include "refinement_surfaces.hpp"
#include "refinement_features.hpp"
#include "decomposition_method.hpp"
#include "region_split.hpp"
#include "fv_mesh_distribute.hpp"
#include "indirect_primitive_patch.hpp"
#include "poly_topo_change.hpp"
#include "remove_cells.hpp"
#include "map_distribute_poly_mesh.hpp"
#include "local_point_region.hpp"
#include "point_mesh.hpp"
#include "point_fields.hpp"
#include "slip_point_patch_fields.hpp"
#include "fixed_value_point_patch_fields.hpp"
#include "calculated_point_patch_fields.hpp"
#include "cyclic_slip_point_patch_fields.hpp"
#include "processor_point_patch.hpp"
#include "global_index.hpp"
#include "mesh_tools.hpp"
#include "ofstream.hpp"
#include "geom_decomp.hpp"
#include "random.hpp"
#include "searchable_surfaces.hpp"
#include "tree_bound_box.hpp"
#include "zero_gradient_fv_patch_fields.hpp"
#include "fv_mesh_tools.hpp"


// Static Data Members
namespace mousse {

DEFINE_TYPE_NAME_AND_DEBUG(meshRefinement, 0);
template<>
const char* mousse::NamedEnum
<
  mousse::meshRefinement::IOdebugType,
  5
>::names[] =
{
  "mesh",
  //"scalarLevels",
  "intersections",
  "featureSeeds",
  "attraction",
  "layerInfo"
};
template<>
const char* mousse::NamedEnum
<
  mousse::meshRefinement::IOoutputType,
  1
>::names[] =
{
  "layerInfo"
};
template<>
const char* mousse::NamedEnum
<
  mousse::meshRefinement::IOwriteType,
  4
>::names[] =
{
  "mesh",
  "scalarLevels",
  "layerSets",
  "layerFields"
};

}


const mousse::NamedEnum<mousse::meshRefinement::IOdebugType, 5>
mousse::meshRefinement::IOdebugTypeNames;

const mousse::NamedEnum<mousse::meshRefinement::IOoutputType, 1>
mousse::meshRefinement::IOoutputTypeNames;

const mousse::NamedEnum<mousse::meshRefinement::IOwriteType, 4>
mousse::meshRefinement::IOwriteTypeNames;

mousse::meshRefinement::writeType mousse::meshRefinement::writeLevel_;

mousse::meshRefinement::outputType mousse::meshRefinement::outputLevel_;


// Private Member Functions 
void mousse::meshRefinement::calcNeighbourData
(
  labelList& neiLevel,
  pointField& neiCc
)  const
{
  const labelList& cellLevel = meshCutter_.cellLevel();
  const pointField& cellCentres = mesh_.cellCentres();
  label nBoundaryFaces = mesh_.nFaces() - mesh_.nInternalFaces();
  if (neiLevel.size() != nBoundaryFaces || neiCc.size() != nBoundaryFaces) {
    FATAL_ERROR_IN("meshRefinement::calcNeighbour(..)") << "nBoundaries:"
      << nBoundaryFaces << " neiLevel:" << neiLevel.size()
      << abort(FatalError);
  }
  const polyBoundaryMesh& patches = mesh_.boundaryMesh();
  labelHashSet addedPatchIDSet{meshedPatches()};
  FOR_ALL(patches, patchI) {
    const polyPatch& pp = patches[patchI];
    const labelUList& faceCells = pp.faceCells();
    const vectorField::subField faceCentres = pp.faceCentres();
    const vectorField::subField faceAreas = pp.faceAreas();
    label bFaceI = pp.start() - mesh_.nInternalFaces();
    if (pp.coupled()) {
      FOR_ALL(faceCells, i) {
        neiLevel[bFaceI] = cellLevel[faceCells[i]];
        neiCc[bFaceI] = cellCentres[faceCells[i]];
        bFaceI++;
      }
    } else if (addedPatchIDSet.found(patchI)) {
      // Face was introduced from cell-cell intersection. Try to
      // reconstruct other side cell(centre). Three possibilities:
      // - cells same size.
      // - preserved cell smaller. Not handled.
      // - preserved cell larger.
      FOR_ALL(faceCells, i) {
        // Extrapolate the face centre.
        vector fn = faceAreas[i];
        fn /= mag(fn) + VSMALL;
        label own = faceCells[i];
        label ownLevel = cellLevel[own];
        label faceLevel = meshCutter_.getAnchorLevel(pp.start()+i);
        // Normal distance from face centre to cell centre
        scalar d = ((faceCentres[i] - cellCentres[own]) & fn);
        if (faceLevel > ownLevel) {
          // Other cell more refined. Adjust normal distance
          d *= 0.5;
        }
        neiLevel[bFaceI] = faceLevel;
        // Calculate other cell centre by extrapolation
        neiCc[bFaceI] = faceCentres[i] + d*fn;
        bFaceI++;
      }
    } else {
      FOR_ALL(faceCells, i) {
        neiLevel[bFaceI] = cellLevel[faceCells[i]];
        neiCc[bFaceI] = faceCentres[i];
        bFaceI++;
      }
    }
  }
  // Swap coupled boundaries. Apply separation to cc since is coordinate.
  syncTools::swapBoundaryFacePositions(mesh_, neiCc);
  syncTools::swapBoundaryFaceList(mesh_, neiLevel);
}


// Find intersections of edges (given by their two endpoints) with surfaces.
// Returns first intersection if there are more than one.
void mousse::meshRefinement::updateIntersections(const labelList& changedFaces)
{
  const pointField& cellCentres = mesh_.cellCentres();
  // Stats on edges to test. Count proc faces only once.
  PackedBoolList isMasterFace{syncTools::getMasterFaces(mesh_)};

  {
    label nMasterFaces = 0;
    FOR_ALL(isMasterFace, faceI) {
      if (isMasterFace.get(faceI) == 1) {
        nMasterFaces++;
      }
    }
    reduce(nMasterFaces, sumOp<label>());
    label nChangedFaces = 0;
    FOR_ALL(changedFaces, i) {
      if (isMasterFace.get(changedFaces[i]) == 1) {
        nChangedFaces++;
      }
    }
    reduce(nChangedFaces, sumOp<label>());
    Info << "Edge intersection testing:" << nl
      << "    Number of edges             : " << nMasterFaces << nl
      << "    Number of edges to retest   : " << nChangedFaces
      << endl;
  }
  // Get boundary face centre and level. Coupled aware.
  labelList neiLevel{mesh_.nFaces() - mesh_.nInternalFaces()};
  pointField neiCc{mesh_.nFaces() - mesh_.nInternalFaces()};
  calcNeighbourData(neiLevel, neiCc);
  // Collect segments we want to test for
  pointField start{changedFaces.size()};
  pointField end{changedFaces.size()};
  FOR_ALL(changedFaces, i) {
    label faceI = changedFaces[i];
    label own = mesh_.faceOwner()[faceI];
    start[i] = cellCentres[own];
    if (mesh_.isInternalFace(faceI)) {
      end[i] = cellCentres[mesh_.faceNeighbour()[faceI]];
    } else {
      end[i] = neiCc[faceI-mesh_.nInternalFaces()];
    }
  }
  // Extend segments a bit
  {
    const vectorField smallVec{ROOTSMALL*(end-start)};
    start -= smallVec;
    end += smallVec;
  }
  // Do tests in one go
  labelList surfaceHit;

  {
    labelList surfaceLevel;
    surfaces_.findHigherIntersection
      (
        start,
        end,
        labelList(start.size(), -1),    // accept any intersection
        surfaceHit,
        surfaceLevel
      );
  }
  // Keep just surface hit
  FOR_ALL(surfaceHit, i) {
    surfaceIndex_[changedFaces[i]] = surfaceHit[i];
  }
  // Make sure both sides have same information. This should be
  // case in general since same vectors but just to make sure.
  syncTools::syncFaceList(mesh_, surfaceIndex_, maxEqOp<label>());
  label nHits = countHits();
  label nTotHits = returnReduce(nHits, sumOp<label>());
  Info << "    Number of intersected edges : " << nTotHits << endl;
  // Set files to same time as mesh
  setInstance(mesh_.facesInstance());
}


void mousse::meshRefinement::testSyncPointList
(
  const string& msg,
  const polyMesh& mesh,
  const List<scalar>& fld
)
{
  if (fld.size() != mesh.nPoints()) {
    FATAL_ERROR_IN
    (
      "meshRefinement::testSyncPointList(const polyMesh&"
      ", const List<scalar>&)"
    )
    << msg << endl
    << "fld size:" << fld.size() << " mesh points:" << mesh.nPoints()
    << abort(FatalError);
  }
  Pout << "Checking field " << msg << endl;
  scalarField minFld{fld};
  syncTools::syncPointList
    (
      mesh,
      minFld,
      minEqOp<scalar>(),
      GREAT
    );
  scalarField maxFld{fld};
  syncTools::syncPointList
    (
      mesh,
      maxFld,
      maxEqOp<scalar>(),
      -GREAT
    );
  FOR_ALL(minFld, pointI) {
    const scalar& minVal = minFld[pointI];
    const scalar& maxVal = maxFld[pointI];
    if (mag(minVal-maxVal) > SMALL) {
      Pout << msg << " at:" << mesh.points()[pointI] << nl
        << "    minFld:" << minVal << nl
        << "    maxFld:" << maxVal << nl
        << endl;
    }
  }
}


void mousse::meshRefinement::testSyncPointList
(
  const string& msg,
  const polyMesh& mesh,
  const List<point>& fld
)
{
  if (fld.size() != mesh.nPoints()) {
    FATAL_ERROR_IN
    (
      "meshRefinement::testSyncPointList(const polyMesh&"
      ", const List<point>&)"
    )
    << msg << endl
    << "fld size:" << fld.size() << " mesh points:" << mesh.nPoints()
    << abort(FatalError);
  }
  Pout << "Checking field " << msg << endl;
  pointField minFld{fld};
  syncTools::syncPointList
    (
      mesh,
      minFld,
      minMagSqrEqOp<point>(),
      point(GREAT, GREAT, GREAT)
    );
  pointField maxFld{fld};
  syncTools::syncPointList
    (
      mesh,
      maxFld,
      maxMagSqrEqOp<point>(),
      vector::zero
    );
  FOR_ALL(minFld, pointI) {
    const point& minVal = minFld[pointI];
    const point& maxVal = maxFld[pointI];
    if (mag(minVal-maxVal) > SMALL) {
      Pout << msg << " at:" << mesh.points()[pointI] << nl
        << "    minFld:" << minVal << nl
        << "    maxFld:" << maxVal << nl
        << endl;
    }
  }
}


void mousse::meshRefinement::checkData()
{
  Pout << "meshRefinement::checkData() : Checking refinement structure."
    << endl;
  meshCutter_.checkMesh();
  Pout << "meshRefinement::checkData() : Checking refinement levels."
    << endl;
  meshCutter_.checkRefinementLevels(1, labelList(0));
  label nBnd = mesh_.nFaces() - mesh_.nInternalFaces();
  Pout << "meshRefinement::checkData() : Checking synchronization."
    << endl;
  // Check face centres
  {
    // Boundary face centres
    pointField::subList boundaryFc
    {
      mesh_.faceCentres(),
      nBnd,
      mesh_.nInternalFaces()
    };
    // Get neighbouring face centres
    pointField neiBoundaryFc{boundaryFc};
    syncTools::syncBoundaryFacePositions
      (
        mesh_,
        neiBoundaryFc,
        eqOp<point>()
      );
    // Compare
    testSyncBoundaryFaceList
      (
        mergeDistance_,
        "testing faceCentres : ",
        boundaryFc,
        neiBoundaryFc
      );
  }
  // Check meshRefinement
  {
    // Get boundary face centre and level. Coupled aware.
    labelList neiLevel{nBnd};
    pointField neiCc{nBnd};
    calcNeighbourData(neiLevel, neiCc);
    // Collect segments we want to test for
    pointField start{mesh_.nFaces()};
    pointField end{mesh_.nFaces()};
    FOR_ALL(start, faceI) {
      start[faceI] = mesh_.cellCentres()[mesh_.faceOwner()[faceI]];
      if (mesh_.isInternalFace(faceI)) {
        end[faceI] = mesh_.cellCentres()[mesh_.faceNeighbour()[faceI]];
      } else {
        end[faceI] = neiCc[faceI - mesh_.nInternalFaces()];
      }
    }
    // Extend segments a bit
    {
      const vectorField smallVec{ROOTSMALL*(end-start)};
      start -= smallVec;
      end += smallVec;
    }
    // Do tests in one go
    labelList surfaceHit;

    {
      labelList surfaceLevel;
      surfaces_.findHigherIntersection
        (
          start,
          end,
          labelList(start.size(), -1),    // accept any intersection
          surfaceHit,
          surfaceLevel
        );
    }
    // Get the coupled hit
    labelList neiHit
    {
      SubList<label>
      {
        surfaceHit,
        nBnd,
        mesh_.nInternalFaces()
      }
    };
    syncTools::swapBoundaryFaceList(mesh_, neiHit);
    // Check
    FOR_ALL(surfaceHit, faceI) {
      if (surfaceIndex_[faceI] != surfaceHit[faceI]) {
        if (mesh_.isInternalFace(faceI)) {
          WARNING_IN("meshRefinement::checkData()")
            << "Internal face:" << faceI
            << " fc:" << mesh_.faceCentres()[faceI]
            << " cached surfaceIndex_:" << surfaceIndex_[faceI]
            << " current:" << surfaceHit[faceI]
            << " ownCc:"
            << mesh_.cellCentres()[mesh_.faceOwner()[faceI]]
            << " neiCc:"
            << mesh_.cellCentres()[mesh_.faceNeighbour()[faceI]]
            << endl;
        } else if (surfaceIndex_[faceI] != neiHit[faceI-mesh_.nInternalFaces()]) {
          WARNING_IN("meshRefinement::checkData()")
            << "Boundary face:" << faceI
            << " fc:" << mesh_.faceCentres()[faceI]
            << " cached surfaceIndex_:" << surfaceIndex_[faceI]
            << " current:" << surfaceHit[faceI]
            << " ownCc:"
            << mesh_.cellCentres()[mesh_.faceOwner()[faceI]]
            << " end:" << end[faceI]
            << endl;
        }
      }
    }
  }

  {
    labelList::subList boundarySurface
    {
      surfaceIndex_,
      mesh_.nFaces()-mesh_.nInternalFaces(),
      mesh_.nInternalFaces()
    };
    labelList neiBoundarySurface{boundarySurface};
    syncTools::swapBoundaryFaceList
      (
        mesh_,
        neiBoundarySurface
      );
    // Compare
    testSyncBoundaryFaceList
      (
        0,                              // tolerance
        "testing surfaceIndex() : ",
        boundarySurface,
        neiBoundarySurface
      );
  }
  // Find duplicate faces
  Pout << "meshRefinement::checkData() : Counting duplicate faces."
    << endl;
  labelList duplicateFace
  {
    localPointRegion::findDuplicateFaces
    (
      mesh_,
      identity(mesh_.nFaces() - mesh_.nInternalFaces()) + mesh_.nInternalFaces()
    )
  };
  // Count
  {
    label nDup = 0;
    FOR_ALL(duplicateFace, i) {
      if (duplicateFace[i] != -1) {
        nDup++;
      }
    }
    nDup /= 2;  // will have counted both faces of duplicate
    Pout << "meshRefinement::checkData() : Found " << nDup
      << " duplicate pairs of faces." << endl;
  }
}


void mousse::meshRefinement::setInstance(const fileName& inst)
{
  meshCutter_.setInstance(inst);
  surfaceIndex_.instance() = inst;
}


// Remove cells. Put exposedFaces (output of getExposedFaces(cellsToRemove))
// into exposedPatchIDs.
mousse::autoPtr<mousse::mapPolyMesh> mousse::meshRefinement::doRemoveCells
(
  const labelList& cellsToRemove,
  const labelList& exposedFaces,
  const labelList& exposedPatchIDs,
  removeCells& cellRemover
)
{
  polyTopoChange meshMod{mesh_};
  // Arbitrary: put exposed faces into last patch.
  cellRemover.setRefinement
    (
      cellsToRemove,
      exposedFaces,
      exposedPatchIDs,
      meshMod
    );
  // Change the mesh (no inflation)
  autoPtr<mapPolyMesh> map = meshMod.changeMesh(mesh_, false, true);
  // Update fields
  mesh_.updateMesh(map);
  // Move mesh (since morphing might not do this)
  if (map().hasMotionPoints()) {
    mesh_.movePoints(map().preMotionPoints());
  } else {
    // Delete mesh volumes. No other way to do this?
    mesh_.clearOut();
  }
  // Reset the instance for if in overwrite mode
  mesh_.setInstance(timeName());
  setInstance(mesh_.facesInstance());
  // Update local mesh data
  cellRemover.updateMesh(map);
  // Update intersections. Recalculate intersections for exposed faces.
  labelList newExposedFaces =
    renumber
    (
      map().reverseFaceMap(),
      exposedFaces
    );
  updateMesh(map, newExposedFaces);
  return map;
}


// Split faces
mousse::autoPtr<mousse::mapPolyMesh> mousse::meshRefinement::splitFaces
(
  const labelList& splitFaces,
  const labelPairList& splits
)
{
  polyTopoChange meshMod{mesh_};
  FOR_ALL(splitFaces, i) {
    label faceI = splitFaces[i];
    const face& f = mesh_.faces()[faceI];
    // Split as start and end index in face
    const labelPair& split = splits[i];
    label nVerts = split[1] - split[0];
    if (nVerts < 0) {
      nVerts += f.size();
    }
    nVerts += 1;
    // Split into f0, f1
    face f0{nVerts};
    label fp = split[0];
    FOR_ALL(f0, i) {
      f0[i] = f[fp];
      fp = f.fcIndex(fp);
    }
    face f1{f.size()-f0.size()+2};
    fp = split[1];
    FOR_ALL(f1, i) {
      f1[i] = f[fp];
      fp = f.fcIndex(fp);
    }
    // Determine face properties
    label own = mesh_.faceOwner()[faceI];
    label nei = -1;
    label patchI = -1;
    if (faceI >= mesh_.nInternalFaces()) {
      patchI = mesh_.boundaryMesh().whichPatch(faceI);
    } else {
      nei = mesh_.faceNeighbour()[faceI];
    }
    label zoneI = mesh_.faceZones().whichZone(faceI);
    bool zoneFlip = false;
    if (zoneI != -1) {
      const faceZone& fz = mesh_.faceZones()[zoneI];
      zoneFlip = fz.flipMap()[fz.whichFace(faceI)];
    }
    Pout << "face:" << faceI << " verts:" << f
      << " split into f0:" << f0
      << " f1:" << f1 << endl;
    // Change/add faces
    meshMod.modifyFace
      (
        f0,                         // modified face
        faceI,                      // label of face
        own,                        // owner
        nei,                        // neighbour
        false,                      // face flip
        patchI,                     // patch for face
        zoneI,                      // zone for face
        zoneFlip                    // face flip in zone
      );
    meshMod.addFace
      (
        f1,                         // modified face
        own,                        // owner
        nei,                        // neighbour
        -1,                         // master point
        -1,                         // master edge
        faceI,                      // master face
        false,                      // face flip
        patchI,                     // patch for face
        zoneI,                      // zone for face
        zoneFlip                    // face flip in zone
      );
  }
  // Change the mesh (no inflation)
  autoPtr<mapPolyMesh> map = meshMod.changeMesh(mesh_, false, true);
  // Update fields
  mesh_.updateMesh(map);
  // Move mesh (since morphing might not do this)
  if (map().hasMotionPoints()) {
    mesh_.movePoints(map().preMotionPoints());
  } else {
    // Delete mesh volumes. No other way to do this?
    mesh_.clearOut();
  }
  // Reset the instance for if in overwrite mode
  mesh_.setInstance(timeName());
  setInstance(mesh_.facesInstance());
  // Update local mesh data
  const labelList& oldToNew = map().reverseFaceMap();
  labelList newSplitFaces{renumber(oldToNew, splitFaces)};
  // Add added faces (every splitFaces becomes two faces)
  label sz = newSplitFaces.size();
  newSplitFaces.setSize(2*sz);
  FOR_ALL(map().faceMap(), faceI) {
    label oldFaceI = map().faceMap()[faceI];
    if (oldToNew[oldFaceI] != faceI) {
      // Added face
      newSplitFaces[sz++] = faceI;
    }
  }
  updateMesh(map, newSplitFaces);
  return map;
}


// Constructors 

// Construct from components
mousse::meshRefinement::meshRefinement
(
  fvMesh& mesh,
  const scalar mergeDistance,
  const bool overwrite,
  const refinementSurfaces& surfaces,
  const refinementFeatures& features,
  const shellSurfaces& shells
)
:
  mesh_{mesh},
  mergeDistance_{mergeDistance},
  overwrite_{overwrite},
  oldInstance_{mesh.pointsInstance()},
  surfaces_{surfaces},
  features_{features},
  shells_{shells},
  meshCutter_
  {
    mesh,
    false   // do not try to read history.
  },
  surfaceIndex_
  {
    {
      "surfaceIndex",
      mesh_.facesInstance(),
      fvMesh::meshSubDir,
      mesh_,
      IOobject::NO_READ,
      IOobject::NO_WRITE,
      false
    },
    labelList{mesh_.nFaces(), -1}
  },
  userFaceData_{0}
{
  // recalculate intersections for all faces
  updateIntersections(identity(mesh_.nFaces()));
}


// Member Functions 
mousse::label mousse::meshRefinement::countHits() const
{
  // Stats on edges to test. Count proc faces only once.
  PackedBoolList isMasterFace{syncTools::getMasterFaces(mesh_)};
  label nHits = 0;
  FOR_ALL(surfaceIndex_, faceI) {
    if (surfaceIndex_[faceI] >= 0 && isMasterFace.get(faceI) == 1) {
      nHits++;
    }
  }
  return nHits;
}


mousse::autoPtr<mousse::mapDistributePolyMesh> mousse::meshRefinement::balance
(
  const bool keepZoneFaces,
  const bool keepBaffles,
  const scalarField& cellWeights,
  decompositionMethod& decomposer,
  fvMeshDistribute& distributor
)
{
  autoPtr<mapDistributePolyMesh> map;
  if (Pstream::parRun()) {
    // Wanted distribution
    labelList distribution;
    // Faces where owner and neighbour are not 'connected' so can
    // go to different processors.
    boolList blockedFace;
    label nUnblocked = 0;
    // Faces that move as block onto single processor
    PtrList<labelList> specifiedProcessorFaces;
    labelList specifiedProcessor;
    // Pairs of baffles
    List<labelPair> couples;
    // Constraints from decomposeParDict
    decomposer.setConstraints
      (
        mesh_,
        blockedFace,
        specifiedProcessorFaces,
        specifiedProcessor,
        couples
      );
    if (keepZoneFaces || keepBaffles) {
      if (keepZoneFaces) {
        // Determine decomposition to keep/move surface zones
        // on one processor. The reason is that snapping will make these
        // into baffles, move and convert them back so if they were
        // proc boundaries after baffling&moving the points might be no
        // longer synchronised so recoupling will fail. To prevent this
        // keep owner&neighbour of such a surface zone on the same
        // processor.
        const PtrList<surfaceZonesInfo>& surfZones = surfaces().surfZones();
        const faceZoneMesh& fZones = mesh_.faceZones();
        const polyBoundaryMesh& pbm = mesh_.boundaryMesh();
        // Get faces whose owner and neighbour should stay together,
        // i.e. they are not 'blocked'.
        FOR_ALL(surfZones, surfI) {
          const word& fzName = surfZones[surfI].faceZoneName();
          if (fzName.size()) {
            // Get zone
            const faceZone& fZone = fZones[fzName];
            FOR_ALL(fZone, i) {
              label faceI = fZone[i];
              if (blockedFace[faceI]) {
                if (mesh_.isInternalFace(faceI)
                    || pbm[pbm.whichPatch(faceI)].coupled()) {
                  blockedFace[faceI] = false;
                  nUnblocked++;
                }
              }
            }
          }
        }
        // If the faceZones are not synchronised the blockedFace
        // might not be synchronised. If you are sure the faceZones
        // are synchronised remove below check.
        syncTools::syncFaceList
          (
            mesh_,
            blockedFace,
            andEqOp<bool>()     // combine operator
          );
      }
      reduce(nUnblocked, sumOp<label>());
      if (keepZoneFaces) {
        Info << "Found " << nUnblocked
          << " zoned faces to keep together." << endl;
      }
      // Extend un-blockedFaces with any cyclics
      {
        boolList separatedCoupledFace{mesh_.nFaces(), false};
        selectSeparatedCoupledFaces(separatedCoupledFace);
        label nSeparated = 0;
        FOR_ALL(separatedCoupledFace, faceI) {
          if (separatedCoupledFace[faceI]) {
            if (blockedFace[faceI]) {
              blockedFace[faceI] = false;
              nSeparated++;
            }
          }
        }
        reduce(nSeparated, sumOp<label>());
        Info << "Found " << nSeparated
          << " separated coupled faces to keep together." << endl;
        nUnblocked += nSeparated;
      }
      if (keepBaffles) {
        label nBnd = mesh_.nFaces()-mesh_.nInternalFaces();
        labelList coupledFace{mesh_.nFaces(), -1};

        {
          // Get boundary baffles that need to stay together
          List<labelPair> allCouples
          {
            localPointRegion::findDuplicateFacePairs(mesh_)
          };
          // Merge with any couples from
          // decompositionMethod::setConstraints
          FOR_ALL(couples, i) {
            const labelPair& baffle = couples[i];
            coupledFace[baffle.first()] = baffle.second();
            coupledFace[baffle.second()] = baffle.first();
          }
          FOR_ALL(allCouples, i) {
            const labelPair& baffle = allCouples[i];
            coupledFace[baffle.first()] = baffle.second();
            coupledFace[baffle.second()] = baffle.first();
          }
        }
        couples.setSize(nBnd);
        label nCpl = 0;
        FOR_ALL(coupledFace, faceI) {
          if (coupledFace[faceI] != -1 && faceI < coupledFace[faceI]) {
            couples[nCpl++] = labelPair(faceI, coupledFace[faceI]);
          }
        }
        couples.setSize(nCpl);
      }
      label nCouples = returnReduce(couples.size(), sumOp<label>());
      if (keepBaffles) {
        Info << "Found " << nCouples << " baffles to keep together."
          << endl;
      }
    }
    // Make sure blockedFace not set on couples
    FOR_ALL(couples, i) {
      const labelPair& baffle = couples[i];
      blockedFace[baffle.first()] = false;
      blockedFace[baffle.second()] = false;
    }
    distribution =
      decomposer.decompose
      (
        mesh_,
        cellWeights,
        blockedFace,
        specifiedProcessorFaces,
        specifiedProcessor,
        couples                 // explicit connections
      );
    if (debug) {
      labelList nProcCells{distributor.countCells(distribution)};
      Pout << "Wanted distribution:" << nProcCells << endl;
      Pstream::listCombineGather(nProcCells, plusEqOp<label>());
      Pstream::listCombineScatter(nProcCells);
      Pout << "Wanted resulting decomposition:" << endl;
      FOR_ALL(nProcCells, procI) {
        Pout << "    " << procI << '\t' << nProcCells[procI] << endl;
      }
      Pout<< endl;
    }
    // Do actual sending/receiving of mesh
    map = distributor.distribute(distribution);
    // Update numbering of meshRefiner
    distribute(map);
    // Set correct instance (for if overwrite)
    mesh_.setInstance(timeName());
    setInstance(mesh_.facesInstance());
  }
  return map;
}


// Helper function to get intersected faces
mousse::labelList mousse::meshRefinement::intersectedFaces() const
{
  label nBoundaryFaces = 0;
  FOR_ALL(surfaceIndex_, faceI) {
    if (surfaceIndex_[faceI] != -1) {
      nBoundaryFaces++;
    }
  }
  labelList surfaceFaces{nBoundaryFaces};
  nBoundaryFaces = 0;
  FOR_ALL(surfaceIndex_, faceI) {
    if (surfaceIndex_[faceI] != -1) {
      surfaceFaces[nBoundaryFaces++] = faceI;
    }
  }
  return surfaceFaces;
}


// Helper function to get points used by faces
mousse::labelList mousse::meshRefinement::intersectedPoints() const
{
  const faceList& faces = mesh_.faces();
  // Mark all points on faces that will become baffles
  PackedBoolList isBoundaryPoint{mesh_.nPoints()};
  label nBoundaryPoints = 0;
  FOR_ALL(surfaceIndex_, faceI) {
    if (surfaceIndex_[faceI] != -1) {
      const face& f = faces[faceI];
      FOR_ALL(f, fp) {
        if (isBoundaryPoint.set(f[fp], 1u)) {
          nBoundaryPoints++;
        }
      }
    }
  }
  // Pack
  labelList boundaryPoints{nBoundaryPoints};
  nBoundaryPoints = 0;
  FOR_ALL(isBoundaryPoint, pointI) {
    if (isBoundaryPoint.get(pointI) == 1u) {
      boundaryPoints[nBoundaryPoints++] = pointI;
    }
  }
  return boundaryPoints;
}


//- Create patch from set of patches
mousse::autoPtr<mousse::indirectPrimitivePatch> mousse::meshRefinement::makePatch
(
  const polyMesh& mesh,
  const labelList& patchIDs
)
{
  const polyBoundaryMesh& patches = mesh.boundaryMesh();
  // Count faces.
  label nFaces = 0;
  FOR_ALL(patchIDs, i) {
    const polyPatch& pp = patches[patchIDs[i]];
    nFaces += pp.size();
  }
  // Collect faces.
  labelList addressing{nFaces};
  nFaces = 0;
  FOR_ALL(patchIDs, i) {
    const polyPatch& pp = patches[patchIDs[i]];
    label meshFaceI = pp.start();
    FOR_ALL(pp, i) {
      addressing[nFaces++] = meshFaceI++;
    }
  }
  return
    autoPtr<indirectPrimitivePatch>
    {
      new indirectPrimitivePatch
      {
        IndirectList<face>{mesh.faces(), addressing},
        mesh.points()
      }
    };
}


// Construct pointVectorField with correct boundary conditions
mousse::tmp<mousse::pointVectorField> mousse::meshRefinement::makeDisplacementField
(
  const pointMesh& pMesh,
  const labelList& adaptPatchIDs
)
{
  const polyMesh& mesh = pMesh();
  // Construct displacement field.
  const pointBoundaryMesh& pointPatches = pMesh.boundary();
  wordList patchFieldTypes
  {
    pointPatches.size(),
    slipPointPatchVectorField::typeName
  };
  FOR_ALL(adaptPatchIDs, i) {
    patchFieldTypes[adaptPatchIDs[i]] =
      fixedValuePointPatchVectorField::typeName;
  }
  FOR_ALL(pointPatches, patchI) {
    if (isA<processorPointPatch>(pointPatches[patchI])) {
      patchFieldTypes[patchI] = calculatedPointPatchVectorField::typeName;
    } else if (isA<cyclicPointPatch>(pointPatches[patchI])) {
      patchFieldTypes[patchI] = cyclicSlipPointPatchVectorField::typeName;
    }
  }
  // Note: time().timeName() instead of meshRefinement::timeName() since
  // postprocessable field.
  tmp<pointVectorField> tfld
  {
    new pointVectorField
    {
      {
        "pointDisplacement",
        mesh.time().timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::AUTO_WRITE
      },
      pMesh,
      {"displacement", dimLength, vector::zero},
      patchFieldTypes
    }
  };
  return tfld;
}


void mousse::meshRefinement::checkCoupledFaceZones(const polyMesh& mesh)
{
  const faceZoneMesh& fZones = mesh.faceZones();
  // Check any zones are present anywhere and in same order
  {
    List<wordList> zoneNames{Pstream::nProcs()};
    zoneNames[Pstream::myProcNo()] = fZones.names();
    Pstream::gatherList(zoneNames);
    Pstream::scatterList(zoneNames);
    // All have same data now. Check.
    FOR_ALL(zoneNames, procI) {
      if (procI != Pstream::myProcNo()) {
        if (zoneNames[procI] != zoneNames[Pstream::myProcNo()]) {
          FATAL_ERROR_IN
          (
            "meshRefinement::checkCoupledFaceZones(const polyMesh&)"
          )
          << "faceZones are not synchronised on processors." << nl
          << "Processor " << procI << " has faceZones "
          << zoneNames[procI] << nl
          << "Processor " << Pstream::myProcNo()
          << " has faceZones "
          << zoneNames[Pstream::myProcNo()] << nl
          << exit(FatalError);
        }
      }
    }
  }
  // Check that coupled faces are present on both sides.
  labelList faceToZone{mesh.nFaces()-mesh.nInternalFaces(), -1};
  FOR_ALL(fZones, zoneI) {
    const faceZone& fZone = fZones[zoneI];
    FOR_ALL(fZone, i) {
      label bFaceI = fZone[i]-mesh.nInternalFaces();
      if (bFaceI >= 0) {
        if (faceToZone[bFaceI] == -1) {
          faceToZone[bFaceI] = zoneI;
        } else if (faceToZone[bFaceI] == zoneI) {
          FATAL_ERROR_IN
          (
            "meshRefinement::checkCoupledFaceZones(const polyMesh&)"
          )
          << "Face " << fZone[i] << " in zone "
          << fZone.name()
          << " is twice in zone!"
          << abort(FatalError);
        } else {
          FATAL_ERROR_IN
          (
            "meshRefinement::checkCoupledFaceZones(const polyMesh&)"
          )
          << "Face " << fZone[i] << " in zone "
          << fZone.name()
          << " is also in zone "
          << fZones[faceToZone[bFaceI]].name()
          << abort(FatalError);
        }
      }
    }
  }
  labelList neiFaceToZone{faceToZone};
  syncTools::swapBoundaryFaceList(mesh, neiFaceToZone);
  FOR_ALL(faceToZone, i) {
    if (faceToZone[i] != neiFaceToZone[i]) {
      FATAL_ERROR_IN
      (
        "meshRefinement::checkCoupledFaceZones(const polyMesh&)"
      )
      << "Face " << mesh.nInternalFaces()+i
      << " is in zone " << faceToZone[i]
      << ", its coupled face is in zone " << neiFaceToZone[i]
      << abort(FatalError);
    }
  }
}


void mousse::meshRefinement::calculateEdgeWeights
(
  const polyMesh& mesh,
  const PackedBoolList& isMasterEdge,
  const labelList& meshPoints,
  const edgeList& edges,
  scalarField& edgeWeights,
  scalarField& invSumWeight
)
{
  const pointField& pts = mesh.points();
  // Calculate edgeWeights and inverse sum of edge weights
  edgeWeights.setSize(isMasterEdge.size());
  invSumWeight.setSize(meshPoints.size());
  FOR_ALL(edges, edgeI) {
    const edge& e = edges[edgeI];
    scalar eMag =
      max(SMALL, mag(pts[meshPoints[e[1]]] - pts[meshPoints[e[0]]]));
    edgeWeights[edgeI] = 1.0/eMag;
  }
  // Sum per point all edge weights
  weightedSum
    (
      mesh,
      isMasterEdge,
      meshPoints,
      edges,
      edgeWeights,
      scalarField(meshPoints.size(), 1.0),  // data
      invSumWeight
    );
  // Inplace invert
  FOR_ALL(invSumWeight, pointI) {
    scalar w = invSumWeight[pointI];
    if (w > 0.0) {
      invSumWeight[pointI] = 1.0/w;
    }
  }
}


mousse::label mousse::meshRefinement::appendPatch
(
  fvMesh& mesh,
  const label insertPatchI,
  const word& patchName,
  const dictionary& patchDict
)
{
  // Clear local fields and e.g. polyMesh parallelInfo.
  mesh.clearOut();
  polyBoundaryMesh& polyPatches =
    const_cast<polyBoundaryMesh&>(mesh.boundaryMesh());
  fvBoundaryMesh& fvPatches = const_cast<fvBoundaryMesh&>(mesh.boundary());
  label patchI = polyPatches.size();
  // Add polyPatch at the end
  polyPatches.setSize(patchI+1);
  polyPatches.set
    (
      patchI,
      polyPatch::New
      (
        patchName,
        patchDict,
        insertPatchI,
        polyPatches
      )
    );
  fvPatches.setSize(patchI+1);
  fvPatches.set
    (
      patchI,
      fvPatch::New
      (
        polyPatches[patchI],  // point to newly added polyPatch
        mesh.boundary()
      )
    );
  addPatchFields<volScalarField>
    (
      mesh,
      calculatedFvPatchField<scalar>::typeName
    );
  addPatchFields<volVectorField>
    (
      mesh,
      calculatedFvPatchField<vector>::typeName
    );
  addPatchFields<volSphericalTensorField>
    (
      mesh,
      calculatedFvPatchField<sphericalTensor>::typeName
    );
  addPatchFields<volSymmTensorField>
    (
      mesh,
      calculatedFvPatchField<symmTensor>::typeName
    );
  addPatchFields<volTensorField>
    (
      mesh,
      calculatedFvPatchField<tensor>::typeName
    );
  // Surface fields
  addPatchFields<surfaceScalarField>
    (
      mesh,
      calculatedFvPatchField<scalar>::typeName
    );
  addPatchFields<surfaceVectorField>
    (
      mesh,
      calculatedFvPatchField<vector>::typeName
    );
  addPatchFields<surfaceSphericalTensorField>
    (
      mesh,
      calculatedFvPatchField<sphericalTensor>::typeName
    );
  addPatchFields<surfaceSymmTensorField>
    (
      mesh,
      calculatedFvPatchField<symmTensor>::typeName
    );
  addPatchFields<surfaceTensorField>
    (
      mesh,
      calculatedFvPatchField<tensor>::typeName
    );
  return patchI;
}


// Adds patch if not yet there. Returns patchID.
mousse::label mousse::meshRefinement::addPatch
(
  fvMesh& mesh,
  const word& patchName,
  const dictionary& patchInfo
)
{
  polyBoundaryMesh& polyPatches =
    const_cast<polyBoundaryMesh&>(mesh.boundaryMesh());
  fvBoundaryMesh& fvPatches = const_cast<fvBoundaryMesh&>(mesh.boundary());
  const label patchI = polyPatches.findPatchID(patchName);
  if (patchI != -1) {
    // Already there
    return patchI;
  }
  label insertPatchI = polyPatches.size();
  label startFaceI = mesh.nFaces();
  FOR_ALL(polyPatches, patchI) {
    const polyPatch& pp = polyPatches[patchI];
    if (isA<processorPolyPatch>(pp)) {
      insertPatchI = patchI;
      startFaceI = pp.start();
      break;
    }
  }
  dictionary patchDict(patchInfo);
  patchDict.set("nFaces", 0);
  patchDict.set("startFace", startFaceI);
  // Below is all quite a hack. Feel free to change once there is a better
  // mechanism to insert and reorder patches.
  label addedPatchI = appendPatch(mesh, insertPatchI, patchName, patchDict);
  // Create reordering list
  // patches before insert position stay as is
  labelList oldToNew{addedPatchI+1};
  for (label i = 0; i < insertPatchI; i++) {
    oldToNew[i] = i;
  }
  // patches after insert position move one up
  for (label i = insertPatchI; i < addedPatchI; i++) {
    oldToNew[i] = i+1;
  }
  // appended patch gets moved to insert position
  oldToNew[addedPatchI] = insertPatchI;
  // Shuffle into place
  polyPatches.reorder(oldToNew, true);
  fvPatches.reorder(oldToNew);
  reorderPatchFields<volScalarField>(mesh, oldToNew);
  reorderPatchFields<volVectorField>(mesh, oldToNew);
  reorderPatchFields<volSphericalTensorField>(mesh, oldToNew);
  reorderPatchFields<volSymmTensorField>(mesh, oldToNew);
  reorderPatchFields<volTensorField>(mesh, oldToNew);
  reorderPatchFields<surfaceScalarField>(mesh, oldToNew);
  reorderPatchFields<surfaceVectorField>(mesh, oldToNew);
  reorderPatchFields<surfaceSphericalTensorField>(mesh, oldToNew);
  reorderPatchFields<surfaceSymmTensorField>(mesh, oldToNew);
  reorderPatchFields<surfaceTensorField>(mesh, oldToNew);
  return insertPatchI;
}


mousse::label mousse::meshRefinement::addMeshedPatch
(
  const word& name,
  const dictionary& patchInfo
)
{
  label meshedI = findIndex(meshedPatches_, name);
  if (meshedI != -1) {
    // Already there. Get corresponding polypatch
    return mesh_.boundaryMesh().findPatchID(name);
  } else {
    // Add patch
    label patchI = addPatch(mesh_, name, patchInfo);
    // Store
    meshedPatches_.append(name);
    return patchI;
  }
}


mousse::labelList mousse::meshRefinement::meshedPatches() const
{
  const polyBoundaryMesh& patches = mesh_.boundaryMesh();
  DynamicList<label> patchIDs{meshedPatches_.size()};
  FOR_ALL(meshedPatches_, i) {
    label patchI = patches.findPatchID(meshedPatches_[i]);
    if (patchI == -1) {
      FATAL_ERROR_IN("meshRefinement::meshedPatches() const")
        << "Problem : did not find patch " << meshedPatches_[i]
        << endl << "Valid patches are " << patches.names()
        << abort(FatalError);
    }
    if (!polyPatch::constraintType(patches[patchI].type())) {
      patchIDs.append(patchI);
    }
  }
  return patchIDs;
}


void mousse::meshRefinement::selectSeparatedCoupledFaces(boolList& selected) const
{
  const polyBoundaryMesh& patches = mesh_.boundaryMesh();
  FOR_ALL(patches, patchI) {
    // Check all coupled. Avoid using .coupled() so we also pick up AMI.
    if (isA<coupledPolyPatch>(patches[patchI])) {
      const coupledPolyPatch& cpp =
        refCast<const coupledPolyPatch>(patches[patchI]);
      if (cpp.separated() || !cpp.parallel()) {
        FOR_ALL(cpp, i) {
          selected[cpp.start()+i] = true;
        }
      }
    }
  }
}


mousse::label mousse::meshRefinement::findRegion
(
  const polyMesh& mesh,
  const labelList& cellToRegion,
  const vector& perturbVec,
  const point& p
)
{
  label regionI = -1;
  label cellI = mesh.findCell(p);
  if (cellI != -1) {
    regionI = cellToRegion[cellI];
  }
  reduce(regionI, maxOp<label>());
  if (regionI == -1) {
    // See if we can perturb a bit
    cellI = mesh.findCell(p+perturbVec);
    if (cellI != -1) {
      regionI = cellToRegion[cellI];
    }
    reduce(regionI, maxOp<label>());
  }
  return regionI;
}


mousse::autoPtr<mousse::mapPolyMesh> mousse::meshRefinement::splitMeshRegions
(
  const labelList& globalToMasterPatch,
  const labelList& /*globalToSlavePatch*/,
  const point& keepPoint
)
{
  // Force calculation of face decomposition (used in findCell)
  (void)mesh_.tetBasePtIs();
  // Determine connected regions. regionSplit is the labelList with the
  // region per cell.
  boolList blockedFace{mesh_.nFaces(), false};
  selectSeparatedCoupledFaces(blockedFace);
  regionSplit cellRegion{mesh_, blockedFace};
  label regionI =
    findRegion
    (
      mesh_,
      cellRegion,
      mergeDistance_*vector(1,1,1), // note:1,1,1 should really be normalised
      keepPoint
    );
  if (regionI == -1) {
    FATAL_ERROR_IN
    (
      "meshRefinement::splitMeshRegions(const point&)"
    )
    << "Point " << keepPoint
    << " is not inside the mesh." << nl
    << "Bounding box of the mesh:" << mesh_.bounds()
    << exit(FatalError);
  }
  // Subset
  // ~~~~~~
  // Get cells to remove
  DynamicList<label> cellsToRemove{mesh_.nCells()};
  FOR_ALL(cellRegion, cellI) {
    if (cellRegion[cellI] != regionI) {
      cellsToRemove.append(cellI);
    }
  }
  cellsToRemove.shrink();
  label nCellsToKeep = mesh_.nCells() - cellsToRemove.size();
  reduce(nCellsToKeep, sumOp<label>());
  Info << "Keeping all cells in region " << regionI
    << " containing point " << keepPoint << endl
    << "Selected for keeping : "
    << nCellsToKeep
    << " cells." << endl;
  // Remove cells
  removeCells cellRemover(mesh_);
  labelList exposedFaces{cellRemover.getExposedFaces(cellsToRemove)};
  labelList exposedPatch;
  label nExposedFaces = returnReduce(exposedFaces.size(), sumOp<label>());
  if (nExposedFaces) {
    // Patch for exposed faces for lack of anything sensible.
    label defaultPatch = 0;
    if (globalToMasterPatch.size()) {
      defaultPatch = globalToMasterPatch[0];
    }
    WARNING_IN
    (
      "meshRefinement::splitMeshRegions(const point&)"
    )
    << "Removing non-reachable cells exposes "
    << nExposedFaces << " internal or coupled faces." << endl
    << "    These get put into patch " << defaultPatch << endl;
    exposedPatch.setSize(exposedFaces.size(), defaultPatch);
  }
  return
    doRemoveCells
    (
      cellsToRemove,
      exposedFaces,
      exposedPatch,
      cellRemover
    );
}


void mousse::meshRefinement::distribute(const mapDistributePolyMesh& map)
{
  // mesh_ already distributed; distribute my member data
  // surfaceQueries_ ok.
  // refinement
  meshCutter_.distribute(map);
  // surfaceIndex is face data.
  map.distributeFaceData(surfaceIndex_);
  // maintainedFaces are indices of faces.
  FOR_ALL(userFaceData_, i) {
    map.distributeFaceData(userFaceData_[i].second());
  }
  // Redistribute surface and any fields on it.
  {
    Random rndGen{653213};
    // Get local mesh bounding box. Single box for now.
    List<treeBoundBox> meshBb{1};
    treeBoundBox& bb = meshBb[0];
    bb = treeBoundBox{mesh_.points()};
    bb = bb.extend(rndGen, 1e-4);
    // Distribute all geometry (so refinementSurfaces and shellSurfaces)
    searchableSurfaces& geometry =
      const_cast<searchableSurfaces&>(surfaces_.geometry());
    FOR_ALL(geometry, i) {
      autoPtr<mapDistribute> faceMap;
      autoPtr<mapDistribute> pointMap;
      geometry[i].distribute(meshBb, false, faceMap, pointMap);
      if (faceMap.valid()) {
        // (ab)use the instance() to signal current modification time
        geometry[i].instance() = geometry[i].time().timeName();
      }
      faceMap.clear();
      pointMap.clear();
    }
  }
}


// Update local data for a mesh change
void mousse::meshRefinement::updateMesh
(
  const mapPolyMesh& map,
  const labelList& changedFaces
)
{
  Map<label> dummyMap{0};
  updateMesh(map, changedFaces, dummyMap, dummyMap, dummyMap);
}


void mousse::meshRefinement::storeData
(
  const labelList& pointsToStore,
  const labelList& facesToStore,
  const labelList& cellsToStore
)
{
  // For now only meshCutter has storable/retrievable data.
  meshCutter_.storeData
    (
      pointsToStore,
      facesToStore,
      cellsToStore
    );
}


void mousse::meshRefinement::updateMesh
(
  const mapPolyMesh& map,
  const labelList& changedFaces,
  const Map<label>& pointsToRestore,
  const Map<label>& facesToRestore,
  const Map<label>& cellsToRestore
)
{
  // For now only meshCutter has storable/retrievable data.
  // Update numbering of cells/vertices.
  meshCutter_.updateMesh
    (
      map,
      pointsToRestore,
      facesToRestore,
      cellsToRestore
    );
  // Update surfaceIndex
  updateList(map.faceMap(), label(-1), surfaceIndex_);
  // Update cached intersection information
  updateIntersections(changedFaces);
  // Update maintained faces
  FOR_ALL(userFaceData_, i) {
    labelList& data = userFaceData_[i].second();
    if (userFaceData_[i].first() == KEEPALL) {
      // extend list with face-from-face data
      updateList(map.faceMap(), label(-1), data);
    } else if (userFaceData_[i].first() == MASTERONLY) {
      // keep master only
      labelList newFaceData{map.faceMap().size(), -1};
      FOR_ALL(newFaceData, faceI) {
        label oldFaceI = map.faceMap()[faceI];
        if (oldFaceI >= 0 && map.reverseFaceMap()[oldFaceI] == faceI) {
          newFaceData[faceI] = data[oldFaceI];
        }
      }
      data.transfer(newFaceData);
    } else {
      // remove any face that has been refined i.e. referenced more than
      // once.
      // 1. Determine all old faces that get referenced more than once.
      // These get marked with -1 in reverseFaceMap
      labelList reverseFaceMap{map.reverseFaceMap()};
      FOR_ALL(map.faceMap(), faceI) {
        label oldFaceI = map.faceMap()[faceI];
        if (oldFaceI >= 0) {
          if (reverseFaceMap[oldFaceI] != faceI) {
            // faceI is slave face. Mark old face.
            reverseFaceMap[oldFaceI] = -1;
          }
        }
      }
      // 2. Map only faces with intact reverseFaceMap
      labelList newFaceData{map.faceMap().size(), -1};
      FOR_ALL(newFaceData, faceI) {
        label oldFaceI = map.faceMap()[faceI];
        if (oldFaceI >= 0) {
          if (reverseFaceMap[oldFaceI] == faceI) {
            newFaceData[faceI] = data[oldFaceI];
          }
        }
      }
      data.transfer(newFaceData);
    }
  }
}


bool mousse::meshRefinement::write() const
{
  bool writeOk =
    mesh_.write() && meshCutter_.write() && surfaceIndex_.write();
  // Make sure that any distributed surfaces (so ones which probably have
  // been changed) get written as well.
  // Note: should ideally have some 'modified' flag to say whether it
  // has been changed or not.
  searchableSurfaces& geometry =
    const_cast<searchableSurfaces&>(surfaces_.geometry());
  FOR_ALL(geometry, i) {
    searchableSurface& s = geometry[i];
    // Check if instance() of surface is not constant or system.
    // Is good hint that surface is distributed.
    if (s.instance() != s.time().system()
        && s.instance() != s.time().caseSystem()
        && s.instance() != s.time().constant()
        && s.instance() != s.time().caseConstant()) {
      // Make sure it gets written to current time, not constant.
      s.instance() = s.time().timeName();
      writeOk = writeOk && s.write();
    }
  }
  return writeOk;
}


mousse::PackedBoolList mousse::meshRefinement::getMasterPoints
(
  const polyMesh& mesh,
  const labelList& meshPoints
)
{
  const globalIndex globalPoints{meshPoints.size()};
  labelList myPoints{meshPoints.size()};
  FOR_ALL(meshPoints, pointI) {
    myPoints[pointI] = globalPoints.toGlobal(pointI);
  }
  syncTools::syncPointList
    (
      mesh,
      meshPoints,
      myPoints,
      minEqOp<label>(),
      labelMax
    );
  PackedBoolList isPatchMasterPoint{meshPoints.size()};
  FOR_ALL(meshPoints, pointI) {
    if (myPoints[pointI] == globalPoints.toGlobal(pointI)) {
      isPatchMasterPoint[pointI] = true;
    }
  }
  return isPatchMasterPoint;
}


mousse::PackedBoolList mousse::meshRefinement::getMasterEdges
(
  const polyMesh& mesh,
  const labelList& meshEdges
)
{
  const globalIndex globalEdges{meshEdges.size()};
  labelList myEdges{meshEdges.size()};
  FOR_ALL(meshEdges, edgeI) {
    myEdges[edgeI] = globalEdges.toGlobal(edgeI);
  }
  syncTools::syncEdgeList
    (
      mesh,
      meshEdges,
      myEdges,
      minEqOp<label>(),
      labelMax
    );
  PackedBoolList isMasterEdge{meshEdges.size()};
  FOR_ALL(meshEdges, edgeI) {
    if (myEdges[edgeI] == globalEdges.toGlobal(edgeI)) {
      isMasterEdge[edgeI] = true;
    }
  }
  return isMasterEdge;
}


void mousse::meshRefinement::printMeshInfo(const bool debug, const string& msg)
const
{
  const globalMeshData& pData = mesh_.globalData();
  if (debug) {
    Pout << msg.c_str()
      << " : cells(local):" << mesh_.nCells()
      << "  faces(local):" << mesh_.nFaces()
      << "  points(local):" << mesh_.nPoints()
      << endl;
  }

  {
    PackedBoolList isMasterFace{syncTools::getMasterFaces(mesh_)};
    label nMasterFaces = 0;
    FOR_ALL(isMasterFace, i) {
      if (isMasterFace[i]) {
        nMasterFaces++;
      }
    }
    PackedBoolList isMeshMasterPoint{syncTools::getMasterPoints(mesh_)};
    label nMasterPoints = 0;
    FOR_ALL(isMeshMasterPoint, i) {
      if (isMeshMasterPoint[i]) {
        nMasterPoints++;
      }
    }
    Info << msg.c_str()
      << " : cells:" << pData.nTotalCells()
      << "  faces:" << returnReduce(nMasterFaces, sumOp<label>())
      << "  points:" << returnReduce(nMasterPoints, sumOp<label>())
      << endl;
  }
  //if (debug)
  {
    const labelList& cellLevel = meshCutter_.cellLevel();
    labelList nCells{gMax(cellLevel)+1, 0};
    FOR_ALL(cellLevel, cellI) {
      nCells[cellLevel[cellI]]++;
    }
    Pstream::listCombineGather(nCells, plusEqOp<label>());
    Pstream::listCombineScatter(nCells);
    Info << "Cells per refinement level:" << endl;
    FOR_ALL(nCells, levelI) {
      Info << "    " << levelI << '\t' << nCells[levelI]
        << endl;
    }
  }
}


//- Return either time().constant() or oldInstance
mousse::word mousse::meshRefinement::timeName() const
{
  if (overwrite_ && mesh_.time().timeIndex() == 0) {
    return oldInstance_;
  } else {
    return mesh_.time().timeName();
  }
}


void mousse::meshRefinement::dumpRefinementLevel() const
{
  // Note: use time().timeName(), not meshRefinement::timeName()
  // so as to dump the fields to 0, not to constant.
  {
    volScalarField volRefLevel
    {
      {
        "cellLevel",
        mesh_.time().timeName(),
        mesh_,
        IOobject::NO_READ,
        IOobject::AUTO_WRITE,
        false
      },
      mesh_,
      {"zero", dimless, 0},
      zeroGradientFvPatchScalarField::typeName
    };
    const labelList& cellLevel = meshCutter_.cellLevel();
    FOR_ALL(volRefLevel, cellI) {
      volRefLevel[cellI] = cellLevel[cellI];
    }
    volRefLevel.write();
  }
  // Dump pointLevel
  {
    const pointMesh& pMesh = pointMesh::New(mesh_);
    pointScalarField pointRefLevel
    {
      {
        "pointLevel",
        mesh_.time().timeName(),
        mesh_,
        IOobject::NO_READ,
        IOobject::NO_WRITE,
        false
      },
      pMesh,
      {"zero", dimless, 0}
    };
    const labelList& pointLevel = meshCutter_.pointLevel();
    FOR_ALL(pointRefLevel, pointI) {
      pointRefLevel[pointI] = pointLevel[pointI];
    }
    pointRefLevel.write();
  }
}


void mousse::meshRefinement::dumpIntersections(const fileName& prefix) const
{
  {
    const pointField& cellCentres = mesh_.cellCentres();
    OFstream str{prefix + "_edges.obj"};
    label vertI = 0;
    Pout << "meshRefinement::dumpIntersections :"
      << " Writing cellcentre-cellcentre intersections to file "
      << str.name() << endl;
    // Redo all intersections
    // ~~~~~~~~~~~~~~~~~~~~~~
    // Get boundary face centre and level. Coupled aware.
    labelList neiLevel{mesh_.nFaces()-mesh_.nInternalFaces()};
    pointField neiCc{mesh_.nFaces()-mesh_.nInternalFaces()};
    calcNeighbourData(neiLevel, neiCc);
    labelList intersectionFaces{intersectedFaces()};
    // Collect segments we want to test for
    pointField start{intersectionFaces.size()};
    pointField end{intersectionFaces.size()};
    FOR_ALL(intersectionFaces, i) {
      label faceI = intersectionFaces[i];
      start[i] = cellCentres[mesh_.faceOwner()[faceI]];
      if (mesh_.isInternalFace(faceI)) {
        end[i] = cellCentres[mesh_.faceNeighbour()[faceI]];
      } else {
        end[i] = neiCc[faceI-mesh_.nInternalFaces()];
      }
    }
    // Extend segments a bit
    {
      const vectorField smallVec{ROOTSMALL*(end-start)};
      start -= smallVec;
      end += smallVec;
    }
    // Do tests in one go
    labelList surfaceHit;
    List<pointIndexHit> surfaceHitInfo;
    surfaces_.findAnyIntersection
      (
        start,
        end,
        surfaceHit,
        surfaceHitInfo
      );
    FOR_ALL(intersectionFaces, i) {
      if (surfaceHit[i] != -1) {
        meshTools::writeOBJ(str, start[i]);
        vertI++;
        meshTools::writeOBJ(str, surfaceHitInfo[i].hitPoint());
        vertI++;
        meshTools::writeOBJ(str, end[i]);
        vertI++;
        str << "l " << vertI-2 << ' ' << vertI-1 << nl
          << "l " << vertI-1 << ' ' << vertI << nl;
      }
    }
  }
  Pout << endl;
}


void mousse::meshRefinement::write
(
  const debugType debugFlags,
  const writeType writeFlags,
  const fileName& prefix
) const
{
  if (writeFlags & WRITEMESH) {
    write();
  }
  if (writeFlags & WRITELEVELS) {
    dumpRefinementLevel();
  }
  if (debugFlags & OBJINTERSECTIONS && prefix.size()) {
    dumpIntersections(prefix);
  }
}


mousse::meshRefinement::writeType mousse::meshRefinement::writeLevel()
{
  return writeLevel_;
}


void mousse::meshRefinement::writeLevel(const writeType flags)
{
  writeLevel_ = flags;
}


mousse::meshRefinement::outputType mousse::meshRefinement::outputLevel()
{
  return outputLevel_;
}


void mousse::meshRefinement::outputLevel(const outputType flags)
{
  outputLevel_ = flags;
}

