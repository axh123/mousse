#ifndef FINITE_VOLUME_FINITE_VOLUME_FVC_FVC_SMOOTH_SWEEP_DATA_HPP_
#define FINITE_VOLUME_FINITE_VOLUME_FVC_FVC_SMOOTH_SWEEP_DATA_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::sweepData
// Description
//   Helper class used by fvc::sweep function.
// SourceFiles
//   sweep_data.hpp
#include "transform.hpp"
namespace mousse
{
class sweepData
{
  scalar value_;
  point origin_;
  // Private Member Functions
    //- Update - gets information from neighbouring face/cell and
    //  uses this to update itself (if necessary) and return true
    template<class TrackingData>
    inline bool update
    (
      const sweepData& svf,
      const point& position,
      const scalar tol,
      TrackingData& td
    );
public:
  // Constructors
    //- Construct null
    inline sweepData();
    //- Construct from component
    inline sweepData(const scalar value, const point& origin);
  // Member Functions
    // Access
      //- Return value
      scalar value() const
      {
        return value_;
      }
      //- Return origin
      const point& origin() const
      {
        return origin_;
      }
    // Needed by FaceCellWave
      //- Check whether origin has been changed at all or
      //  still contains original (invalid) value
      template<class TrackingData>
      inline bool valid(TrackingData& td) const;
      //- Check for identical geometrical data
      //  Used for cyclics checking
      template<class TrackingData>
      inline bool sameGeometry
      (
        const polyMesh&,
        const sweepData&,
        const scalar,
        TrackingData& td
      ) const;
      //- Convert any absolute coordinates into relative to
      //  (patch)face centre
      template<class TrackingData>
      inline void leaveDomain
      (
        const polyMesh&,
        const polyPatch&,
        const label patchFaceI,
        const point& faceCentre,
        TrackingData& td
      );
      //- Reverse of leaveDomain
      template<class TrackingData>
      inline void enterDomain
      (
        const polyMesh&,
        const polyPatch&,
        const label patchFaceI,
        const point& faceCentre,
        TrackingData& td
      );
      //- Apply rotation matrix to any coordinates
      template<class TrackingData>
      inline void transform
      (
        const polyMesh&,
        const tensor&,
        TrackingData& td
      );
      //- Influence of neighbouring face
      template<class TrackingData>
      inline bool updateCell
      (
        const polyMesh&,
        const label thisCellI,
        const label neighbourFaceI,
        const sweepData& svf,
        const scalar tol,
        TrackingData& td
      );
      //- Influence of neighbouring cell
      template<class TrackingData>
      inline bool updateFace
      (
        const polyMesh&,
        const label thisFaceI,
        const label neighbourCellI,
        const sweepData& svf,
        const scalar tol,
        TrackingData& td
      );
      //- Influence of different value on same face
      template<class TrackingData>
      inline bool updateFace
      (
        const polyMesh&,
        const label thisFaceI,
        const sweepData& svf,
        const scalar tol,
        TrackingData& td
      );
      //- Same (like operator==)
      template<class TrackingData>
      inline bool equal(const sweepData&, TrackingData& td) const;
    // Member Operators
      inline void operator=(const scalar value);
      // Needed for List IO
      inline bool operator==(const sweepData&) const;
      inline bool operator!=(const sweepData&) const;
    // IOstream Operators
      friend Ostream& operator<<
      (
        Ostream& os,
        const sweepData& svf
      )
      {
        return os << svf.value_ << svf.origin_;
      }
      friend Istream& operator>>(Istream& is, sweepData& svf)
      {
        return is >> svf.value_ >> svf.origin_;
      }
};
//- Data associated with sweepData type are contiguous
template<>
inline bool contiguous<sweepData>()
{
  return true;
}
}  // namespace mousse

// Private Member Functions 
template<class TrackingData>
inline bool mousse::sweepData::update
(
  const sweepData& svf,
  const point& position,
  const scalar /*tol*/,
  TrackingData& td
)
{
  if (!valid(td))
  {
    operator=(svf);
    return true;
  }
  scalar myDist2 = magSqr(position - origin());
  if (myDist2 < SMALL)
  {
    if (svf.value() > value())
    {
      operator=(svf);
      return true;
    }
    else
    {
      return false;
    }
  }
  scalar dist2 = magSqr(position - svf.origin());
  if (dist2 < myDist2)
  {
    operator=(svf);
    return true;
  }
  return false;
}
// Constructors 
inline mousse::sweepData::sweepData()
:
  value_{-GREAT},
  origin_{vector::max}
{}
inline mousse::sweepData::sweepData(const scalar value, const point& origin)
:
  value_{value},
  origin_{origin}
{}
// Member Functions 
template<class TrackingData>
inline bool mousse::sweepData::valid(TrackingData&) const
{
  return value_ > -SMALL;
}
template<class TrackingData>
inline bool mousse::sweepData::sameGeometry
(
  const polyMesh&,
  const sweepData&,
  const scalar,
  TrackingData&
) const
{
  return true;
}
template<class TrackingData>
inline void mousse::sweepData::leaveDomain
(
  const polyMesh&,
  const polyPatch&,
  const label,
  const point& faceCentre,
  TrackingData&
)
{
  origin_ -= faceCentre;
}
template<class TrackingData>
inline void mousse::sweepData::transform
(
  const polyMesh&,
  const tensor& rotTensor,
  TrackingData&
)
{
  origin_ = mousse::transform(rotTensor, origin_);
}
template<class TrackingData>
inline void mousse::sweepData::enterDomain
(
  const polyMesh&,
  const polyPatch&,
  const label,
  const point& faceCentre,
  TrackingData&
)
{
  // back to absolute form
  origin_ += faceCentre;
}
template<class TrackingData>
inline bool mousse::sweepData::updateCell
(
  const polyMesh& mesh,
  const label thisCellI,
  const label,
  const sweepData& svf,
  const scalar tol,
  TrackingData& td
)
{
  return update(svf, mesh.cellCentres()[thisCellI], tol, td);
}
template<class TrackingData>
inline bool mousse::sweepData::updateFace
(
  const polyMesh& mesh,
  const label thisFaceI,
  const label,
  const sweepData& svf,
  const scalar tol,
  TrackingData& td
)
{
  return update(svf, mesh.faceCentres()[thisFaceI], tol, td);
}
// Update this (face) with coupled face information.
template<class TrackingData>
inline bool mousse::sweepData::updateFace
(
  const polyMesh& mesh,
  const label thisFaceI,
  const sweepData& svf,
  const scalar tol,
  TrackingData& td
)
{
  return update(svf, mesh.faceCentres()[thisFaceI], tol, td);
}
template<class TrackingData>
inline bool mousse::sweepData::equal
(
  const sweepData& rhs,
  TrackingData&
) const
{
  return operator==(rhs);
}
// Member Operators 
inline void mousse::sweepData::operator=
(
  const scalar value
)
{
  value_ = value;
}
inline bool mousse::sweepData::operator==
(
  const sweepData& rhs
) const
{
  return origin() == rhs.origin();
}
inline bool mousse::sweepData::operator!=
(
  const sweepData& rhs
) const
{
  return !(*this == rhs);
}
#endif
