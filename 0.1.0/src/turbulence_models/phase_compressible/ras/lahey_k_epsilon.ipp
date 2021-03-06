// mousse: CFD toolbox
// Copyright (C) 2013-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "lahey_k_epsilon.hpp"
#include "add_to_run_time_selection_table.hpp"
#include "two_phase_system.hpp"
#include "drag_model.hpp"


namespace mousse {
namespace RASModels {

// Constructors 
template<class BasicTurbulenceModel>
LaheyKEpsilon<BasicTurbulenceModel>::LaheyKEpsilon
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
  kEpsilon<BasicTurbulenceModel>
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
      0.25
    )
  },
  C3_
  {
    dimensioned<scalar>::lookupOrAddToDict
    (
      "C3",
      this->coeffDict_,
      this->C2_.value()
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
bool LaheyKEpsilon<BasicTurbulenceModel>::read()
{
  if (kEpsilon<BasicTurbulenceModel>::read()) {
    alphaInversion_.readIfPresent(this->coeffDict());
    Cp_.readIfPresent(this->coeffDict());
    C3_.readIfPresent(this->coeffDict());
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
LaheyKEpsilon<BasicTurbulenceModel>::gasTurbulence() const
{
  if (!gasTurbulencePtr_) {
    const volVectorField& U = this->U_;
    const transportModel& liquid = this->transport();
    const twoPhaseSystem& fluid =
      refCast<const twoPhaseSystem>(liquid.fluid());
    const transportModel& gas = fluid.otherPhase(liquid);
    gasTurbulencePtr_ =
      &U.db().lookupObject<PhaseCompressibleTurbulenceModel<transportModel>>
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
void LaheyKEpsilon<BasicTurbulenceModel>::correctNut()
{
  const PhaseCompressibleTurbulenceModel<transportModel>& gasTurbulence =
    this->gasTurbulence();
  this->nut_ =
    this->Cmu_*sqr(this->k_)/this->epsilon_
    + Cmub_*gasTurbulence.transport().d()*gasTurbulence.alpha()
    *(mag(this->U_ - gasTurbulence.U()));
  this->nut_.correctBoundaryConditions();
}


template<class BasicTurbulenceModel>
tmp<volScalarField> LaheyKEpsilon<BasicTurbulenceModel>::bubbleG() const
{
  const PhaseCompressibleTurbulenceModel<transportModel>& gasTurbulence =
    this->gasTurbulence();
  const transportModel& liquid = this->transport();
  const twoPhaseSystem& fluid = refCast<const twoPhaseSystem>(liquid.fluid());
  const transportModel& gas = fluid.otherPhase(liquid);
  volScalarField magUr{mag(this->U_ - gasTurbulence.U())};
  tmp<volScalarField> bubbleG
  {
    Cp_
    *(pow3(magUr)
      + pow(fluid.drag(gas).CdRe()*liquid.nu()/gas.d(), 4.0/3.0)
      *pow(magUr, 5.0/3.0))
    *gas/gas.d()
  };
  return bubbleG;
}


template<class BasicTurbulenceModel>
tmp<volScalarField>
LaheyKEpsilon<BasicTurbulenceModel>::phaseTransferCoeff() const
{
  const volVectorField& U = this->U_;
  const alphaField& alpha = this->alpha_;
  const rhoField& rho = this->rho_;
  const turbulenceModel& gasTurbulence = this->gasTurbulence();
  return
    (
      max(alphaInversion_ - alpha, scalar(0))
      *rho
      *min(gasTurbulence.epsilon()/gasTurbulence.k(), 1.0/U.time().deltaT())
    );
}


template<class BasicTurbulenceModel>
tmp<fvScalarMatrix> LaheyKEpsilon<BasicTurbulenceModel>::kSource() const
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


template<class BasicTurbulenceModel>
tmp<fvScalarMatrix> LaheyKEpsilon<BasicTurbulenceModel>::epsilonSource() const
{
  const alphaField& alpha = this->alpha_;
  const rhoField& rho = this->rho_;
  const PhaseCompressibleTurbulenceModel<transportModel>& gasTurbulence =
    this->gasTurbulence();
  const volScalarField phaseTransferCoeff(this->phaseTransferCoeff());
  return
    alpha*rho*this->C3_*this->epsilon_*bubbleG()/this->k_
    + phaseTransferCoeff*gasTurbulence.epsilon()
    - fvm::Sp(phaseTransferCoeff, this->epsilon_);
}


template<class BasicTurbulenceModel>
void LaheyKEpsilon<BasicTurbulenceModel>::correct()
{
  kEpsilon<BasicTurbulenceModel>::correct();
}

}  // namespace RASModels
}  // namespace mousse

