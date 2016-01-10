// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "tet_wedge_matcher.hpp"
#include "cell_matcher.hpp"
#include "primitive_mesh.hpp"
#include "primitive_mesh.hpp"
#include "cell_modeller.hpp"
#include "list_ops.hpp"

// Static Data Members
const mousse::label mousse::tetWedgeMatcher::vertPerCell = 5;
const mousse::label mousse::tetWedgeMatcher::facePerCell = 4;
const mousse::label mousse::tetWedgeMatcher::maxVertPerFace = 4;

// Constructors
mousse::tetWedgeMatcher::tetWedgeMatcher()
:
  cellMatcher
  (
    vertPerCell,
    facePerCell,
    maxVertPerFace,
   "tetWedge"
  )
{}

// Destructor
mousse::tetWedgeMatcher::~tetWedgeMatcher()
{}

// Member Functions
bool mousse::tetWedgeMatcher::matchShape
(
  const bool checkOnly,
  const faceList& faces,
  const labelList& owner,
  const label cellI,
  const labelList& myFaces
)
{
  if (!faceSizeMatch(faces, myFaces))
  {
    return false;
  }
  // Is tetWedge for sure now. No other shape has two tri, two quad
  if (checkOnly)
  {
    return true;
  }
  // Calculate localFaces_ and mapping pointMap_, faceMap_
  label numVert = calcLocalFaces(faces, myFaces);
  if (numVert != vertPerCell)
  {
    return false;
  }
  // Set up 'edge' to face mapping.
  calcEdgeAddressing(numVert);
  // Set up point on face to index-in-face mapping
  calcPointFaceIndex();
  // Storage for maps -vertex to mesh and -face to mesh
  vertLabels_.setSize(vertPerCell);
  faceLabels_.setSize(facePerCell);
  //
  // Try first triangular face. Rotate in all directions.
  // Walk path to other triangular face.
  //
  label face0I = -1;
  FOR_ALL(faceSize_, faceI)
  {
    if (faceSize_[faceI] == 3)
    {
      face0I = faceI;
      break;
    }
  }
  const face& face0 = localFaces_[face0I];
  // Try all rotations of this face
  for (label face0vert0 = 0; face0vert0 < faceSize_[face0I]; face0vert0++)
  {
    //
    // Try to follow prespecified path on faces of cell,
    // starting at face0vert0
    //
    vertLabels_[0] = pointMap_[face0[face0vert0]];
    faceLabels_[0] = faceMap_[face0I];
    // Walk face 0 from vertex 0 to 1
    label face0vert1 =
      nextVert
      (
        face0vert0,
        faceSize_[face0I],
        !(owner[faceMap_[face0I]] == cellI)
      );
    vertLabels_[1] = pointMap_[face0[face0vert1]];
    // Jump edge from face0 to face1 (the other triangular face)
    label face1I =
      otherFace
      (
        numVert,
        face0[face0vert0],
        face0[face0vert1],
        face0I
      );
    if (faceSize_[face1I] != 3)
    {
      continue;
    }
    faceLabels_[1] = faceMap_[face1I];
    // Now correctly oriented tet-wedge for sure.
    // Walk face 0 from vertex 1 to 2
    label face0vert2 =
      nextVert
      (
        face0vert1,
        faceSize_[face0I],
        !(owner[faceMap_[face0I]] == cellI)
      );
    vertLabels_[2] = pointMap_[face0[face0vert2]];
    // Jump edge from face0 to face3
    label face3I =
      otherFace
      (
        numVert,
        face0[face0vert1],
        face0[face0vert2],
        face0I
      );
    faceLabels_[3] = faceMap_[face3I];
    // Jump edge from face0 to face2
    label face2I =
      otherFace
      (
        numVert,
        face0[face0vert2],
        face0[face0vert0],
        face0I
      );
    faceLabels_[2] = faceMap_[face2I];
    // Get index of vertex 2 in face3
    label face3vert2 = pointFaceIndex_[face0[face0vert2]][face3I];
    // Walk face 3 from vertex 2 to 4
    label face3vert4 =
      nextVert
      (
        face3vert2,
        faceSize_[face3I],
        (owner[faceMap_[face3I]] == cellI)
      );
    const face& face3 = localFaces_[face3I];
    vertLabels_[4] = pointMap_[face3[face3vert4]];
    // Walk face 3 from vertex 4 to 3
    label face3vert3 =
      nextVert
      (
        face3vert4,
        faceSize_[face3I],
        (owner[faceMap_[face3I]] == cellI)
      );
    vertLabels_[3] = pointMap_[face3[face3vert3]];
    return true;
  }
  // Tried all triangular faces, in all rotations but no match found
  return false;
}
mousse::label mousse::tetWedgeMatcher::faceHashValue() const
{
  return 2*3 + 2*4;
}
bool mousse::tetWedgeMatcher::faceSizeMatch
(
  const faceList& faces,
  const labelList& myFaces
) const
{
  if (myFaces.size() != 4)
  {
    return false;
  }
  label nTris = 0;
  label nQuads = 0;
  FOR_ALL(myFaces, myFaceI)
  {
    label size = faces[myFaces[myFaceI]].size();
    if (size == 3)
    {
      nTris++;
    }
    else if (size == 4)
    {
      nQuads++;
    }
    else
    {
      return false;
    }
  }
  if ((nTris == 2) && (nQuads == 2))
  {
    return true;
  }
  else
  {
    return false;
  }
}
bool mousse::tetWedgeMatcher::isA(const primitiveMesh& mesh, const label cellI)
{
  return matchShape
  (
    true,
    mesh.faces(),
    mesh.faceOwner(),
    cellI,
    mesh.cells()[cellI]
  );
}
bool mousse::tetWedgeMatcher::isA(const faceList& faces)
{
  // Do as if mesh with one cell only
  return matchShape
  (
    true,
    faces,                      // all faces in mesh
    labelList(faces.size(), 0), // cell 0 is owner of all faces
    0,                          // cell label
    identity(faces.size())      // faces of cell 0
  );
}
bool mousse::tetWedgeMatcher::matches
(
  const primitiveMesh& mesh,
  const label cellI,
  cellShape& shape
)
{
  if
  (
    matchShape
    (
      false,
      mesh.faces(),
      mesh.faceOwner(),
      cellI,
      mesh.cells()[cellI]
    )
  )
  {
    shape = cellShape(model(), vertLabels());
    return true;
  }
  else
  {
    return false;
  }
}
