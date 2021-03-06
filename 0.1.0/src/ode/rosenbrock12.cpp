// mousse: CFD toolbox
// Copyright (C) 2013-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "rosenbrock12.hpp"
#include "add_to_run_time_selection_table.hpp"


// Static Data Members
namespace mousse {

DEFINE_TYPE_NAME_AND_DEBUG(Rosenbrock12, 0);
ADD_TO_RUN_TIME_SELECTION_TABLE(ODESolver, Rosenbrock12, dictionary);

const scalar Rosenbrock12::gamma = 1 + 1.0/std::sqrt(2.0);
const scalar Rosenbrock12::a21 = 1.0/gamma;
const scalar Rosenbrock12::c2 = 1.0;
const scalar Rosenbrock12::c21 = -2.0/gamma;
const scalar Rosenbrock12::b1 = (3.0/2.0)/gamma;
const scalar Rosenbrock12::b2 = (1.0/2.0)/gamma;
const scalar Rosenbrock12::e1 = b1 - 1.0/gamma;
const scalar Rosenbrock12::e2 = b2;
const scalar Rosenbrock12::d1 = gamma;
const scalar Rosenbrock12::d2 = -gamma;

}


// Constructors 
mousse::Rosenbrock12::Rosenbrock12(const ODESystem& ode, const dictionary& dict)
:
  ODESolver{ode, dict},
  adaptiveSolver{ode, dict},
  k1_{n_},
  k2_{n_},
  err_{n_},
  dydx_{n_},
  dfdx_{n_},
  dfdy_{n_, n_},
  a_{n_, n_},
  pivotIndices_{n_}
{}


// Member Functions 
mousse::scalar mousse::Rosenbrock12::solve
(
  const scalar x0,
  const scalarField& y0,
  const scalarField& dydx0,
  const scalar dx,
  scalarField& y
) const
{
  odes_.jacobian(x0, y0, dfdx_, dfdy_);
  for (label i=0; i<n_; i++) {
    for (label j=0; j<n_; j++) {
      a_[i][j] = -dfdy_[i][j];
    }
    a_[i][i] += 1.0/(gamma*dx);
  }
  LUDecompose(a_, pivotIndices_);
  // Calculate k1:
  FOR_ALL(k1_, i) {
    k1_[i] = dydx0[i] + dx*d1*dfdx_[i];
  }
  LUBacksubstitute(a_, pivotIndices_, k1_);
  // Calculate k2:
  FOR_ALL(y, i) {
    y[i] = y0[i] + a21*k1_[i];
  }
  odes_.derivatives(x0 + c2*dx, y, dydx_);
  FOR_ALL(k2_, i) {
    k2_[i] = dydx_[i] + dx*d2*dfdx_[i] + c21*k1_[i]/dx;
  }
  LUBacksubstitute(a_, pivotIndices_, k2_);
  // Calculate error and update state:
  FOR_ALL(y, i) {
    y[i] = y0[i] + b1*k1_[i] + b2*k2_[i];
    err_[i] = e1*k1_[i] + e2*k2_[i];
  }
  return normalizeError(y0, y, err_);
}


void mousse::Rosenbrock12::solve
(
  scalar& x,
  scalarField& y,
  scalar& dxTry
) const
{
  adaptiveSolver::solve(odes_, x, y, dxTry);
}

