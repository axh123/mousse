// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "block_mesh.hpp"


// Private Member Functions 
void mousse::blockMesh::checkBlockMesh(const polyMesh& bm) const
{
  if (verboseOutput) {
    Info << nl << "Check topology" << endl;
  }
  bool ok = true;
  const pointField& points = bm.points();
  const faceList& faces = bm.faces();
  const cellList& cells = bm.cells();
  const polyPatchList& patches = bm.boundaryMesh();
  label nBoundaryFaces = 0;
  FOR_ALL(cells, celli) {
    nBoundaryFaces += cells[celli].nFaces();
  }
  nBoundaryFaces -= 2*bm.nInternalFaces();
  label nDefinedBoundaryFaces = 0;
  FOR_ALL(patches, patchi) {
    nDefinedBoundaryFaces += patches[patchi].size();
  }
  if (verboseOutput) {
    Info << nl << tab << "Basic statistics" << nl
      << tab << tab << "Number of internal faces : "
      << bm.nInternalFaces() << nl
      << tab << tab << "Number of boundary faces : "
      << nBoundaryFaces << nl
      << tab << tab << "Number of defined boundary faces : "
      << nDefinedBoundaryFaces << nl
      << tab << tab << "Number of undefined boundary faces : "
      << nBoundaryFaces - nDefinedBoundaryFaces << nl;
    if ((nBoundaryFaces - nDefinedBoundaryFaces) > 0) {
      Info << tab << tab << tab
        << "(Warning : only leave undefined the front and back planes "
        << "of 2D planar geometries!)" << endl;
    }
    Info << tab << "Checking patch -> block consistency" << endl;
  }
  FOR_ALL(patches, patchi) {
    const faceList& Patch = patches[patchi];
    FOR_ALL(Patch, patchFacei) {
      const face& patchFace = Patch[patchFacei];
      bool patchFaceOK = false;
      FOR_ALL(cells, celli) {
        const labelList& cellFaces = cells[celli];
        FOR_ALL(cellFaces, cellFacei) {
          if (patchFace == faces[cellFaces[cellFacei]]) {
            patchFaceOK = true;
            if ((patchFace.normal(points)
                 & faces[cellFaces[cellFacei]].normal(points)) < 0.0) {
              Info << tab << tab
                << "Face " << patchFacei
                << " of patch " << patchi
                << " (" << patches[patchi].name() << ")"
                << " points inwards"
                << endl;
              ok = false;
            }
          }
        }
      }
      if (!patchFaceOK) {
        Info << tab << tab
          << "Face " << patchFacei
          << " of patch " << patchi
          << " (" << patches[patchi].name() << ")"
          << " does not match any block faces" << endl;
        ok = false;
      }
    }
  }
  if (verboseOutput) {
    Info << endl;
  }
  if (!ok) {
    FATAL_ERROR_IN("blockMesh::checkBlockMesh(const polyMesh& bm)")
      << "Block mesh topology incorrect, stopping mesh generation!"
      << exit(FatalError);
  }
}


bool mousse::blockMesh::blockLabelsOK
(
  const label blockLabel,
  const pointField& points,
  const cellShape& blockShape
) const
{
  bool ok = true;
  FOR_ALL(blockShape, blockI) {
    if (blockShape[blockI] < 0) {
      ok = false;
      WARNING_IN
      (
        "bool mousse::blockMesh::blockLabelsOK(...)"
      )
      << "out-of-range point label " << blockShape[blockI]
      << " (min = 0"
      << ") in block " << blockLabel << endl;
    } else if (blockShape[blockI] >= points.size()) {
      ok = false;
      WARNING_IN
      (
        "bool mousse::blockMesh::blockLabelsOK(...)"
      )
      << "out-of-range point label " << blockShape[blockI]
      << " (max = " << points.size() - 1
      << ") in block " << blockLabel << endl;
    }
  }
  return ok;
}


bool mousse::blockMesh::patchLabelsOK
(
  const label patchLabel,
  const pointField& points,
  const faceList& patchFaces
) const
{
  bool ok = true;
  FOR_ALL(patchFaces, faceI) {
    const labelList& f = patchFaces[faceI];
    FOR_ALL(f, fp) {
      if (f[fp] < 0) {
        ok = false;
        WARNING_IN
        (
          "bool mousse::blockMesh::patchLabelsOK(...)"
        )
        << "out-of-range point label " << f[fp]
        << " (min = 0"
        << ") on patch " << patchLabel
        << ", face " << faceI << endl;
      } else if (f[fp] >= points.size()) {
        ok = false;
        WARNING_IN
        (
          "bool mousse::blockMesh::patchLabelsOK(...)"
        )
        << "out-of-range point label " << f[fp]
        << " (max = " << points.size() - 1
        << ") on patch " << patchLabel
        << ", face " << faceI << endl;
      }
    }
  }
  return ok;
}

