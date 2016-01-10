// mousse: CFD toolbox
// Copyright (C) 2011-2013 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "poly_mesh.hpp"
#include "transform.hpp"
// Private Member Functions 
// Update this with w2 if w2 nearer to pt.
template<class TrackingData>
inline bool mousse::pointEdgePoint::update
(
  const point& pt,
  const pointEdgePoint& w2,
  const scalar tol,
  TrackingData& td
)
{
  scalar dist2 = magSqr(pt - w2.origin());
  if (!valid(td))
  {
    // current not yet set so use any value
    distSqr_ = dist2;
    origin_ = w2.origin();
    return true;
  }
  scalar diff = distSqr_ - dist2;
  if (diff < 0)
  {
    // already nearer to pt
    return false;
  }
  if ((diff < SMALL) || ((distSqr_ > SMALL) && (diff/distSqr_ < tol)))
  {
    // don't propagate small changes
    return false;
  }
  else
  {
    // update with new values
    distSqr_ = dist2;
    origin_ = w2.origin();
    return true;
  }
}
// Update this with w2 (information on same point)
template<class TrackingData>
inline bool mousse::pointEdgePoint::update
(
  const pointEdgePoint& w2,
  const scalar tol,
  TrackingData& td
)
{
  if (!valid(td))
  {
    // current not yet set so use any value
    distSqr_ = w2.distSqr();
    origin_ = w2.origin();
    return true;
  }
  scalar diff = distSqr_ - w2.distSqr();
  if (diff < 0)
  {
    // already nearer to pt
    return false;
  }
  if ((diff < SMALL) || ((distSqr_ > SMALL) && (diff/distSqr_ < tol)))
  {
    // don't propagate small changes
    return false;
  }
  else
  {
    // update with new values
    distSqr_ =  w2.distSqr();
    origin_ = w2.origin();
    return true;
  }
}
// Constructors 
// Null constructor
inline mousse::pointEdgePoint::pointEdgePoint()
:
  origin_(point::max),
  distSqr_(GREAT)
{}
// Construct from origin, distance
inline mousse::pointEdgePoint::pointEdgePoint
(
  const point& origin,
  const scalar distSqr
)
:
  origin_(origin),
  distSqr_(distSqr)
{}
// Construct as copy
inline mousse::pointEdgePoint::pointEdgePoint(const pointEdgePoint& wpt)
:
  origin_(wpt.origin()),
  distSqr_(wpt.distSqr())
{}
// Member Functions 
inline const mousse::point& mousse::pointEdgePoint::origin() const
{
  return origin_;
}
inline mousse::scalar mousse::pointEdgePoint::distSqr() const
{
  return distSqr_;
}
template<class TrackingData>
inline bool mousse::pointEdgePoint::valid(TrackingData&) const
{
  return origin_ != point::max;
}
// Checks for cyclic points
template<class TrackingData>
inline bool mousse::pointEdgePoint::sameGeometry
(
  const pointEdgePoint& w2,
  const scalar tol,
  TrackingData&
) const
{
  scalar diff = mousse::mag(distSqr() - w2.distSqr());
  if (diff < SMALL)
  {
    return true;
  }
  else
  {
    if ((distSqr() > SMALL) && ((diff/distSqr()) < tol))
    {
      return true;
    }
    else
    {
      return false;
    }
  }
}
template<class TrackingData>
inline void mousse::pointEdgePoint::leaveDomain
(
  const polyPatch&,
  const label /*patchPointI*/,
  const point& coord,
  TrackingData&
)
{
  origin_ -= coord;
}
template<class TrackingData>
inline void mousse::pointEdgePoint::transform
(
  const tensor& rotTensor,
  TrackingData&
)
{
  origin_ = mousse::transform(rotTensor, origin_);
}
// Update absolute geometric quantities. Note that distance (distSqr_)
// is not affected by leaving/entering domain.
template<class TrackingData>
inline void mousse::pointEdgePoint::enterDomain
(
  const polyPatch&,
  const label /*patchPointI*/,
  const point& coord,
  TrackingData&
)
{
  // back to absolute form
  origin_ += coord;
}
// Update this with information from connected edge
template<class TrackingData>
inline bool mousse::pointEdgePoint::updatePoint
(
  const polyMesh& mesh,
  const label pointI,
  const label /*edgeI*/,
  const pointEdgePoint& edgeInfo,
  const scalar tol,
  TrackingData& td
)
{
  return update(mesh.points()[pointI], edgeInfo, tol, td);
}
// Update this with new information on same point
template<class TrackingData>
inline bool mousse::pointEdgePoint::updatePoint
(
  const polyMesh& mesh,
  const label pointI,
  const pointEdgePoint& newPointInfo,
  const scalar tol,
  TrackingData& td
)
{
  return update(mesh.points()[pointI], newPointInfo, tol, td);
}
// Update this with new information on same point. No extra information.
template<class TrackingData>
inline bool mousse::pointEdgePoint::updatePoint
(
  const pointEdgePoint& newPointInfo,
  const scalar tol,
  TrackingData& td
)
{
  return update(newPointInfo, tol, td);
}
// Update this with information from connected point
template<class TrackingData>
inline bool mousse::pointEdgePoint::updateEdge
(
  const polyMesh& mesh,
  const label edgeI,
  const label /*pointI*/,
  const pointEdgePoint& pointInfo,
  const scalar tol,
  TrackingData& td
)
{
  const edge& e = mesh.edges()[edgeI];
  return update(e.centre(mesh.points()), pointInfo, tol, td);
}
template<class TrackingData>
inline bool mousse::pointEdgePoint::equal
(
  const pointEdgePoint& rhs,
  TrackingData&
) const
{
  return operator==(rhs);
}
// Member Operators 
inline bool mousse::pointEdgePoint::operator==(const mousse::pointEdgePoint& rhs)
const
{
  return (origin() == rhs.origin()) && (distSqr() == rhs.distSqr());
}
inline bool mousse::pointEdgePoint::operator!=(const mousse::pointEdgePoint& rhs)
const
{
  return !(*this == rhs);
}
