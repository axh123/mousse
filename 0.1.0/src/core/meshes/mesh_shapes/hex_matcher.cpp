// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "hex_matcher.hpp"
#include "primitive_mesh.hpp"
#include "list_ops.hpp"

// Static Data Members
const mousse::label mousse::hexMatcher::vertPerCell = 8;
const mousse::label mousse::hexMatcher::facePerCell = 6;
const mousse::label mousse::hexMatcher::maxVertPerFace = 4;

// Constructors
mousse::hexMatcher::hexMatcher()
:
  cellMatcher
  (
    vertPerCell,
    facePerCell,
    maxVertPerFace,
    "hex"
  )
{}

// Destructor
mousse::hexMatcher::~hexMatcher()
{}

// Member Functions
bool mousse::hexMatcher::matchShape
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
  // Is hex for sure since all faces are quads
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
  // Try bottom face (face 4).
  // Only need to try one orientation of this face since hex is
  // rotation symmetric
  //
  label face4I = 0;
  const face& face4 = localFaces_[face4I];
  label face4vert0 = 0;
  vertLabels_[0] = pointMap_[face4[face4vert0]];
  faceLabels_[4] = faceMap_[face4I];
  // Walk face 4 from vertex 0 to 1
  label face4vert1 =
    nextVert
    (
      face4vert0,
      faceSize_[face4I],
      !(owner[faceMap_[face4I]] == cellI)
    );
  vertLabels_[1] = pointMap_[face4[face4vert1]];
  // Walk face 4 from vertex 1 to 2
  label face4vert2 =
    nextVert
    (
      face4vert1,
      faceSize_[face4I],
      !(owner[faceMap_[face4I]] == cellI)
    );
  vertLabels_[2] = pointMap_[face4[face4vert2]];
  // Walk face 4 from vertex 2 to 3
  label face4vert3 =
    nextVert
    (
      face4vert2,
      faceSize_[face4I],
      !(owner[faceMap_[face4I]] == cellI)
    );
  vertLabels_[3] = pointMap_[face4[face4vert3]];
  // Jump edge from face4 to face0
  label face0I =
    otherFace
    (
      numVert,
      face4[face4vert3],
      face4[face4vert0],
      face4I
    );
  faceLabels_[0] = faceMap_[face0I];
  const face& face0 = localFaces_[face0I];
  label face0vert0 = pointFaceIndex_[face4[face4vert0]][face0I];
  // Walk face 0 from vertex 0 to 4
  label face0vert4 =
    nextVert
    (
      face0vert0,
      faceSize_[face0I],
      (owner[faceMap_[face0I]] == cellI)
    );
  vertLabels_[4] = pointMap_[face0[face0vert4]];
  // Walk face 0 from vertex 4 to 7
  label face0vert7 =
    nextVert
    (
      face0vert4,
      faceSize_[face0I],
      (owner[faceMap_[face0I]] == cellI)
    );
  vertLabels_[7] = pointMap_[face0[face0vert7]];
  // Jump edge from face0 to face5
  label face5I =
    otherFace
    (
      numVert,
      face0[face0vert4],
      face0[face0vert7],
      face0I
    );
  const face& face5 = localFaces_[face5I];
  faceLabels_[5] = faceMap_[face5I];
  label face5vert4 = pointFaceIndex_[face0[face0vert4]][face5I];
  // Walk face 5 from vertex 4 to 5
  label face5vert5 =
    nextVert
    (
      face5vert4,
      faceSize_[face5I],
      (owner[faceMap_[face5I]] == cellI)
    );
  vertLabels_[5] = pointMap_[face5[face5vert5]];
  // Walk face 5 from vertex 5 to 6
  label face5vert6 =
    nextVert
    (
      face5vert5,
      faceSize_[face5I],
      (owner[faceMap_[face5I]] == cellI)
    );
  vertLabels_[6] = pointMap_[face5[face5vert6]];
  // Jump edge from face4 to face2
  label face2I =
    otherFace
    (
      numVert,
      face4[face4vert0],
      face4[face4vert1],
      face4I
    );
  faceLabels_[2] = faceMap_[face2I];
  // Jump edge from face4 to face1
  label face1I =
    otherFace
    (
      numVert,
      face4[face4vert1],
      face4[face4vert2],
      face4I
    );
  faceLabels_[1] = faceMap_[face1I];
  // Jump edge from face4 to face3
  label face3I =
    otherFace
    (
      numVert,
      face4[face4vert2],
      face4[face4vert3],
      face4I
    );
  faceLabels_[3] = faceMap_[face3I];
  return true;
}
mousse::label mousse::hexMatcher::faceHashValue() const
{
  return facePerCell*vertPerCell;
}
bool mousse::hexMatcher::faceSizeMatch
(
  const faceList& faces,
  const labelList& myFaces
) const
{
  if (myFaces.size() != facePerCell)
  {
    return false;
  }
  FOR_ALL(myFaces, myFaceI)
  {
    label size = faces[myFaces[myFaceI]].size();
    if (size != 4)
    {
      return false;
    }
  }
  return true;
}
bool mousse::hexMatcher::isA(const primitiveMesh& mesh, const label cellI)
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
bool mousse::hexMatcher::isA(const faceList& faces)
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
bool mousse::hexMatcher::matches
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
