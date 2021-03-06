// mousse: CFD toolbox
// Copyright (C) 2011-2012 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "swap.hpp"
#include "tensor_2d.hpp"


namespace mousse {

// Static Data Members
template<>
const char* const tensor2D::typeName = "tensor2D";

template<>
const char* tensor2D::componentNames[] =
{
  "xx", "xy",
  "yx", "yy"
};

template<>
const tensor2D tensor2D::zero
{
  0, 0,
  0, 0
};

template<>
const tensor2D tensor2D::one
{
  1, 1,
  1, 1
};

template<>
const tensor2D tensor2D::max
{
  VGREAT, VGREAT,
  VGREAT, VGREAT
};

template<>
const tensor2D tensor2D::min
{
  -VGREAT, -VGREAT,
  -VGREAT, -VGREAT
};

template<>
const tensor2D tensor2D::I
{
  1, 0,
  0, 1
};

// Return eigenvalues in ascending order of absolute values
vector2D eigenValues(const tensor2D& t)
{
  scalar i = 0;
  scalar ii = 0;
  if (mag(t.xy()) < SMALL && mag(t.yx()) < SMALL) {
    i = t.xx();
    ii = t.yy();
  } else {
    scalar mb = t.xx() + t.yy();
    scalar c = t.xx()*t.yy() - t.xy()*t.yx();
    // If there is a zero root
    if (mag(c) < SMALL) {
      i = 0;
      ii = mb;
    } else {
      scalar disc = sqr(mb) - 4*c;
      if (disc > 0) {
        scalar q = sqrt(disc);
        i = 0.5*(mb - q);
        ii = 0.5*(mb + q);
      } else {
        FATAL_ERROR_IN("eigenValues(const tensor2D&)")
          << "zero and complex eigenvalues in tensor2D: " << t
          << abort(FatalError);
      }
    }
  }
  // Sort the eigenvalues into ascending order
  if (i > ii) {
    Swap(i, ii);
  }
  return {i, ii};
}


vector2D eigenVector(const tensor2D& t, const scalar lambda)
{
  if (lambda < SMALL) {
    return vector2D::zero;
  }
  if (mag(t.xy()) < SMALL && mag(t.yx()) < SMALL) {
    if (lambda > min(t.xx(), t.yy())) {
      return {1, 0};
    } else {
      return {0, 1};
    }
  } else if (mag(t.xy()) < SMALL) {
    return {lambda - t.yy(), t.yx()};
  } else {
    return {t.xy(), lambda - t.yy()};
  }
}


tensor2D eigenVectors(const tensor2D& t)
{
  vector2D evals{eigenValues(t)};
  tensor2D evs
  {
    eigenVector(t, evals.x()),
    eigenVector(t, evals.y())
  };
  return evs;
}

}  // namespace mousse
