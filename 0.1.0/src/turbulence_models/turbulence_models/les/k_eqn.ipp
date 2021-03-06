// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "k_eqn.hpp"


namespace mousse {
namespace LESModels {

// Protected Member Functions 
template<class BasicTurbulenceModel>
void kEqn<BasicTurbulenceModel>::correctNut()
{
  this->nut_ = Ck_*sqrt(k_)*this->delta();
  this->nut_.correctBoundaryConditions();
  BasicTurbulenceModel::correctNut();
}


template<class BasicTurbulenceModel>
tmp<fvScalarMatrix> kEqn<BasicTurbulenceModel>::kSource() const
{
  return
    tmp<fvScalarMatrix>
    {
      new fvScalarMatrix
      {
        k_,
        dimVolume*this->rho_.dimensions()*k_.dimensions()/dimTime
      }
    };
}


// Constructors 
template<class BasicTurbulenceModel>
kEqn<BasicTurbulenceModel>::kEqn
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
  k_
  {
    {
      IOobject::groupName("k", this->U_.group()),
      this->runTime_.timeName(),
      this->mesh_,
      IOobject::MUST_READ,
      IOobject::AUTO_WRITE
    },
    this->mesh_
  },
  Ck_
  {
    dimensioned<scalar>::lookupOrAddToDict
    (
      "Ck",
      this->coeffDict_,
      0.094
    )
  }
{
  bound(k_, this->kMin_);
  if (type == typeName) {
    correctNut();
    this->printCoeffs(type);
  }
}


// Member Functions 
template<class BasicTurbulenceModel>
bool kEqn<BasicTurbulenceModel>::read()
{
  if (LESeddyViscosity<BasicTurbulenceModel>::read()) {
    Ck_.readIfPresent(this->coeffDict());
    return true;
  }
  return false;
}


template<class BasicTurbulenceModel>
tmp<volScalarField> kEqn<BasicTurbulenceModel>::epsilon() const
{
  return
    tmp<volScalarField>
    {
      new volScalarField
      {
        {
          IOobject::groupName("epsilon", this->U_.group()),
          this->runTime_.timeName(),
          this->mesh_,
          IOobject::NO_READ,
          IOobject::NO_WRITE
        },
        this->Ce_*k()*sqrt(k())/this->delta()
      }
    };
}


template<class BasicTurbulenceModel>
void kEqn<BasicTurbulenceModel>::correct()
{
  if (!this->turbulence_) {
    return;
  }
  // Local references
  const alphaField& alpha = this->alpha_;
  const rhoField& rho = this->rho_;
  const surfaceScalarField& alphaRhoPhi = this->alphaRhoPhi_;
  const volVectorField& U = this->U_;
  volScalarField& nut = this->nut_;
  LESeddyViscosity<BasicTurbulenceModel>::correct();
  volScalarField divU{fvc::div(fvc::absolute(this->phi(), U))};
  tmp<volTensorField> tgradU(fvc::grad(U));
  volScalarField G{this->GName(), nut*(tgradU() && dev(twoSymm(tgradU())))};
  tgradU.clear();
  tmp<fvScalarMatrix> kEqn {
    fvm::ddt(alpha, rho, k_)
  + fvm::div(alphaRhoPhi, k_)
  - fvm::laplacian(alpha*rho*DkEff(), k_)
  ==
    alpha*rho*G
  - fvm::SuSp((2.0/3.0)*alpha*rho*divU, k_)
  - fvm::Sp(this->Ce_*alpha*rho*sqrt(k_)/this->delta(), k_)
  + kSource()
  };
  kEqn().relax();
  solve(kEqn);
  bound(k_, this->kMin_);
  correctNut();
}

}  // namespace LESModels
}  // namespace mousse

