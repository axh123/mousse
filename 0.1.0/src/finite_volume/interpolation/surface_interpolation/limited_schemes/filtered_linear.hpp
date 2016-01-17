// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::filteredLinearLimiter
// Description
//   Class to generate weighting factors for the filteredLinear
//   differencing scheme.
//   The aim is to remove high-frequency modes with "staggering"
//   characteristics by comparing the face gradient relative to the
//   background distribution represented by the neighbouring cell gradients
//   with those gradients and introduce small amounts of upwind in order to
//   damp these modes.
//   Used in conjunction with the template class LimitedScheme.
// SourceFiles
//   filtered_linear.cpp
#ifndef filtered_linear_hpp_
#define filtered_linear_hpp_
#include "vector.hpp"
namespace mousse
{
template<class LimiterFunc>
class filteredLinearLimiter
:
  public LimiterFunc
{
public:
  filteredLinearLimiter(Istream&)
  {}
  scalar limiter
  (
    const scalar /*cdWeight*/,
    const scalar /*faceFlux*/,
    const typename LimiterFunc::phiType& phiP,
    const typename LimiterFunc::phiType& phiN,
    const typename LimiterFunc::gradPhiType& gradcP,
    const typename LimiterFunc::gradPhiType& gradcN,
    const vector& d
  ) const
  {
    scalar df = phiN - phiP;
    scalar dcP = d & gradcP;
    scalar dcN = d & gradcN;
    scalar limiter =
      2
     - 0.5*min(mag(df - dcP), mag(df - dcN))
     /(max(mag(dcP), mag(dcN)) + SMALL);
    // Limit the limiter between linear and 20% upwind
    return max(min(limiter, 1), 0.8);
  }
};
}  // namespace mousse
#endif
