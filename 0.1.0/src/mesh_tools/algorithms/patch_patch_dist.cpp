// mousse: CFD toolbox
// Copyright (C) 2011-2013 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "patch_patch_dist.hpp"
#include "patch_edge_face_wave.hpp"
#include "sync_tools.hpp"
#include "poly_mesh.hpp"
#include "patch_edge_face_info.hpp"
#include "pstream_reduce_ops.hpp"


// Constructors
mousse::patchPatchDist::patchPatchDist
(
  const polyPatch& patch,
  const labelHashSet& nbrPatchIDs
)
:
  patch_{patch},
  nbrPatchIDs_{nbrPatchIDs},
  nUnset_{0}
{
  patchPatchDist::correct();
}


// Destructor
mousse::patchPatchDist::~patchPatchDist()
{}


// Member Functions
void mousse::patchPatchDist::correct()
{
  // Mark all edge connected to a nbrPatch.
  label nBnd = 0;
  FOR_ALL_CONST_ITER(labelHashSet, nbrPatchIDs_, iter) {
    label nbrPatchI = iter.key();
    const polyPatch& nbrPatch = patch_.boundaryMesh()[nbrPatchI];
    nBnd += nbrPatch.nEdges() - nbrPatch.nInternalEdges();
  }
  // Mark all edges. Note: should use HashSet but have no syncTools
  // functionality for these.
  EdgeMap<label> nbrEdges{2*nBnd};
  FOR_ALL_CONST_ITER(labelHashSet, nbrPatchIDs_, iter) {
    label nbrPatchI = iter.key();
    const polyPatch& nbrPatch = patch_.boundaryMesh()[nbrPatchI];
    const labelList& nbrMp = nbrPatch.meshPoints();
    for
    (
      label edgeI = nbrPatch.nInternalEdges();
      edgeI < nbrPatch.nEdges();
      edgeI++
    ) {
      const edge& e = nbrPatch.edges()[edgeI];
      const edge meshE = edge(nbrMp[e[0]], nbrMp[e[1]]);
      nbrEdges.insert(meshE, nbrPatchI);
    }
  }
  // Make sure these boundary edges are marked everywhere.
  syncTools::syncEdgeMap
  (
    patch_.boundaryMesh().mesh(),
    nbrEdges,
    maxEqOp<label>()
  );
  // Data on all edges and faces
  List<patchEdgeFaceInfo> allEdgeInfo{patch_.nEdges()};
  List<patchEdgeFaceInfo> allFaceInfo{patch_.size()};
  // Initial seed
  label nBndEdges = patch_.nEdges() - patch_.nInternalEdges();
  DynamicList<label> initialEdges{2*nBndEdges};
  DynamicList<patchEdgeFaceInfo> initialEdgesInfo{2*nBndEdges};
  // Seed all my edges that are also nbrEdges
  const labelList& mp = patch_.meshPoints();
  for
  (
    label edgeI = patch_.nInternalEdges();
    edgeI < patch_.nEdges();
    edgeI++
  ) {
    const edge& e = patch_.edges()[edgeI];
    const edge meshE = edge(mp[e[0]], mp[e[1]]);
    EdgeMap<label>::const_iterator edgeFnd = nbrEdges.find(meshE);
    if (edgeFnd != nbrEdges.end()) {
      initialEdges.append(edgeI);
      initialEdgesInfo.append
      (
        // patchEdgeFaceInfo
        {
          e.centre(patch_.localPoints()),
          0.0
        }
      );
    }
  }
  // Walk
  PatchEdgeFaceWave
  <
    primitivePatch,
    patchEdgeFaceInfo
  > calc
  (
    patch_.boundaryMesh().mesh(),
    patch_,
    initialEdges,
    initialEdgesInfo,
    allEdgeInfo,
    allFaceInfo,
    returnReduce(patch_.nEdges(), sumOp<label>())
  );
  // Extract into *this
  setSize(patch_.size());
  nUnset_ = 0;
  FOR_ALL(allFaceInfo, faceI) {
    if (allFaceInfo[faceI].valid(calc.data())) {
      operator[](faceI) =  mousse::sqrt(allFaceInfo[faceI].distSqr());
    } else {
      nUnset_++;
    }
  }
}

