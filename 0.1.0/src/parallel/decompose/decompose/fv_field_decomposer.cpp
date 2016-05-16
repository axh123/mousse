// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "fv_field_decomposer.hpp"


// Constructors 
mousse::fvFieldDecomposer::patchFieldDecomposer::patchFieldDecomposer
(
  const labelUList& addressingSlice,
  const label addressingOffset
)
:
  directAddressing_{addressingSlice}
{
  FOR_ALL(directAddressing_, i) {
    // Subtract one to align addressing.
    directAddressing_[i] -= addressingOffset + 1;
  }
}


mousse::fvFieldDecomposer::processorVolPatchFieldDecomposer::
processorVolPatchFieldDecomposer
(
  const fvMesh& mesh,
  const labelUList& addressingSlice
)
:
  directAddressing_{addressingSlice.size()}
{
  const labelList& own = mesh.faceOwner();
  const labelList& neighb = mesh.faceNeighbour();
  FOR_ALL(directAddressing_, i) {
    // Subtract one to align addressing.
    label ai = mag(addressingSlice[i]) - 1;
    if (ai < neighb.size()) {
      // This is a regular face. it has been an internal face
      // of the original mesh and now it has become a face
      // on the parallel boundary.
      // Give face the value of the neighbour.
      if (addressingSlice[i] >= 0) {
        // I have the owner so use the neighbour value
        directAddressing_[i] = neighb[ai];
      } else {
        directAddressing_[i] = own[ai];
      }
    } else {
      // This is a face that used to be on a cyclic boundary
      // but has now become a parallel patch face. I cannot
      // do the interpolation properly (I would need to look
      // up the different (face) list of data), so I will
      // just grab the value from the owner cell
      directAddressing_[i] = own[ai];
    }
  }
}


mousse::fvFieldDecomposer::processorSurfacePatchFieldDecomposer::
processorSurfacePatchFieldDecomposer
(
  const labelUList& addressingSlice
)
:
  addressing_{addressingSlice.size()},
  weights_{addressingSlice.size()}
{
  FOR_ALL(addressing_, i) {
    addressing_[i].setSize(1);
    weights_[i].setSize(1);
    addressing_[i][0] = mag(addressingSlice[i]) - 1;
    weights_[i][0] = sign(addressingSlice[i]);
  }
}


mousse::fvFieldDecomposer::fvFieldDecomposer
(
  const fvMesh& completeMesh,
  const fvMesh& procMesh,
  const labelList& faceAddressing,
  const labelList& cellAddressing,
  const labelList& boundaryAddressing
)
:
  completeMesh_{completeMesh},
  procMesh_{procMesh},
  faceAddressing_{faceAddressing},
  cellAddressing_{cellAddressing},
  boundaryAddressing_{boundaryAddressing},
  patchFieldDecomposerPtrs_
  {
    procMesh_.boundary().size(),
    static_cast<patchFieldDecomposer*>(NULL)
  },
  processorVolPatchFieldDecomposerPtrs_
  {
    procMesh_.boundary().size(),
    static_cast<processorVolPatchFieldDecomposer*>(NULL)
  },
  processorSurfacePatchFieldDecomposerPtrs_
  {
    procMesh_.boundary().size(),
    static_cast<processorSurfacePatchFieldDecomposer*>(NULL)
  }
{
  FOR_ALL(boundaryAddressing_, patchi) {
    if (boundaryAddressing_[patchi] >= 0
        && !isA<processorLduInterface>(procMesh.boundary()[patchi])) {
      patchFieldDecomposerPtrs_[patchi] =
        new patchFieldDecomposer
        {
          procMesh_.boundary()[patchi].patchSlice(faceAddressing_),
          completeMesh_.boundaryMesh()[boundaryAddressing_[patchi]].start()
        };
    }
    else
    {
      processorVolPatchFieldDecomposerPtrs_[patchi] =
        new processorVolPatchFieldDecomposer
        {
          completeMesh_,
          procMesh_.boundary()[patchi].patchSlice(faceAddressing_)
        };
      processorSurfacePatchFieldDecomposerPtrs_[patchi] =
        new processorSurfacePatchFieldDecomposer
        {
          static_cast<const labelUList&>
          (
            procMesh_.boundary()[patchi].patchSlice(faceAddressing_)
          )
        };
    }
  }
}


// Destructor 
mousse::fvFieldDecomposer::~fvFieldDecomposer()
{
  FOR_ALL(patchFieldDecomposerPtrs_, patchi) {
    if (patchFieldDecomposerPtrs_[patchi]) {
      delete patchFieldDecomposerPtrs_[patchi];
    }
  }
  FOR_ALL(processorVolPatchFieldDecomposerPtrs_, patchi) {
    if (processorVolPatchFieldDecomposerPtrs_[patchi]) {
      delete processorVolPatchFieldDecomposerPtrs_[patchi];
    }
  }
  FOR_ALL(processorSurfacePatchFieldDecomposerPtrs_, patchi) {
    if (processorSurfacePatchFieldDecomposerPtrs_[patchi]) {
      delete processorSurfacePatchFieldDecomposerPtrs_[patchi];
    }
  }
}

