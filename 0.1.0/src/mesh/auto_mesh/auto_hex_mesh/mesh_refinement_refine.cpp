// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "mesh_refinement.hpp"
#include "tracked_particle.hpp"
#include "sync_tools.hpp"
#include "time.hpp"
#include "refinement_surfaces.hpp"
#include "refinement_features.hpp"
#include "shell_surfaces.hpp"
#include "face_set.hpp"
#include "decomposition_method.hpp"
#include "fv_mesh_distribute.hpp"
#include "poly_topo_change.hpp"
#include "map_distribute_poly_mesh.hpp"
#include "cloud.hpp"
#include "obj_stream.hpp"
#include "cell_set.hpp"
#include "tree_data_cell.hpp"


// Static Data Members
namespace mousse {

//- To compare normals
static bool less(const vector& x, const vector& y)
{
  for (direction i = 0; i < vector::nComponents; i++) {
    if (x[i] < y[i]) {
      return true;
    } else if (x[i] > y[i]) {
      return false;
    }
  }
  // All components the same
  return false;
}


//- To compare normals
class normalLess
{
  const vectorList& values_;
public:
  normalLess(const vectorList& values)
  :
    values_{values}
  {}
  bool operator()(const label a, const label b) const
  {
    return less(values_[a], values_[b]);
  }
};


//- Template specialization for pTraits<labelList> so we can have fields
template<>
class pTraits<labelList>
{
public:
  //- Component type
  typedef labelList cmptType;
};


//- Template specialization for pTraits<labelList> so we can have fields
template<>
class pTraits<vectorList>
{
public:
  //- Component type
  typedef vectorList cmptType;
};

}


// Private Member Functions 
// Get faces (on the new mesh) that have in some way been affected by the
// mesh change. Picks up all faces but those that are between two
// unrefined faces. (Note that of an unchanged face the edge still might be
// split but that does not change any face centre or cell centre.
mousse::labelList mousse::meshRefinement::getChangedFaces
(
  const mapPolyMesh& map,
  const labelList& oldCellsToRefine
)
{
  const polyMesh& mesh = map.mesh();
  labelList changedFaces;
  // For reporting: number of masterFaces changed
  label nMasterChanged = 0;
  PackedBoolList isMasterFace{syncTools::getMasterFaces(mesh)};

  {
    // Mark any face on a cell which has been added or changed
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Note that refining a face changes the face centre (for a warped face)
    // which changes the cell centre. This again changes the cellcentre-
    // cellcentre edge across all faces of the cell.
    // Note: this does not happen for unwarped faces but unfortunately
    // we don't have this information.
    const labelList& faceOwner = mesh.faceOwner();
    const labelList& faceNeighbour = mesh.faceNeighbour();
    const cellList& cells = mesh.cells();
    const label nInternalFaces = mesh.nInternalFaces();
    // Mark refined cells on old mesh
    PackedBoolList oldRefineCell{map.nOldCells()};
    FOR_ALL(oldCellsToRefine, i) {
      oldRefineCell.set(oldCellsToRefine[i], 1u);
    }
    // Mark refined faces
    PackedBoolList refinedInternalFace{nInternalFaces};
    // 1. Internal faces
    for (label faceI = 0; faceI < nInternalFaces; faceI++) {
      label oldOwn = map.cellMap()[faceOwner[faceI]];
      label oldNei = map.cellMap()[faceNeighbour[faceI]];
      if (oldOwn >= 0 && oldRefineCell.get(oldOwn) == 0u
          && oldNei >= 0 && oldRefineCell.get(oldNei) == 0u) {
        // Unaffected face since both neighbours were not refined.
      } else {
        refinedInternalFace.set(faceI, 1u);
      }
    }
    // 2. Boundary faces
    boolList refinedBoundaryFace{mesh.nFaces() - nInternalFaces, false};
    FOR_ALL(mesh.boundaryMesh(), patchI) {
      const polyPatch& pp = mesh.boundaryMesh()[patchI];
      label faceI = pp.start();
      FOR_ALL(pp, i) {
        label oldOwn = map.cellMap()[faceOwner[faceI]];
        if (oldOwn >= 0 && oldRefineCell.get(oldOwn) == 0u) {
          // owner did exist and wasn't refined.
        } else {
          refinedBoundaryFace[faceI-nInternalFaces] = true;
        }
        faceI++;
      }
    }
    // Synchronise refined face status
    syncTools::syncBoundaryFaceList
      (
        mesh,
        refinedBoundaryFace,
        orEqOp<bool>()
      );
    // 3. Mark all faces affected by refinement. Refined faces are in
    //    - refinedInternalFace
    //    - refinedBoundaryFace
    boolList changedFace{mesh.nFaces(), false};
    FOR_ALL(refinedInternalFace, faceI) {
      if (refinedInternalFace.get(faceI) == 1u) {
        const cell& ownFaces = cells[faceOwner[faceI]];
        FOR_ALL(ownFaces, ownI) {
          changedFace[ownFaces[ownI]] = true;
        }
        const cell& neiFaces = cells[faceNeighbour[faceI]];
        FOR_ALL(neiFaces, neiI) {
          changedFace[neiFaces[neiI]] = true;
        }
      }
    }
    FOR_ALL(refinedBoundaryFace, i) {
      if (refinedBoundaryFace[i]) {
        const cell& ownFaces = cells[faceOwner[i+nInternalFaces]];
        FOR_ALL(ownFaces, ownI) {
          changedFace[ownFaces[ownI]] = true;
        }
      }
    }
    syncTools::syncFaceList
      (
        mesh,
        changedFace,
        orEqOp<bool>()
      );
    // Now we have in changedFace marked all affected faces. Pack.
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    changedFaces = findIndices(changedFace, true);
    // Count changed master faces.
    nMasterChanged = 0;
    FOR_ALL(changedFace, faceI) {
      if (changedFace[faceI] && isMasterFace[faceI]) {
        nMasterChanged++;
      }
    }
  }
  if (debug & meshRefinement::MESH) {
    Pout << "getChangedFaces : Detected "
      << " local:" << changedFaces.size()
      << " global:" << returnReduce(nMasterChanged, sumOp<label>())
      << " changed faces out of " << mesh.globalData().nTotalFaces()
      << endl;
    faceSet changedFacesSet{mesh, "changedFaces", changedFaces};
    Pout << "getChangedFaces : Writing " << changedFaces.size()
      << " changed faces to faceSet " << changedFacesSet.name()
      << endl;
    changedFacesSet.write();
  }
  return changedFaces;
}


// Mark cell for refinement (if not already marked). Return false if
// refinelimit hit. Keeps running count (in nRefine) of cells marked for
// refinement
bool mousse::meshRefinement::markForRefine
(
  const label markValue,
  const label nAllowRefine,
  label& cellValue,
  label& nRefine
)
{
  if (cellValue == -1) {
    cellValue = markValue;
    nRefine++;
  }
  return nRefine <= nAllowRefine;
}


void mousse::meshRefinement::markFeatureCellLevel
(
  const pointField& keepPoints,
  labelList& maxFeatureLevel
) const
{
  // We want to refine all cells containing a feature edge.
  // - don't want to search over whole mesh
  // - don't want to build octree for whole mesh
  // - so use tracking to follow the feature edges
  //
  // 1. find non-manifold points on feature edges (i.e. start of feature edge
  //    or 'knot')
  // 2. seed particle starting at keepPoint going to this non-manifold point
  // 3. track particles to their non-manifold point
  //
  // 4. track particles across their connected feature edges, marking all
  //    visited cells with their level (through trackingData)
  // 5. do 4 until all edges have been visited.
  // Find all start cells of features. Is done by tracking from keepPoint.
  Cloud<trackedParticle> startPointCloud
  {
    mesh_,
    "startPointCloud",
    IDLList<trackedParticle>{}
  };
  // Features are identical on all processors. Number them so we know
  // what to seed. Do this on only the processor that
  // holds the keepPoint.
  FOR_ALL(keepPoints, i) {
    const point& keepPoint = keepPoints[i];
    label cellI = -1;
    label tetFaceI = -1;
    label tetPtI = -1;
    // Force construction of search tree even if processor holds no
    // cells
    (void)mesh_.cellTree();
    if (mesh_.nCells()) {
      mesh_.findCellFacePt(keepPoint, cellI, tetFaceI, tetPtI);
    }
    if (cellI == -1)
      continue;
    // I am the processor that holds the keepPoint
    FOR_ALL(features_, featI) {
      const edgeMesh& featureMesh = features_[featI];
      const label featureLevel = features_.levels()[featI][0];
      const labelListList& pointEdges = featureMesh.pointEdges();
      // Find regions on edgeMesh
      labelList edgeRegion;
      label nRegions = featureMesh.regions(edgeRegion);
      PackedBoolList regionVisited{nRegions};
      // 1. Seed all 'knots' in edgeMesh
      FOR_ALL(pointEdges, pointI) {
        if (pointEdges[pointI].size() != 2) {
          if (debug & meshRefinement::FEATURESEEDS) {
            Pout<< "Adding particle from point:" << pointI
              << " coord:" << featureMesh.points()[pointI]
              << " since number of emanating edges:"
              << pointEdges[pointI].size()
              << endl;
          }
          // Non-manifold point. Create particle.
          startPointCloud.addParticle
            (
              new trackedParticle
              {
                mesh_,
                keepPoint,
                cellI,
                tetFaceI,
                tetPtI,
                featureMesh.points()[pointI],   // endpos
                featureLevel,                   // level
                featI,                          // featureMesh
                pointI,                         // end point
                -1                              // feature edge
              }
            );
          // Mark
          if (pointEdges[pointI].size() > 0) {
            label e0 = pointEdges[pointI][0];
            label regionI = edgeRegion[e0];
            regionVisited[regionI] = 1u;
          }
        }
      }
      // 2. Any regions that have not been visited at all? These can
      //    only be circular regions!
      FOR_ALL(featureMesh.edges(), edgeI) {
        if (regionVisited.set(edgeRegion[edgeI], 1u)) {
          const edge& e = featureMesh.edges()[edgeI];
          label pointI = e.start();
          if (debug & meshRefinement::FEATURESEEDS) {
            Pout << "Adding particle from point:" << pointI
              << " coord:" << featureMesh.points()[pointI]
              << " on circular region:" << edgeRegion[edgeI]
              << endl;
          }
          // Non-manifold point. Create particle.
          startPointCloud.addParticle
            (
              new trackedParticle
              {
                mesh_,
                keepPoint,
                cellI,
                tetFaceI,
                tetPtI,
                featureMesh.points()[pointI],   // endpos
                featureLevel,                   // level
                featI,                          // featureMesh
                pointI,                         // end point
                -1                              // feature edge
              }
            );
        }
      }
    }
  }
  // Largest refinement level of any feature passed through
  maxFeatureLevel = labelList{mesh_.nCells(), -1};
  // Whether edge has been visited.
  List<PackedBoolList> featureEdgeVisited{features_.size()};
  FOR_ALL(features_, featI) {
    featureEdgeVisited[featI].setSize(features_[featI].edges().size());
    featureEdgeVisited[featI] = 0u;
  }
  // Database to pass into trackedParticle::move
  trackedParticle::trackingData td
  {
    startPointCloud,
    maxFeatureLevel,
    featureEdgeVisited
  };
  // Track all particles to their end position (= starting feature point)
  // Note that the particle might have started on a different processor
  // so this will transfer across nicely until we can start tracking proper.
  scalar maxTrackLen = 2.0*mesh_.bounds().mag();
  if (debug & meshRefinement::FEATURESEEDS) {
    Pout << "Tracking " << startPointCloud.size()
      << " particles over distance " << maxTrackLen
      << " to find the starting cell" << endl;
  }
  startPointCloud.move(td, maxTrackLen);
  // Reset levels
  maxFeatureLevel = -1;
  FOR_ALL(features_, featI) {
    featureEdgeVisited[featI] = 0u;
  }
  Cloud<trackedParticle> cloud
  {
    mesh_,
    "featureCloud",
    IDLList<trackedParticle>{}
  };
  if (debug&meshRefinement::FEATURESEEDS) {
    Pout << "Constructing cloud for cell marking" << endl;
  }
  FOR_ALL_ITER(Cloud<trackedParticle>, startPointCloud, iter) {
    const trackedParticle& startTp = iter();
    label featI = startTp.i();
    label pointI = startTp.j();
    const edgeMesh& featureMesh = features_[featI];
    const labelList& pEdges = featureMesh.pointEdges()[pointI];
    // Now shoot particles down all pEdges.
    FOR_ALL(pEdges, pEdgeI) {
      label edgeI = pEdges[pEdgeI];
      if (!featureEdgeVisited[featI].set(edgeI, 1u))
        continue;
      // Unvisited edge. Make the particle go to the other point
      // on the edge.
      const edge& e = featureMesh.edges()[edgeI];
      label otherPointI = e.otherVertex(pointI);
      trackedParticle* tp{new trackedParticle{startTp}};
      tp->end() = featureMesh.points()[otherPointI];
      tp->j() = otherPointI;
      tp->k() = edgeI;
      if (debug & meshRefinement::FEATURESEEDS) {
        Pout << "Adding particle for point:" << pointI
          << " coord:" << tp->position()
          << " feature:" << featI
          << " to track to:" << tp->end()
          << endl;
      }
      cloud.addParticle(tp);
    }
  }
  startPointCloud.clear();
  while (true) {
    // Track all particles to their end position.
    if (debug & meshRefinement::FEATURESEEDS) {
      Pout << "Tracking " << cloud.size()
        << " particles over distance " << maxTrackLen
        << " to mark cells" << endl;
    }
    cloud.move(td, maxTrackLen);
    // Make particle follow edge.
    FOR_ALL_ITER(Cloud<trackedParticle>, cloud, iter) {
      trackedParticle& tp = iter();
      label featI = tp.i();
      label pointI = tp.j();
      const edgeMesh& featureMesh = features_[featI];
      const labelList& pEdges = featureMesh.pointEdges()[pointI];
      // Particle now at pointI. Check connected edges to see which one
      // we have to visit now.
      bool keepParticle = false;
      FOR_ALL(pEdges, i) {
        label edgeI = pEdges[i];
        if (!featureEdgeVisited[featI].set(edgeI, 1u))
          continue;
        // Unvisited edge. Make the particle go to the other point
        // on the edge.
        const edge& e = featureMesh.edges()[edgeI];
        label otherPointI = e.otherVertex(pointI);
        tp.end() = featureMesh.points()[otherPointI];
        tp.j() = otherPointI;
        tp.k() = edgeI;
        keepParticle = true;
        break;
      }
      if (!keepParticle) {
        // Particle at 'knot' where another particle already has been
        // seeded. Delete particle.
        cloud.deleteParticle(tp);
      }
    }
    if (debug&meshRefinement::FEATURESEEDS) {
      Pout << "Remaining particles " << cloud.size() << endl;
    }
    if (returnReduce(cloud.size(), sumOp<label>()) == 0) {
      break;
    }
  }
}


// Calculates list of cells to refine based on intersection with feature edge.
mousse::label mousse::meshRefinement::markFeatureRefinement
(
  const pointField& keepPoints,
  const label nAllowRefine,
  labelList& refineCell,
  label& nRefine
) const
{
  // Largest refinement level of any feature passed through
  labelList maxFeatureLevel;
  markFeatureCellLevel(keepPoints, maxFeatureLevel);
  // See which cells to refine. maxFeatureLevel will hold highest level
  // of any feature edge that passed through.
  const labelList& cellLevel = meshCutter_.cellLevel();
  label oldNRefine = nRefine;
  FOR_ALL(maxFeatureLevel, cellI) {
    if (maxFeatureLevel[cellI] > cellLevel[cellI]) {
      const bool m =
        markForRefine(0, nAllowRefine, refineCell[cellI], nRefine);
      // Mark
      if (!m) {
        // Reached limit
        break;
      }
    }
  }
  if (returnReduce(nRefine, sumOp<label>())
      > returnReduce(nAllowRefine, sumOp<label>())) {
    Info << "Reached refinement limit." << endl;
  }
  return returnReduce(nRefine - oldNRefine,  sumOp<label>());
}


// Mark cells for distance-to-feature based refinement.
mousse::label mousse::meshRefinement::markInternalDistanceToFeatureRefinement
(
  const label nAllowRefine,
  labelList& refineCell,
  label& nRefine
) const
{
  const labelList& cellLevel = meshCutter_.cellLevel();
  const pointField& cellCentres = mesh_.cellCentres();
  // Detect if there are any distance shells
  if (features_.maxDistance() <= 0.0) {
    return 0;
  }
  label oldNRefine = nRefine;
  // Collect cells to test
  pointField testCc{cellLevel.size()-nRefine};
  labelList testLevels{cellLevel.size()-nRefine};
  label testI = 0;
  FOR_ALL(cellLevel, cellI) {
    if (refineCell[cellI] == -1) {
      testCc[testI] = cellCentres[cellI];
      testLevels[testI] = cellLevel[cellI];
      testI++;
    }
  }
  // Do test to see whether cells is inside/outside shell with higher level
  labelList maxLevel;
  features_.findHigherLevel(testCc, testLevels, maxLevel);
  // Mark for refinement. Note that we didn't store the original cellID so
  // now just reloop in same order.
  testI = 0;
  FOR_ALL(cellLevel, cellI) {
    if (refineCell[cellI] == -1) {
      if (maxLevel[testI] > testLevels[testI]) {
        bool reachedLimit =
          !markForRefine
          (
            maxLevel[testI],    // mark with any positive value
            nAllowRefine,
            refineCell[cellI],
            nRefine
          );
        if (reachedLimit) {
          if (debug) {
            Pout << "Stopped refining internal cells"
              << " since reaching my cell limit of "
              << mesh_.nCells()+7*nRefine << endl;
          }
          break;
        }
      }
      testI++;
    }
  }
  if (returnReduce(nRefine, sumOp<label>())
      > returnReduce(nAllowRefine, sumOp<label>())) {
    Info << "Reached refinement limit." << endl;
  }
  return returnReduce(nRefine-oldNRefine, sumOp<label>());
}


// Mark cells for non-surface intersection based refinement.
mousse::label mousse::meshRefinement::markInternalRefinement
(
  const label nAllowRefine,
  labelList& refineCell,
  label& nRefine
) const
{
  const labelList& cellLevel = meshCutter_.cellLevel();
  const pointField& cellCentres = mesh_.cellCentres();
  label oldNRefine = nRefine;
  // Collect cells to test
  pointField testCc{cellLevel.size()-nRefine};
  labelList testLevels{cellLevel.size()-nRefine};
  label testI = 0;
  FOR_ALL(cellLevel, cellI) {
    if (refineCell[cellI] == -1) {
      testCc[testI] = cellCentres[cellI];
      testLevels[testI] = cellLevel[cellI];
      testI++;
    }
  }
  // Do test to see whether cells is inside/outside shell with higher level
  labelList maxLevel;
  shells_.findHigherLevel(testCc, testLevels, maxLevel);
  // Mark for refinement. Note that we didn't store the original cellID so
  // now just reloop in same order.
  testI = 0;
  FOR_ALL(cellLevel, cellI) {
    if (refineCell[cellI] == -1) {
      if (maxLevel[testI] > testLevels[testI]) {
        bool reachedLimit =
          !markForRefine
          (
            maxLevel[testI],    // mark with any positive value
            nAllowRefine,
            refineCell[cellI],
            nRefine
          );
        if (reachedLimit) {
          if (debug) {
            Pout << "Stopped refining internal cells"
              << " since reaching my cell limit of "
              << mesh_.nCells()+7*nRefine << endl;
          }
          break;
        }
      }
      testI++;
    }
  }
  if (returnReduce(nRefine, sumOp<label>())
      > returnReduce(nAllowRefine, sumOp<label>()))
  {
    Info << "Reached refinement limit." << endl;
  }
  return returnReduce(nRefine-oldNRefine, sumOp<label>());
}


// Collect faces that are intersected and whose neighbours aren't yet marked
// for refinement.
mousse::labelList mousse::meshRefinement::getRefineCandidateFaces
(
  const labelList& refineCell
) const
{
  labelList testFaces{mesh_.nFaces()};
  label nTest = 0;
  FOR_ALL(surfaceIndex_, faceI) {
    if (surfaceIndex_[faceI] != -1) {
      label own = mesh_.faceOwner()[faceI];
      if (mesh_.isInternalFace(faceI)) {
        label nei = mesh_.faceNeighbour()[faceI];
        if (refineCell[own] == -1 || refineCell[nei] == -1) {
          testFaces[nTest++] = faceI;
        }
      } else {
        if (refineCell[own] == -1) {
          testFaces[nTest++] = faceI;
        }
      }
    }
  }
  testFaces.setSize(nTest);
  return testFaces;
}


// Mark cells for surface intersection based refinement.
mousse::label mousse::meshRefinement::markSurfaceRefinement
(
  const label nAllowRefine,
  const labelList& neiLevel,
  const pointField& neiCc,
  labelList& refineCell,
  label& nRefine
) const
{
  const labelList& cellLevel = meshCutter_.cellLevel();
  const pointField& cellCentres = mesh_.cellCentres();
  label oldNRefine = nRefine;
  // Use cached surfaceIndex_ to detect if any intersection. If so
  // re-intersect to determine level wanted.
  // Collect candidate faces
  // ~~~~~~~~~~~~~~~~~~~~~~~
  labelList testFaces{getRefineCandidateFaces(refineCell)};
  // Collect segments
  // ~~~~~~~~~~~~~~~~
  pointField start{testFaces.size()};
  pointField end{testFaces.size()};
  labelList minLevel{testFaces.size()};
  FOR_ALL(testFaces, i) {
    label faceI = testFaces[i];
    label own = mesh_.faceOwner()[faceI];
    if (mesh_.isInternalFace(faceI)) {
      label nei = mesh_.faceNeighbour()[faceI];
      start[i] = cellCentres[own];
      end[i] = cellCentres[nei];
      minLevel[i] = min(cellLevel[own], cellLevel[nei]);
    } else {
      label bFaceI = faceI - mesh_.nInternalFaces();
      start[i] = cellCentres[own];
      end[i] = neiCc[bFaceI];
      minLevel[i] = min(cellLevel[own], neiLevel[bFaceI]);
    }
  }
  // Extend segments a bit
  {
    const vectorField smallVec{ROOTSMALL*(end-start)};
    start -= smallVec;
    end += smallVec;
  }
  // Do test for higher intersections
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  labelList surfaceHit;
  labelList surfaceMinLevel;
  surfaces_.findHigherIntersection
    (
      start,
      end,
      minLevel,
      surfaceHit,
      surfaceMinLevel
    );
  // Mark cells for refinement
  // ~~~~~~~~~~~~~~~~~~~~~~~~~
  FOR_ALL(testFaces, i) {
    label faceI = testFaces[i];
    label surfI = surfaceHit[i];
    if (surfI != -1) {
      // Found intersection with surface with higher wanted
      // refinement. Check if the level field on that surface
      // specifies an even higher level. Note:this is weird. Should
      // do the check with the surfaceMinLevel whilst intersecting the
      // surfaces?
      label own = mesh_.faceOwner()[faceI];
      if (surfaceMinLevel[i] > cellLevel[own]) {
        // Owner needs refining
        const bool m =
          markForRefine(surfI, nAllowRefine, refineCell[own], nRefine);
        if (!m) {
          break;
        }
      }
      if (mesh_.isInternalFace(faceI)) {
        label nei = mesh_.faceNeighbour()[faceI];
        if (surfaceMinLevel[i] > cellLevel[nei]) {
          // Neighbour needs refining
          const bool m =
            markForRefine(surfI, nAllowRefine, refineCell[nei], nRefine);
          if (!m) {
            break;
          }
        }
      }
    }
  }
  if (returnReduce(nRefine, sumOp<label>())
      > returnReduce(nAllowRefine, sumOp<label>())) {
    Info << "Reached refinement limit." << endl;
  }
  return returnReduce(nRefine-oldNRefine, sumOp<label>());
}


// Count number of matches of first argument in second argument
mousse::label mousse::meshRefinement::countMatches
(
  const List<point>& normals1,
  const List<point>& normals2,
  const scalar tol
) const
{
  label nMatches = 0;
  FOR_ALL(normals1, i) {
    const vector& n1 = normals1[i];
    FOR_ALL(normals2, j) {
      const vector& n2 = normals2[j];
      if (magSqr(n1-n2) < tol) {
        nMatches++;
        break;
      }
    }
  }
  return nMatches;
}


// Mark cells for surface curvature based refinement.
mousse::label mousse::meshRefinement::markSurfaceCurvatureRefinement
(
  const scalar curvature,
  const label nAllowRefine,
  const labelList& neiLevel,
  const pointField& neiCc,
  labelList& refineCell,
  label& nRefine
) const
{
  const labelList& cellLevel = meshCutter_.cellLevel();
  const pointField& cellCentres = mesh_.cellCentres();
  label oldNRefine = nRefine;
  // 1. local test: any cell on more than one surface gets refined
  // (if its current level is < max of the surface max level)
  // 2. 'global' test: any cell on only one surface with a neighbour
  // on a different surface gets refined (if its current level etc.)
  const PackedBoolList isMasterFace{syncTools::getMasterFaces(mesh_)};
  // Collect candidate faces (i.e. intersecting any surface and
  // owner/neighbour not yet refined.
  labelList testFaces{getRefineCandidateFaces(refineCell)};
  // Collect segments
  pointField start{testFaces.size()};
  pointField end{testFaces.size()};
  labelList minLevel{testFaces.size()};
  FOR_ALL(testFaces, i) {
    label faceI = testFaces[i];
    label own = mesh_.faceOwner()[faceI];
    if (mesh_.isInternalFace(faceI)) {
      label nei = mesh_.faceNeighbour()[faceI];
      start[i] = cellCentres[own];
      end[i] = cellCentres[nei];
      minLevel[i] = min(cellLevel[own], cellLevel[nei]);
    } else {
      label bFaceI = faceI - mesh_.nInternalFaces();
      start[i] = cellCentres[own];
      end[i] = neiCc[bFaceI];
      if (!isMasterFace[faceI]) {
        Swap(start[i], end[i]);
      }
      minLevel[i] = min(cellLevel[own], neiLevel[bFaceI]);
    }
  }
  // Extend segments a bit
  {
    const vectorField smallVec{ROOTSMALL*(end-start)};
    start -= smallVec;
    end += smallVec;
  }
  // Test for all intersections (with surfaces of higher max level than
  // minLevel) and cache per cell the interesting inter
  labelListList cellSurfLevels{mesh_.nCells()};
  List<vectorList> cellSurfNormals{mesh_.nCells()};

  {
    // Per segment the normals of the surfaces
    List<vectorList> surfaceNormal;
    // Per segment the list of levels of the surfaces
    labelListList surfaceLevel;
    surfaces_.findAllHigherIntersections
      (
        start,
        end,
        minLevel,           // max level of surface has to be bigger
        // than min level of neighbouring cells
        surfaces_.maxLevel(),
        surfaceNormal,
        surfaceLevel
      );
    // Sort the data according to surface location. This will guarantee
    // that on coupled faces both sides visit the intersections in
    // the same order so will decide the same
    labelList visitOrder;
    FOR_ALL(surfaceNormal, pointI) {
      vectorList& pNormals = surfaceNormal[pointI];
      labelList& pLevel = surfaceLevel[pointI];
      sortedOrder(pNormals, visitOrder, normalLess(pNormals));
      pNormals = List<point>{pNormals, visitOrder};
      pLevel = UIndirectList<label>{pLevel, visitOrder};
    }
    // Clear out unnecessary data
    start.clear();
    end.clear();
    minLevel.clear();
    // Convert face-wise data to cell.
    FOR_ALL(testFaces, i) {
      label faceI = testFaces[i];
      label own = mesh_.faceOwner()[faceI];
      const vectorList& fNormals = surfaceNormal[i];
      const labelList& fLevels = surfaceLevel[i];
      FOR_ALL(fNormals, hitI) {
        if (fLevels[hitI] > cellLevel[own]) {
          cellSurfLevels[own].append(fLevels[hitI]);
          cellSurfNormals[own].append(fNormals[hitI]);
        }
        if (mesh_.isInternalFace(faceI)) {
          label nei = mesh_.faceNeighbour()[faceI];
          if (fLevels[hitI] > cellLevel[nei]) {
            cellSurfLevels[nei].append(fLevels[hitI]);
            cellSurfNormals[nei].append(fNormals[hitI]);
          }
        }
      }
    }
  }
  // Bit of statistics
  if (debug) {
    label nSet = 0;
    label nNormals = 9;
    FOR_ALL(cellSurfNormals, cellI) {
      const vectorList& normals = cellSurfNormals[cellI];
      if (normals.size()) {
        nSet++;
        nNormals += normals.size();
      }
    }
    reduce(nSet, sumOp<label>());
    reduce(nNormals, sumOp<label>());
    Info << "markSurfaceCurvatureRefinement :"
      << " cells:" << mesh_.globalData().nTotalCells()
      << " of which with normals:" << nSet
      << " ; total normals stored:" << nNormals
      << endl;
  }
  bool reachedLimit = false;
  // 1. Check normals per cell
  for (label cellI = 0;
       !reachedLimit && cellI < cellSurfNormals.size();
       cellI++) {
    const vectorList& normals = cellSurfNormals[cellI];
    const labelList& levels = cellSurfLevels[cellI];
    // n^2 comparison of all normals in a cell
    for (label i = 0; !reachedLimit && i < normals.size(); i++) {
      for (label j = i+1; !reachedLimit && j < normals.size(); j++) {
        if ((normals[i] & normals[j]) < curvature) {
          label maxLevel = max(levels[i], levels[j]);
          if (cellLevel[cellI] < maxLevel) {
            const bool m =
              markForRefine(maxLevel, nAllowRefine, refineCell[cellI], nRefine);
            if (!m) {
              if (debug) {
                Pout << "Stopped refining since reaching my cell"
                  << " limit of " << mesh_.nCells()+7*nRefine
                  << endl;
              }
              reachedLimit = true;
              break;
            }
          }
        }
      }
    }
  }
  // 2. Find out a measure of surface curvature
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Look at normals between neighbouring surfaces
  // Loop over all faces. Could only be checkFaces, except if they're coupled
  // Internal faces
  for (label faceI = 0;
       !reachedLimit && faceI < mesh_.nInternalFaces();
       faceI++) {
    label own = mesh_.faceOwner()[faceI];
    label nei = mesh_.faceNeighbour()[faceI];
    const vectorList& ownNormals = cellSurfNormals[own];
    const labelList& ownLevels = cellSurfLevels[own];
    const vectorList& neiNormals = cellSurfNormals[nei];
    const labelList& neiLevels = cellSurfLevels[nei];
    // Special case: owner normals set is a subset of the neighbour
    // normals. Do not do curvature refinement since other cell's normals
    // do not add any information. Avoids weird corner extensions of
    // refinement regions.
    bool ownIsSubset =
      countMatches(ownNormals, neiNormals) == ownNormals.size();
    bool neiIsSubset =
      countMatches(neiNormals, ownNormals) == neiNormals.size();
    if (!ownIsSubset && !neiIsSubset) {
      // n^2 comparison of between ownNormals and neiNormals
      for (label i = 0; !reachedLimit &&  i < ownNormals.size(); i++) {
        for (label j = 0; !reachedLimit && j < neiNormals.size(); j++) {
          // Have valid data on both sides. Check curvature.
          if ((ownNormals[i] & neiNormals[j]) < curvature) {
            // See which side to refine.
            if (cellLevel[own] < ownLevels[i]) {
              const bool m =
                markForRefine(ownLevels[i], nAllowRefine, refineCell[own],
                              nRefine);
              if (!m) {
                if (debug) {
                  Pout << "Stopped refining since reaching"
                    << " my cell limit of "
                    << mesh_.nCells()+7*nRefine << endl;
                }
                reachedLimit = true;
                break;
              }
            }
            if (cellLevel[nei] < neiLevels[j]) {
              const bool m =
                markForRefine(neiLevels[j], nAllowRefine, refineCell[nei],
                              nRefine);
              if (!m) {
                if (debug) {
                  Pout << "Stopped refining since reaching"
                    << " my cell limit of "
                    << mesh_.nCells()+7*nRefine << endl;
                }
                reachedLimit = true;
                break;
              }
            }
          }
        }
      }
    }
  }
  // Send over surface normal to neighbour cell.
  List<vectorList> neiSurfaceNormals;
  syncTools::swapBoundaryCellList(mesh_, cellSurfNormals, neiSurfaceNormals);
  // Boundary faces
  for (label faceI = mesh_.nInternalFaces();
       !reachedLimit && faceI < mesh_.nFaces();
       faceI++) {
    label own = mesh_.faceOwner()[faceI];
    label bFaceI = faceI - mesh_.nInternalFaces();
    const vectorList& ownNormals = cellSurfNormals[own];
    const labelList& ownLevels = cellSurfLevels[own];
    const vectorList& neiNormals = neiSurfaceNormals[bFaceI];
    // Special case: owner normals set is a subset of the neighbour
    // normals. Do not do curvature refinement since other cell's normals
    // do not add any information. Avoids weird corner extensions of
    // refinement regions.
    bool ownIsSubset =
      countMatches(ownNormals, neiNormals) == ownNormals.size();
    bool neiIsSubset =
      countMatches(neiNormals, ownNormals) == neiNormals.size();
    if (!ownIsSubset && !neiIsSubset) {
      // n^2 comparison of between ownNormals and neiNormals
      for (label i = 0; !reachedLimit &&  i < ownNormals.size(); i++) {
        for (label j = 0; !reachedLimit && j < neiNormals.size(); j++) {
          // Have valid data on both sides. Check curvature.
          if ((ownNormals[i] & neiNormals[j]) < curvature) {
            if (cellLevel[own] < ownLevels[i]) {
              const bool m = markForRefine(ownLevels[i], nAllowRefine,
                                           refineCell[own], nRefine);
              if (!m) {
                if (debug) {
                  Pout << "Stopped refining since reaching"
                    << " my cell limit of "
                    << mesh_.nCells()+7*nRefine
                    << endl;
                }
                reachedLimit = true;
                break;
              }
            }
          }
        }
      }
    }
  }
  if (returnReduce(nRefine, sumOp<label>())
      > returnReduce(nAllowRefine, sumOp<label>())) {
    Info << "Reached refinement limit." << endl;
  }
  return returnReduce(nRefine-oldNRefine, sumOp<label>());
}


bool mousse::meshRefinement::isGap
(
  const scalar planarCos,
  const vector& point0,
  const vector& normal0,
  const vector& point1,
  const vector& normal1
) const
{
  //- Hits differ and angles are oppositeish and
  //  hits have a normal distance
  vector d = point1-point0;
  scalar magD = mag(d);
  if (magD > mergeDistance()) {
    scalar cosAngle = (normal0 & normal1);
    vector avg = vector::zero;
    if (cosAngle < (-1+planarCos)) {
      // Opposite normals
      avg = 0.5*(normal0-normal1);
    } else if (cosAngle > (1-planarCos)) {
      avg = 0.5*(normal0+normal1);
    }
    if (avg != vector::zero) {
      avg /= mag(avg);
      // Check normal distance of intersection locations
      return (mag(avg&d) > mergeDistance());
    }
  }
  return false;
}


// Mark small gaps
bool mousse::meshRefinement::isNormalGap
(
  const scalar planarCos,
  const vector& point0,
  const vector& normal0,
  const vector& point1,
  const vector& normal1
) const
{
  //- Hits differ and angles are oppositeish and
  //  hits have a normal distance
  vector d = point1-point0;
  scalar magD = mag(d);
  if (magD > mergeDistance()) {
    scalar cosAngle = (normal0 & normal1);
    vector avg = vector::zero;
    if (cosAngle < (-1 + planarCos)) {
      // Opposite normals
      avg = 0.5*(normal0-normal1);
    } else if (cosAngle > (1-planarCos)) {
      avg = 0.5*(normal0+normal1);
    }
    if (avg != vector::zero) {
      avg /= mag(avg);
      d /= magD;
      // Check average normal with respect to intersection locations
      return (mag(avg&d) > mousse::cos(degToRad(45)));
    }
  }
  return false;
}


bool mousse::meshRefinement::checkProximity
(
  const scalar planarCos,
  const label nAllowRefine,
  const label surfaceLevel,       // current intersection max level
  const vector& surfaceLocation,  // current intersection location
  const vector& surfaceNormal,    // current intersection normal
  const label cellI,
  label& cellMaxLevel,        // cached max surface level for this cell
  vector& cellMaxLocation,    // cached surface normal for this cell
  vector& cellMaxNormal,      // cached surface normal for this cell
  labelList& refineCell,
  label& nRefine
) const
{
  const labelList& cellLevel = meshCutter_.cellLevel();
  // Test if surface applicable
  if (surfaceLevel <= cellLevel[cellI])
    return true;
  if (cellMaxLevel == -1) {
    // First visit of cell. Store
    cellMaxLevel = surfaceLevel;
    cellMaxLocation = surfaceLocation;
    cellMaxNormal = surfaceNormal;
  } else {
    // Second or more visit.
    // Check if
    //  - different location
    //  - opposite surface
    bool closeSurfaces =
      isNormalGap
      (
        planarCos,
        cellMaxLocation,
        cellMaxNormal,
        surfaceLocation,
        surfaceNormal
      );
    // Set normal to that of highest surface. Not really necessary
    // over here but we reuse cellMax info when doing coupled faces.
    if (surfaceLevel > cellMaxLevel) {
      cellMaxLevel = surfaceLevel;
      cellMaxLocation = surfaceLocation;
      cellMaxNormal = surfaceNormal;
    }
    if (closeSurfaces) {
      return
        markForRefine
        (
          surfaceLevel,   // mark with any non-neg number.
          nAllowRefine,
          refineCell[cellI],
          nRefine
        );
    }
  }
  // Did not reach refinement limit.
  return true;
}


mousse::label mousse::meshRefinement::markProximityRefinement
(
  const scalar planarCos,
  const label nAllowRefine,
  const labelList& neiLevel,
  const pointField& neiCc,
  labelList& refineCell,
  label& nRefine
) const
{
  const labelList& cellLevel = meshCutter_.cellLevel();
  const pointField& cellCentres = mesh_.cellCentres();
  label oldNRefine = nRefine;
  // 1. local test: any cell on more than one surface gets refined
  // (if its current level is < max of the surface max level)
  // 2. 'global' test: any cell on only one surface with a neighbour
  // on a different surface gets refined (if its current level etc.)
  // Collect candidate faces (i.e. intersecting any surface and
  // owner/neighbour not yet refined.
  labelList testFaces{getRefineCandidateFaces(refineCell)};
  // Collect segments
  pointField start{testFaces.size()};
  pointField end{testFaces.size()};
  labelList minLevel{testFaces.size()};
  FOR_ALL(testFaces, i) {
    label faceI = testFaces[i];
    label own = mesh_.faceOwner()[faceI];
    if (mesh_.isInternalFace(faceI)) {
      label nei = mesh_.faceNeighbour()[faceI];
      start[i] = cellCentres[own];
      end[i] = cellCentres[nei];
      minLevel[i] = min(cellLevel[own], cellLevel[nei]);
    } else {
      label bFaceI = faceI - mesh_.nInternalFaces();
      start[i] = cellCentres[own];
      end[i] = neiCc[bFaceI];
      minLevel[i] = min(cellLevel[own], neiLevel[bFaceI]);
    }
  }
  // Extend segments a bit
  {
    const vectorField smallVec{ROOTSMALL*(end-start)};
    start -= smallVec;
    end += smallVec;
  }
  // Test for all intersections (with surfaces of higher gap level than
  // minLevel) and cache per cell the max surface level and the local normal
  // on that surface.
  labelList cellMaxLevel{mesh_.nCells(), -1};
  vectorField cellMaxNormal{mesh_.nCells(), vector::zero};
  pointField cellMaxLocation{mesh_.nCells(), vector::zero};

  {
    // Per segment the normals of the surfaces
    List<vectorList> surfaceLocation;
    List<vectorList> surfaceNormal;
    // Per segment the list of levels of the surfaces
    labelListList surfaceLevel;
    surfaces_.findAllHigherIntersections
      (
        start,
        end,
        minLevel,           // gap level of surface has to be bigger
        // than min level of neighbouring cells
        surfaces_.gapLevel(),
        surfaceLocation,
        surfaceNormal,
        surfaceLevel
      );
    // Clear out unnecessary data
    start.clear();
    end.clear();
    minLevel.clear();
    FOR_ALL(testFaces, i) {
      label faceI = testFaces[i];
      label own = mesh_.faceOwner()[faceI];
      const labelList& fLevels = surfaceLevel[i];
      const vectorList& fPoints = surfaceLocation[i];
      const vectorList& fNormals = surfaceNormal[i];
      FOR_ALL(fLevels, hitI) {
        checkProximity
          (
            planarCos,
            nAllowRefine,
            fLevels[hitI],
            fPoints[hitI],
            fNormals[hitI],
            own,
            cellMaxLevel[own],
            cellMaxLocation[own],
            cellMaxNormal[own],
            refineCell,
            nRefine
          );
      }
      if (mesh_.isInternalFace(faceI)) {
        label nei = mesh_.faceNeighbour()[faceI];
        FOR_ALL(fLevels, hitI) {
          checkProximity
            (
              planarCos,
              nAllowRefine,
              fLevels[hitI],
              fPoints[hitI],
              fNormals[hitI],
              nei,
              cellMaxLevel[nei],
              cellMaxLocation[nei],
              cellMaxNormal[nei],
              refineCell,
              nRefine
            );
        }
      }
    }
  }
  // 2. Find out a measure of surface curvature and region edges.
  // Send over surface region and surface normal to neighbour cell.
  labelList neiBndMaxLevel{mesh_.nFaces()-mesh_.nInternalFaces()};
  pointField neiBndMaxLocation{mesh_.nFaces()-mesh_.nInternalFaces()};
  vectorField neiBndMaxNormal{mesh_.nFaces()-mesh_.nInternalFaces()};
  for (label faceI = mesh_.nInternalFaces(); faceI < mesh_.nFaces(); faceI++) {
    label bFaceI = faceI-mesh_.nInternalFaces();
    label own = mesh_.faceOwner()[faceI];
    neiBndMaxLevel[bFaceI] = cellMaxLevel[own];
    neiBndMaxLocation[bFaceI] = cellMaxLocation[own];
    neiBndMaxNormal[bFaceI] = cellMaxNormal[own];
  }
  syncTools::swapBoundaryFaceList(mesh_, neiBndMaxLevel);
  syncTools::swapBoundaryFaceList(mesh_, neiBndMaxLocation);
  syncTools::swapBoundaryFaceList(mesh_, neiBndMaxNormal);
  // Loop over all faces. Could only be checkFaces.. except if they're coupled
  // Internal faces
  for (label faceI = 0; faceI < mesh_.nInternalFaces(); faceI++) {
    label own = mesh_.faceOwner()[faceI];
    label nei = mesh_.faceNeighbour()[faceI];
    if (cellMaxLevel[own] == -1 || cellMaxLevel[nei] == -1)
      continue;
    // Have valid data on both sides. Check planarCos.
    const auto g =
      isNormalGap(planarCos, cellMaxLocation[own], cellMaxNormal[own],
                  cellMaxLocation[nei], cellMaxNormal[nei]);
    if (g) {
      // See which side to refine
      if (cellLevel[own] < cellMaxLevel[own]) {
        const bool m =
          markForRefine(cellMaxLevel[own], nAllowRefine, refineCell[own],
                        nRefine);
        if (!m) {
          if (debug) {
            Pout << "Stopped refining since reaching my cell"
              << " limit of " << mesh_.nCells()+7*nRefine
              << endl;
          }
          break;
        }
      }
      if (cellLevel[nei] < cellMaxLevel[nei]) {
        const bool m =
          markForRefine(cellMaxLevel[nei], nAllowRefine, refineCell[nei],
                        nRefine);
        if (!m) {
          if (debug) {
            Pout << "Stopped refining since reaching my cell"
              << " limit of " << mesh_.nCells()+7*nRefine
              << endl;
          }
          break;
        }
      }
    }
  }
  // Boundary faces
  for (label faceI = mesh_.nInternalFaces(); faceI < mesh_.nFaces(); faceI++) {
    label own = mesh_.faceOwner()[faceI];
    label bFaceI = faceI - mesh_.nInternalFaces();
    if (cellLevel[own] < cellMaxLevel[own] && neiBndMaxLevel[bFaceI] != -1) {
      // Have valid data on both sides. Check planarCos.
      const auto g =
        isNormalGap(planarCos, cellMaxLocation[own], cellMaxNormal[own],
                    neiBndMaxLocation[bFaceI], neiBndMaxNormal[bFaceI]);
      if (g) {
        const bool m =
          markForRefine(cellMaxLevel[own], nAllowRefine, refineCell[own],
                        nRefine);

        if (!m) {
          if (debug) {
            Pout << "Stopped refining since reaching my cell"
              << " limit of " << mesh_.nCells()+7*nRefine
              << endl;
          }
          break;
        }
      }
    }
  }
  if (returnReduce(nRefine, sumOp<label>())
      > returnReduce(nAllowRefine, sumOp<label>())) {
    Info << "Reached refinement limit." << endl;
  }
  return returnReduce(nRefine-oldNRefine, sumOp<label>());
}


// Member Functions 
// Calculate list of cells to refine. Gets for any edge (start - end)
// whether it intersects the surface. Does more accurate test and checks
// the wanted level on the surface intersected.
// Does approximate precalculation of how many cells can be refined before
// hitting overall limit maxGlobalCells.
mousse::labelList mousse::meshRefinement::refineCandidates
(
  const pointField& keepPoints,
  const scalar curvature,
  const scalar planarAngle,
  const bool featureRefinement,
  const bool featureDistanceRefinement,
  const bool internalRefinement,
  const bool surfaceRefinement,
  const bool curvatureRefinement,
  const bool gapRefinement,
  const label maxGlobalCells,
  const label /*maxLocalCells*/
) const
{
  label totNCells = mesh_.globalData().nTotalCells();
  labelList cellsToRefine;
  if (totNCells >= maxGlobalCells) {
    Info << "No cells marked for refinement since reached limit "
      << maxGlobalCells << '.' << endl;
  } else {
    // Every cell I refine adds 7 cells. Estimate the number of cells
    // I am allowed to refine.
    // Assume perfect distribution so can only refine as much the fraction
    // of the mesh I hold. This prediction step prevents us having to do
    // lots of reduces to keep count of the total number of cells selected
    // for refinement.
    //scalar fraction = scalar(mesh_.nCells())/totNCells;
    //label myMaxCells = label(maxGlobalCells*fraction);
    //label nAllowRefine = (myMaxCells - mesh_.nCells())/7;
    ////label nAllowRefine = (maxLocalCells - mesh_.nCells())/7;
    //
    //Pout<< "refineCandidates:" << nl
    //    << "    total cells:" << totNCells << nl
    //    << "    local cells:" << mesh_.nCells() << nl
    //    << "    local fraction:" << fraction << nl
    //    << "    local allowable cells:" << myMaxCells << nl
    //    << "    local allowable refinement:" << nAllowRefine << nl
    //    << endl;
    //- Disable refinement shortcut. nAllowRefine is per processor limit.
    label nAllowRefine = labelMax/Pstream::nProcs();
    // Marked for refinement (>= 0) or not (-1). Actual value is the
    // index of the surface it intersects.
    labelList refineCell{mesh_.nCells(), -1};
    label nRefine = 0;
    // Swap neighbouring cell centres and cell level
    labelList neiLevel{mesh_.nFaces()-mesh_.nInternalFaces()};
    pointField neiCc{mesh_.nFaces()-mesh_.nInternalFaces()};
    calcNeighbourData(neiLevel, neiCc);
    // Cells pierced by feature lines
    if (featureRefinement) {
      label nFeatures =
        markFeatureRefinement
        (
          keepPoints,
          nAllowRefine,
          refineCell,
          nRefine
        );
      Info << "Marked for refinement due to explicit features             "
        << ": " << nFeatures << " cells."  << endl;
    }
    // Inside distance-to-feature shells
    if (featureDistanceRefinement) {
      label nShell =
        markInternalDistanceToFeatureRefinement
        (
          nAllowRefine,
          refineCell,
          nRefine
        );
      Info << "Marked for refinement due to distance to explicit features "
        ": " << nShell << " cells."  << endl;
    }
    // Inside refinement shells
    if (internalRefinement) {
      label nShell =
        markInternalRefinement
        (
          nAllowRefine,
          refineCell,
          nRefine
        );
      Info << "Marked for refinement due to refinement shells             "
        << ": " << nShell << " cells."  << endl;
    }
    // Refinement based on intersection of surface
    if (surfaceRefinement) {
      label nSurf =
        markSurfaceRefinement
        (
          nAllowRefine,
          neiLevel,
          neiCc,
          refineCell,
          nRefine
        );
      Info << "Marked for refinement due to surface intersection          "
        << ": " << nSurf << " cells."  << endl;
    }
    // Refinement based on curvature of surface
    if (curvatureRefinement && (curvature >= -1 && curvature <= 1)
        && (surfaces_.minLevel() != surfaces_.maxLevel())) {
      label nCurv =
        markSurfaceCurvatureRefinement
        (
          curvature,
          nAllowRefine,
          neiLevel,
          neiCc,
          refineCell,
          nRefine
        );
      Info << "Marked for refinement due to curvature/regions             "
        << ": " << nCurv << " cells."  << endl;
    }
    const scalar planarCos = mousse::cos(degToRad(planarAngle));
    if (gapRefinement && (planarCos >= -1 && planarCos <= 1)
        && (max(surfaces_.gapLevel()) > -1)) {
      Info << "Specified gap level : " << max(surfaces_.gapLevel())
        << ", planar angle " << planarAngle << endl;
      label nGap =
        markProximityRefinement
        (
          planarCos,
          nAllowRefine,
          neiLevel,
          neiCc,
          refineCell,
          nRefine
        );
      Info << "Marked for refinement due to close opposite surfaces       "
        << ": " << nGap << " cells."  << endl;
    }
    // Pack cells-to-refine
    cellsToRefine.setSize(nRefine);
    nRefine = 0;
    FOR_ALL(refineCell, cellI) {
      if (refineCell[cellI] != -1) {
        cellsToRefine[nRefine++] = cellI;
      }
    }
  }
  return cellsToRefine;
}


mousse::autoPtr<mousse::mapPolyMesh> mousse::meshRefinement::refine
(
  const labelList& cellsToRefine
)
{
  // Mesh changing engine.
  polyTopoChange meshMod{mesh_};
  // Play refinement commands into mesh changer.
  meshCutter_.setRefinement(cellsToRefine, meshMod);
  // Create mesh (no inflation), return map from old to new mesh.
  autoPtr<mapPolyMesh> map = meshMod.changeMesh(mesh_, false);
  // Update fields
  mesh_.updateMesh(map);
  // Optionally inflate mesh
  if (map().hasMotionPoints()) {
    mesh_.movePoints(map().preMotionPoints());
  } else {
    // Delete mesh volumes.
    mesh_.clearOut();
  }
  // Reset the instance for if in overwrite mode
  mesh_.setInstance(timeName());
  // Update intersection info
  updateMesh(map, getChangedFaces(map, cellsToRefine));
  return map;
}


// Do refinement of consistent set of cells followed by truncation and
// load balancing.
mousse::autoPtr<mousse::mapDistributePolyMesh>
mousse::meshRefinement::refineAndBalance
(
  const string& msg,
  decompositionMethod& decomposer,
  fvMeshDistribute& distributor,
  const labelList& cellsToRefine,
  const scalar maxLoadUnbalance
)
{
  // Do all refinement
  refine(cellsToRefine);
  if (debug & meshRefinement::MESH) {
    Pout << "Writing refined but unbalanced " << msg
      << " mesh to time " << timeName() << endl;
    write
      (
        debugType(debug),
        writeType(writeLevel() | WRITEMESH),
        mesh_.time().path()/timeName()
      );
    Pout << "Dumped debug data in = "
      << mesh_.time().cpuTimeIncrement() << " s" << endl;
    // test all is still synced across proc patches
    checkData();
  }
  Info << "Refined mesh in = "
    << mesh_.time().cpuTimeIncrement() << " s" << endl;
  printMeshInfo(debug, "After refinement " + msg);
  // Load balancing
  // ~~~~~~~~~~~~~~
  autoPtr<mapDistributePolyMesh> distMap;
  if (Pstream::nProcs() > 1) {
    scalar nIdealCells =
      mesh_.globalData().nTotalCells()/Pstream::nProcs();
    scalar unbalance =
      returnReduce
      (
        mag(1.0-mesh_.nCells()/nIdealCells),
        maxOp<scalar>()
      );
    if (unbalance <= maxLoadUnbalance) {
      Info << "Skipping balancing since max unbalance " << unbalance
        << " is less than allowable " << maxLoadUnbalance
        << endl;
    } else {
      scalarField cellWeights{mesh_.nCells(), 1};
      distMap =
        balance
        (
          false,  //keepZoneFaces
          false,  //keepBaffles
          cellWeights,
          decomposer,
          distributor
        );
      Info << "Balanced mesh in = "
        << mesh_.time().cpuTimeIncrement() << " s" << endl;
      printMeshInfo(debug, "After balancing " + msg);
      if (debug&meshRefinement::MESH) {
        Pout << "Writing balanced " << msg
          << " mesh to time " << timeName() << endl;
        write
          (
            debugType(debug),
            writeType(writeLevel() | WRITEMESH),
            mesh_.time().path()/timeName()
          );
        Pout << "Dumped debug data in = "
          << mesh_.time().cpuTimeIncrement() << " s" << endl;
        // test all is still synced across proc patches
        checkData();
      }
    }
  }
  return distMap;
}


// Do load balancing followed by refinement of consistent set of cells.
mousse::autoPtr<mousse::mapDistributePolyMesh>
mousse::meshRefinement::balanceAndRefine
(
  const string& msg,
  decompositionMethod& decomposer,
  fvMeshDistribute& distributor,
  const labelList& initCellsToRefine,
  const scalar maxLoadUnbalance
)
{
  labelList cellsToRefine{initCellsToRefine};
  // Load balancing
  // ~~~~~~~~~~~~~~
  autoPtr<mapDistributePolyMesh> distMap;
  if (Pstream::nProcs() > 1) {
    // First check if we need to balance at all. Precalculate number of
    // cells after refinement and see what maximum difference is.
    scalar nNewCells =
      static_cast<scalar>(mesh_.nCells() + 7*cellsToRefine.size());
    scalar nIdealNewCells =
      returnReduce(nNewCells, sumOp<scalar>())/Pstream::nProcs();
    scalar unbalance =
      returnReduce(mag(1.0-nNewCells/nIdealNewCells), maxOp<scalar>());
    if (unbalance <= maxLoadUnbalance) {
      Info << "Skipping balancing since max unbalance " << unbalance
        << " is less than allowable " << maxLoadUnbalance
        << endl;
    } else {
      scalarField cellWeights{mesh_.nCells(), 1};
      FOR_ALL(cellsToRefine, i) {
        cellWeights[cellsToRefine[i]] += 7;
      }
      distMap =
        balance
        (
          false,  //keepZoneFaces
          false,  //keepBaffles
          cellWeights,
          decomposer,
          distributor
        );
      // Update cells to refine
      distMap().distributeCellIndices(cellsToRefine);
      Info << "Balanced mesh in = "
        << mesh_.time().cpuTimeIncrement() << " s" << endl;
    }
    printMeshInfo(debug, "After balancing " + msg);
    if (debug & meshRefinement::MESH) {
      Pout << "Writing balanced " << msg
        << " mesh to time " << timeName() << endl;
      write
        (
          debugType(debug),
          writeType(writeLevel() | WRITEMESH),
          mesh_.time().path()/timeName()
        );
      Pout << "Dumped debug data in = "
        << mesh_.time().cpuTimeIncrement() << " s" << endl;
      // test all is still synced across proc patches
      checkData();
    }
  }
  // Refinement
  refine(cellsToRefine);
  if (debug & meshRefinement::MESH) {
    Pout << "Writing refined " << msg
      << " mesh to time " << timeName() << endl;
    write
      (
        debugType(debug),
        writeType(writeLevel() | WRITEMESH),
        mesh_.time().path()/timeName()
      );
    Pout << "Dumped debug data in = "
      << mesh_.time().cpuTimeIncrement() << " s" << endl;
    // test all is still synced across proc patches
    checkData();
  }
  Info << "Refined mesh in = "
    << mesh_.time().cpuTimeIncrement() << " s" << endl;
  printMeshInfo(debug, "After refinement " + msg);
  return distMap;
}

