#ifndef MESH_TOOLS_MESH_TOOLS_MESH_TOOLS_HPP_
#define MESH_TOOLS_MESH_TOOLS_MESH_TOOLS_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011-2013 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "label.hpp"
#include "vector.hpp"
#include "triad.hpp"
#include "label_list.hpp"
#include "point_field.hpp"
#include "face_list.hpp"
#include "cell_list.hpp"
#include "primitive_patch.hpp"


namespace mousse {

class primitiveMesh;
class polyMesh;

namespace meshTools {

// Bit identifiers for octants (p=plus, m=min e.g. plusXminYminZ)
static const label mXmYmZ = 0;
static const label pXmYmZ = 1;
static const label mXpYmZ = 2;
static const label pXpYmZ = 3;
static const label mXmYpZ = 4;
static const label pXmYpZ = 5;
static const label mXpYpZ = 6;
static const label pXpYpZ = 7;
static const label mXmYmZMask = 1 << mXmYmZ;
static const label pXmYmZMask = 1 << pXmYmZ;
static const label mXpYmZMask = 1 << mXpYmZ;
static const label pXpYmZMask = 1 << pXpYmZ;
static const label mXmYpZMask = 1 << mXmYpZ;
static const label pXmYpZMask = 1 << pXmYpZ;
static const label mXpYpZMask = 1 << mXpYpZ;
static const label pXpYpZMask = 1 << pXpYpZ;

// Normal handling
  //- Check if n is in same direction as normals of all faceLabels
  bool visNormal
  (
    const vector& n,
    const vectorField& faceNormals,
    const labelList& faceLabels
  );
  //- Calculate point normals on a 'box' mesh (all edges aligned with
  //  coordinate axes)
  vectorField calcBoxPointNormals(const primitivePatch& pp);
  //- Normalized edge vector
  vector normEdgeVec(const primitiveMesh&, const label edgeI);
// OBJ writing
  //- Write obj representation of point
  void writeOBJ
  (
    Ostream& os,
    const point& pt
  );
  //- Write obj representation of a triad. Requires the location of the
  //  triad to be supplied
  void writeOBJ
  (
    Ostream& os,
    const triad& t,
    const point& pt
  );
  //- Write obj representation of a line connecting two points
  //  Need to keep track of points that have been added. count starts at 0
  void writeOBJ
  (
    Ostream& os,
    const point& p1,
    const point& p2,
    label& count
  );
  //- Write obj representation of a point p1 with a vector from p1 to p2
  void writeOBJ
  (
    Ostream& os,
    const point& p1,
    const point& p2
  );
  //- Write obj representation of faces subset
  template<class FaceType>
  void writeOBJ
  (
    Ostream& os,
    const UList<FaceType>&,
    const pointField&,
    const labelList& faceLabels
  );
  //- Write obj representation of faces
  template<class FaceType>
  void writeOBJ
  (
    Ostream& os,
    const UList<FaceType>&,
    const pointField&
  );
  //- Write obj representation of cell subset
  void writeOBJ
  (
    Ostream& os,
    const cellList&,
    const faceList&,
    const pointField&,
    const labelList& cellLabels
  );
// Cell/face/edge walking
  //- Is edge used by cell
  bool edgeOnCell
  (
    const primitiveMesh&,
    const label cellI,
    const label edgeI
  );
  //- Is edge used by face
  bool edgeOnFace
  (
    const primitiveMesh&,
    const label faceI,
    const label edgeI
  );
  //- Is face used by cell
  bool faceOnCell
  (
    const primitiveMesh&,
    const label cellI,
    const label faceI
  );
  //- Return edge among candidates that uses the two vertices.
  label findEdge
  (
    const edgeList& edges,
    const labelList& candidates,
    const label v0,
    const label v1
  );
  //- Return edge between two vertices. Returns -1 if no edge.
  label findEdge
  (
    const primitiveMesh&,
    const label v0,
    const label v1
  );
  //- Return edge shared by two faces. Throws error if no edge found.
  label getSharedEdge
  (
    const primitiveMesh&,
    const label f0,
    const label f1
  );
  //- Return face shared by two cells. Throws error if none found.
  label getSharedFace
  (
    const primitiveMesh&,
    const label cell0,
    const label cell1
  );
  //- Get faces on cell using edgeI. Throws error if no two found.
  void getEdgeFaces
  (
    const primitiveMesh&,
    const label cellI,
    const label edgeI,
    label& face0,
    label& face1
  );
  //- Return label of other edge (out of candidates edgeLabels)
  //  connected to vertex but not edgeI. Throws error if none found.
  label otherEdge
  (
    const primitiveMesh&,
    const labelList& edgeLabels,
    const label edgeI,
    const label vertI
  );
  //- Return face on cell using edgeI but not faceI. Throws error
  //  if none found.
  label otherFace
  (
    const primitiveMesh&,
    const label cellI,
    const label faceI,
    const label edgeI
  );
  //- Return cell on other side of face. Throws error
  //  if face not internal.
  label otherCell
  (
    const primitiveMesh&,
    const label cellI,
    const label faceI
  );
  //- Returns label of edge nEdges away from startEdge (in the direction
  // of startVertI)
  label walkFace
  (
    const primitiveMesh&,
    const label faceI,
    const label startEdgeI,
    const label startVertI,
    const label nEdges
  );
// Constraints on position
  //- Set the constrained components of position to mesh centre
  void constrainToMeshCentre
  (
    const polyMesh& mesh,
    point& pt
  );
  void constrainToMeshCentre
  (
    const polyMesh& mesh,
    pointField& pt
  );
  //- Set the constrained components of directions/velocity to zero
  void constrainDirection
  (
    const polyMesh& mesh,
    const Vector<label>& dirs,
    vector& d
  );
  void constrainDirection
  (
    const polyMesh& mesh,
    const Vector<label>& dirs,
    vectorField& d
  );
// Hex only functionality.
  //- Given edge on hex find other 'parallel', non-connected edges.
  void getParallelEdges
  (
    const primitiveMesh&,
    const label cellI,
    const label e0,
    label&,
    label&,
    label&
  );
  //- Given edge on hex find all 'parallel' (i.e. non-connected)
  //  edges and average direction of them
  vector edgeToCutDir
  (
    const primitiveMesh&,
    const label cellI,
    const label edgeI
  );
  //- Reverse of edgeToCutDir: given direction find edge bundle and
  //  return one of them.
  label cutDirToEdge
  (
    const primitiveMesh&,
    const label cellI,
    const vector& cutDir
  );
}  // namespace meshTools
}  // namespace mousse

#include "mesh_tools.ipp"

#endif
