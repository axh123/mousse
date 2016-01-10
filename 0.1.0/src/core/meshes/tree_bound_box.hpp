// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::treeBoundBox
// Description
//   Standard boundBox + extra functionality for use in octree.
//   Numbering of corner points is according to octant numbering.
//   On the back plane (z=0):
//   \verbatim
//     Y
//     ^
//     |
//     +--------+
//     |2      3|
//     |        |
//     |        |
//     |        |
//     |0      1|
//     +--------+->X
//   \endverbatim
//   For the front plane add 4 to the point labels.
// SourceFiles
//   tree_bound_box_i.hpp
//   tree_bound_box.cpp
//   tree_bound_box_templates.cpp

#ifndef tree_bound_box_hpp_
#define tree_bound_box_hpp_

#include "bound_box.hpp"
#include "direction.hpp"
#include "point_field.hpp"
#include "face_list.hpp"
#include "random.hpp"

namespace mousse
{

class Random;

class treeBoundBox
:
  public boundBox
{
private:
    //- To initialise edges.
    static edgeList calcEdges(const label[12][2]);

    //- To initialise faceNormals.
    static FixedList<vector, 6> calcFaceNormals();

public:
  // Static data members

    //- The great value used for greatBox and invertedBox
    static const scalar great;

    //- As per boundBox::greatBox, but with GREAT instead of VGREAT
    static const treeBoundBox greatBox;

    //- As per boundBox::invertedBox, but with GREAT instead of VGREAT
    static const treeBoundBox invertedBox;

    //- Bits used for octant/point coding.
    //  Every octant/corner point is the combination of three faces.
    enum octantBit
    {
      RIGHTHALF = 0x1 << 0,
      TOPHALF   = 0x1 << 1,
      FRONTHALF = 0x1 << 2
    };

    //- Face codes
    enum faceId
    {
      LEFT   = 0,
      RIGHT  = 1,
      BOTTOM = 2,
      TOP    = 3,
      BACK   = 4,
      FRONT  = 5
    };

    //- Bits used for face coding
    enum faceBit
    {
      NOFACE    = 0,
      LEFTBIT   = 0x1 << LEFT,    //1
      RIGHTBIT  = 0x1 << RIGHT,   //2
      BOTTOMBIT = 0x1 << BOTTOM,  //4
      TOPBIT    = 0x1 << TOP,     //8
      BACKBIT   = 0x1 << BACK,    //16
      FRONTBIT  = 0x1 << FRONT,   //32
    };

    //- Edges codes.
    //  E01 = edge between 0 and 1.
    enum edgeId
    {
      E01 = 0,
      E13 = 1,
      E23 = 2,
      E02 = 3,
      E45 = 4,
      E57 = 5,
      E67 = 6,
      E46 = 7,
      E04 = 8,
      E15 = 9,
      E37 = 10,
      E26 = 11
    };

    //- Face to point addressing
    static const faceList faces;

    //- Edge to point addressing
    static const edgeList edges;

    //- Per face the unit normal
    static const FixedList<vector, 6> faceNormals;

  // Constructors

    //- Construct null setting points to zero
    inline treeBoundBox();

    //- Construct from a boundBox
    explicit inline treeBoundBox(const boundBox& bb);

    //- Construct from components
    inline treeBoundBox(const point& min, const point& max);

    //- Construct as the bounding box of the given pointField.
    //  Local processor domain only (no reduce as in boundBox)
    explicit treeBoundBox(const UList<point>&);

    //- Construct as subset of points
    //  Local processor domain only (no reduce as in boundBox)
    treeBoundBox(const UList<point>&, const labelUList& indices);

    //- Construct as subset of points
    //  The indices could be from edge/triFace etc.
    //  Local processor domain only (no reduce as in boundBox)
    template<unsigned Size>
    treeBoundBox
    (
      const UList<point>&,
      const FixedList<label, Size>& indices
    );

    //- Construct from Istream
    inline treeBoundBox(Istream&);

  // Member functions

    // Access

      //- Typical dimension length,height,width
      inline scalar typDim() const;

      //- Vertex coordinates. In octant coding.
      tmp<pointField> points() const;

    // Check

      //- Corner point given octant
      inline point corner(const direction) const;

      //- Sub box given by octant number. Midpoint calculated.
      treeBoundBox subBbox(const direction) const;

      //- Sub box given by octant number. Midpoint provided.
      treeBoundBox subBbox(const point& mid, const direction) const;

      //- Returns octant number given point and the calculated midpoint.
      inline direction subOctant
      (
        const point& pt
      ) const;

      //- Returns octant number given point and midpoint.
      static inline direction subOctant
      (
        const point& mid,
        const point& pt
      );

      //- Returns octant number given point and the calculated midpoint.
      //  onEdge set if the point is on edge of subOctant
      inline direction subOctant
      (
        const point& pt,
        bool& onEdge
      ) const;

      //- Returns octant number given point and midpoint.
      //  onEdge set if the point is on edge of subOctant
      static inline direction subOctant
      (
        const point& mid,
        const point& pt,
        bool& onEdge
      );

      //- Returns octant number given intersection and midpoint.
      //  onEdge set if the point is on edge of subOctant
      //  If onEdge, the direction vector determines which octant to use
      //  (acc. to which octant the point would be if it were moved
      //  along dir)
      static inline direction subOctant
      (
        const point& mid,
        const vector& dir,
        const point& pt,
        bool& onEdge
      );

      //- Calculates optimal order to look for nearest to point.
      //  First will be the octant containing the point,
      //  second the octant with boundary nearest to the point etc.
      inline void searchOrder
      (
        const point& pt,
        FixedList<direction, 8>& octantOrder
      ) const;

      //- Overlaps other bounding box?
      using boundBox::overlaps;

      //- Intersects segment; set point to intersection position and face,
      //  return true if intersection found.
      //  (pt argument used during calculation even if not intersecting).
      //  Calculates intersections from outside supplied vector
      //  (overallStart, overallVec). This is so when
      //  e.g. tracking through lots of consecutive boxes
      // (typical octree) we're not accumulating truncation errors. Set
      // to start, (end-start) if not used.
      bool intersects
      (
        const point& overallStart,
        const vector& overallVec,
        const point& start,
        const point& end,
        point& pt,
        direction& ptBits
      ) const;

      //- Like above but does not return faces point is on
      bool intersects
      (
        const point& start,
        const point& end,
        point& pt
      ) const;

      //- Contains point or other bounding box?
      using boundBox::contains;

      //- Contains point (inside or on edge) and moving in direction
      //  dir would cause it to go inside.
      bool contains(const vector& dir, const point&) const;

      //- Code position of point on bounding box faces
      direction faceBits(const point&) const;

      //- Position of point relative to bounding box
      direction posBits(const point&) const;

      //- Calculate nearest and furthest (to point) vertex coords of
      //  bounding box
      void calcExtremities
      (
        const point& pt,
        point& nearest,
        point& furthest
      ) const;

      //- Returns distance point to furthest away corner.
      scalar maxDist(const point&) const;

      //- Compare distance to point with other bounding box
      //  return:
      //  -1 : all vertices of my bounding box are nearer than any of
      //       other
      //  +1 : all vertices of my bounding box are further away than
      //       any of other
      //   0 : none of the above.
      label distanceCmp(const point&, const treeBoundBox& other) const;

      //- Return slightly wider bounding box
      //  Extends all dimensions with s*span*Random::scalar01()
      //  and guarantees in any direction s*mag(span) minimum width
      inline treeBoundBox extend(Random&, const scalar s) const;

  // Friend Operators
    friend bool operator==(const treeBoundBox&, const treeBoundBox&);
    friend bool operator!=(const treeBoundBox&, const treeBoundBox&);

  // IOstream operator
    friend Istream& operator>>(Istream& is, treeBoundBox&);
    friend Ostream& operator<<(Ostream& os, const treeBoundBox&);
};

//- Data associated with treeBoundBox type are contiguous
template<>
inline bool contiguous<treeBoundBox>() {return contiguous<boundBox>();}
}  // namespace mousse

// Constructors 
inline mousse::treeBoundBox::treeBoundBox()
:
  boundBox{}
{}


inline mousse::treeBoundBox::treeBoundBox(const point& min, const point& max)
:
  boundBox{min, max}
{}


inline mousse::treeBoundBox::treeBoundBox(const boundBox& bb)
:
  boundBox{bb}
{}


inline mousse::treeBoundBox::treeBoundBox(Istream& is)
:
  boundBox{is}
{}


// Member Functions 
inline mousse::scalar mousse::treeBoundBox::typDim() const
{
  return avgDim();
}


inline mousse::point mousse::treeBoundBox::corner(const direction octant) const
{
  return point
  {
    (octant & RIGHTHALF) ? max().x() : min().x(),
    (octant & TOPHALF)   ? max().y() : min().y(),
    (octant & FRONTHALF) ? max().z() : min().z()
  };
}


// Returns octant in which point resides. Reverse of subBbox.
inline mousse::direction mousse::treeBoundBox::subOctant(const point& pt) const
{
  return subOctant(midpoint(), pt);
}


// Returns octant in which point resides. Reverse of subBbox.
// Precalculated midpoint
inline mousse::direction mousse::treeBoundBox::subOctant
(
  const point& mid,
  const point& pt
)
{
  direction octant = 0;
  if (pt.x() > mid.x())
  {
    octant |= treeBoundBox::RIGHTHALF;
  }
  if (pt.y() > mid.y())
  {
    octant |= treeBoundBox::TOPHALF;
  }
  if (pt.z() > mid.z())
  {
    octant |= treeBoundBox::FRONTHALF;
  }
  return octant;
}


// Returns octant in which point resides. Reverse of subBbox.
// Flags point exactly on edge.
inline mousse::direction mousse::treeBoundBox::subOctant
(
  const point& pt,
  bool& onEdge
) const
{
  return subOctant(midpoint(), pt, onEdge);
}


// Returns octant in which point resides. Reverse of subBbox.
// Precalculated midpoint
inline mousse::direction mousse::treeBoundBox::subOctant
(
  const point& mid,
  const point& pt,
  bool& onEdge
)
{
  direction octant = 0;
  onEdge = false;
  if (pt.x() > mid.x())
  {
    octant |= treeBoundBox::RIGHTHALF;
  }
  else if (pt.x() == mid.x())
  {
    onEdge = true;
  }
  if (pt.y() > mid.y())
  {
    octant |= treeBoundBox::TOPHALF;
  }
  else if (pt.y() == mid.y())
  {
    onEdge = true;
  }
  if (pt.z() > mid.z())
  {
    octant |= treeBoundBox::FRONTHALF;
  }
  else if (pt.z() == mid.z())
  {
    onEdge = true;
  }
  return octant;
}


// Returns octant in which intersection resides.
// Precalculated midpoint. If the point is on the dividing line between
// the octants the direction vector determines which octant to use
// (i.e. in which octant the point would be if it were moved along dir)
inline mousse::direction mousse::treeBoundBox::subOctant
(
  const point& mid,
  const vector& dir,
  const point& pt,
  bool& onEdge
)
{
  direction octant = 0;
  onEdge = false;
  if (pt.x() > mid.x())
  {
    octant |= treeBoundBox::RIGHTHALF;
  }
  else if (pt.x() == mid.x())
  {
    onEdge = true;
    if (dir.x() > 0)
    {
      octant |= treeBoundBox::RIGHTHALF;
    }
  }
  if (pt.y() > mid.y())
  {
    octant |= treeBoundBox::TOPHALF;
  }
  else if (pt.y() == mid.y())
  {
    onEdge = true;
    if (dir.y() > 0)
    {
      octant |= treeBoundBox::TOPHALF;
    }
  }
  if (pt.z() > mid.z())
  {
    octant |= treeBoundBox::FRONTHALF;
  }
  else if (pt.z() == mid.z())
  {
    onEdge = true;
    if (dir.z() > 0)
    {
      octant |= treeBoundBox::FRONTHALF;
    }
  }
  return octant;
}


// Returns reference to octantOrder which defines the
// order to do the search.
inline void mousse::treeBoundBox::searchOrder
(
  const point& pt,
  FixedList<direction,8>& octantOrder
) const
{
  vector dist = midpoint() - pt;
  direction octant = 0;
  if (dist.x() < 0)
  {
    octant |= treeBoundBox::RIGHTHALF;
    dist.x() *= -1;
  }
  if (dist.y() < 0)
  {
    octant |= treeBoundBox::TOPHALF;
    dist.y() *= -1;
  }
  if (dist.z() < 0)
  {
    octant |= treeBoundBox::FRONTHALF;
    dist.z() *= -1;
  }
  direction min = 0;
  direction mid = 0;
  direction max = 0;
  if (dist.x() < dist.y())
  {
    if (dist.y() < dist.z())
    {
      min = treeBoundBox::RIGHTHALF;
      mid = treeBoundBox::TOPHALF;
      max = treeBoundBox::FRONTHALF;
    }
    else if (dist.z() < dist.x())
    {
      min = treeBoundBox::FRONTHALF;
      mid = treeBoundBox::RIGHTHALF;
      max = treeBoundBox::TOPHALF;
    }
    else
    {
      min = treeBoundBox::RIGHTHALF;
      mid = treeBoundBox::FRONTHALF;
      max = treeBoundBox::TOPHALF;
    }
  }
  else
  {
    if (dist.z() < dist.y())
    {
      min = treeBoundBox::FRONTHALF;
      mid = treeBoundBox::TOPHALF;
      max = treeBoundBox::RIGHTHALF;
    }
    else if (dist.x() < dist.z())
    {
      min = treeBoundBox::TOPHALF;
      mid = treeBoundBox::RIGHTHALF;
      max = treeBoundBox::FRONTHALF;
    }
    else
    {
      min = treeBoundBox::TOPHALF;
      mid = treeBoundBox::FRONTHALF;
      max = treeBoundBox::RIGHTHALF;
    }
  }
  // Primary subOctant
  octantOrder[0] = octant;
  // subOctants joined to the primary by faces.
  octantOrder[1] = octant ^ min;
  octantOrder[2] = octant ^ mid;
  octantOrder[3] = octant ^ max;
  // subOctants joined to the primary by edges.
  octantOrder[4] = octantOrder[1] ^ mid;
  octantOrder[5] = octantOrder[1] ^ max;
  octantOrder[6] = octantOrder[2] ^ max;
  // subOctants joined to the primary by corner.
  octantOrder[7] = octantOrder[4] ^ max;
}


//- Return slightly wider bounding box
inline mousse::treeBoundBox mousse::treeBoundBox::extend
(
  Random& rndGen,
  const scalar s
) const
{
  treeBoundBox bb(*this);
  vector newSpan = bb.span();
  // Make 3D
  scalar minSpan = s * mousse::mag(newSpan);
  for (direction dir = 0; dir < vector::nComponents; dir++)
  {
    newSpan[dir] = mousse::max(newSpan[dir], minSpan);
  }
  bb.min() -= cmptMultiply(s * rndGen.vector01(), newSpan);
  bb.max() += cmptMultiply(s * rndGen.vector01(), newSpan);
  return bb;
}

#ifdef NoRepository
#   include "tree_bound_box_templates.cpp"
#endif
#endif
