#ifndef FINITE_VOLUME_INTERPOLATION_SURFACE_INTERPOLATION_LIMITED_SCHEMES_MINMOD_HPP_
#define FINITE_VOLUME_INTERPOLATION_SURFACE_INTERPOLATION_LIMITED_SCHEMES_MINMOD_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::MinmodLimiter
// Description
//   Class with limiter function which returns the limiter for the
//   Minmod differencing scheme.
//   Used in conjunction with the template class LimitedScheme.

#include "vector.hpp"


namespace mousse {

template<class LimiterFunc>
class MinmodLimiter
:
  public LimiterFunc
{
public:
  MinmodLimiter(Istream&)
  {}

  scalar limiter
  (
    const scalar /*cdWeight*/,
    const scalar faceFlux,
    const typename LimiterFunc::phiType& phiP,
    const typename LimiterFunc::phiType& phiN,
    const typename LimiterFunc::gradPhiType& gradcP,
    const typename LimiterFunc::gradPhiType& gradcN,
    const vector& d
  ) const
  {
    scalar r = LimiterFunc::r
    (
      faceFlux, phiP, phiN, gradcP, gradcN, d
    );
    return max(min(r, 1), 0);
  }
};
}  // namespace mousse
#endif
