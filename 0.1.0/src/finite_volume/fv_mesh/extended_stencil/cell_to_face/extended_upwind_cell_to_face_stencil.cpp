// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "extended_upwind_cell_to_face_stencil.hpp"
#include "cell_to_face_stencil.hpp"
#include "sync_tools.hpp"
#include "sortable_list.hpp"
#include "dummy_transform.hpp"
#include "empty_poly_patch.hpp"

// Private Member Functions
void mousse::extendedUpwindCellToFaceStencil::selectOppositeFaces
(
  const boolList& nonEmptyFace,
  const scalar minOpposedness,
  const label faceI,
  const label cellI,
  DynamicList<label>& oppositeFaces
) const
{
  const vectorField& areas = mesh_.faceAreas();
  const labelList& own = mesh_.faceOwner();
  const cell& cFaces = mesh_.cells()[cellI];
  SortableList<scalar> opposedness(cFaces.size(), -GREAT);
  // Pick up all the faces that oppose this one.
  FOR_ALL(cFaces, i)
  {
    label otherFaceI = cFaces[i];
    if (otherFaceI != faceI && nonEmptyFace[otherFaceI])
    {
      if ((own[otherFaceI] == cellI) == (own[faceI] == cellI))
      {
        opposedness[i] = -(areas[otherFaceI] & areas[faceI]);
      }
      else
      {
        opposedness[i] = (areas[otherFaceI] & areas[faceI]);
      }
    }
  }
  label sz = opposedness.size();
  oppositeFaces.clear();
  scalar myAreaSqr = magSqr(areas[faceI]);
  if (myAreaSqr > VSMALL)
  {
    FOR_ALL(opposedness, i)
    {
      opposedness[i] /= myAreaSqr;
    }
    // Sort in incrementing order
    opposedness.sort();
    // Pick largest no matter what
    oppositeFaces.append(cFaces[opposedness.indices()[sz-1]]);
    for (label i = sz-2; i >= 0; --i)
    {
      if (opposedness[i] < minOpposedness)
      {
        break;
      }
      oppositeFaces.append(cFaces[opposedness.indices()[i]]);
    }
  }
  else
  {
    // Sort in incrementing order
    opposedness.sort();
    // Tiny face. Do what?
    // Pick largest no matter what
    oppositeFaces.append(cFaces[opposedness.indices()[sz-1]]);
  }
}
void mousse::extendedUpwindCellToFaceStencil::transportStencil
(
  const boolList& nonEmptyFace,
  const labelListList& faceStencil,
  const scalar minOpposedness,
  const label faceI,
  const label cellI,
  const bool stencilHasNeighbour,
  DynamicList<label>& oppositeFaces,
  labelHashSet& faceStencilSet,
  labelList& transportedStencil
) const
{
  label globalOwn = faceStencil[faceI][0];
  label globalNei = -1;
  if (stencilHasNeighbour && faceStencil[faceI].size() >= 2)
  {
    globalNei = faceStencil[faceI][1];
  }
  selectOppositeFaces
  (
    nonEmptyFace,
    minOpposedness,
    faceI,
    cellI,
    oppositeFaces
  );
  // Collect all stencils of oppositefaces
  faceStencilSet.clear();
  FOR_ALL(oppositeFaces, i)
  {
    const labelList& fStencil = faceStencil[oppositeFaces[i]];
    FOR_ALL(fStencil, j)
    {
      label globalI = fStencil[j];
      if (globalI != globalOwn && globalI != globalNei)
      {
        faceStencilSet.insert(globalI);
      }
    }
  }
  // Add my owner and neighbour first.
  if (stencilHasNeighbour)
  {
    transportedStencil.setSize(faceStencilSet.size()+2);
    label n = 0;
    transportedStencil[n++] = globalOwn;
    transportedStencil[n++] = globalNei;
    FOR_ALL_CONST_ITER(labelHashSet, faceStencilSet, iter)
    {
      if (iter.key() != globalOwn && iter.key() != globalNei)
      {
        transportedStencil[n++] = iter.key();
      }
    }
    if (n != transportedStencil.size())
    {
      FATAL_ERROR_IN
      (
        "extendedUpwindCellToFaceStencil::transportStencil(..)"
      )
      << "problem:" << faceStencilSet
      << abort(FatalError);
    }
  }
  else
  {
    transportedStencil.setSize(faceStencilSet.size()+1);
    label n = 0;
    transportedStencil[n++] = globalOwn;
    FOR_ALL_CONST_ITER(labelHashSet, faceStencilSet, iter)
    {
      if (iter.key() != globalOwn)
      {
        transportedStencil[n++] = iter.key();
      }
    }
    if (n != transportedStencil.size())
    {
      FATAL_ERROR_IN
      (
        "extendedUpwindCellToFaceStencil::transportStencil(..)"
      )
      << "problem:" << faceStencilSet
      << abort(FatalError);
    }
  }
}
void mousse::extendedUpwindCellToFaceStencil::transportStencils
(
  const labelListList& faceStencil,
  const scalar minOpposedness,
  labelListList& ownStencil,
  labelListList& neiStencil
)
{
  const polyBoundaryMesh& patches = mesh_.boundaryMesh();
  const label nBnd = mesh_.nFaces()-mesh_.nInternalFaces();
  const labelList& own = mesh_.faceOwner();
  const labelList& nei = mesh_.faceNeighbour();
  // Work arrays
  DynamicList<label> oppositeFaces;
  labelHashSet faceStencilSet;
  // For quick detection of empty faces
  boolList nonEmptyFace(mesh_.nFaces(), true);
  FOR_ALL(patches, patchI)
  {
    const polyPatch& pp = patches[patchI];
    if (isA<emptyPolyPatch>(pp))
    {
      label faceI = pp.start();
      FOR_ALL(pp, i)
      {
        nonEmptyFace[faceI++] = false;
      }
    }
  }
  // Do the owner side
  // ~~~~~~~~~~~~~~~~~
  // stencil is synchronised at entry so no need to swap.
  ownStencil.setSize(mesh_.nFaces());
  // Internal faces
  for (label faceI = 0; faceI < mesh_.nInternalFaces(); faceI++)
  {
    // Get stencil as owner + neighbour + stencil from 'opposite' faces
    transportStencil
    (
      nonEmptyFace,
      faceStencil,
      minOpposedness,
      faceI,
      own[faceI],
      true,                   //stencilHasNeighbour
      oppositeFaces,
      faceStencilSet,
      ownStencil[faceI]
    );
  }
  // Boundary faces
  FOR_ALL(patches, patchI)
  {
    const polyPatch& pp = patches[patchI];
    label faceI = pp.start();
    if (pp.coupled())
    {
      FOR_ALL(pp, i)
      {
        transportStencil
        (
          nonEmptyFace,
          faceStencil,
          minOpposedness,
          faceI,
          own[faceI],
          true,                   //stencilHasNeighbour
          oppositeFaces,
          faceStencilSet,
          ownStencil[faceI]
        );
        faceI++;
      }
    }
    else if (!isA<emptyPolyPatch>(pp))
    {
      FOR_ALL(pp, i)
      {
        // faceStencil does not contain neighbour
        transportStencil
        (
          nonEmptyFace,
          faceStencil,
          minOpposedness,
          faceI,
          own[faceI],
          false,                  //stencilHasNeighbour
          oppositeFaces,
          faceStencilSet,
          ownStencil[faceI]
        );
        faceI++;
      }
    }
  }
  // Swap coupled boundary stencil
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  labelListList neiBndStencil(nBnd);
  for (label faceI = mesh_.nInternalFaces(); faceI < mesh_.nFaces(); faceI++)
  {
    neiBndStencil[faceI-mesh_.nInternalFaces()] = ownStencil[faceI];
  }
  //syncTools::swapBoundaryFaceList(mesh_, neiBndStencil);
  syncTools::syncBoundaryFaceList
  (
    mesh_,
    neiBndStencil,
    eqOp<labelList>(),
    dummyTransform()
  );
  // Do the neighbour side
  // ~~~~~~~~~~~~~~~~~~~~~
  // - internal faces : get opposite faces on neighbour side
  // - boundary faces : empty
  // - coupled faces  : in neiBndStencil
  neiStencil.setSize(mesh_.nFaces());
  // Internal faces
  for (label faceI = 0; faceI < mesh_.nInternalFaces(); faceI++)
  {
    transportStencil
    (
      nonEmptyFace,
      faceStencil,
      minOpposedness,
      faceI,
      nei[faceI],
      true,                   //stencilHasNeighbour
      oppositeFaces,
      faceStencilSet,
      neiStencil[faceI]
    );
  }
  // Boundary faces
  FOR_ALL(patches, patchI)
  {
    const polyPatch& pp = patches[patchI];
    label faceI = pp.start();
    if (pp.coupled())
    {
      FOR_ALL(pp, i)
      {
        neiStencil[faceI].transfer
        (
          neiBndStencil[faceI-mesh_.nInternalFaces()]
        );
        faceI++;
      }
    }
    else
    {
      // Boundary has empty neighbour stencil
    }
  }
}

// Constructors
mousse::extendedUpwindCellToFaceStencil::extendedUpwindCellToFaceStencil
(
  const cellToFaceStencil& stencil,
  const bool pureUpwind,
  const scalar minOpposedness
)
:
  extendedCellToFaceStencil(stencil.mesh()),
  pureUpwind_(pureUpwind)
{
  //FOR_ALL(stencil, faceI)
  //{
  //    const labelList& fCells = stencil[faceI];
  //
  //    Pout<< "Face:" << faceI << " at:" << mesh_.faceCentres()[faceI]
  //        << endl;
  //
  //    FOR_ALL(fCells, i)
  //    {
  //        label globalI = fCells[i];
  //
  //        if (globalI < mesh_.nCells())
  //        {
  //            Pout<< "    cell:" << globalI
  //                << " at:" << mesh_.cellCentres()[globalI] << endl;
  //        }
  //        else
  //        {
  //            label faceI = globalI-mesh_.nCells() + mesh_.nInternalFaces();
  //
  //            Pout<< "    boundary:" << faceI
  //                << " at:" << mesh_.faceCentres()[faceI] << endl;
  //        }
  //    }
  //}
  //Pout<< endl << endl;
  // Transport centred stencil to upwind/downwind face
  transportStencils
  (
    stencil,
    minOpposedness,
    ownStencil_,
    neiStencil_
  );
  {
    List<Map<label> > compactMap(Pstream::nProcs());
    ownMapPtr_.reset
    (
      new mapDistribute
      (
        stencil.globalNumbering(),
        ownStencil_,
        compactMap
      )
    );
  }
  {
    List<Map<label> > compactMap(Pstream::nProcs());
    neiMapPtr_.reset
    (
      new mapDistribute
      (
        stencil.globalNumbering(),
        neiStencil_,
        compactMap
      )
    );
  }
  // stencil now in compact form
  if (pureUpwind_)
  {
    const fvMesh& mesh = dynamic_cast<const fvMesh&>(stencil.mesh());
    List<List<point> > stencilPoints(ownStencil_.size());
    // Owner stencil
    // ~~~~~~~~~~~~~
    collectData(ownMapPtr_(), ownStencil_, mesh.C(), stencilPoints);
    // Mask off all stencil points on wrong side of face
    FOR_ALL(stencilPoints, faceI)
    {
      const point& fc = mesh.faceCentres()[faceI];
      const vector& fArea = mesh.faceAreas()[faceI];
      const List<point>& points = stencilPoints[faceI];
      const labelList& stencil = ownStencil_[faceI];
      DynamicList<label> newStencil(stencil.size());
      FOR_ALL(points, i)
      {
        if (((points[i]-fc) & fArea) < 0)
        {
          newStencil.append(stencil[i]);
        }
      }
      if (newStencil.size() != stencil.size())
      {
        ownStencil_[faceI].transfer(newStencil);
      }
    }
    // Neighbour stencil
    // ~~~~~~~~~~~~~~~~~
    collectData(neiMapPtr_(), neiStencil_, mesh.C(), stencilPoints);
    // Mask off all stencil points on wrong side of face
    FOR_ALL(stencilPoints, faceI)
    {
      const point& fc = mesh.faceCentres()[faceI];
      const vector& fArea = mesh.faceAreas()[faceI];
      const List<point>& points = stencilPoints[faceI];
      const labelList& stencil = neiStencil_[faceI];
      DynamicList<label> newStencil(stencil.size());
      FOR_ALL(points, i)
      {
        if (((points[i]-fc) & fArea) > 0)
        {
          newStencil.append(stencil[i]);
        }
      }
      if (newStencil.size() != stencil.size())
      {
        neiStencil_[faceI].transfer(newStencil);
      }
    }
    // Note: could compact schedule as well. for if cells are not needed
    // across any boundary anymore. However relatively rare.
  }
}
mousse::extendedUpwindCellToFaceStencil::extendedUpwindCellToFaceStencil
(
  const cellToFaceStencil& stencil
)
:
  extendedCellToFaceStencil(stencil.mesh()),
  pureUpwind_(true)
{
  // Calculate stencil points with full stencil
  ownStencil_ = stencil;
  {
    List<Map<label> > compactMap(Pstream::nProcs());
    ownMapPtr_.reset
    (
      new mapDistribute
      (
        stencil.globalNumbering(),
        ownStencil_,
        compactMap
      )
    );
  }
  const fvMesh& mesh = dynamic_cast<const fvMesh&>(stencil.mesh());
  List<List<point> > stencilPoints(ownStencil_.size());
  collectData(ownMapPtr_(), ownStencil_, mesh.C(), stencilPoints);
  // Split stencil into owner and neighbour
  neiStencil_.setSize(ownStencil_.size());
  FOR_ALL(stencilPoints, faceI)
  {
    const point& fc = mesh.faceCentres()[faceI];
    const vector& fArea = mesh.faceAreas()[faceI];
    const List<point>& points = stencilPoints[faceI];
    const labelList& stencil = ownStencil_[faceI];
    DynamicList<label> newOwnStencil(stencil.size());
    DynamicList<label> newNeiStencil(stencil.size());
    FOR_ALL(points, i)
    {
      if (((points[i]-fc) & fArea) > 0)
      {
        newNeiStencil.append(stencil[i]);
      }
      else
      {
        newOwnStencil.append(stencil[i]);
      }
    }
    if (newNeiStencil.size() > 0)
    {
      ownStencil_[faceI].transfer(newOwnStencil);
      neiStencil_[faceI].transfer(newNeiStencil);
    }
  }
  // Should compact schedule. Or have both return the same schedule.
  neiMapPtr_.reset(new mapDistribute(ownMapPtr_()));
}
