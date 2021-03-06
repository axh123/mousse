// mousse: CFD toolbox
// Copyright (C) 2013-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "niceno_k_eqn.hpp"
#include "add_to_run_time_selection_table.hpp"
#include "two_phase_system.hpp"
#include "drag_model.hpp"


namespace mousse {
namespace LESModels {

// Constructors 
template<class BasicTurbulenceModel>
NicenoKEqn<BasicTurbulenceModel>::NicenoKEqn
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
  kEqn<BasicTurbulenceModel>
  {
    alpha,
    rho,
    U,
    alphaRhoPhi,
    phi,
    transport,
    propertiesName,
    type
  },
  gasTurbulencePtr_{NULL},
  alphaInversion_
  {
    dimensioned<scalar>::lookupOrAddToDict
    (
      "alphaInversion",
      this->coeffDict_,
      0.3
    )
  },
  Cp_
  {
    dimensioned<scalar>::lookupOrAddToDict
    (
      "Cp",
      this->coeffDict_,
      this->Ck_.value()
    )
  },
  Cmub_
  {
    dimensioned<scalar>::lookupOrAddToDict
    (
      "Cmub",
      this->coeffDict_,
      0.6
    )
  }
{
  if (type == typeName) {
    // Cannot correct nut yet: construction of the phases is not complete
    // correctNut();
    this->printCoeffs(type);
  }
}


// Member Functions 
template<class BasicTurbulenceModel>
bool NicenoKEqn<BasicTurbulenceModel>::read()
{
  if (kEqn<BasicTurbulenceModel>::read()) {
    alphaInversion_.readIfPresent(this->coeffDict());
    Cp_.readIfPresent(this->coeffDict());
    Cmub_.readIfPresent(this->coeffDict());
    return true;
  }
  return false;
}


template<class BasicTurbulenceModel>
const PhaseCompressibleTurbulenceModel
<
  typename BasicTurbulenceModel::transportModel
>&
NicenoKEqn<BasicTurbulenceModel>::gasTurbulence() const
{
  if (!gasTurbulencePtr_) {
    const volVectorField& U = this->U_;
    const transportModel& liquid = this->transport();
    const twoPhaseSystem& fluid =
      refCast<const twoPhaseSystem>(liquid.fluid());
    const transportModel& gas = fluid.otherPhase(liquid);
    gasTurbulencePtr_ =
      &U.db()
      .lookupObject<PhaseCompressibleTurbulenceModel<transportModel>>
      (
        IOobject::groupName
        (
          turbulenceModel::propertiesName,
          gas.name()
        )
      );
  }
  return *gasTurbulencePtr_;
}


template<class BasicTurbulenceModel>
void NicenoKEqn<BasicTurbulenceModel>::correctNut()
{
  const PhaseCompressibleTurbulenceModel<transportModel>& gasTurbulence =
    this->gasTurbulence();
  this->nut_ =
    this->Ck_*sqrt(this->k_)*this->delta()
    + Cmub_*gasTurbulence.transport().d()*gasTurbulence.alpha()
    *(mag(this->U_ - gasTurbulence.U()));
  this->nut_.correctBoundaryConditions();
}


template<class BasicTurbulenceModel>
tmp<volScalarField> NicenoKEqn<BasicTurbulenceModel>::bubbleG() const
{
  const PhaseCompressibleTurbulenceModel<transportModel>& gasTurbulence =
    this->gasTurbulence();
  const transportModel& liquid = this->transport();
  const twoPhaseSystem& fluid =
    refCast<const twoPhaseSystem>(liquid.fluid());
  const transportModel& gas = fluid.otherPhase(liquid);
  volScalarField magUr{mag(this->U_ - gasTurbulence.U())};
  tmp<volScalarField> bubbleG
  {
    Cp_*sqr(magUr)*fluid.drag(gas).K()/liquid.rho()
  };
  return bubbleG;
}


template<class BasicTurbulenceModel>
tmp<volScalarField>
NicenoKEqn<BasicTurbulenceModel>::phaseTransferCoeff() const
{
  const volVectorField& U = this->U_;
  const alphaField& alpha = this->alpha_;
  const rhoField& rho = this->rho_;
  const turbulenceModel& gasTurbulence = this->gasTurbulence();
  return
    (
      max(alphaInversion_ - alpha, scalar(0))
      *rho
      *min
      (
        this->Ce_*sqrt(gasTurbulence.k())/this->delta(),
        1.0/U.time().deltaT()
      )
    );
}


template<class BasicTurbulenceModel>
tmp<fvScalarMatrix> NicenoKEqn<BasicTurbulenceModel>::kSource() const
{
  const alphaField& alpha = this->alpha_;
  const rhoField& rho = this->rho_;
  const PhaseCompressibleTurbulenceModel<transportModel>& gasTurbulence =
    this->gasTurbulence();
  const volScalarField phaseTransferCoeff{this->phaseTransferCoeff()};
  return
    alpha*rho*bubbleG()
    + phaseTransferCoeff*gasTurbulence.k()
    - fvm::Sp(phaseTransferCoeff, this->k_);
}

}  // namespace LESModels
}  // namespace mousse

