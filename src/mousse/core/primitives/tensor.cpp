// mousse: CFD toolbox
// Copyright (C) 2011-2014 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "tensor.hpp"
#include "mathematical_constants.hpp"
#include "swap.hpp"


using namespace mousse::constant::mathematical;

// Static Data Members
namespace mousse {

template<>
const char* const tensor::typeName = "tensor";

template<>
const char* tensor::componentNames[] =
{
  "xx", "xy", "xz",
  "yx", "yy", "yz",
  "zx", "zy", "zz"
};

template<>
const tensor tensor::zero
{
  0, 0, 0,
  0, 0, 0,
  0, 0, 0
};

template<>
const tensor tensor::one
{
  1, 1, 1,
  1, 1, 1,
  1, 1, 1
};

template<>
const tensor tensor::max
{
  VGREAT, VGREAT, VGREAT,
  VGREAT, VGREAT, VGREAT,
  VGREAT, VGREAT, VGREAT
};

template<>
const tensor tensor::min
{
  -VGREAT, -VGREAT, -VGREAT,
  -VGREAT, -VGREAT, -VGREAT,
  -VGREAT, -VGREAT, -VGREAT
};

template<>
const tensor tensor::I
{
  1, 0, 0,
  0, 1, 0,
  0, 0, 1
};

}


mousse::vector mousse::eigenValues(const tensor& t)
{
  // The eigenvalues
  scalar i, ii, iii;
  // diagonal matrix
  if ((mag(t.xy()) + mag(t.xz()) + mag(t.yx())
       + mag(t.yz()) + mag(t.zx()) + mag(t.zy())) < SMALL) {
    i = t.xx();
    ii = t.yy();
    iii = t.zz();
  } else {
    // Coefficients of the characteristic polynmial
    // x^3 + a*x^2 + b*x + c = 0
    scalar a = - t.xx() - t.yy() - t.zz();
    scalar b = t.xx()*t.yy() + t.xx()*t.zz() + t.yy()*t.zz()
      - t.xy()*t.yx() - t.yz()*t.zy() - t.zx()*t.xz();
    scalar c = -t.xx()*t.yy()*t.zz()
      - t.xy()*t.yz()*t.zx() - t.xz()*t.zy()*t.yx()
      + t.xx()*t.yz()*t.zy() + t.yy()*t.zx()*t.xz() + t.zz()*t.xy()*t.yx();
    // Auxillary variables
    scalar aBy3 = a/3;
    scalar P = (a*a - 3*b)/9; // == -p_wikipedia/3
    scalar PPP = P*P*P;
    scalar Q = (2*a*a*a - 9*a*b + 27*c)/54; // == q_wikipedia/2
    scalar QQ = Q*Q;
    // Three identical roots
    if (mag(P) < SMALL && mag(Q) < SMALL) {
      return vector(- aBy3, - aBy3, - aBy3);
    } else if (mag(PPP/QQ - 1) < SMALL) {
      // Two identical roots and one distinct root
      scalar sqrtP = sqrt(P);
      scalar signQ = sign(Q);
      i = ii = signQ*sqrtP - aBy3;
      iii = - 2*signQ*sqrtP - aBy3;
    } else if (PPP > QQ) {
      // Three distinct roots
      scalar sqrtP = sqrt(P);
      scalar value = cos(acos(Q/sqrt(PPP))/3);
      scalar delta = sqrt(3 - 3*value*value);
      i = - 2*sqrtP*value - aBy3;
      ii = sqrtP*(value + delta) - aBy3;
      iii = sqrtP*(value - delta) - aBy3;
    } else {
      // One real root, two imaginary roots
      // based on the above logic, PPP must be less than QQ
      WARNING_IN("eigenValues(const tensor&)")
        << "complex eigenvalues detected for tensor: " << t
        << endl;
      if (mag(P) < SMALL) {
        i = cbrt(QQ/2);
      } else {
        scalar w = cbrt(- Q - sqrt(QQ - PPP));
        i = w + P/w - aBy3;
      }
      return {-VGREAT, i, VGREAT};
    }
  }
  // Sort the eigenvalues into ascending order
  if (i > ii) {
    Swap(i, ii);
  }
  if (ii > iii) {
    Swap(ii, iii);
  }
  if (i > ii) {
    Swap(i, ii);
  }
  return vector(i, ii, iii);
}


mousse::vector mousse::eigenVector
(
  const tensor& t,
  const scalar lambda
)
{
  // Constantly rotating direction ensures different eigenvectors are
  // generated when called sequentially with a multiple eigenvalue
  static vector direction(1,0,0);
  vector oldDirection(direction);
  scalar temp = direction[2];
  direction[2] = direction[1];
  direction[1] = direction[0];
  direction[0] = temp;
  // Construct the linear system for this eigenvalue
  tensor A(t - lambda*I);
  // Determinants of the 2x2 sub-matrices used to find the eigenvectors
  scalar sd0, sd1, sd2;
  scalar magSd0, magSd1, magSd2;
  // Sub-determinants for a unique eivenvalue
  sd0 = A.yy()*A.zz() - A.yz()*A.zy();
  sd1 = A.zz()*A.xx() - A.zx()*A.xz();
  sd2 = A.xx()*A.yy() - A.xy()*A.yx();
  magSd0 = mag(sd0);
  magSd1 = mag(sd1);
  magSd2 = mag(sd2);
  // Evaluate the eigenvector using the largest sub-determinant
  if (magSd0 >= magSd1 && magSd0 >= magSd2 && magSd0 > SMALL) {
    vector ev
    {
      1,
      (A.yz()*A.zx() - A.zz()*A.yx())/sd0,
      (A.zy()*A.yx() - A.yy()*A.zx())/sd0
    };
    return ev/mag(ev);
  } else if (magSd1 >= magSd2 && magSd1 > SMALL) {
    vector ev
    {
      (A.xz()*A.zy() - A.zz()*A.xy())/sd1,
      1,
      (A.zx()*A.xy() - A.xx()*A.zy())/sd1
    };
    return ev/mag(ev);
  } else if (magSd2 > SMALL) {
    vector ev
    {
      (A.xy()*A.yz() - A.yy()*A.xz())/sd2,
      (A.yx()*A.xz() - A.xx()*A.yz())/sd2,
      1
    };
    return ev/mag(ev);
  }
  // Sub-determinants for a repeated eigenvalue
  sd0 = A.yy()*direction.z() - A.yz()*direction.y();
  sd1 = A.zz()*direction.x() - A.zx()*direction.z();
  sd2 = A.xx()*direction.y() - A.xy()*direction.x();
  magSd0 = mag(sd0);
  magSd1 = mag(sd1);
  magSd2 = mag(sd2);
  // Evaluate the eigenvector using the largest sub-determinant
  if (magSd0 >= magSd1 && magSd0 >= magSd2 && magSd0 > SMALL) {
    vector ev
    {
      1,
      (A.yz()*direction.x() - direction.z()*A.yx())/sd0,
      (direction.y()*A.yx() - A.yy()*direction.x())/sd0
    };
    return ev/mag(ev);
  } else if (magSd1 >= magSd2 && magSd1 > SMALL) {
    vector ev
    {
      (direction.z()*A.zy() - A.zz()*direction.y())/sd1,
      1,
      (A.zx()*direction.y() - direction.x()*A.zy())/sd1
    };
    return ev/mag(ev);
  } else if (magSd2 > SMALL) {
    vector ev
    {
      (A.xy()*direction.z() - direction.y()*A.xz())/sd2,
      (direction.x()*A.xz() - A.xx()*direction.z())/sd2,
      1
    };
    return ev/mag(ev);
  }
  // Triple eigenvalue
  return oldDirection;
}


mousse::tensor mousse::eigenVectors(const tensor& t)
{
  vector evals{eigenValues(t)};
  tensor evs
  {
    eigenVector(t, evals.x()),
    eigenVector(t, evals.y()),
    eigenVector(t, evals.z())
  };
  return evs;
}


mousse::vector mousse::eigenValues(const symmTensor& t)
{
  return eigenValues(tensor(t));
}


mousse::vector mousse::eigenVector(const symmTensor& t, const scalar lambda)
{
  return eigenVector(tensor(t), lambda);
}


mousse::tensor mousse::eigenVectors(const symmTensor& t)
{
  return eigenVectors(tensor(t));
}
