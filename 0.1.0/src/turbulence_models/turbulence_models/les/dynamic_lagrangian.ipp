// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "dynamic_lagrangian.hpp"


namespace mousse {
namespace LESModels{

// Private Member Functions 
template<class BasicTurbulenceModel>
void dynamicLagrangian<BasicTurbulenceModel>::correctNut
(
  const tmp<volTensorField>& gradU
)
{
  this->nut_ = (flm_/fmm_)*sqr(this->delta())*mag(dev(symm(gradU)));
  this->nut_.correctBoundaryConditions();
}


template<class BasicTurbulenceModel>
void dynamicLagrangian<BasicTurbulenceModel>::correctNut()
{
  correctNut(fvc::grad(this->U_));
}


// Constructors 
template<class BasicTurbulenceModel>
dynamicLagrangian<BasicTurbulenceModel>::dynamicLagrangian
(
  const alphaField& alpha,
  const rhoField& rho,
  const volVectorField& U,
  const surfaceScalarField& alphaRhoPhi,
  const surfaceScalarField& phi,
  const transportModel& transport,
  const word& propertiesName,
  const word& type
)
:
  LESeddyViscosity<BasicTurbulenceModel>
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
  flm_
  {
    {
      IOobject::groupName("flm", this->U_.group()),
      this->runTime_.timeName(),
      this->mesh_,
      IOobject::MUST_READ,
      IOobject::AUTO_WRITE
    },
    this->mesh_
  },
  fmm_
  {
    {
      IOobject::groupName("fmm", this->U_.group()),
      this->runTime_.timeName(),
      this->mesh_,
      IOobject::MUST_READ,
      IOobject::AUTO_WRITE
    },
    this->mesh_
  },
  theta_
  {
    dimensioned<scalar>::lookupOrAddToDict
    (
      "theta",
      this->coeffDict_,
      1.5
    )
  },
  simpleFilter_{U.mesh()},
  filterPtr_{LESfilter::New(U.mesh(), this->coeffDict())},
  filter_{filterPtr_()},
  flm0_{"flm0", flm_.dimensions(), 0.0},
  fmm0_{"fmm0", fmm_.dimensions(), VSMALL}
{
  if (type == typeName) {
    correctNut();
    this->printCoeffs(type);
  }
}


// Member Functions 
template<class BasicTurbulenceModel>
bool dynamicLagrangian<BasicTurbulenceModel>::read()
{
  if (LESeddyViscosity<BasicTurbulenceModel>::read()) {
    filter_.read(this->coeffDict());
    theta_.readIfPresent(this->coeffDict());
    return true;
  }
  return false;
}


template<class BasicTurbulenceModel>
void dynamicLagrangian<BasicTurbulenceModel>::correct()
{
  if (!this->turbulence_) {
    return;
  }
  // Local references
  const surfaceScalarField& phi = this->phi_;
  const volVectorField& U = this->U_;
  LESeddyViscosity<BasicTurbulenceModel>::correct();
  tmp<volTensorField> tgradU(fvc::grad(U));
  const volTensorField& gradU = tgradU();
  volSymmTensorField S{dev(symm(gradU))};
  volScalarField magS{mag(S)};
  volVectorField Uf{filter_(U)};
  volSymmTensorField Sf{dev(symm(fvc::grad(Uf)))};
  volScalarField magSf{mag(Sf)};
  volSymmTensorField L{dev(filter_(sqr(U)) - (sqr(filter_(U))))};
  volSymmTensorField M
  {
    2.0*sqr(this->delta())*(filter_(magS*S) - 4.0*magSf*Sf)
  };
  volScalarField invT
  {
    (1.0/(theta_.value()*this->delta()))*pow(flm_*fmm_, 1.0/8.0)
  };
  volScalarField LM{L && M};
  fvScalarMatrix flmEqn {
    fvm::ddt(flm_)
  + fvm::div(phi, flm_)
  ==
    invT*LM
  - fvm::Sp(invT, flm_)
  };
  flmEqn.relax();
  flmEqn.solve();
  bound(flm_, flm0_);
  volScalarField MM{M && M};
  fvScalarMatrix fmmEqn {
    fvm::ddt(fmm_)
  + fvm::div(phi, fmm_)
  ==
    invT*MM
  - fvm::Sp(invT, fmm_)
  };
  fmmEqn.relax();
  fmmEqn.solve();
  bound(fmm_, fmm0_);
  correctNut(gradU);
}

}  // namespace LESModels
}  // namespace mousse

