// mousse: CFD toolbox
// Copyright (C) 2011-2013 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "cell_to_face_stencil.hpp"
#include "sortable_list.hpp"
#include "empty_poly_patch.hpp"
#include "sync_tools.hpp"
#include "dummy_transform.hpp"
// Private Member Functions 
// Merge two list and guarantee global0,global1 are first.
void mousse::cellToFaceStencil::merge
(
  const label global0,
  const label global1,
  const labelList& listA,
  labelList& listB
)
{
  sort(listB);
  // See if global0, global1 already present in listB
  label nGlobalInsert = 0;
  if (global0 != -1)
  {
    label index0 = findSortedIndex(listB, global0);
    if (index0 == -1)
    {
      nGlobalInsert++;
    }
  }
  if (global1 != -1)
  {
    label index1 = findSortedIndex(listB, global1);
    if (index1 == -1)
    {
      nGlobalInsert++;
    }
  }
  // For all in listA see if they are present
  label nInsert = 0;
  FOR_ALL(listA, i)
  {
    label elem = listA[i];
    if (elem != global0 && elem != global1)
    {
      if (findSortedIndex(listB, elem) == -1)
      {
        nInsert++;
      }
    }
  }
  // Extend B with nInsert and whether global0,global1 need to be inserted.
  labelList result(listB.size() + nGlobalInsert + nInsert);
  label resultI = 0;
  // Insert global0,1 first
  if (global0 != -1)
  {
    result[resultI++] = global0;
  }
  if (global1 != -1)
  {
    result[resultI++] = global1;
  }
  // Insert listB
  FOR_ALL(listB, i)
  {
    label elem = listB[i];
    if (elem != global0 && elem != global1)
    {
      result[resultI++] = elem;
    }
  }
  // Insert listA
  FOR_ALL(listA, i)
  {
    label elem = listA[i];
    if (elem != global0 && elem != global1)
    {
      if (findSortedIndex(listB, elem) == -1)
      {
        result[resultI++] = elem;
      }
    }
  }
  if (resultI != result.size())
  {
    FATAL_ERROR_IN("cellToFaceStencil::merge(..)")
      << "problem" << abort(FatalError);
  }
  listB.transfer(result);
}
// Merge two list and guarantee globalI is first.
void mousse::cellToFaceStencil::merge
(
  const label globalI,
  const labelList& pGlobals,
  labelList& cCells
)
{
  labelHashSet set;
  FOR_ALL(cCells, i)
  {
    if (cCells[i] != globalI)
    {
      set.insert(cCells[i]);
    }
  }
  FOR_ALL(pGlobals, i)
  {
    if (pGlobals[i] != globalI)
    {
      set.insert(pGlobals[i]);
    }
  }
  cCells.setSize(set.size()+1);
  label n = 0;
  cCells[n++] = globalI;
  FOR_ALL_CONST_ITER(labelHashSet, set, iter)
  {
    cCells[n++] = iter.key();
  }
}
void mousse::cellToFaceStencil::validBoundaryFaces(boolList& isValidBFace) const
{
  const polyBoundaryMesh& patches = mesh().boundaryMesh();
  isValidBFace.setSize(mesh().nFaces()-mesh().nInternalFaces(), true);
  FOR_ALL(patches, patchI)
  {
    const polyPatch& pp = patches[patchI];
    if (pp.coupled() || isA<emptyPolyPatch>(pp))
    {
      label bFaceI = pp.start()-mesh().nInternalFaces();
      FOR_ALL(pp, i)
      {
        isValidBFace[bFaceI++] = false;
      }
    }
  }
}
mousse::autoPtr<mousse::indirectPrimitivePatch>
mousse::cellToFaceStencil::allCoupledFacesPatch() const
{
  const polyBoundaryMesh& patches = mesh().boundaryMesh();
  label nCoupled = 0;
  FOR_ALL(patches, patchI)
  {
    const polyPatch& pp = patches[patchI];
    if (pp.coupled())
    {
      nCoupled += pp.size();
    }
  }
  labelList coupledFaces(nCoupled);
  nCoupled = 0;
  FOR_ALL(patches, patchI)
  {
    const polyPatch& pp = patches[patchI];
    if (pp.coupled())
    {
      label faceI = pp.start();
      FOR_ALL(pp, i)
      {
        coupledFaces[nCoupled++] = faceI++;
      }
    }
  }
  return autoPtr<indirectPrimitivePatch>
  (
    new indirectPrimitivePatch
    (
      IndirectList<face>
      (
        mesh().faces(),
        coupledFaces
      ),
      mesh().points()
    )
  );
}
void mousse::cellToFaceStencil::unionEqOp::operator()
(
  labelList& x,
  const labelList& y
) const
{
  if (y.size())
  {
    if (x.empty())
    {
      x = y;
    }
    else
    {
      labelHashSet set(x);
      FOR_ALL(y, i)
      {
        set.insert(y[i]);
      }
      x = set.toc();
    }
  }
}
void mousse::cellToFaceStencil::insertFaceCells
(
  const label exclude0,
  const label exclude1,
  const boolList& isValidBFace,
  const labelList& faceLabels,
  labelHashSet& globals
) const
{
  const labelList& own = mesh().faceOwner();
  const labelList& nei = mesh().faceNeighbour();
  FOR_ALL(faceLabels, i)
  {
    label faceI = faceLabels[i];
    label globalOwn = globalNumbering().toGlobal(own[faceI]);
    if (globalOwn != exclude0 && globalOwn != exclude1)
    {
      globals.insert(globalOwn);
    }
    if (mesh().isInternalFace(faceI))
    {
      label globalNei = globalNumbering().toGlobal(nei[faceI]);
      if (globalNei != exclude0 && globalNei != exclude1)
      {
        globals.insert(globalNei);
      }
    }
    else
    {
      label bFaceI = faceI-mesh().nInternalFaces();
      if (isValidBFace[bFaceI])
      {
        label globalI = globalNumbering().toGlobal
        (
          mesh().nCells()
         + bFaceI
        );
        if (globalI != exclude0 && globalI != exclude1)
        {
          globals.insert(globalI);
        }
      }
    }
  }
}
mousse::labelList mousse::cellToFaceStencil::calcFaceCells
(
  const boolList& isValidBFace,
  const labelList& faceLabels,
  labelHashSet& globals
) const
{
  globals.clear();
  insertFaceCells
  (
    -1,
    -1,
    isValidBFace,
    faceLabels,
    globals
  );
  return globals.toc();
}
// Calculates per face a list of global cell/face indices.
void mousse::cellToFaceStencil::calcFaceStencil
(
  const labelListList& globalCellCells,
  labelListList& faceStencil
) const
{
  const polyBoundaryMesh& patches = mesh_.boundaryMesh();
  const label nBnd = mesh_.nFaces()-mesh_.nInternalFaces();
  const labelList& own = mesh_.faceOwner();
  const labelList& nei = mesh_.faceNeighbour();
  // Determine neighbouring global cell Cells
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  labelListList neiGlobalCellCells(nBnd);
  FOR_ALL(patches, patchI)
  {
    const polyPatch& pp = patches[patchI];
    if (pp.coupled())
    {
      label faceI = pp.start();
      FOR_ALL(pp, i)
      {
        neiGlobalCellCells[faceI-mesh_.nInternalFaces()] =
          globalCellCells[own[faceI]];
        faceI++;
      }
    }
  }
  //syncTools::swapBoundaryFaceList(mesh_, neiGlobalCellCells);
  syncTools::syncBoundaryFaceList
  (
    mesh_,
    neiGlobalCellCells,
    eqOp<labelList>(),
    dummyTransform()
  );
  // Construct stencil in global numbering
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  faceStencil.setSize(mesh_.nFaces());
  labelHashSet faceStencilSet;
  for (label faceI = 0; faceI < mesh_.nInternalFaces(); faceI++)
  {
    faceStencilSet.clear();
    const labelList& ownCCells = globalCellCells[own[faceI]];
    label globalOwn = ownCCells[0];
    // Insert cellCells
    FOR_ALL(ownCCells, i)
    {
      faceStencilSet.insert(ownCCells[i]);
    }
    const labelList& neiCCells = globalCellCells[nei[faceI]];
    label globalNei = neiCCells[0];
    // Insert cellCells
    FOR_ALL(neiCCells, i)
    {
      faceStencilSet.insert(neiCCells[i]);
    }
    // Guarantee owner first, neighbour second.
    faceStencil[faceI].setSize(faceStencilSet.size());
    label n = 0;
    faceStencil[faceI][n++] = globalOwn;
    faceStencil[faceI][n++] = globalNei;
    FOR_ALL_CONST_ITER(labelHashSet, faceStencilSet, iter)
    {
      if (iter.key() != globalOwn && iter.key() != globalNei)
      {
        faceStencil[faceI][n++] = iter.key();
      }
    }
    //Pout<< "internalface:" << faceI << " toc:" << faceStencilSet.toc()
    //    << " faceStencil:" << faceStencil[faceI] << endl;
  }
  FOR_ALL(patches, patchI)
  {
    const polyPatch& pp = patches[patchI];
    label faceI = pp.start();
    if (pp.coupled())
    {
      FOR_ALL(pp, i)
      {
        faceStencilSet.clear();
        const labelList& ownCCells = globalCellCells[own[faceI]];
        label globalOwn = ownCCells[0];
        FOR_ALL(ownCCells, i)
        {
          faceStencilSet.insert(ownCCells[i]);
        }
        // And the neighbours of the coupled cell
        const labelList& neiCCells =
          neiGlobalCellCells[faceI-mesh_.nInternalFaces()];
        label globalNei = neiCCells[0];
        FOR_ALL(neiCCells, i)
        {
          faceStencilSet.insert(neiCCells[i]);
        }
        // Guarantee owner first, neighbour second.
        faceStencil[faceI].setSize(faceStencilSet.size());
        label n = 0;
        faceStencil[faceI][n++] = globalOwn;
        faceStencil[faceI][n++] = globalNei;
        FOR_ALL_CONST_ITER(labelHashSet, faceStencilSet, iter)
        {
          if (iter.key() != globalOwn && iter.key() != globalNei)
          {
            faceStencil[faceI][n++] = iter.key();
          }
        }
        //Pout<< "coupledface:" << faceI
        //    << " toc:" << faceStencilSet.toc()
        //    << " faceStencil:" << faceStencil[faceI] << endl;
        faceI++;
      }
    }
    else if (!isA<emptyPolyPatch>(pp))
    {
      FOR_ALL(pp, i)
      {
        faceStencilSet.clear();
        const labelList& ownCCells = globalCellCells[own[faceI]];
        label globalOwn = ownCCells[0];
        FOR_ALL(ownCCells, i)
        {
          faceStencilSet.insert(ownCCells[i]);
        }
        // Guarantee owner first
        faceStencil[faceI].setSize(faceStencilSet.size());
        label n = 0;
        faceStencil[faceI][n++] = globalOwn;
        FOR_ALL_CONST_ITER(labelHashSet, faceStencilSet, iter)
        {
          if (iter.key() != globalOwn)
          {
            faceStencil[faceI][n++] = iter.key();
          }
        }
        //Pout<< "boundaryface:" << faceI
        //    << " toc:" << faceStencilSet.toc()
        //    << " faceStencil:" << faceStencil[faceI] << endl;
        faceI++;
      }
    }
  }
}
// Constructors 
mousse::cellToFaceStencil::cellToFaceStencil(const polyMesh& mesh)
:
  mesh_(mesh),
  globalNumbering_(mesh_.nCells()+mesh_.nFaces()-mesh_.nInternalFaces())
{}
