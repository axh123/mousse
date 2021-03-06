// mousse: CFD toolbox
// Copyright (C) 2012-2013 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "poly_mesh_tools.hpp"
#include "sync_tools.hpp"
#include "pyramid_point_face_ref.hpp"
#include "primitive_mesh_tools.hpp"
#include "poly_mesh_tools.hpp"


// Member Functions 
mousse::tmp<mousse::scalarField> mousse::polyMeshTools::faceOrthogonality
(
  const polyMesh& mesh,
  const vectorField& areas,
  const vectorField& cc
)
{
  const labelList& own = mesh.faceOwner();
  const labelList& nei = mesh.faceNeighbour();
  const polyBoundaryMesh& pbm = mesh.boundaryMesh();
  tmp<scalarField> tortho{new scalarField{mesh.nFaces(), 1.0}};
  scalarField& ortho = tortho();
  // Internal faces
  FOR_ALL(nei, faceI) {
    ortho[faceI] = primitiveMeshTools::faceOrthogonality
    (
      cc[own[faceI]],
      cc[nei[faceI]],
      areas[faceI]
    );
  }
  // Coupled faces
  pointField neighbourCc;
  syncTools::swapBoundaryCellPositions(mesh, cc, neighbourCc);
  FOR_ALL(pbm, patchI) {
    const polyPatch& pp = pbm[patchI];
    if (pp.coupled()) {
      FOR_ALL(pp, i) {
        label faceI = pp.start() + i;
        label bFaceI = faceI - mesh.nInternalFaces();
        ortho[faceI] = primitiveMeshTools::faceOrthogonality
        (
          cc[own[faceI]],
          neighbourCc[bFaceI],
          areas[faceI]
        );
      }
    }
  }
  return tortho;
}


mousse::tmp<mousse::scalarField> mousse::polyMeshTools::faceSkewness
(
  const polyMesh& mesh,
  const pointField& p,
  const vectorField& fCtrs,
  const vectorField& fAreas,
  const vectorField& cellCtrs
)
{
  const labelList& own = mesh.faceOwner();
  const labelList& nei = mesh.faceNeighbour();
  const polyBoundaryMesh& pbm = mesh.boundaryMesh();
  tmp<scalarField> tskew{new scalarField{mesh.nFaces()}};
  scalarField& skew = tskew();
  FOR_ALL(nei, faceI) {
    skew[faceI] = primitiveMeshTools::faceSkewness
    (
      mesh,
      p,
      fCtrs,
      fAreas,
      faceI,
      cellCtrs[own[faceI]],
      cellCtrs[nei[faceI]]
    );
  }
  // Boundary faces: consider them to have only skewness error.
  // (i.e. treat as if mirror cell on other side)
  pointField neighbourCc;
  syncTools::swapBoundaryCellPositions(mesh, cellCtrs, neighbourCc);
  FOR_ALL(pbm, patchI) {
    const polyPatch& pp = pbm[patchI];
    if (pp.coupled()) {
      FOR_ALL(pp, i) {
        label faceI = pp.start() + i;
        label bFaceI = faceI - mesh.nInternalFaces();
        skew[faceI] = primitiveMeshTools::faceSkewness
        (
          mesh,
          p,
          fCtrs,
          fAreas,
          faceI,
          cellCtrs[own[faceI]],
          neighbourCc[bFaceI]
        );
      }
    } else {
      FOR_ALL(pp, i) {
        label faceI = pp.start() + i;
        skew[faceI] = primitiveMeshTools::boundaryFaceSkewness
        (
          mesh,
          p,
          fCtrs,
          fAreas,
          faceI,
          cellCtrs[own[faceI]]
        );
      }
    }
  }
  return tskew;
}


mousse::tmp<mousse::scalarField> mousse::polyMeshTools::faceWeights
(
  const polyMesh& mesh,
  const vectorField& fCtrs,
  const vectorField& fAreas,
  const vectorField& cellCtrs
)
{
  const labelList& own = mesh.faceOwner();
  const labelList& nei = mesh.faceNeighbour();
  const polyBoundaryMesh& pbm = mesh.boundaryMesh();
  tmp<scalarField> tweight{new scalarField{mesh.nFaces(), 1.0}};
  scalarField& weight = tweight();
  // Internal faces
  FOR_ALL(nei, faceI) {
    const point& fc = fCtrs[faceI];
    const vector& fa = fAreas[faceI];
    scalar dOwn = mag(fa & (fc-cellCtrs[own[faceI]]));
    scalar dNei = mag(fa & (cellCtrs[nei[faceI]]-fc));
    weight[faceI] = min(dNei,dOwn)/(dNei+dOwn+VSMALL);
  }
  // Coupled faces
  pointField neiCc;
  syncTools::swapBoundaryCellPositions(mesh, cellCtrs, neiCc);
  FOR_ALL(pbm, patchI) {
    const polyPatch& pp = pbm[patchI];
    if (pp.coupled()) {
      FOR_ALL(pp, i) {
        label faceI = pp.start() + i;
        label bFaceI = faceI - mesh.nInternalFaces();
        const point& fc = fCtrs[faceI];
        const vector& fa = fAreas[faceI];
        scalar dOwn = mag(fa & (fc-cellCtrs[own[faceI]]));
        scalar dNei = mag(fa & (neiCc[bFaceI]-fc));
        weight[faceI] = min(dNei,dOwn)/(dNei+dOwn+VSMALL);
      }
    }
  }
  return tweight;
}


mousse::tmp<mousse::scalarField> mousse::polyMeshTools::volRatio
(
  const polyMesh& mesh,
  const scalarField& vol
)
{
  const labelList& own = mesh.faceOwner();
  const labelList& nei = mesh.faceNeighbour();
  const polyBoundaryMesh& pbm = mesh.boundaryMesh();
  tmp<scalarField> tratio{new scalarField{mesh.nFaces(), 1.0}};
  scalarField& ratio = tratio();
  // Internal faces
  FOR_ALL(nei, faceI) {
    scalar volOwn = vol[own[faceI]];
    scalar volNei = vol[nei[faceI]];
    ratio[faceI] = min(volOwn,volNei)/(max(volOwn, volNei)+VSMALL);
  }
  // Coupled faces
  scalarField neiVol;
  syncTools::swapBoundaryCellList(mesh, vol, neiVol);
  FOR_ALL(pbm, patchI) {
    const polyPatch& pp = pbm[patchI];
    if (pp.coupled()) {
      FOR_ALL(pp, i) {
        label faceI = pp.start() + i;
        label bFaceI = faceI - mesh.nInternalFaces();
        scalar volOwn = vol[own[faceI]];
        scalar volNei = neiVol[bFaceI];
        ratio[faceI] = min(volOwn,volNei)/(max(volOwn, volNei)+VSMALL);
      }
    }
  }
  return tratio;
}

