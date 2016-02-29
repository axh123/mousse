#ifndef CORE_MESHES_PRIMITIVE_SHAPES_TETRAHEDRON_HPP_
#define CORE_MESHES_PRIMITIVE_SHAPES_TETRAHEDRON_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011-2013 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::tetrahedron
// Description
//   A tetrahedron primitive.
//   Ordering of edges needs to be the same for a tetrahedron
//   class, a tetrahedron cell shape model and a tetCell.
// SourceFiles
//   tetrahedron.cpp
#include "point.hpp"
#include "primitive_fields_fwd.hpp"
#include "point_hit.hpp"
#include "cached_random.hpp"
#include "random.hpp"
#include "fixed_list.hpp"
#include "ulist.hpp"
#include "tri_point_ref.hpp"
#include "triangle.hpp"
#include "iostreams.hpp"
#include "plane.hpp"
namespace mousse
{
class Istream;
class Ostream;
class tetPoints;
class plane;
// Forward declaration of friend functions and operators
template<class Point, class PointRef> class tetrahedron;
template<class Point, class PointRef>
inline Istream& operator>>
(
  Istream&,
  tetrahedron<Point, PointRef>&
);
template<class Point, class PointRef>
inline Ostream& operator<<
(
  Ostream&,
  const tetrahedron<Point, PointRef>&
);
typedef tetrahedron<point, const point&> tetPointRef;
template<class Point, class PointRef>
class tetrahedron
{
public:
  // Public typedefs
    //- Storage type for tets originating from intersecting tets.
    //  (can possibly be smaller than 200)
    typedef FixedList<tetPoints, 200> tetIntersectionList;
    // Classes for use in sliceWithPlane. What to do with decomposition
    // of tet.
      //- Dummy
      class dummyOp
      {
      public:
        inline void operator()(const tetPoints&);
      };
      //- Sum resulting volumes
      class sumVolOp
      {
      public:
        scalar vol_;
        inline sumVolOp();
        inline void operator()(const tetPoints&);
      };
      //- Store resulting tets
      class storeOp
      {
        tetIntersectionList& tets_;
        label& nTets_;
      public:
        inline storeOp(tetIntersectionList&, label&);
        inline void operator()(const tetPoints&);
      };
private:
  // Private data
    PointRef a_, b_, c_, d_;
    inline static point planeIntersection
    (
      const FixedList<scalar, 4>&,
      const tetPoints&,
      const label,
      const label
    );
    template<class TetOp>
    inline static void decomposePrism
    (
      const FixedList<point, 6>& points,
      TetOp& op
    );
    template<class AboveTetOp, class BelowTetOp>
    inline static void tetSliceWithPlane
    (
      const plane& pl,
      const tetPoints& tet,
      AboveTetOp& aboveOp,
      BelowTetOp& belowOp
    );
public:
  // Member constants
    enum
    {
      nVertices = 4,  // Number of vertices in tetrahedron
      nEdges = 6      // Number of edges in tetrahedron
    };
  // Constructors
    //- Construct from points
    inline tetrahedron
    (
      const Point& a,
      const Point& b,
      const Point& c,
      const Point& d
    );
    //- Construct from four points in the list of points
    inline tetrahedron
    (
      const UList<Point>&,
      const FixedList<label, 4>& indices
    );
    //- Construct from Istream
    inline tetrahedron(Istream&);
  // Member Functions
    // Access
      //- Return vertices
      inline const Point& a() const;
      inline const Point& b() const;
      inline const Point& c() const;
      inline const Point& d() const;
      //- Return i-th face
      inline triPointRef tri(const label faceI) const;
    // Properties
      //- Return face normal
      inline vector Sa() const;
      inline vector Sb() const;
      inline vector Sc() const;
      inline vector Sd() const;
      //- Return centre (centroid)
      inline Point centre() const;
      //- Return volume
      inline scalar mag() const;
      //- Return circum-centre
      inline Point circumCentre() const;
      //- Return circum-radius
      inline scalar circumRadius() const;
      //- Return quality: Ratio of tetrahedron and circum-sphere
      //  volume, scaled so that a regular tetrahedron has a
      //  quality of 1
      inline scalar quality() const;
      //- Return a random point in the tetrahedron from a
      //  uniform distribution
      inline Point randomPoint(Random& rndGen) const;
      //- Return a random point in the tetrahedron from a
      //  uniform distribution
      inline Point randomPoint(cachedRandom& rndGen) const;
      //- Calculate the barycentric coordinates of the given
      //  point, in the same order as a, b, c, d.  Returns the
      //  determinant of the solution.
      inline scalar barycentric
      (
        const point& pt,
        List<scalar>& bary
      ) const;
      //- Return nearest point to p on tetrahedron. Is p itself
      //  if inside.
      inline pointHit nearestPoint(const point& p) const;
      //- Return true if point is inside tetrahedron
      inline bool inside(const point& pt) const;
      //- Decompose tet into tets above and below plane
      template<class AboveTetOp, class BelowTetOp>
      inline void sliceWithPlane
      (
        const plane& pl,
        AboveTetOp& aboveOp,
        BelowTetOp& belowOp
      ) const;
      //- Decompose tet into tets inside and outside other tet
      inline void tetOverlap
      (
        const tetrahedron<Point, PointRef>& tetB,
        tetIntersectionList& insideTets,
        label& nInside,
        tetIntersectionList& outsideTets,
        label& nOutside
      ) const;
      //- Return (min)containment sphere, i.e. the smallest sphere with
      //  all points inside. Returns pointHit with:
      //  - hit         : if sphere is equal to circumsphere
      //                  (biggest sphere)
      //  - point       : centre of sphere
      //  - distance    : radius of sphere
      //  - eligiblemiss: false
      // Tol (small compared to 1, e.g. 1e-9) is used to determine
      // whether point is inside: mag(pt - ctr) < (1+tol)*radius.
      pointHit containmentSphere(const scalar tol) const;
      //- Fill buffer with shape function products
      void gradNiSquared(scalarField& buffer) const;
      void gradNiDotGradNj(scalarField& buffer) const;
      void gradNiGradNi(tensorField& buffer) const;
      void gradNiGradNj(tensorField& buffer) const;
  // IOstream operators
    friend Istream& operator>> <Point, PointRef>
    (
      Istream&,
      tetrahedron&
    );
    friend Ostream& operator<< <Point, PointRef>
    (
      Ostream&,
      const tetrahedron&
    );
};
}  // namespace mousse

#include "tet_points.hpp"
// Constructors 
template<class Point, class PointRef>
inline mousse::tetrahedron<Point, PointRef>::tetrahedron
(
  const Point& a,
  const Point& b,
  const Point& c,
  const Point& d
)
:
  a_{a},
  b_{b},
  c_{c},
  d_{d}
{}
template<class Point, class PointRef>
inline mousse::tetrahedron<Point, PointRef>::tetrahedron
(
  const UList<Point>& points,
  const FixedList<label, 4>& indices
)
:
  a_{points[indices[0]]},
  b_{points[indices[1]]},
  c_{points[indices[2]]},
  d_{points[indices[3]]}
{}
template<class Point, class PointRef>
inline mousse::tetrahedron<Point, PointRef>::tetrahedron(Istream& is)
{
  is  >> *this;
}
// Member Functions 
template<class Point, class PointRef>
inline const Point& mousse::tetrahedron<Point, PointRef>::a() const
{
  return a_;
}
template<class Point, class PointRef>
inline const Point& mousse::tetrahedron<Point, PointRef>::b() const
{
  return b_;
}
template<class Point, class PointRef>
inline const Point& mousse::tetrahedron<Point, PointRef>::c() const
{
  return c_;
}
template<class Point, class PointRef>
inline const Point& mousse::tetrahedron<Point, PointRef>::d() const
{
  return d_;
}
template<class Point, class PointRef>
inline mousse::triPointRef mousse::tetrahedron<Point, PointRef>::tri
(
  const label faceI
) const
{
  // Warning. Ordering of faces needs to be the same for a tetrahedron
  // class, a tetrahedron cell shape model and a tetCell
  if (faceI == 0)
  {
    return triPointRef(b_, c_, d_);
  }
  else if (faceI == 1)
  {
    return triPointRef(a_, d_, c_);
  }
  else if (faceI == 2)
  {
    return triPointRef(a_, b_, d_);
  }
  else if (faceI == 3)
  {
    return triPointRef(a_, c_, b_);
  }
  else
  {
    FATAL_ERROR_IN("tetrahedron::tri(const label faceI) const")
      << "index out of range 0 -> 3. faceI = " << faceI
      << abort(FatalError);
    return triPointRef(b_, c_, d_);
  }
}
template<class Point, class PointRef>
inline mousse::vector mousse::tetrahedron<Point, PointRef>::Sa() const
{
  return triangle<Point, PointRef>(b_, c_, d_).normal();
}
template<class Point, class PointRef>
inline mousse::vector mousse::tetrahedron<Point, PointRef>::Sb() const
{
  return triangle<Point, PointRef>(a_, d_, c_).normal();
}
template<class Point, class PointRef>
inline mousse::vector mousse::tetrahedron<Point, PointRef>::Sc() const
{
  return triangle<Point, PointRef>(a_, b_, d_).normal();
}
template<class Point, class PointRef>
inline mousse::vector mousse::tetrahedron<Point, PointRef>::Sd() const
{
  return triangle<Point, PointRef>(a_, c_, b_).normal();
}
template<class Point, class PointRef>
inline Point mousse::tetrahedron<Point, PointRef>::centre() const
{
  return 0.25*(a_ + b_ + c_ + d_);
}
template<class Point, class PointRef>
inline mousse::scalar mousse::tetrahedron<Point, PointRef>::mag() const
{
  return (1.0/6.0)*(((b_ - a_) ^ (c_ - a_)) & (d_ - a_));
}
template<class Point, class PointRef>
inline Point mousse::tetrahedron<Point, PointRef>::circumCentre() const
{
  vector a = b_ - a_;
  vector b = c_ - a_;
  vector c = d_ - a_;
  scalar lambda = magSqr(c) - (a & c);
  scalar mu = magSqr(b) - (a & b);
  vector ba = b ^ a;
  vector ca = c ^ a;
  vector num = lambda*ba - mu*ca;
  scalar denom = (c & ba);
  if (mousse::mag(denom) < ROOTVSMALL)
  {
    // Degenerate tetrahedron, returning centre instead of circumCentre.
    return centre();
  }
  return a_ + 0.5*(a + num/denom);
}
template<class Point, class PointRef>
inline mousse::scalar mousse::tetrahedron<Point, PointRef>::circumRadius() const
{
  vector a = b_ - a_;
  vector b = c_ - a_;
  vector c = d_ - a_;
  scalar lambda = magSqr(c) - (a & c);
  scalar mu = magSqr(b) - (a & b);
  vector ba = b ^ a;
  vector ca = c ^ a;
  vector num = lambda*ba - mu*ca;
  scalar denom = (c & ba);
  if (mousse::mag(denom) < ROOTVSMALL)
  {
    // Degenerate tetrahedron, returning GREAT for circumRadius.
    return GREAT;
  }
  return mousse::mag(0.5*(a + num/denom));
}
template<class Point, class PointRef>
inline mousse::scalar mousse::tetrahedron<Point, PointRef>::quality() const
{
  return
    mag()
   /(
      8.0/(9.0*sqrt(3.0))
     *pow3(min(circumRadius(), GREAT))
     + ROOTVSMALL
    );
}
template<class Point, class PointRef>
inline Point mousse::tetrahedron<Point, PointRef>::randomPoint
(
  Random& rndGen
) const
{
  // Adapted from
  // http://vcg.isti.cnr.it/activities/geometryegraphics/pointintetraedro.html
  scalar s = rndGen.scalar01();
  scalar t = rndGen.scalar01();
  scalar u = rndGen.scalar01();
  if (s + t > 1.0)
  {
    s = 1.0 - s;
    t = 1.0 - t;
  }
  if (t + u > 1.0)
  {
    scalar tmp = u;
    u = 1.0 - s - t;
    t = 1.0 - tmp;
  }
  else if (s + t + u > 1.0)
  {
    scalar tmp = u;
    u = s + t + u - 1.0;
    s = 1.0 - t - tmp;
  }
  return (1 - s - t - u)*a_ + s*b_ + t*c_ + u*d_;
}
template<class Point, class PointRef>
inline Point mousse::tetrahedron<Point, PointRef>::randomPoint
(
  cachedRandom& rndGen
) const
{
  // Adapted from
  // http://vcg.isti.cnr.it/activities/geometryegraphics/pointintetraedro.html
  scalar s = rndGen.sample01<scalar>();
  scalar t = rndGen.sample01<scalar>();
  scalar u = rndGen.sample01<scalar>();
  if (s + t > 1.0)
  {
    s = 1.0 - s;
    t = 1.0 - t;
  }
  if (t + u > 1.0)
  {
    scalar tmp = u;
    u = 1.0 - s - t;
    t = 1.0 - tmp;
  }
  else if (s + t + u > 1.0)
  {
    scalar tmp = u;
    u = s + t + u - 1.0;
    s = 1.0 - t - tmp;
  }
  return (1 - s - t - u)*a_ + s*b_ + t*c_ + u*d_;
}
template<class Point, class PointRef>
mousse::scalar mousse::tetrahedron<Point, PointRef>::barycentric
(
  const point& pt,
  List<scalar>& bary
) const
{
  // From:
  // http://en.wikipedia.org/wiki/Barycentric_coordinate_system_(mathematics)
  vector e0(a_ - d_);
  vector e1(b_ - d_);
  vector e2(c_ - d_);
  tensor t
  (
    e0.x(), e1.x(), e2.x(),
    e0.y(), e1.y(), e2.y(),
    e0.z(), e1.z(), e2.z()
  );
  scalar detT = det(t);
  if (mousse::mag(detT) < SMALL)
  {
    // Degenerate tetrahedron, returning 1/4 barycentric coordinates.
    bary = List<scalar>(4, 0.25);
    return detT;
  }
  vector res = inv(t, detT) & (pt - d_);
  bary.setSize(4);
  bary[0] = res.x();
  bary[1] = res.y();
  bary[2] = res.z();
  bary[3] = (1.0 - res.x() - res.y() - res.z());
  return detT;
}
template<class Point, class PointRef>
inline mousse::pointHit mousse::tetrahedron<Point, PointRef>::nearestPoint
(
  const point& p
) const
{
  // Adapted from:
  // Real-time collision detection, Christer Ericson, 2005, p142-144
  // Assuming initially that the point is inside all of the
  // halfspaces, so closest to itself.
  point closestPt = p;
  scalar minOutsideDistance = VGREAT;
  bool inside = true;
  if (((p - b_) & Sa()) >= 0)
  {
    // p is outside halfspace plane of tri
    pointHit info = triangle<Point, PointRef>(b_, c_, d_).nearestPoint(p);
    inside = false;
    if (info.distance() < minOutsideDistance)
    {
      closestPt = info.rawPoint();
      minOutsideDistance = info.distance();
    }
  }
  if (((p - a_) & Sb()) >= 0)
  {
    // p is outside halfspace plane of tri
    pointHit info = triangle<Point, PointRef>(a_, d_, c_).nearestPoint(p);
    inside = false;
    if (info.distance() < minOutsideDistance)
    {
      closestPt = info.rawPoint();
      minOutsideDistance = info.distance();
    }
  }
  if (((p - a_) & Sc()) >= 0)
  {
    // p is outside halfspace plane of tri
    pointHit info = triangle<Point, PointRef>(a_, b_, d_).nearestPoint(p);
    inside = false;
    if (info.distance() < minOutsideDistance)
    {
      closestPt = info.rawPoint();
      minOutsideDistance = info.distance();
    }
  }
  if (((p - a_) & Sd()) >= 0)
  {
    // p is outside halfspace plane of tri
    pointHit info = triangle<Point, PointRef>(a_, c_, b_).nearestPoint(p);
    inside = false;
    if (info.distance() < minOutsideDistance)
    {
      closestPt = info.rawPoint();
      minOutsideDistance = info.distance();
    }
  }
  // If the point is inside, then the distance to the closest point
  // is zero
  if (inside)
  {
    minOutsideDistance = 0;
  }
  return pointHit
  (
    inside,
    closestPt,
    minOutsideDistance,
    !inside
  );
}
template<class Point, class PointRef>
bool mousse::tetrahedron<Point, PointRef>::inside(const point& pt) const
{
  // For robustness, assuming that the point is in the tet unless
  // "definitively" shown otherwise by obtaining a positive dot
  // product greater than a tolerance of SMALL.
  // The tet is defined: tet(Cc, tetBasePt, pA, pB) where the normal
  // vectors and base points for the half-space planes are:
  // area[0] = Sa();
  // area[1] = Sb();
  // area[2] = Sc();
  // area[3] = Sd();
  // planeBase[0] = tetBasePt = b_
  // planeBase[1] = ptA       = c_
  // planeBase[2] = tetBasePt = b_
  // planeBase[3] = tetBasePt = b_
  vector n = vector::zero;
  {
    // 0, a
    const point& basePt = b_;
    n = Sa();
    n /= (mousse::mag(n) + VSMALL);
    if (((pt - basePt) & n) > SMALL)
    {
      return false;
    }
  }
  {
    // 1, b
    const point& basePt = c_;
    n = Sb();
    n /= (mousse::mag(n) + VSMALL);
    if (((pt - basePt) & n) > SMALL)
    {
      return false;
    }
  }
  {
    // 2, c
    const point& basePt = b_;
    n = Sc();
    n /= (mousse::mag(n) + VSMALL);
    if (((pt - basePt) & n) > SMALL)
    {
      return false;
    }
  }
  {
    // 3, d
    const point& basePt = b_;
    n = Sd();
    n /= (mousse::mag(n) + VSMALL);
    if (((pt - basePt) & n) > SMALL)
    {
      return false;
    }
  }
  return true;
}
template<class Point, class PointRef>
inline void mousse::tetrahedron<Point, PointRef>::dummyOp::operator()
(
  const tetPoints&
)
{}
template<class Point, class PointRef>
inline mousse::tetrahedron<Point, PointRef>::sumVolOp::sumVolOp()
:
  vol_(0.0)
{}
template<class Point, class PointRef>
inline void mousse::tetrahedron<Point, PointRef>::sumVolOp::operator()
(
  const tetPoints& tet
)
{
  vol_ += tet.tet().mag();
}
template<class Point, class PointRef>
inline mousse::tetrahedron<Point, PointRef>::storeOp::storeOp
(
  tetIntersectionList& tets,
  label& nTets
)
:
  tets_(tets),
  nTets_(nTets)
{}
template<class Point, class PointRef>
inline void mousse::tetrahedron<Point, PointRef>::storeOp::operator()
(
  const tetPoints& tet
)
{
  tets_[nTets_++] = tet;
}
template<class Point, class PointRef>
inline mousse::point mousse::tetrahedron<Point, PointRef>::planeIntersection
(
  const FixedList<scalar, 4>& d,
  const tetPoints& t,
  const label negI,
  const label posI
)
{
  return
    (d[posI]*t[negI] - d[negI]*t[posI])
   / (-d[negI]+d[posI]);
}
template<class Point, class PointRef>
template<class TetOp>
inline void mousse::tetrahedron<Point, PointRef>::decomposePrism
(
  const FixedList<point, 6>& points,
  TetOp& op
)
{
  op(tetPoints(points[1], points[3], points[2], points[0]));
  op(tetPoints(points[1], points[2], points[3], points[4]));
  op(tetPoints(points[4], points[2], points[3], points[5]));
}
template<class Point, class PointRef>
template<class AboveTetOp, class BelowTetOp>
inline void mousse::tetrahedron<Point, PointRef>::
tetSliceWithPlane
(
  const plane& pl,
  const tetPoints& tet,
  AboveTetOp& aboveOp,
  BelowTetOp& belowOp
)
{
  // Distance to plane
  FixedList<scalar, 4> d;
  label nPos = 0;
  FOR_ALL(tet, i)
  {
    d[i] = ((tet[i]-pl.refPoint()) & pl.normal());
    if (d[i] > 0)
    {
      nPos++;
    }
  }
  if (nPos == 4)
  {
    aboveOp(tet);
  }
  else if (nPos == 3)
  {
    // Sliced into below tet and above prism. Prism gets split into
    // two tets.
    // Find the below tet
    label i0 = -1;
    FOR_ALL(d, i)
    {
      if (d[i] <= 0)
      {
        i0 = i;
        break;
      }
    }
    label i1 = d.fcIndex(i0);
    label i2 = d.fcIndex(i1);
    label i3 = d.fcIndex(i2);
    point p01 = planeIntersection(d, tet, i0, i1);
    point p02 = planeIntersection(d, tet, i0, i2);
    point p03 = planeIntersection(d, tet, i0, i3);
    // i0 = tetCell vertex 0: p01,p02,p03 outwards pointing triad
    //          ,,         1 :     ,,     inwards pointing triad
    //          ,,         2 :     ,,     outwards pointing triad
    //          ,,         3 :     ,,     inwards pointing triad
    //Pout<< "Split 3pos tet " << tet << " d:" << d << " into" << nl;
    if (i0 == 0 || i0 == 2)
    {
      tetPoints t(tet[i0], p01, p02, p03);
      //Pout<< "    belowtet:" << t << " around i0:" << i0 << endl;
      //checkTet(t, "nPos 3, belowTet i0==0 or 2");
      belowOp(t);
      // Prism
      FixedList<point, 6> p;
      p[0] = tet[i1];
      p[1] = tet[i3];
      p[2] = tet[i2];
      p[3] = p01;
      p[4] = p03;
      p[5] = p02;
      //Pout<< "    aboveprism:" << p << endl;
      decomposePrism(p, aboveOp);
    }
    else
    {
      tetPoints t(p01, p02, p03, tet[i0]);
      //Pout<< "    belowtet:" << t << " around i0:" << i0 << endl;
      //checkTet(t, "nPos 3, belowTet i0==1 or 3");
      belowOp(t);
      // Prism
      FixedList<point, 6> p;
      p[0] = tet[i3];
      p[1] = tet[i1];
      p[2] = tet[i2];
      p[3] = p03;
      p[4] = p01;
      p[5] = p02;
      //Pout<< "    aboveprism:" << p << endl;
      decomposePrism(p, aboveOp);
    }
  }
  else if (nPos == 2)
  {
    // Tet cut into two prisms. Determine the positive one.
    label pos0 = -1;
    label pos1 = -1;
    FOR_ALL(d, i)
    {
      if (d[i] > 0)
      {
        if (pos0 == -1)
        {
          pos0 = i;
        }
        else
        {
          pos1 = i;
        }
      }
    }
    //Pout<< "Split 2pos tet " << tet << " d:" << d
    //    << " around pos0:" << pos0 << " pos1:" << pos1
    //    << " neg0:" << neg0 << " neg1:" << neg1 << " into" << nl;
    const edge posEdge(pos0, pos1);
    if (posEdge == edge(0, 1))
    {
      point p02 = planeIntersection(d, tet, 0, 2);
      point p03 = planeIntersection(d, tet, 0, 3);
      point p12 = planeIntersection(d, tet, 1, 2);
      point p13 = planeIntersection(d, tet, 1, 3);
      // Split the resulting prism
      {
        FixedList<point, 6> p;
        p[0] = tet[0];
        p[1] = p02;
        p[2] = p03;
        p[3] = tet[1];
        p[4] = p12;
        p[5] = p13;
        //Pout<< "    01 aboveprism:" << p << endl;
        decomposePrism(p, aboveOp);
      }
      {
        FixedList<point, 6> p;
        p[0] = tet[2];
        p[1] = p02;
        p[2] = p12;
        p[3] = tet[3];
        p[4] = p03;
        p[5] = p13;
        //Pout<< "    01 belowprism:" << p << endl;
        decomposePrism(p, belowOp);
      }
    }
    else if (posEdge == edge(1, 2))
    {
      point p01 = planeIntersection(d, tet, 0, 1);
      point p13 = planeIntersection(d, tet, 1, 3);
      point p02 = planeIntersection(d, tet, 0, 2);
      point p23 = planeIntersection(d, tet, 2, 3);
      // Split the resulting prism
      {
        FixedList<point, 6> p;
        p[0] = tet[1];
        p[1] = p01;
        p[2] = p13;
        p[3] = tet[2];
        p[4] = p02;
        p[5] = p23;
        //Pout<< "    12 aboveprism:" << p << endl;
        decomposePrism(p, aboveOp);
      }
      {
        FixedList<point, 6> p;
        p[0] = tet[3];
        p[1] = p23;
        p[2] = p13;
        p[3] = tet[0];
        p[4] = p02;
        p[5] = p01;
        //Pout<< "    12 belowprism:" << p << endl;
        decomposePrism(p, belowOp);
      }
    }
    else if (posEdge == edge(2, 0))
    {
      point p01 = planeIntersection(d, tet, 0, 1);
      point p03 = planeIntersection(d, tet, 0, 3);
      point p12 = planeIntersection(d, tet, 1, 2);
      point p23 = planeIntersection(d, tet, 2, 3);
      // Split the resulting prism
      {
        FixedList<point, 6> p;
        p[0] = tet[2];
        p[1] = p12;
        p[2] = p23;
        p[3] = tet[0];
        p[4] = p01;
        p[5] = p03;
        //Pout<< "    20 aboveprism:" << p << endl;
        decomposePrism(p, aboveOp);
      }
      {
        FixedList<point, 6> p;
        p[0] = tet[1];
        p[1] = p12;
        p[2] = p01;
        p[3] = tet[3];
        p[4] = p23;
        p[5] = p03;
        //Pout<< "    20 belowprism:" << p << endl;
        decomposePrism(p, belowOp);
      }
    }
    else if (posEdge == edge(0, 3))
    {
      point p01 = planeIntersection(d, tet, 0, 1);
      point p02 = planeIntersection(d, tet, 0, 2);
      point p13 = planeIntersection(d, tet, 1, 3);
      point p23 = planeIntersection(d, tet, 2, 3);
      // Split the resulting prism
      {
        FixedList<point, 6> p;
        p[0] = tet[3];
        p[1] = p23;
        p[2] = p13;
        p[3] = tet[0];
        p[4] = p02;
        p[5] = p01;
        //Pout<< "    03 aboveprism:" << p << endl;
        decomposePrism(p, aboveOp);
      }
      {
        FixedList<point, 6> p;
        p[0] = tet[2];
        p[1] = p23;
        p[2] = p02;
        p[3] = tet[1];
        p[4] = p13;
        p[5] = p01;
        //Pout<< "    03 belowprism:" << p << endl;
        decomposePrism(p, belowOp);
      }
    }
    else if (posEdge == edge(1, 3))
    {
      point p01 = planeIntersection(d, tet, 0, 1);
      point p12 = planeIntersection(d, tet, 1, 2);
      point p03 = planeIntersection(d, tet, 0, 3);
      point p23 = planeIntersection(d, tet, 2, 3);
      // Split the resulting prism
      {
        FixedList<point, 6> p;
        p[0] = tet[1];
        p[1] = p12;
        p[2] = p01;
        p[3] = tet[3];
        p[4] = p23;
        p[5] = p03;
        //Pout<< "    13 aboveprism:" << p << endl;
        decomposePrism(p, aboveOp);
      }
      {
        FixedList<point, 6> p;
        p[0] = tet[2];
        p[1] = p12;
        p[2] = p23;
        p[3] = tet[0];
        p[4] = p01;
        p[5] = p03;
        //Pout<< "    13 belowprism:" << p << endl;
        decomposePrism(p, belowOp);
      }
    }
    else if (posEdge == edge(2, 3))
    {
      point p02 = planeIntersection(d, tet, 0, 2);
      point p12 = planeIntersection(d, tet, 1, 2);
      point p03 = planeIntersection(d, tet, 0, 3);
      point p13 = planeIntersection(d, tet, 1, 3);
      // Split the resulting prism
      {
        FixedList<point, 6> p;
        p[0] = tet[2];
        p[1] = p02;
        p[2] = p12;
        p[3] = tet[3];
        p[4] = p03;
        p[5] = p13;
        //Pout<< "    23 aboveprism:" << p << endl;
        decomposePrism(p, aboveOp);
      }
      {
        FixedList<point, 6> p;
        p[0] = tet[0];
        p[1] = p02;
        p[2] = p03;
        p[3] = tet[1];
        p[4] = p12;
        p[5] = p13;
        //Pout<< "    23 belowprism:" << p << endl;
        decomposePrism(p, belowOp);
      }
    }
    else
    {
      FATAL_ERROR_IN("tetSliceWithPlane(..)")
        << "Missed edge:" << posEdge
        << abort(FatalError);
    }
  }
  else if (nPos == 1)
  {
    // Find the positive tet
    label i0 = -1;
    FOR_ALL(d, i)
    {
      if (d[i] > 0)
      {
        i0 = i;
        break;
      }
    }
    label i1 = d.fcIndex(i0);
    label i2 = d.fcIndex(i1);
    label i3 = d.fcIndex(i2);
    point p01 = planeIntersection(d, tet, i0, i1);
    point p02 = planeIntersection(d, tet, i0, i2);
    point p03 = planeIntersection(d, tet, i0, i3);
    //Pout<< "Split 1pos tet " << tet << " d:" << d << " into" << nl;
    if (i0 == 0 || i0 == 2)
    {
      tetPoints t(tet[i0], p01, p02, p03);
      //Pout<< "    abovetet:" << t << " around i0:" << i0 << endl;
      //checkTet(t, "nPos 1, aboveTets i0==0 or 2");
      aboveOp(t);
      // Prism
      FixedList<point, 6> p;
      p[0] = tet[i1];
      p[1] = tet[i3];
      p[2] = tet[i2];
      p[3] = p01;
      p[4] = p03;
      p[5] = p02;
      //Pout<< "    belowprism:" << p << endl;
      decomposePrism(p, belowOp);
    }
    else
    {
      tetPoints t(p01, p02, p03, tet[i0]);
      //Pout<< "    abovetet:" << t << " around i0:" << i0 << endl;
      //checkTet(t, "nPos 1, aboveTets i0==1 or 3");
      aboveOp(t);
      // Prism
      FixedList<point, 6> p;
      p[0] = tet[i3];
      p[1] = tet[i1];
      p[2] = tet[i2];
      p[3] = p03;
      p[4] = p01;
      p[5] = p02;
      //Pout<< "    belowprism:" << p << endl;
      decomposePrism(p, belowOp);
    }
  }
  else    // nPos == 0
  {
    belowOp(tet);
  }
}
template<class Point, class PointRef>
template<class AboveTetOp, class BelowTetOp>
inline void mousse::tetrahedron<Point, PointRef>::sliceWithPlane
(
  const plane& pl,
  AboveTetOp& aboveOp,
  BelowTetOp& belowOp
) const
{
  tetSliceWithPlane(pl, tetPoints(a_, b_, c_, d_), aboveOp, belowOp);
}
// Ostream Operator 
template<class Point, class PointRef>
inline mousse::Istream& mousse::operator>>
(
  Istream& is,
  tetrahedron<Point, PointRef>& t
)
{
  is.readBegin("tetrahedron");
  is  >> t.a_ >> t.b_ >> t.c_ >> t.d_;
  is.readEnd("tetrahedron");
  is.check("Istream& operator>>(Istream&, tetrahedron&)");
  return is;
}
template<class Point, class PointRef>
inline mousse::Ostream& mousse::operator<<
(
  Ostream& os,
  const tetrahedron<Point, PointRef>& t
)
{
  os  << nl
    << token::BEGIN_LIST
    << t.a_ << token::SPACE
    << t.b_ << token::SPACE
    << t.c_ << token::SPACE
    << t.d_
    << token::END_LIST;
  return os;
}
#ifdef NoRepository
#include "tetrahedron.cpp"
#endif
#endif
