// mousse: CFD toolbox
// Copyright (C) 2011-2012 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "walk_patch.hpp"
#include "list_ops.hpp"


// Static Data Members
namespace mousse {

DEFINE_TYPE_NAME_AND_DEBUG(walkPatch, 0);

}


// Private Member Functions 
// Get other face using v0, v1 (in localFaces numbering). Or -1.
mousse::label mousse::walkPatch::getNeighbour
(
  const label faceI,
  const label fp,
  const label v0,
  const label v1
) const
{
  const labelList& fEdges = pp_.faceEdges()[faceI];
  const edgeList& edges = pp_.edges();
  label nbrEdgeI = -1;
  // Shortcut: maybe faceEdges are sorted(?) in which case fEdges[fp] is
  // edge between v0 and v1.
  const edge& e = edges[fEdges[fp]];
  if ((e[0] == v0 && e[1] == v1) || (e[0] == v1 && e[1] == v0)) {
    // Correct edge.
    nbrEdgeI = fEdges[fp];
  } else {
    // Loop over all faceEdges.
    FOR_ALL(fEdges, i) {
      label edgeI = fEdges[i];
      const edge& e = edges[edgeI];
      if ((e[0] == v0 && e[1] == v1) || (e[0] == v1 && e[1] == v0)) {
        // Found edge on face which uses v0, v1.
        nbrEdgeI = edgeI;
        break;
      }
    }
  }
  if (nbrEdgeI == -1) {
    FATAL_ERROR_IN("getNeighbour")
      << "Did not find edge on face " << faceI << " that uses vertices"
      << v0 << " and " << v1 << abort(FatalError);
  }
  // Get neighbouring face.
  const labelList& eFaces = pp_.edgeFaces()[nbrEdgeI];
  if (eFaces.size() == 1) {
    return -1;
  } else if (eFaces.size() == 2) {
    label nbrFaceI = eFaces[0];
    if (nbrFaceI == faceI) {
      nbrFaceI = eFaces[1];
    }
    return nbrFaceI;
  } else {
    FATAL_ERROR_IN("getNeighbour")
      << "Illegal surface on patch. Face " << faceI
      << " at vertices " << v0 << ',' << v1
      << " has fewer than 1 or more than 2 neighbours"
      << abort(FatalError);
    return -1;
  }
}


// Gets labels of changed faces and enterVertices on faces.
// Returns labels of faces changed and enterVertices on them.
void mousse::walkPatch::faceToFace
(
  const labelList& changedFaces,
  const labelList& enterVerts,
  labelList& nbrFaces,
  labelList& nbrEnterVerts
)
{
  nbrFaces.setSize(pp_.size());
  nbrEnterVerts.setSize(pp_.size());
  label changedI = 0;
  FOR_ALL(changedFaces, i) {
    label faceI = changedFaces[i];
    label enterVertI = enterVerts[i];
    if (!visited_[faceI]) {
      // Do this face
      visited_[faceI] = true;
      visitOrder_.append(faceI);
      const face& f = pp_.localFaces()[faceI];
      label fp = findIndex(f, enterVertI);
      indexInFace_.append(fp);
      // Visit neighbouring faces in order, starting at fp.
      FOR_ALL(f, i) {
        label fp1 = reverse_ ? f.rcIndex(fp) : f.fcIndex(fp);
        label nbr = getNeighbour(faceI, fp, f[fp], f[fp1]);
        if (nbr != -1 && !visited_[nbr] && faceZone_[nbr] == faceZone_[faceI]) {
          nbrFaces[changedI] = nbr;
          nbrEnterVerts[changedI] = f[fp];
          changedI++;
        }
        fp = fp1;
      }
    }
  }
  nbrFaces.setSize(changedI);
  nbrEnterVerts.setSize(changedI);
}


// Constructors 
// Construct from components
mousse::walkPatch::walkPatch
(
  const primitivePatch& pp,
  const labelList& faceZone,
  const bool reverse,
  const label faceI,
  const label enterVertI,
  boolList& visited
)
:
  pp_{pp},
  faceZone_{faceZone},
  reverse_{reverse},
  visited_{visited},
  visitOrder_{pp.size()},
  indexInFace_{pp.size()}
{
  // List of faces that have been visited in the current iteration.
  labelList changedFaces{1, faceI};
  // Corresponding list of entry vertices
  labelList enterVerts{1, enterVertI};
  while (true) {
    labelList nbrFaces;
    labelList nbrEnterVerts;
    faceToFace
    (
      changedFaces,
      enterVerts,
      nbrFaces,
      nbrEnterVerts
    );
    if (nbrFaces.empty()) {
      break;
    }
    changedFaces = nbrFaces;
    enterVerts = nbrEnterVerts;
  }
  visitOrder_.shrink();
  indexInFace_.shrink();
}

