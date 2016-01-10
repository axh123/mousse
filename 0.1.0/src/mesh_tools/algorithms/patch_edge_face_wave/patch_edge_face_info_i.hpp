// mousse: CFD toolbox
// Copyright (C) 2011-2013 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "poly_mesh.hpp"
#include "transform.hpp"
// Private Member Functions 
// Update this with w2 if w2 nearer to pt.
template<class TrackingData>
inline bool mousse::patchEdgeFaceInfo::update
(
  const point& pt,
  const patchEdgeFaceInfo& w2,
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
// Update this with w2 (information on same edge)
template<class TrackingData>
inline bool mousse::patchEdgeFaceInfo::update
(
  const patchEdgeFaceInfo& w2,
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
inline mousse::patchEdgeFaceInfo::patchEdgeFaceInfo()
:
  origin_(point::max),
  distSqr_(sqr(GREAT))
{}
// Construct from origin, distance
inline mousse::patchEdgeFaceInfo::patchEdgeFaceInfo
(
  const point& origin,
  const scalar distSqr
)
:
  origin_(origin),
  distSqr_(distSqr)
{}
// Construct as copy
inline mousse::patchEdgeFaceInfo::patchEdgeFaceInfo(const patchEdgeFaceInfo& wpt)
:
  origin_(wpt.origin()),
  distSqr_(wpt.distSqr())
{}
// Member Functions 
inline const mousse::point& mousse::patchEdgeFaceInfo::origin() const
{
  return origin_;
}
inline mousse::scalar mousse::patchEdgeFaceInfo::distSqr() const
{
  return distSqr_;
}
template<class TrackingData>
inline bool mousse::patchEdgeFaceInfo::valid(TrackingData&) const
{
  return origin_ != point::max;
}
template<class TrackingData>
inline void mousse::patchEdgeFaceInfo::transform
(
  const polyMesh&,
  const primitivePatch&,
  const tensor& rotTensor,
  const scalar /*tol*/,
  TrackingData&
)
{
  origin_ = mousse::transform(rotTensor, origin_);
}
template<class TrackingData>
inline bool mousse::patchEdgeFaceInfo::updateEdge
(
  const polyMesh&,
  const primitivePatch& patch,
  const label edgeI,
  const label /*faceI*/,
  const patchEdgeFaceInfo& faceInfo,
  const scalar tol,
  TrackingData& td
)
{
  const edge& e = patch.edges()[edgeI];
  point eMid =
    0.5
   * (
      patch.points()[patch.meshPoints()[e[0]]]
     + patch.points()[patch.meshPoints()[e[1]]]
    );
  return update(eMid, faceInfo, tol, td);
}
template<class TrackingData>
inline bool mousse::patchEdgeFaceInfo::updateEdge
(
  const polyMesh&,
  const primitivePatch&,
  const patchEdgeFaceInfo& edgeInfo,
  const bool /*sameOrientation*/,
  const scalar tol,
  TrackingData& td
)
{
  return update(edgeInfo, tol, td);
}
template<class TrackingData>
inline bool mousse::patchEdgeFaceInfo::updateFace
(
  const polyMesh&,
  const primitivePatch& patch,
  const label faceI,
  const label /*edgeI*/,
  const patchEdgeFaceInfo& edgeInfo,
  const scalar tol,
  TrackingData& td
)
{
  return update(patch.faceCentres()[faceI], edgeInfo, tol, td);
}
template<class TrackingData>
inline bool mousse::patchEdgeFaceInfo::equal
(
  const patchEdgeFaceInfo& rhs,
  TrackingData&
) const
{
  return operator==(rhs);
}
// Member Operators 
inline bool mousse::patchEdgeFaceInfo::operator==
(
  const mousse::patchEdgeFaceInfo& rhs
) const
{
  return origin() == rhs.origin();
}
inline bool mousse::patchEdgeFaceInfo::operator!=
(
  const mousse::patchEdgeFaceInfo& rhs
) const
{
  return !(*this == rhs);
}
