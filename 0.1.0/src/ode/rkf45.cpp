// mousse: CFD toolbox
// Copyright (C) 2013 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "rkf45.hpp"
#include "add_to_run_time_selection_table.hpp"


// Static Data Members
namespace mousse {

DEFINE_TYPE_NAME_AND_DEBUG(RKF45, 0);
ADD_TO_RUN_TIME_SELECTION_TABLE(ODESolver, RKF45, dictionary);

const scalar RKF45::c2  = 1.0/4.0;
const scalar RKF45::c3  = 3.0/8.0;
const scalar RKF45::c4  = 12.0/13.0;
const scalar RKF45::c5  = 1.0;
const scalar RKF45::c6  = 1.0/2.0;
const scalar RKF45::a21 = 1.0/4.0;
const scalar RKF45::a31 = 3.0/32.0;
const scalar RKF45::a32 = 9.0/32.0;
const scalar RKF45::a41 = 1932.0/2197.0;
const scalar RKF45::a42 = -7200.0/2197.0;
const scalar RKF45::a43 = 7296.0/2197.0;
const scalar RKF45::a51 = 439.0/216.0;
const scalar RKF45::a52 = -8.0;
const scalar RKF45::a53 = 3680.0/513.0;
const scalar RKF45::a54 = -845.0/4104.0;
const scalar RKF45::a61 = -8.0/27.0;
const scalar RKF45::a62 = 2.0;
const scalar RKF45::a63 = -3544.0/2565.0;
const scalar RKF45::a64 = 1859.0/4104.0;
const scalar RKF45::a65 = -11.0/40.0;
const scalar RKF45::b1  = 16.0/135.0;
const scalar RKF45::b3  = 6656.0/12825.0;
const scalar RKF45::b4  = 28561.0/56430.0;
const scalar RKF45::b5  = -9.0/50.0;
const scalar RKF45::b6  = 2.0/55.0;
const scalar RKF45::e1  = 25.0/216.0 - RKF45::b1;
const scalar RKF45::e3  = 1408.0/2565.0 - RKF45::b3;
const scalar RKF45::e4  = 2197.0/4104.0 - RKF45::b4;
const scalar RKF45::e5  = -1.0/5.0 - RKF45::b5;
const scalar RKF45::e6  = -RKF45::b6;

}


// Constructors 
mousse::RKF45::RKF45(const ODESystem& ode, const dictionary& dict)
:
  ODESolver{ode, dict},
  adaptiveSolver{ode, dict},
  yTemp_{n_},
  k2_{n_},
  k3_{n_},
  k4_{n_},
  k5_{n_},
  k6_{n_},
  err_{n_}
{}


// Member Functions 
mousse::scalar mousse::RKF45::solve
(
  const scalar x0,
  const scalarField& y0,
  const scalarField& dydx0,
  const scalar dx,
  scalarField& y
) const
{
  FOR_ALL(yTemp_, i) {
    yTemp_[i] = y0[i] + a21*dx*dydx0[i];
  }
  odes_.derivatives(x0 + c2*dx, yTemp_, k2_);
  FOR_ALL(yTemp_, i) {
    yTemp_[i] = y0[i] + dx*(a31*dydx0[i] + a32*k2_[i]);
  }
  odes_.derivatives(x0 + c3*dx, yTemp_, k3_);
  FOR_ALL(yTemp_, i) {
    yTemp_[i] = y0[i] + dx*(a41*dydx0[i] + a42*k2_[i] + a43*k3_[i]);
  }
  odes_.derivatives(x0 + c4*dx, yTemp_, k4_);
  FOR_ALL(yTemp_, i) {
    yTemp_[i] = y0[i]
      + dx*(a51*dydx0[i] + a52*k2_[i] + a53*k3_[i] + a54*k4_[i]);
  }
  odes_.derivatives(x0 + c5*dx, yTemp_, k5_);
  FOR_ALL(yTemp_, i) {
    yTemp_[i] = y0[i]
      + dx*(a61*dydx0[i] + a62*k2_[i] + a63*k3_[i] + a64*k4_[i] + a65*k5_[i]);
  }
  odes_.derivatives(x0 + c6*dx, yTemp_, k6_);
  // Calculate the 5th-order solution
  FOR_ALL(y, i) {
    y[i] = y0[i]
      + dx*(b1*dydx0[i] + b3*k3_[i] + b4*k4_[i] + b5*k5_[i] + b6*k6_[i]);
  }
  // Calculate the error estimate from the difference between the
  // 4th-order and 5th-order solutions
  FOR_ALL(err_, i) {
    err_[i] =
      dx*(e1*dydx0[i] + e3*k3_[i] + e4*k4_[i] + e5*k5_[i] + e6*k6_[i]);
  }
  return normalizeError(y0, y, err_);
}


void mousse::RKF45::solve
(
  scalar& x,
  scalarField& y,
  scalar& dxTry
) const
{
  adaptiveSolver::solve(odes_, x, y, dxTry);
}

