// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "shih_quadratic_ke.hpp"
#include "bound.hpp"
#include "wall_fv_patch.hpp"
#include "nutk_wall_function_fv_patch_scalar_field.hpp"
#include "add_to_run_time_selection_table.hpp"


namespace mousse {
namespace incompressible {
namespace RASModels {

// Static Data Members
DEFINE_TYPE_NAME_AND_DEBUG(ShihQuadraticKE, 0);
ADD_TO_RUN_TIME_SELECTION_TABLE(RASModel, ShihQuadraticKE, dictionary);


// Protected Member Functions 
void ShihQuadraticKE::correctNut()
{
  correctNonlinearStress(fvc::grad(U_));
}


void ShihQuadraticKE::correctNonlinearStress(const volTensorField& gradU)
{
  volSymmTensorField S{symm(gradU)};
  volTensorField W{skew(gradU)};
  volScalarField sBar{(k_/epsilon_)*sqrt(2.0)*mag(S)};
  volScalarField wBar{(k_/epsilon_)*sqrt(2.0)*mag(W)};
  volScalarField Cmu{(2.0/3.0)/(Cmu1_ + sBar + Cmu2_*wBar)};
  nut_ = Cmu*sqr(k_)/epsilon_;
  nut_.correctBoundaryConditions();
  nonlinearStress_ =
    k_*sqr(k_/epsilon_)/(Cbeta_ + pow3(sBar))
    *(Cbeta1_*dev(innerSqr(S)) + Cbeta2_*twoSymm(S&W) + Cbeta3_*dev(symm(W&W)));
}


// Constructors 
ShihQuadraticKE::ShihQuadraticKE
(
  const geometricOneField& alpha,
  const geometricOneField& rho,
  const volVectorField& U,
  const surfaceScalarField& alphaRhoPhi,
  const surfaceScalarField& phi,
  const transportModel& transport,
  const word& propertiesName,
  const word& type
)
:
  nonlinearEddyViscosity<incompressible::RASModel>
  {
    type,
    alpha,
    rho,
    U,
    alphaRhoPhi,
    phi,
    transport,
    propertiesName
  },
  Ceps1_
  {
    dimensioned<scalar>::lookupOrAddToDict
    (
      "Ceps1",
      coeffDict_,
      1.44
    )
  },
  Ceps2_
  {
    dimensioned<scalar>::lookupOrAddToDict
    (
      "Ceps2",
      coeffDict_,
      1.92
    )
  },
  sigmak_
  {
    dimensioned<scalar>::lookupOrAddToDict
    (
      "sigmak",
      coeffDict_,
      1.0
    )
  },
  sigmaEps_
  {
    dimensioned<scalar>::lookupOrAddToDict
    (
      "sigmaEps",
      coeffDict_,
      1.3
    )
  },
  Cmu1_
  {
    dimensioned<scalar>::lookupOrAddToDict
    (
      "Cmu1",
      coeffDict_,
      1.25
    )
  },
  Cmu2_
  {
    dimensioned<scalar>::lookupOrAddToDict
    (
      "Cmu2",
      coeffDict_,
      0.9
    )
  },
  Cbeta_
  {
    dimensioned<scalar>::lookupOrAddToDict
    (
      "Cbeta",
      coeffDict_,
      1000.0
    )
  },
  Cbeta1_
  {
    dimensioned<scalar>::lookupOrAddToDict
    (
      "Cbeta1",
      coeffDict_,
      3.0
    )
  },
  Cbeta2_
  {
    dimensioned<scalar>::lookupOrAddToDict
    (
      "Cbeta2",
      coeffDict_,
      15.0
    )
  },
  Cbeta3_
  {
    dimensioned<scalar>::lookupOrAddToDict
    (
      "Cbeta3",
      coeffDict_,
      -19.0
    )
  },
  k_
  {
    IOobject
    {
      IOobject::groupName("k", U.group()),
      runTime_.timeName(),
      mesh_,
      IOobject::MUST_READ,
      IOobject::AUTO_WRITE
    },
    mesh_
  },
  epsilon_
  {
    IOobject
    {
      IOobject::groupName("epsilon", U.group()),
      runTime_.timeName(),
      mesh_,
      IOobject::MUST_READ,
      IOobject::AUTO_WRITE
    },
    mesh_
  }
{
  bound(k_, kMin_);
  bound(epsilon_, epsilonMin_);
  if (type == typeName) {
    correctNut();
    printCoeffs(type);
  }
}


// Member Functions 
bool ShihQuadraticKE::read()
{
  if (nonlinearEddyViscosity<incompressible::RASModel>::read()) {
    Ceps1_.readIfPresent(coeffDict());
    Ceps2_.readIfPresent(coeffDict());
    sigmak_.readIfPresent(coeffDict());
    sigmaEps_.readIfPresent(coeffDict());
    Cmu1_.readIfPresent(coeffDict());
    Cmu2_.readIfPresent(coeffDict());
    Cbeta_.readIfPresent(coeffDict());
    Cbeta1_.readIfPresent(coeffDict());
    Cbeta2_.readIfPresent(coeffDict());
    Cbeta3_.readIfPresent(coeffDict());
    return true;
  }
  return false;
}


void ShihQuadraticKE::correct()
{
  if (!turbulence_) {
    return;
  }
  nonlinearEddyViscosity<incompressible::RASModel>::correct();
  tmp<volTensorField> tgradU = fvc::grad(U_);
  const volTensorField& gradU = tgradU();
  volScalarField G
  {
    GName(),
    (nut_*twoSymm(gradU) - nonlinearStress_) && gradU
  };
  // Update epsilon and G at the wall
  epsilon_.boundaryField().updateCoeffs();
  // Dissipation equation
  tmp<fvScalarMatrix> epsEqn {
    fvm::ddt(epsilon_)
  + fvm::div(phi_, epsilon_)
  - fvm::laplacian(DepsilonEff(), epsilon_)
  ==
    Ceps1_*G*epsilon_/k_
  - fvm::Sp(Ceps2_*epsilon_/k_, epsilon_)
  };
  epsEqn().relax();
  epsEqn().boundaryManipulate(epsilon_.boundaryField());
  solve(epsEqn);
  bound(epsilon_, epsilonMin_);
  // Turbulent kinetic energy equation
  tmp<fvScalarMatrix> kEqn {
    fvm::ddt(k_)
  + fvm::div(phi_, k_)
  - fvm::laplacian(DkEff(), k_)
  ==
    G
  - fvm::Sp(epsilon_/k_, k_)
  };
  kEqn().relax();
  solve(kEqn);
  bound(k_, kMin_);
  // Re-calculate viscosity and non-linear stress
  correctNonlinearStress(gradU);
}

}  // namespace RASModels
}  // namespace incompressible
}  // namespace mousse

