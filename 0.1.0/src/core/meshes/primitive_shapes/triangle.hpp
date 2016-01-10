// mousse: CFD toolbox
// Copyright (C) 2011-2013 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::triangle
// Description
//   A triangle primitive used to calculate face normals and swept volumes.
// SourceFiles
//   triangle_i.hpp
#ifndef triangle_hpp_
#define triangle_hpp_
#include "intersection.hpp"
#include "vector.hpp"
#include "tensor.hpp"
#include "point_hit.hpp"
#include "random.hpp"
#include "cached_random.hpp"
#include "fixed_list.hpp"
#include "ulist.hpp"
#include "line_point_ref.hpp"
namespace mousse
{
class Istream;
class Ostream;
// Forward declaration of friend functions and operators
template<class Point, class PointRef> class triangle;
template<class Point, class PointRef>
inline Istream& operator>>
(
  Istream&,
  triangle<Point, PointRef>&
);
template<class Point, class PointRef>
inline Ostream& operator<<
(
  Ostream&,
  const triangle<Point, PointRef>&
);
template<class Point, class PointRef>
class triangle
{
  // Private data
    PointRef a_, b_, c_;
public:
  //- Return types for classify
  enum proxType
  {
    NONE,
    POINT,  // Close to point
    EDGE    // Close to edge
  };
  // Constructors
    //- Construct from three points
    inline triangle(const Point& a, const Point& b, const Point& c);
    //- Construct from three points in the list of points
    //  The indices could be from triFace etc.
    inline triangle
    (
      const UList<Point>&,
      const FixedList<label, 3>& indices
    );
    //- Construct from Istream
    inline triangle(Istream&);
  // Member Functions
    // Access
      //- Return first vertex
      inline const Point& a() const;
      //- Return second vertex
      inline const Point& b() const;
      //- Return third vertex
      inline const Point& c() const;
    // Properties
      //- Return centre (centroid)
      inline Point centre() const;
      //- Return scalar magnitude
      inline scalar mag() const;
      //- Return vector normal
      inline vector normal() const;
      //- Return circum-centre
      inline Point circumCentre() const;
      //- Return circum-radius
      inline scalar circumRadius() const;
      //- Return quality: Ratio of triangle and circum-circle
      //  area, scaled so that an equilateral triangle has a
      //  quality of 1
      inline scalar quality() const;
      //- Return swept-volume
      inline scalar sweptVol(const triangle& t) const;
      //- Return the inertia tensor, with optional reference
      //  point and density specification
      inline tensor inertia
      (
        PointRef refPt = vector::zero,
        scalar density = 1.0
      ) const;
      //- Return a random point on the triangle from a uniform
      //  distribution
      inline Point randomPoint(Random& rndGen) const;
      //- Return a random point on the triangle from a uniform
      //  distribution
      inline Point randomPoint(cachedRandom& rndGen) const;
      //- Calculate the barycentric coordinates of the given
      //  point, in the same order as a, b, c.  Returns the
      //  determinant of the solution.
      inline scalar barycentric
      (
        const point& pt,
        List<scalar>& bary
      ) const;
      //- Return point intersection with a ray.
      //  For a hit, the distance is signed. Positive number
      //  represents the point in front of triangle.
      //  In case of miss pointHit is set to nearest point
      //  on triangle and its distance to the distance between
      //  the original point and the plane intersection point
      inline pointHit ray
      (
        const point& p,
        const vector& q,
        const intersection::algorithm = intersection::FULL_RAY,
        const intersection::direction dir = intersection::VECTOR
      ) const;
      //- Fast intersection with a ray.
      //  For a hit, the pointHit.distance() is the line parameter t :
      //  intersection=p+t*q. Only defined for VISIBLE, FULL_RAY or
      //  HALF_RAY. tol increases the virtual size of the triangle
      // by a relative factor.
      inline pointHit intersection
      (
        const point& p,
        const vector& q,
        const intersection::algorithm alg,
        const scalar tol = 0.0
      ) const;
      //- Find the nearest point to p on the triangle and classify it:
      //  + near point (nearType=POINT, nearLabel=0, 1, 2)
      //  + near edge (nearType=EDGE, nearLabel=0, 1, 2)
      //    Note: edges are counted from starting
      //    vertex so e.g. edge 2 is from f[2] to f[0]
      pointHit nearestPointClassify
      (
        const point& p,
        label& nearType,
        label& nearLabel
      ) const;
      //- Return nearest point to p on triangle
      inline pointHit nearestPoint(const point& p) const;
      //- Classify nearest point to p in triangle plane
      //  w.r.t. triangle edges and points.  Returns inside
      //  (true)/outside (false).
      bool classify
      (
        const point& p,
        label& nearType,
        label& nearLabel
      ) const;
      //- Return nearest point to line on triangle. Returns hit if
      //  point is inside triangle. Sets edgePoint to point on edge
      //  (hit if nearest is inside line)
      inline pointHit nearestPoint
      (
        const linePointRef& edge,
        pointHit& edgePoint
      ) const;
  // IOstream operators
    friend Istream& operator>> <Point, PointRef>
    (
      Istream&,
      triangle&
    );
    friend Ostream& operator<< <Point, PointRef>
    (
      Ostream&,
      const triangle&
    );
};
}  // namespace mousse
#include "triangle_i.hpp"
#endif
