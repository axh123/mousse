#ifndef CORE_MESHES_MESH_SHAPES_FACE_HPP_
#define CORE_MESHES_MESH_SHAPES_FACE_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::face
// Description
//   A face is a list of labels corresponding to mesh vertices.
// SeeAlso
//   mousse::triFace

#include "point_field.hpp"
#include "label_list.hpp"
#include "edge_list.hpp"
#include "vector_field.hpp"
#include "face_list_fwd.hpp"
#include "intersection.hpp"
#include "point_hit.hpp"
#include "list_list_ops.hpp"
#include "istream.hpp"


namespace mousse {

// Forward declaration of friend functions and operators
class face;
class triFace;
template<class T, unsigned SizeInc, unsigned SizeMult, unsigned SizeDiv>
class DynamicList;
inline bool operator==(const face& a, const face& b);
inline bool operator!=(const face& a, const face& b);
inline Istream& operator>>(Istream&, face&);


class face
:
  public labelList
{
  // Private Member Functions
    //- Edge to the right of face vertex i
    inline label right(const label i) const;
    //- Edge to the left of face vertex i
    inline label left(const label i) const;
    //- Construct list of edge vectors for face
    tmp<vectorField> calcEdges
    (
      const pointField& points
    ) const;
    //- Cos between neighbouring edges
    scalar edgeCos
    (
      const vectorField& edges,
      const label index
    ) const;
    //- Find index of largest internal angle on face
    label mostConcaveAngle
    (
      const pointField& points,
      const vectorField& edges,
      scalar& edgeCos
    ) const;
    //- Enumeration listing the modes for split()
    enum splitMode
    {
      COUNTTRIANGLE,  // count if split into triangles
      COUNTQUAD,      // count if split into triangles&quads
      SPLITTRIANGLE,  // split into triangles
      SPLITQUAD       // split into triangles&quads
    };
    //- Split face into triangles or triangles&quads.
    //  Stores results quadFaces[quadI], triFaces[triI]
    //  Returns number of new faces created
    label split
    (
      const splitMode mode,
      const pointField& points,
      label& triI,
      label& quadI,
      faceList& triFaces,
      faceList& quadFaces
    ) const;
public:
  //- Return types for classify
  enum proxType
  {
    NONE,
    POINT,  // Close to point
    EDGE    // Close to edge
  };
  // Static data members
    static const char* const typeName;
  // Constructors
    //- Construct null
    inline face();
    //- Construct given size
    explicit inline face(label);
    //- Construct from list of labels
    explicit inline face(const labelUList&);
    //- Construct from list of labels
    explicit inline face(const labelList&);
    //- Construct by transferring the parameter contents
    explicit inline face(const Xfer<labelList>&);
    //- Copy construct from triFace
    face(const triFace&);
    //- Construct from Istream
    inline face(Istream&);
  // Member Functions
    //- Collapse face by removing duplicate point labels
    //  return the collapsed size
    label collapse();
    //- Flip the face in-place.
    //  The starting points of the original and flipped face are identical.
    void flip();
    //- Return the points corresponding to this face
    inline pointField points(const pointField&) const;
    //- Centre point of face
    point centre(const pointField&) const;
    //- Calculate average value at centroid of face
    template<class Type>
    Type average(const pointField&, const Field<Type>&) const;
    //- Magnitude of face area
    inline scalar mag(const pointField&) const;
    //- Vector normal; magnitude is equal to area of face
    vector normal(const pointField&) const;
    //- Return face with reverse direction
    //  The starting points of the original and reverse face are identical.
    face reverseFace() const;
    //- Navigation through face vertices
      //- Which vertex on face (face index given a global index)
      //  returns -1 if not found
      label which(const label globalIndex) const;
      //- Next vertex on face
      inline label nextLabel(const label i) const;
      //- Previous vertex on face
      inline label prevLabel(const label i) const;
    //- Return the volume swept out by the face when its points move
    scalar sweptVol
    (
      const pointField& oldPoints,
      const pointField& newPoints
    ) const;
    //- Return the inertia tensor, with optional reference
    //  point and density specification
    tensor inertia
    (
      const pointField&,
      const point& refPt = vector::zero,
      scalar density = 1.0
    ) const;
    //- Return potential intersection with face with a ray starting
    //  at p, direction n (does not need to be normalized)
    //  Does face-centre decomposition and returns triangle intersection
    //  point closest to p. Face-centre is calculated from point average.
    //  For a hit, the distance is signed.  Positive number
    //  represents the point in front of triangle
    //  In case of miss the point is the nearest point on the face
    //  and the distance is the distance between the intersection point
    //  and the original point.
    //  The half-ray or full-ray intersection and the contact
    //  sphere adjustment of the projection vector is set by the
    //  intersection parameters
    pointHit ray
    (
      const point& p,
      const vector& n,
      const pointField&,
      const intersection::algorithm alg = intersection::FULL_RAY,
      const intersection::direction dir = intersection::VECTOR
    ) const;
    //- Fast intersection with a ray.
    //  Does face-centre decomposition and returns triangle intersection
    //  point closest to p. See triangle::intersection for details.
    pointHit intersection
    (
      const point& p,
      const vector& q,
      const point& ctr,
      const pointField&,
      const intersection::algorithm alg,
      const scalar tol = 0.0
    ) const;
    //- Return nearest point to face
    pointHit nearestPoint
    (
      const point& p,
      const pointField&
    ) const;
    //- Return nearest point to face and classify it:
    //  + near point (nearType=POINT, nearLabel=0, 1, 2)
    //  + near edge (nearType=EDGE, nearLabel=0, 1, 2)
    //    Note: edges are counted from starting vertex so
    //    e.g. edge n is from f[n] to f[0], where the face has n + 1
    //    points
    pointHit nearestPointClassify
    (
      const point& p,
      const pointField&,
      label& nearType,
      label& nearLabel
    ) const;
    //- Return contact sphere diameter
    scalar contactSphereDiameter
    (
      const point& p,
      const vector& n,
      const pointField&
    ) const;
    //- Return area in contact, given the displacement in vertices
    scalar areaInContact
    (
      const pointField&,
      const scalarField& v
    ) const;
    //- Return number of edges
    inline label nEdges() const;
    //- Return edges in face point ordering,
    //  i.e. edges()[0] is edge between [0] and [1]
    edgeList edges() const;
    //- Return n-th face edge
    inline edge faceEdge(const label n) const;
    //- Return the edge direction on the face
    //  Returns:
    //  -  0: edge not found on the face
    //  - +1: forward (counter-clockwise) on the face
    //  - -1: reverse (clockwise) on the face
    int edgeDirection(const edge&) const;
    // Face splitting utilities
      //- Number of triangles after splitting
      inline label nTriangles() const;
      //- Number of triangles after splitting
      label nTriangles(const pointField& points) const;
      //- Split into triangles using existing points.
      //  Result in triFaces[triI..triI+nTri].
      //  Splits intelligently to maximize triangle quality.
      //  Returns number of faces created.
      label triangles
      (
        const pointField& points,
        label& triI,
        faceList& triFaces
      ) const;
      //- Split into triangles using existing points.
      //  Append to DynamicList.
      //  Returns number of faces created.
      template<unsigned SizeInc, unsigned SizeMult, unsigned SizeDiv>
      label triangles
      (
        const pointField& points,
        DynamicList<face, SizeInc, SizeMult, SizeDiv>& triFaces
      ) const;
      //- Number of triangles and quads after splitting
      //  Returns the sum of both
      label nTrianglesQuads
      (
        const pointField& points,
        label& nTris,
        label& nQuads
      ) const;
      //- Split into triangles and quads.
      //  Results in triFaces (starting at triI) and quadFaces
      //  (starting at quadI).
      //  Returns number of new faces created.
      label trianglesQuads
      (
        const pointField& points,
        label& triI,
        label& quadI,
        faceList& triFaces,
        faceList& quadFaces
      ) const;
    //- Compare faces
    //   0: different
    //  +1: identical
    //  -1: same face, but different orientation
    static int compare(const face&, const face&);
    //- Return true if the faces have the same vertices
    static bool sameVertices(const face&, const face&);
  // Friend Operators
    friend bool operator==(const face& a, const face& b);
    friend bool operator!=(const face& a, const face& b);
  // Istream Operator
    friend Istream& operator>>(Istream&, face&);
};


//- Hash specialization to offset faces in ListListOps::combineOffset
template<>
class offsetOp<face>
{
public:
  inline face operator()
  (
    const face& x,
    const label offset
  ) const
  {
    face result{x.size()};
    FOR_ALL(x, xI) {
      result[xI] = x[xI] + offset;
    }
    return result;
  }
};


// Global functions

//- Find the longest edge on a face. Face point labels index into pts.
label longestEdge(const face& f, const pointField& pts);

}  // namespace mousse


// Private Member Functions 

// Edge to the right of face vertex i
inline mousse::label mousse::face::right(const label i) const
{
  return i;
}


// Edge to the left of face vertex i
inline mousse::label mousse::face::left(const label i) const
{
  return rcIndex(i);
}


// Constructors 
inline mousse::face::face()
{}


inline mousse::face::face(label s)
:
  labelList{s, -1}
{}


inline mousse::face::face(const labelUList& lst)
:
  labelList{lst}
{}


inline mousse::face::face(const labelList& lst)
:
  labelList{lst}
{}


inline mousse::face::face(const Xfer<labelList>& lst)
:
  labelList{lst}
{}


inline mousse::face::face(Istream& is)
{
  is >> *this;
}


// Member Functions 
inline mousse::pointField mousse::face::points(const pointField& meshPoints) const
{
  // There are as many points as there labels for them
  pointField p{size()};
  // For each point in list, set it to the point in 'pnts' addressed
  // by 'labs'
  FOR_ALL(p, i) {
    p[i] = meshPoints[operator[](i)];
  }
  // Return list
  return p;
}


inline mousse::scalar mousse::face::mag(const pointField& p) const
{
  return ::mousse::mag(normal(p));
}


inline mousse::label mousse::face::nEdges() const
{
  // for a closed polygon a number of edges is the same as number of points
  return size();
}


inline mousse::edge mousse::face::faceEdge(const label n) const
{
  return {operator[](n), operator[](fcIndex(n))};
}


// Next vertex on face
inline mousse::label mousse::face::nextLabel(const label i) const
{
  return operator[](fcIndex(i));
}


// Previous vertex on face
inline mousse::label mousse::face::prevLabel(const label i) const
{
  return operator[](rcIndex(i));
}


// Number of triangles directly known from number of vertices
inline mousse::label mousse::face::nTriangles() const
{
  return size() - 2;
}


// Friend Operators 
inline bool mousse::operator==(const face& a, const face& b)
{
  return face::compare(a,b) != 0;
}


inline bool mousse::operator!=(const face& a, const face& b)
{
  return face::compare(a,b) == 0;
}


// IOstream Operators 
inline mousse::Istream& mousse::operator>>(Istream& is, face& f)
{
  if (is.version() == IOstream::originalVersion) {
    // Read starting (
    is.readBegin("face");
    // Read the 'name' token for the face
    token t(is);
    // Read labels
    is >> static_cast<labelList&>(f);
    // Read end)
    is.readEnd("face");
  } else {
    is >> static_cast<labelList&>(f);
  }
  // Check state of Ostream
  is.check("Istream& operator>>(Istream&, face&)");
  return is;
}

#include "face.ipp"

#endif
