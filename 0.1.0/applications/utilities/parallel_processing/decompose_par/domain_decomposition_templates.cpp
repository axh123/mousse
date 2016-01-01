// mousse: CFD toolbox
// Copyright (C) 2014 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "cyclic_poly_patch.hpp"
// Private Member Functions 
template <class BinaryOp>
void mousse::domainDecomposition::processInterCyclics
(
  const polyBoundaryMesh& patches,
  List<DynamicList<DynamicList<label> > >& interPatchFaces,
  List<Map<label> >& procNbrToInterPatch,
  List<labelListList>& subPatchIDs,
  List<labelListList>& subPatchStarts,
  bool owner,
  BinaryOp bop
) const
{
  // Processor boundaries from split cyclics
  forAll(patches, patchi)
  {
    if (isA<cyclicPolyPatch>(patches[patchi]))
    {
      const cyclicPolyPatch& pp = refCast<const cyclicPolyPatch>
      (
        patches[patchi]
      );
      if (pp.owner() != owner)
      {
        continue;
      }
      // cyclic: check opposite side on this processor
      const labelUList& patchFaceCells = pp.faceCells();
      const labelUList& nbrPatchFaceCells =
        pp.neighbPatch().faceCells();
      // Store old sizes. Used to detect which inter-proc patches
      // have been added to.
      labelListList oldInterfaceSizes(nProcs_);
      forAll(oldInterfaceSizes, procI)
      {
        labelList& curOldSizes = oldInterfaceSizes[procI];
        curOldSizes.setSize(interPatchFaces[procI].size());
        forAll(curOldSizes, interI)
        {
          curOldSizes[interI] =
            interPatchFaces[procI][interI].size();
        }
      }
      // Add faces with different owner and neighbour processors
      forAll(patchFaceCells, facei)
      {
        const label ownerProc = cellToProc_[patchFaceCells[facei]];
        const label nbrProc = cellToProc_[nbrPatchFaceCells[facei]];
        if (bop(ownerProc, nbrProc))
        {
          // inter - processor patch face found.
          addInterProcFace
          (
            pp.start()+facei,
            ownerProc,
            nbrProc,
            procNbrToInterPatch,
            interPatchFaces
          );
        }
      }
      // 1. Check if any faces added to existing interfaces
      forAll(oldInterfaceSizes, procI)
      {
        const labelList& curOldSizes = oldInterfaceSizes[procI];
        forAll(curOldSizes, interI)
        {
          label oldSz = curOldSizes[interI];
          if (interPatchFaces[procI][interI].size() > oldSz)
          {
            // Added faces to this interface. Add an entry
            append(subPatchIDs[procI][interI], patchi);
            append(subPatchStarts[procI][interI], oldSz);
          }
        }
      }
      // 2. Any new interfaces
      forAll(subPatchIDs, procI)
      {
        label nIntfcs = interPatchFaces[procI].size();
        subPatchIDs[procI].setSize(nIntfcs, labelList(1, patchi));
        subPatchStarts[procI].setSize(nIntfcs, labelList(1, label(0)));
      }
    }
  }
}
