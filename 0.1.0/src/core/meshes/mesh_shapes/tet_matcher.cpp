// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "tet_matcher.hpp"
#include "cell_matcher.hpp"
#include "primitive_mesh.hpp"
#include "primitive_mesh.hpp"
#include "cell_modeller.hpp"
#include "list_ops.hpp"


// Static Data Members
const mousse::label mousse::tetMatcher::vertPerCell = 4;
const mousse::label mousse::tetMatcher::facePerCell = 4;
const mousse::label mousse::tetMatcher::maxVertPerFace = 3;


// Constructors
mousse::tetMatcher::tetMatcher()
:
  cellMatcher
  {
    vertPerCell,
    facePerCell,
    maxVertPerFace,
    "tet"
  }
{}


// Destructor
mousse::tetMatcher::~tetMatcher()
{}


// Member Functions
bool mousse::tetMatcher::matchShape
(
  const bool checkOnly,
  const faceList& faces,
  const labelList& owner,
  const label cellI,
  const labelList& myFaces
)
{
  if (!faceSizeMatch(faces, myFaces)) {
    return false;
  }
  // Tet for sure now
  if (checkOnly) {
    return true;
  }
  // Calculate localFaces_ and mapping pointMap_, faceMap_
  label numVert = calcLocalFaces(faces, myFaces);
  if (numVert != vertPerCell) {
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
  // Try bottom face (face 3)
  //
  label face3I = 0;
  const face& face3 = localFaces_[face3I];
  label face3vert0 = 0;
  //
  // Try to follow prespecified path on faces of cell,
  // starting at face3vert0
  //
  vertLabels_[0] = pointMap_[face3[face3vert0]];
  faceLabels_[3] = faceMap_[face3I];
  // Walk face 3 from vertex 0 to 1
  label face3vert1 =
    nextVert
    (
      face3vert0,
      faceSize_[face3I],
      !(owner[faceMap_[face3I]] == cellI)
    );
  vertLabels_[1] = pointMap_[face3[face3vert1]];
  // Walk face 3 from vertex 1 to 2
  label face3vert2 =
    nextVert
    (
      face3vert1,
      faceSize_[face3I],
      !(owner[faceMap_[face3I]] == cellI)
    );
  vertLabels_[2] = pointMap_[face3[face3vert2]];
  // Jump edge from face3 to face2
  label face2I =
    otherFace
    (
      numVert,
      face3[face3vert0],
      face3[face3vert1],
      face3I
    );
  faceLabels_[2] = faceMap_[face2I];
  // Jump edge from face3 to face0
  label face0I =
    otherFace
    (
      numVert,
      face3[face3vert1],
      face3[face3vert2],
      face3I
    );
  faceLabels_[0] = faceMap_[face0I];
  // Jump edge from face3 to face1
  label face1I =
    otherFace
    (
      numVert,
      face3[face3vert2],
      face3[face3vert0],
      face3I
    );
  faceLabels_[1] = faceMap_[face1I];
  const face& face1 = localFaces_[face1I];
  // Get index of vert0 in face 1
  label face1vert0 = pointFaceIndex_[face3[face3vert0]][face1I];
  // Walk face 1 from vertex 0 to 3
  label face1vert3 =
    nextVert
    (
      face1vert0,
      faceSize_[face1I],
      (owner[faceMap_[face1I]] == cellI)
    );
  vertLabels_[3] = pointMap_[face1[face1vert3]];
  return true;
}


mousse::label mousse::tetMatcher::faceHashValue() const
{
  return 4*3;
}


bool mousse::tetMatcher::faceSizeMatch
(
  const faceList& faces,
  const labelList& myFaces
) const
{
  if (myFaces.size() != 4) {
    return false;
  }
  FOR_ALL(myFaces, myFaceI) {
    label size = faces[myFaces[myFaceI]].size();
    if (size != 3) {
      return false;
    }
  }
  return true;
}


bool mousse::tetMatcher::isA(const primitiveMesh& mesh, const label cellI)
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


bool mousse::tetMatcher::isA(const faceList& faces)
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


bool mousse::tetMatcher::matches
(
  const primitiveMesh& mesh,
  const label cellI,
  cellShape& shape
)
{
  if (matchShape
      (
        false,
        mesh.faces(),
        mesh.faceOwner(),
        cellI,
        mesh.cells()[cellI]
      )) {
    shape = cellShape(model(), vertLabels());
    return true;
  } else {
    return false;
  }
}

