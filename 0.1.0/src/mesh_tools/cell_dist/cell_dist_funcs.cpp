// mousse: CFD toolbox
// Copyright (C) 2011-2012 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "cell_dist_funcs.hpp"
#include "poly_mesh.hpp"
#include "wall_poly_patch.hpp"
#include "poly_boundary_mesh.hpp"


// Static Data Members
namespace mousse {

DEFINE_TYPE_NAME_AND_DEBUG(cellDistFuncs, 0);

}


// Private Member Functions 

// Find val in first nElems elements of elems.
mousse::label mousse::cellDistFuncs::findIndex
(
  const label nElems,
  const labelList& elems,
  const label val
)
{
  for (label i = 0; i < nElems; i++) {
    if (elems[i] == val) {
      return i;
    }
  }
  return -1;
}


// Constructors 
mousse::cellDistFuncs::cellDistFuncs(const polyMesh& mesh)
:
  mesh_{mesh}
{}


// Member Functions 
mousse::labelHashSet mousse::cellDistFuncs::getPatchIDs
(
  const wordReList& patchNames
) const
{
  return mesh().boundaryMesh().patchSet(patchNames, false);
}


// Return smallest true distance from p to any of wallFaces.
// Note that even if normal hits face we still check other faces.
// Note that wallFaces is untruncated and we explicitly pass in size.
mousse::scalar mousse::cellDistFuncs::smallestDist
(
  const point& p,
  const polyPatch& patch,
  const label nWallFaces,
  const labelList& wallFaces,
  label& minFaceI
) const
{
  const pointField& points = patch.points();
  scalar minDist = GREAT;
  minFaceI = -1;
  for (label wallFaceI = 0; wallFaceI < nWallFaces; wallFaceI++) {
    label patchFaceI = wallFaces[wallFaceI];
    pointHit curHit = patch[patchFaceI].nearestPoint(p, points);
    if (curHit.distance() < minDist) {
      minDist = curHit.distance();
      minFaceI = patch.start() + patchFaceI;
    }
  }
  return minDist;
}


// Get point neighbours of faceI (including faceI). Returns number of faces.
// Note: does not allocate storage but does use linear search to determine
// uniqueness. For polygonal faces this might be quite inefficient.
mousse::label mousse::cellDistFuncs::getPointNeighbours
(
  const primitivePatch& patch,
  const label patchFaceI,
  labelList& neighbours
) const
{
  label nNeighbours = 0;
  // Add myself
  neighbours[nNeighbours++] = patchFaceI;
  // Add all face neighbours
  const labelList& faceNeighbours = patch.faceFaces()[patchFaceI];
  FOR_ALL(faceNeighbours, faceNeighbourI) {
    neighbours[nNeighbours++] = faceNeighbours[faceNeighbourI];
  }
  // Remember part of neighbours that contains edge-connected faces.
  label nEdgeNbs = nNeighbours;
  // Add all point-only neighbours by linear searching in edge neighbours.
  // Assumes that point-only neighbours are not using multiple points on
  // face.
  const face& f = patch.localFaces()[patchFaceI];
  FOR_ALL(f, fp) {
    label pointI = f[fp];
    const labelList& pointNbs = patch.pointFaces()[pointI];
    FOR_ALL(pointNbs, nbI) {
      label faceI = pointNbs[nbI];
      // Check for faceI in edge-neighbours part of neighbours
      if (findIndex(nEdgeNbs, neighbours, faceI) == -1) {
        neighbours[nNeighbours++] = faceI;
      }
    }
  }
  if (debug) {
    // Check for duplicates
    // Use hashSet to determine nbs.
    labelHashSet nbs{4*f.size()};
    FOR_ALL(f, fp) {
      const labelList& pointNbs = patch.pointFaces()[f[fp]];
      FOR_ALL(pointNbs, i) {
        nbs.insert(pointNbs[i]);
      }
    }
    // Subtract ours.
    for (label i = 0; i < nNeighbours; i++) {
      label nb = neighbours[i];
      if (!nbs.found(nb)) {
        SERIOUS_ERROR_IN("mousse::cellDistFuncs::getPointNeighbours")
          << "getPointNeighbours : patchFaceI:" << patchFaceI
          << " verts:" << f << endl;
        FOR_ALL(f, fp) {
          SERIOUS_ERROR_IN("mousse::cellDistFuncs::getPointNeighbours")
            << "point:" << f[fp] << " pointFaces:"
            << patch.pointFaces()[f[fp]] << endl;
        }
        for (label i = 0; i < nNeighbours; i++) {
          SERIOUS_ERROR_IN("mousse::cellDistFuncs::getPointNeighbours")
            << "fast nbr:" << neighbours[i]
            << endl;
        }
        FATAL_ERROR_IN("getPointNeighbours")
          << "Problem: fast pointNeighbours routine included " << nb
          << " which is not in proper neigbour list " << nbs.toc()
          << abort(FatalError);
      }
      nbs.erase(nb);
    }
    if (nbs.size()) {
      FATAL_ERROR_IN("getPointNeighbours")
        << "Problem: fast pointNeighbours routine did not find "
        << nbs.toc() << abort(FatalError);
    }
  }
  return nNeighbours;
}


// size of largest patch (out of supplied subset of patches)
mousse::label mousse::cellDistFuncs::maxPatchSize
(
  const labelHashSet& patchIDs
) const
{
  label maxSize = 0;
  FOR_ALL(mesh().boundaryMesh(), patchI) {
    if (patchIDs.found(patchI)) {
      const polyPatch& patch = mesh().boundaryMesh()[patchI];
      maxSize = mousse::max(maxSize, patch.size());
    }
  }
  return maxSize;
}


// sum of patch sizes (out of supplied subset of patches)
mousse::label mousse::cellDistFuncs::sumPatchSize
(
  const labelHashSet& patchIDs
)
const
{
  label sum = 0;
  FOR_ALL(mesh().boundaryMesh(), patchI) {
    if (patchIDs.found(patchI)) {
      const polyPatch& patch = mesh().boundaryMesh()[patchI];
      sum += patch.size();
    }
  }
  return sum;
}


// Gets nearest wall for cells next to wall
void mousse::cellDistFuncs::correctBoundaryFaceCells
(
  const labelHashSet& patchIDs,
  scalarField& wallDistCorrected,
  Map<label>& nearestFace
) const
{
  // Size neighbours array for maximum possible (= size of largest patch)
  label maxPointNeighbours = maxPatchSize(patchIDs);
  labelList neighbours{maxPointNeighbours};
  // Correct all cells with face on wall
  const vectorField& cellCentres = mesh().cellCentres();
  const labelList& faceOwner = mesh().faceOwner();
  FOR_ALL(mesh().boundaryMesh(), patchI) {
    if (patchIDs.found(patchI)) {
      const polyPatch& patch = mesh().boundaryMesh()[patchI];
      // Check cells with face on wall
      FOR_ALL(patch, patchFaceI) {
        label nNeighbours = getPointNeighbours
        (
          patch,
          patchFaceI,
          neighbours
        );
        label cellI = faceOwner[patch.start() + patchFaceI];
        label minFaceI = -1;
        wallDistCorrected[cellI] = smallestDist
        (
          cellCentres[cellI],
          patch,
          nNeighbours,
          neighbours,
          minFaceI
        );
        // Store wallCell and its nearest neighbour
        nearestFace.insert(cellI, minFaceI);
      }
    }
  }
}


// Correct all cells connected to wall (via point) and not in nearestFace
void mousse::cellDistFuncs::correctBoundaryPointCells
(
  const labelHashSet& patchIDs,
  scalarField& wallDistCorrected,
  Map<label>& nearestFace
) const
{
  // Correct all (non-visited) cells with point on wall
  const vectorField& cellCentres = mesh().cellCentres();
  FOR_ALL(mesh().boundaryMesh(), patchI) {
    if (patchIDs.found(patchI)) {
      const polyPatch& patch = mesh().boundaryMesh()[patchI];
      const labelList& meshPoints = patch.meshPoints();
      const labelListList& pointFaces = patch.pointFaces();
      FOR_ALL(meshPoints, meshPointI) {
        label vertI = meshPoints[meshPointI];
        const labelList& neighbours = mesh().pointCells(vertI);
        FOR_ALL(neighbours, neighbourI) {
          label cellI = neighbours[neighbourI];
          if (!nearestFace.found(cellI)) {
            const labelList& wallFaces = pointFaces[meshPointI];
            label minFaceI = -1;
            wallDistCorrected[cellI] = smallestDist
            (
              cellCentres[cellI],
              patch,
              wallFaces.size(),
              wallFaces,
              minFaceI
            );
            // Store wallCell and its nearest neighbour
            nearestFace.insert(cellI, minFaceI);
          }
        }
      }
    }
  }
}

