// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "wedge_matcher.hpp"
#include "primitive_mesh.hpp"
#include "list_ops.hpp"

// Static Data Members
const mousse::label mousse::wedgeMatcher::vertPerCell = 7;
const mousse::label mousse::wedgeMatcher::facePerCell = 6;
const mousse::label mousse::wedgeMatcher::maxVertPerFace = 4;

// Constructors
mousse::wedgeMatcher::wedgeMatcher()
:
  cellMatcher
  (
    vertPerCell,
    facePerCell,
    maxVertPerFace,
    "wedge"
  )
{}

// Destructor
mousse::wedgeMatcher::~wedgeMatcher()
{}

// Member Functions
bool mousse::wedgeMatcher::matchShape
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
    //Info<< endl << "Wedge vertex 0: vertex " <<  face0[face0vert0]
    //    << " at position " << face0vert0 << " in face " << face0
    //    << endl;
    // Walk face 0 from vertex 0 to 1
    label face0vert1 =
      nextVert
      (
        face0vert0,
        faceSize_[face0I],
        !(owner[faceMap_[face0I]] == cellI)
      );
    vertLabels_[1] = pointMap_[face0[face0vert1]];
    //Info<< "Wedge vertex 1: vertex " <<  face0[face0vert1]
    //    << " at position " << face0vert1 << " in face " << face0
    //    << endl;
    // Jump edge from face0 to face4
    label face4I =
      otherFace
      (
        numVert,
        face0[face0vert0],
        face0[face0vert1],
        face0I
      );
    const face& face4 = localFaces_[face4I];
    //Info<< "Stepped to wedge face 4 " << face4
    //    << " across edge " << face0[face0vert0] << " "
    //    << face0[face0vert1]
    //    << endl;
    if (faceSize_[face4I] != 4)
    {
      //Info<< "Cannot be Wedge Face 4 since size="
      //    << faceSize_[face4I] << endl;
      continue;
    }
    // Is wedge for sure now
    if (checkOnly)
    {
      return true;
    }
    faceLabels_[4] = faceMap_[face4I];
    // Get index of vertex 0 in face4
    label face4vert0 = pointFaceIndex_[face0[face0vert0]][face4I];
    //Info<< "Wedge vertex 0 also: vertex " <<  face4[face4vert0]
    //    << " at position " << face4vert0 << " in face " << face4
    //    << endl;
    // Walk face 4 from vertex 4 to 3
    label face4vert3 =
      nextVert
      (
        face4vert0,
        faceSize_[face4I],
        !(owner[faceMap_[face4I]] == cellI)
      );
    vertLabels_[3] = pointMap_[face4[face4vert3]];
    //Info<< "Wedge vertex 3: vertex " <<  face4[face4vert3]
    //    << " at position " << face4vert3 << " in face " << face4
    //    << endl;
    // Jump edge from face4 to face2
    label face2I =
      otherFace
      (
        numVert,
        face4[face4vert0],
        face4[face4vert3],
        face4I
      );
    const face& face2 = localFaces_[face2I];
    //Info<< "Stepped to wedge face 2 " << face2
    //    << " across edge " << face4[face4vert0] << " "
    //    << face4[face4vert3]
    //    << endl;
    if (faceSize_[face2I] != 3)
    {
      //Info<< "Cannot be Wedge Face 2 since size="
      //    << faceSize_[face2I] << endl;
      continue;
    }
    faceLabels_[2] = faceMap_[face2I];
    // Is wedge for sure now
    //Info<< "** WEDGE **" << endl;
    //
    // Walk to other faces and vertices and assign mapping.
    //
    // Vertex 6
    label face2vert3 = pointFaceIndex_[face4[face4vert3]][face2I];
    // Walk face 2 from vertex 3 to 6
    label face2vert6 =
      nextVert
      (
        face2vert3,
        faceSize_[face2I],
        (owner[faceMap_[face2I]] == cellI)
      );
    vertLabels_[6] = pointMap_[face2[face2vert6]];
    // Jump edge from face2 to face1
    label face1I =
      otherFace
      (
        numVert,
        face2[face2vert3],
        face2[face2vert6],
        face2I
      );
    faceLabels_[1] = faceMap_[face1I];
    const face& face1 = localFaces_[face1I];
    //Info<< "Stepped to wedge face 1 " << face1
    //    << " across edge " << face2[face2vert3] << " "
    //    << face2[face2vert6]
    //    << endl;
    label face1vert6 = pointFaceIndex_[face2[face2vert6]][face1I];
    // Walk face 1 from vertex 6 to 5
    label face1vert5 =
      nextVert
      (
        face1vert6,
        faceSize_[face1I],
        !(owner[faceMap_[face1I]] == cellI)
      );
    vertLabels_[5] = pointMap_[face1[face1vert5]];
    // Walk face 1 from vertex 5 to 4
    label face1vert4 =
      nextVert
      (
        face1vert5,
        faceSize_[face1I],
        !(owner[faceMap_[face1I]] == cellI)
      );
    vertLabels_[4] = pointMap_[face1[face1vert4]];
    // Walk face 0 from vertex 1 to 2
    label face0vert2 =
      nextVert
      (
        face0vert1,
        faceSize_[face0I],
        !(owner[faceMap_[face0I]] == cellI)
      );
    vertLabels_[2] = pointMap_[face0[face0vert2]];
    //Info<< "Wedge vertex 2: vertex " <<  face0[face0vert2]
    //    << " at position " << face0vert2 << " in face " << face0
    //    << endl;
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
    //const face& face3 = localFaces_[face3I];
    //Info<< "Stepped to wedge face 3 " << face3
    //    << " across edge " << face0[face0vert1] << " "
    //    << face0[face0vert2]
    //    << endl;
    // Jump edge from face0 to face5
    label face5I =
      otherFace
      (
        numVert,
        face0[face0vert2],
        face0[face0vert0],
        face0I
      );
    faceLabels_[5] = faceMap_[face5I];
    //const face& face5 = localFaces_[face5I];
    //Info<< "Stepped to wedge face 5 " << face5
    //    << " across edge " << face0[face0vert2] << " "
    //    << face0[face0vert0]
    //    << endl;
    return true;
  }
  // Tried all triangular faces, in all rotations but no match found
  return false;
}
mousse::label mousse::wedgeMatcher::faceHashValue() const
{
  return 2*3 + 4*4;
}
bool mousse::wedgeMatcher::faceSizeMatch
(
  const faceList& faces,
  const labelList& myFaces
) const
{
  if (myFaces.size() != 6)
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
  if ((nTris == 2) && (nQuads == 4))
  {
    return true;
  }
  else
  {
    return false;
  }
}
bool mousse::wedgeMatcher::isA(const primitiveMesh& mesh, const label cellI)
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
bool mousse::wedgeMatcher::isA(const faceList& faces)
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
bool mousse::wedgeMatcher::matches
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
