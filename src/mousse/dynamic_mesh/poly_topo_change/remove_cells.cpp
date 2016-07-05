// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "remove_cells.hpp"
#include "poly_mesh.hpp"
#include "poly_topo_change.hpp"
#include "poly_remove_cell.hpp"
#include "poly_remove_face.hpp"
#include "poly_modify_face.hpp"
#include "poly_remove_point.hpp"
#include "sync_tools.hpp"


// Static Data Members
namespace mousse {

DEFINE_TYPE_NAME_AND_DEBUG(removeCells, 0);

}


// Private Member Functions 
void mousse::removeCells::uncount
(
  const labelList& f,
  labelList& nUsage
)
{
  FOR_ALL(f, fp) {
    nUsage[f[fp]]--;
  }
}


// Constructors 
mousse::removeCells::removeCells
(
  const polyMesh& mesh,
  const bool syncPar
)
:
  mesh_{mesh},
  syncPar_{syncPar}
{}


// Member Functions 
mousse::labelList mousse::removeCells::getExposedFaces
(
  const labelList& cellLabels
) const
{
  // Create list of cells to be removed
  boolList removedCell{mesh_.nCells(), false};
  // Go from labelList of cells-to-remove to a boolList.
  FOR_ALL(cellLabels, i) {
    removedCell[cellLabels[i]] = true;
  }
  const labelList& faceOwner = mesh_.faceOwner();
  const labelList& faceNeighbour = mesh_.faceNeighbour();
  // Count cells using face.
  labelList nCellsUsingFace{mesh_.nFaces(), 0};
  for (label faceI = 0; faceI < mesh_.nInternalFaces(); faceI++) {
    label own = faceOwner[faceI];
    label nei = faceNeighbour[faceI];
    if (!removedCell[own]) {
      nCellsUsingFace[faceI]++;
    }
    if (!removedCell[nei]) {
      nCellsUsingFace[faceI]++;
    }
  }
  for (label faceI = mesh_.nInternalFaces(); faceI < mesh_.nFaces(); faceI++) {
    label own = faceOwner[faceI];
    if (!removedCell[own]) {
      nCellsUsingFace[faceI]++;
    }
  }
  // Coupled faces: add number of cells using face across couple.
  if (syncPar_) {
    syncTools::syncFaceList(mesh_, nCellsUsingFace, plusEqOp<label>());
  }
  // Now nCellsUsingFace:
  // 0 : internal face whose both cells get deleted
  //     boundary face whose all cells get deleted
  // 1 : internal face that gets exposed
  //     unaffected (uncoupled) boundary face
  //     coupled boundary face that gets exposed ('uncoupled')
  // 2 : unaffected internal face
  //     unaffected coupled boundary face
  DynamicList<label> exposedFaces{mesh_.nFaces()/10};
  for (label faceI = 0; faceI < mesh_.nInternalFaces(); faceI++) {
    if (nCellsUsingFace[faceI] == 1) {
      exposedFaces.append(faceI);
    }
  }
  const polyBoundaryMesh& patches = mesh_.boundaryMesh();
  FOR_ALL(patches, patchI) {
    const polyPatch& pp = patches[patchI];
    if (pp.coupled()) {
      label faceI = pp.start();
      FOR_ALL(pp, i) {
        label own = faceOwner[faceI];
        if (nCellsUsingFace[faceI] == 1 && !removedCell[own]) {
          // My owner not removed but other side is so has to become
          // normal, uncoupled, boundary face
          exposedFaces.append(faceI);
        }
        faceI++;
      }
    }
  }
  return exposedFaces.shrink();
}


void mousse::removeCells::setRefinement
(
  const labelList& cellLabels,
  const labelList& exposedFaceLabels,
  const labelList& exposedPatchIDs,
  polyTopoChange& meshMod
) const
{
  const polyBoundaryMesh& patches = mesh_.boundaryMesh();
  if (exposedFaceLabels.size() != exposedPatchIDs.size()) {
    FATAL_ERROR_IN
    (
      "removeCells::setRefinement(const labelList&"
      ", const labelList&, const labelList&, polyTopoChange&)"
    )
    << "Size of exposedFaceLabels " << exposedFaceLabels.size()
    << " differs from size of exposedPatchIDs "
    << exposedPatchIDs.size()
    << abort(FatalError);
  }
  // List of new patchIDs
  labelList newPatchID{mesh_.nFaces(), -1};
  FOR_ALL(exposedFaceLabels, i) {
    label patchI = exposedPatchIDs[i];
    if (patchI < 0 || patchI >= patches.size()) {
      FATAL_ERROR_IN
      (
        "removeCells::setRefinement(const labelList&"
        ", const labelList&, const labelList&, polyTopoChange&)"
      )
      << "Invalid patch " << patchI
      << " for exposed face " << exposedFaceLabels[i] << endl
      << "Valid patches 0.." << patches.size()-1
      << abort(FatalError);
    }
    if (patches[patchI].coupled()) {
      FATAL_ERROR_IN
      (
        "removeCells::setRefinement(const labelList&"
        ", const labelList&, const labelList&, polyTopoChange&)"
      )
      << "Trying to put exposed face " << exposedFaceLabels[i]
      << " into a coupled patch : " << patches[patchI].name()
      << endl
      << "This is illegal."
      << abort(FatalError);
    }
    newPatchID[exposedFaceLabels[i]] = patchI;
  }
  // Create list of cells to be removed
  boolList removedCell{mesh_.nCells(), false};
  // Go from labelList of cells-to-remove to a boolList and remove all
  // cells mentioned.
  FOR_ALL(cellLabels, i) {
    label cellI = cellLabels[i];
    removedCell[cellI] = true;
    //Pout<< "Removing cell " << cellI
    //    << " cc:" << mesh_.cellCentres()[cellI] << endl;
    meshMod.setAction(polyRemoveCell(cellI));
  }
  // Remove faces that are no longer used. Modify faces that
  // are used by one cell only.
  const faceList& faces = mesh_.faces();
  const labelList& faceOwner = mesh_.faceOwner();
  const labelList& faceNeighbour = mesh_.faceNeighbour();
  const faceZoneMesh& faceZones = mesh_.faceZones();
  // Count starting number of faces using each point. Keep up to date whenever
  // removing a face.
  labelList nFacesUsingPoint{mesh_.nPoints(), 0};
  FOR_ALL(faces, faceI) {
    const face& f = faces[faceI];
    FOR_ALL(f, fp) {
      nFacesUsingPoint[f[fp]]++;
    }
  }
  for (label faceI = 0; faceI < mesh_.nInternalFaces(); faceI++) {
    const face& f = faces[faceI];
    label own = faceOwner[faceI];
    label nei = faceNeighbour[faceI];
    if (removedCell[own]) {
      if (removedCell[nei]) {
        // Face no longer used
        meshMod.setAction(polyRemoveFace(faceI));
        uncount(f, nFacesUsingPoint);
      } else {
        if (newPatchID[faceI] == -1) {
          FATAL_ERROR_IN
          (
            "removeCells::setRefinement(const labelList&"
            ", const labelList&, const labelList&"
            ", polyTopoChange&)"
          )
          << "No patchID provided for exposed face " << faceI
          << " on cell " << nei << nl
          << "Did you provide patch IDs for all exposed faces?"
          << abort(FatalError);
        }
        // nei is remaining cell. FaceI becomes external cell
        label zoneID = faceZones.whichZone(faceI);
        bool zoneFlip = false;
        if (zoneID >= 0) {
          const faceZone& fZone = faceZones[zoneID];
          // Note: we reverse the owner/neighbour of the face
          // so should also select the other side of the zone
          zoneFlip = !fZone.flipMap()[fZone.whichFace(faceI)];
        }
        meshMod.setAction
          (
            polyModifyFace
            {
              f.reverseFace(),        // modified face
              faceI,                  // label of face being modified
              nei,                    // owner
              -1,                     // neighbour
              true,                   // face flip
              newPatchID[faceI],      // patch for face
              false,                  // remove from zone
              zoneID,                 // zone for face
              zoneFlip                // face flip in zone
            }
          );
      }
    } else if (removedCell[nei]) {
      if (newPatchID[faceI] == -1) {
        FATAL_ERROR_IN
        (
          "removeCells::setRefinement(const labelList&"
          ", const labelList&, const labelList&"
          ", polyTopoChange&)"
        )
        << "No patchID provided for exposed face " << faceI
        << " on cell " << own << nl
        << "Did you provide patch IDs for all exposed faces?"
        << abort(FatalError);
      }
      // own is remaining cell. FaceI becomes external cell.
      label zoneID = faceZones.whichZone(faceI);
      bool zoneFlip = false;
      if (zoneID >= 0) {
        const faceZone& fZone = faceZones[zoneID];
        zoneFlip = fZone.flipMap()[fZone.whichFace(faceI)];
      }
      meshMod.setAction
        (
          polyModifyFace
          {
            f,                      // modified face
            faceI,                  // label of face being modified
            own,                    // owner
            -1,                     // neighbour
            false,                  // face flip
            newPatchID[faceI],      // patch for face
            false,                  // remove from zone
            zoneID,                 // zone for face
            zoneFlip                // face flip in zone
          }
        );
    }
  }
  FOR_ALL(patches, patchI) {
    const polyPatch& pp = patches[patchI];
    if (pp.coupled()) {
      label faceI = pp.start();
      FOR_ALL(pp, i) {
        if (newPatchID[faceI] != -1) {
          label zoneID = faceZones.whichZone(faceI);
          bool zoneFlip = false;
          if (zoneID >= 0) {
            const faceZone& fZone = faceZones[zoneID];
            zoneFlip = fZone.flipMap()[fZone.whichFace(faceI)];
          }
          meshMod.setAction
            (
              polyModifyFace
              {
                faces[faceI],           // modified face
                faceI,                  // label of face
                faceOwner[faceI],       // owner
                -1,                     // neighbour
                false,                  // face flip
                newPatchID[faceI],      // patch for face
                false,                  // remove from zone
                zoneID,                 // zone for face
                zoneFlip                // face flip in zone
              }
            );
        } else if (removedCell[faceOwner[faceI]]) {
          // Face no longer used
          meshMod.setAction(polyRemoveFace(faceI));
          uncount(faces[faceI], nFacesUsingPoint);
        }
        faceI++;
      }
    } else {
      label faceI = pp.start();
      FOR_ALL(pp, i) {
        if (newPatchID[faceI] != -1) {
          FATAL_ERROR_IN
          (
            "removeCells::setRefinement(const labelList&"
            ", const labelList&, const labelList&"
            ", polyTopoChange&)"
          )
          << "new patchID provided for boundary face " << faceI
          << " even though it is not on a coupled face."
          << abort(FatalError);
        }
        if (removedCell[faceOwner[faceI]]) {
          // Face no longer used
          meshMod.setAction(polyRemoveFace(faceI));
          uncount(faces[faceI], nFacesUsingPoint);
        }
        faceI++;
      }
    }
  }
  // Remove points that are no longer used.
  // Loop rewritten to not use pointFaces.
  FOR_ALL(nFacesUsingPoint, pointI) {
    if (nFacesUsingPoint[pointI] == 0) {
      meshMod.setAction(polyRemovePoint(pointI));
    } else if (nFacesUsingPoint[pointI] == 1) {
      WARNING_IN
      (
        "removeCells::setRefinement(const labelList&"
        ", const labelList&, const labelList&"
        ", polyTopoChange&)"
      )
      << "point " << pointI << " at coordinate "
      << mesh_.points()[pointI]
      << " is only used by 1 face after removing cells."
      << " This probably results in an illegal mesh."
      << endl;
    }
  }
}
