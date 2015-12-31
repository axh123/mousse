// mousse: CFD toolbox
// Copyright (C) 2011-2013 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "cfc_face_to_cell_stencil.hpp"
#include "sync_tools.hpp"
#include "empty_poly_patch.hpp"
#include "dummy_transform.hpp"
// Private Member Functions 
// Calculates per face the neighbour data (= faces of cell). Leaves out the
// face itself since this is already in stencil.
void mousse::CFCFaceToCellStencil::calcFaceBoundaryData
(
  labelListList& neiGlobal
) const
{
  const polyBoundaryMesh& patches = mesh().boundaryMesh();
  const label nBnd = mesh().nFaces()-mesh().nInternalFaces();
  const labelList& own = mesh().faceOwner();
  neiGlobal.setSize(nBnd);
  forAll(patches, patchI)
  {
    const polyPatch& pp = patches[patchI];
    label faceI = pp.start();
    if (pp.coupled())
    {
      // For coupled faces get the faces of the cell on the other side
      forAll(pp, i)
      {
        const labelList& cFaces = mesh().cells()[own[faceI]];
        labelList& globFaces = neiGlobal[faceI-mesh().nInternalFaces()];
        globFaces.setSize(cFaces.size()-1);
        label globI = 0;
        forAll(cFaces, j)
        {
          if (cFaces[j] != faceI)
          {
            globFaces[globI++] = globalNumbering().toGlobal
            (
              cFaces[j]
            );
          }
        }
        faceI++;
      }
    }
    else if (isA<emptyPolyPatch>(pp))
    {
      // Do nothing.
    }
    else
    {
      // Do nothing since face itself already in stencil
    }
  }
  //syncTools::swapBoundaryFaceList(mesh(), neiGlobal);
  syncTools::syncBoundaryFaceList
  (
    mesh(),
    neiGlobal,
    eqOp<labelList>(),
    dummyTransform()
  );
}
// Calculates per cell the neighbour data (= cell or boundary in global
// numbering). First element is always cell itself!
void mousse::CFCFaceToCellStencil::calcCellStencil(labelListList& globalCellFaces)
const
{
  const label nBnd = mesh().nFaces()-mesh().nInternalFaces();
  const labelList& own = mesh().faceOwner();
  const labelList& nei = mesh().faceNeighbour();
  // Calculate faces of coupled neighbour (in global numbering)
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  labelListList neiGlobal(nBnd);
  calcFaceBoundaryData(neiGlobal);
  // Non-empty boundary faces
  boolList validBFace(mesh().nFaces()-mesh().nInternalFaces(), true);
  const polyBoundaryMesh& patches = mesh().boundaryMesh();
  forAll(patches, patchI)
  {
    const polyPatch& pp = patches[patchI];
    if (isA<emptyPolyPatch>(pp))
    {
      label bFaceI = pp.start()-mesh().nInternalFaces();
      forAll(pp, i)
      {
        validBFace[bFaceI++] = false;
      }
    }
  }
  // Determine faces of cellCells in global numbering
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  DynamicList<label> allGlobalFaces(100);
  globalCellFaces.setSize(mesh().nCells());
  forAll(globalCellFaces, cellI)
  {
    const cell& cFaces = mesh().cells()[cellI];
    allGlobalFaces.clear();
    // My faces first
    forAll(cFaces, i)
    {
      label faceI = cFaces[i];
      if
      (
        mesh().isInternalFace(faceI)
      || validBFace[faceI-mesh().nInternalFaces()]
      )
      {
        allGlobalFaces.append(globalNumbering().toGlobal(faceI));
      }
    }
    // faces of neighbouring cells second
    forAll(cFaces, i)
    {
      label faceI = cFaces[i];
      if (mesh().isInternalFace(faceI))
      {
        label nbrCellI = own[faceI];
        if (nbrCellI == cellI)
        {
          nbrCellI = nei[faceI];
        }
        const cell& nbrFaces = mesh().cells()[nbrCellI];
        forAll(nbrFaces, j)
        {
          label nbrFaceI = nbrFaces[j];
          if
          (
            mesh().isInternalFace(nbrFaceI)
          || validBFace[nbrFaceI-mesh().nInternalFaces()]
          )
          {
            label nbrGlobalI = globalNumbering().toGlobal(nbrFaceI);
            // Check if already there. Note:should use hashset?
            if (findIndex(allGlobalFaces, nbrGlobalI) == -1)
            {
              allGlobalFaces.append(nbrGlobalI);
            }
          }
        }
      }
      else
      {
        const labelList& nbrGlobalFaces =
          neiGlobal[faceI-mesh().nInternalFaces()];
        forAll(nbrGlobalFaces, j)
        {
          label nbrGlobalI = nbrGlobalFaces[j];
          // Check if already there. Note:should use hashset?
          if (findIndex(allGlobalFaces, nbrGlobalI) == -1)
          {
            allGlobalFaces.append(nbrGlobalI);
          }
        }
      }
    }
    globalCellFaces[cellI] = allGlobalFaces;
    //Pout<< "** cell:" << cellI
    //    << " at:" << mesh().cellCentres()[cellI]
    //    << endl;
    //const labelList& globalFaces = globalCellFaces[cellI];
    //forAll(globalFaces, i)
    //{
    //    label faceI = globalNumbering().toLocal(globalFaces[i]);
    //    Pout<< "    face:" << faceI
    //        << " at:" << mesh().faceCentres()[faceI]
    //        << endl;
    //}
  }
}
// Constructors 
mousse::CFCFaceToCellStencil::CFCFaceToCellStencil(const polyMesh& mesh)
:
  faceToCellStencil(mesh)
{
  // Calculate per cell the (face) connected cells (in global numbering)
  calcCellStencil(*this);
}
