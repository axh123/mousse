// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "line.hpp"
namespace mousse
{
// Member Functions 
template<>
scalar line<point2D, const point2D&>::nearestDist
(
  const line<point2D, const point2D&>& e,
  point2D& thisPt,
  point2D& edgePt
) const
{
  vector2D u = end()-start();
  vector2D v = e.end()-e.start();
  vector2D w = start()-e.start();
  scalar d = u.perp(v);
  if (mousse::mag(d) > VSMALL)
  {
    scalar s = v.perp(w) / d;
    if (s <= SMALL)
    {
      thisPt = start();
    }
    else if (s >= (1-SMALL))
    {
      thisPt = end();
    }
    else
    {
      thisPt = start()+s*u;
    }
    scalar t = u.perp(w) / d;
    if (t <= SMALL)
    {
      edgePt = e.start();
    }
    else if (t >= (1-SMALL))
    {
      edgePt = e.end();
    }
    else
    {
      edgePt = e.start()+t*v;
    }
  }
  else
  {
    // Parallel lines. Find overlap of both lines by projecting onto
    // direction vector (now equal for both lines).
    scalar edge0 = e.start() & u;
    scalar edge1 = e.end() & u;
    bool edgeOrder = edge0 < edge1;
    scalar minEdge = (edgeOrder ? edge0 : edge1);
    scalar maxEdge = (edgeOrder ? edge1 : edge0);
    const point2D& minEdgePt = (edgeOrder ? e.start() : e.end());
    const point2D& maxEdgePt = (edgeOrder ? e.end() : e.start());
    scalar this0 = start() & u;
    scalar this1 = end() & u;
    bool thisOrder = this0 < this1;
    scalar minThis = min(this0, this1);
    scalar maxThis = max(this1, this0);
    const point2D& minThisPt = (thisOrder ? start() : end());
    const point2D& maxThisPt = (thisOrder ? end() : start());
    if (maxEdge < minThis)
    {
      // edge completely below *this
      edgePt = maxEdgePt;
      thisPt = minThisPt;
    }
    else if (maxEdge < maxThis)
    {
      // maxEdge inside interval of *this
      edgePt = maxEdgePt;
      thisPt = nearestDist(edgePt).rawPoint();
    }
    else
    {
      // maxEdge outside. Check if minEdge inside.
      if (minEdge < minThis)
      {
        // Edge completely envelops this. Take any this point and
        // determine nearest on edge.
        thisPt = minThisPt;
        edgePt = e.nearestDist(thisPt).rawPoint();
      }
      else if (minEdge < maxThis)
      {
        // minEdge inside this interval.
        edgePt = minEdgePt;
        thisPt = nearestDist(edgePt).rawPoint();
      }
      else
      {
        // minEdge outside this interval
        edgePt = minEdgePt;
        thisPt = maxThisPt;
      }
    }
  }
  return mousse::mag(thisPt - edgePt);
}
}  // namespace mousse
