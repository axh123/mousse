#ifndef SOLVERS_MULTIPHASE_TWO_PHASE_EULER_TWO_PHASE_SYSTEM_PHASE_MODEL_HPP_
#define SOLVERS_MULTIPHASE_TWO_PHASE_EULER_TWO_PHASE_SYSTEM_PHASE_MODEL_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::phaseModel
// SourceFiles
//   phase_model.cpp
#include "dictionary.hpp"
#include "dimensioned_scalar.hpp"
#include "vol_fields.hpp"
#include "surface_fields.hpp"
#include "transport_model.hpp"
#include "rho_thermo.hpp"
namespace mousse
{
// Forward declarations
class twoPhaseSystem;
class diameterModel;
template<class Phase>
class PhaseCompressibleTurbulenceModel;
class phaseModel
:
  public volScalarField,
  public transportModel
{
  // Private data
    //- Reference to the twoPhaseSystem to which this phase belongs
    const twoPhaseSystem& fluid_;
    //- Name of phase
    word name_;
    dictionary phaseDict_;
    //- Return the residual phase-fraction for given phase
    //  Used to stabilize the phase momentum as the phase-fraction -> 0
    dimensionedScalar residualAlpha_;
    //- Optional maximum phase-fraction (e.g. packing limit)
    scalar alphaMax_;
    //- Thermophysical properties
    autoPtr<rhoThermo> thermo_;
    //- Velocity
    volVectorField U_;
    //- Volumetric flux of the phase
    surfaceScalarField alphaPhi_;
    //- Mass flux of the phase
    surfaceScalarField alphaRhoPhi_;
    //- Volumetric flux of the phase
    autoPtr<surfaceScalarField> phiPtr_;
    //- Diameter model
    autoPtr<diameterModel> dPtr_;
    //- Turbulence model
    autoPtr<PhaseCompressibleTurbulenceModel<phaseModel> > turbulence_;
public:
  // Constructors
    phaseModel
    (
      const twoPhaseSystem& fluid,
      const dictionary& phaseProperties,
      const word& phaseName
    );
  //- Destructor
  virtual ~phaseModel();
  // Member Functions
    //- Return the name of this phase
    const word& name() const
    {
      return name_;
    }
    //- Return the twoPhaseSystem to which this phase belongs
    const twoPhaseSystem& fluid() const
    {
      return fluid_;
    }
    //- Return the other phase in this two-phase system
    const phaseModel& otherPhase() const;
    //- Return the residual phase-fraction for given phase
    //  Used to stabilize the phase momentum as the phase-fraction -> 0
    const dimensionedScalar& residualAlpha() const
    {
      return residualAlpha_;
    }
    //- Optional maximum phase-fraction (e.g. packing limit)
    //  Defaults to 1
    scalar alphaMax() const
    {
      return alphaMax_;
    }
    //- Return the Sauter-mean diameter
    tmp<volScalarField> d() const;
    //- Return the turbulence model
    const PhaseCompressibleTurbulenceModel<phaseModel>&
      turbulence() const;
    //- Return non-const access to the turbulence model
    //  for correction
    PhaseCompressibleTurbulenceModel<phaseModel>&
      turbulence();
    //- Return the thermophysical model
    const rhoThermo& thermo() const
    {
      return thermo_();
    }
    //- Return non-const access to the thermophysical model
    //  for correction
    rhoThermo& thermo()
    {
      return thermo_();
    }
    //- Return the laminar viscosity
    tmp<volScalarField> nu() const
    {
      return thermo_->nu();
    }
    //- Return the laminar viscosity for patch
    tmp<scalarField> nu(const label patchi) const
    {
      return thermo_->nu(patchi);
    }
    //- Return the laminar dynamic viscosity
    tmp<volScalarField> mu() const
    {
      return thermo_->mu();
    }
    //- Return the laminar dynamic viscosity for patch
    tmp<scalarField> mu(const label patchi) const
    {
      return thermo_->mu(patchi);
    }
    //- Return the thermal conductivity on a patch
    tmp<scalarField> kappa(const label patchi) const
    {
      return thermo_->kappa(patchi);
    }
    //- Return the thermal conductivity
    tmp<volScalarField> kappa() const
    {
      return thermo_->kappa();
    }
    //- Return the laminar thermal conductivity
    tmp<volScalarField> kappaEff
    (
      const volScalarField& alphat
    ) const
    {
      return thermo_->kappaEff(alphat);
    }
    //- Return the laminar thermal conductivity on a patch
    tmp<scalarField> kappaEff
    (
      const scalarField& alphat,
      const label patchi
    ) const
    {
      return thermo_->kappaEff(alphat, patchi);
    }
    //- Return the laminar thermal diffusivity for enthalpy
    tmp<volScalarField> alpha() const
    {
      return thermo_->alpha();
    }
    //- Return the laminar thermal diffusivity for enthalpy on a patch
    tmp<scalarField> alpha(const label patchi) const
    {
      return thermo_->alpha(patchi);
    }
    //- Return the effective thermal diffusivity for enthalpy
    tmp<volScalarField> alphaEff
    (
      const volScalarField& alphat
    ) const
    {
      return thermo_->alphaEff(alphat);
    }
    //- Return the effective thermal diffusivity for enthalpy on a patch
    tmp<scalarField> alphaEff
    (
      const scalarField& alphat,
      const label patchi
    ) const
    {
      return thermo_->alphaEff(alphat, patchi);
    }
    //- Return the specific heat capacity
    tmp<volScalarField> Cp() const
    {
      return thermo_->Cp();
    }
    //- Return the density
    const volScalarField& rho() const
    {
      return thermo_->rho();
    }
    //- Return the velocity
    const volVectorField& U() const
    {
      return U_;
    }
    //- Return non-const access to the velocity
    //  Used in the momentum equation
    volVectorField& U()
    {
      return U_;
    }
    //- Return the volumetric flux
    const surfaceScalarField& phi() const
    {
      return phiPtr_();
    }
    //- Return non-const access to the volumetric flux
    surfaceScalarField& phi()
    {
      return phiPtr_();
    }
    //- Return the volumetric flux of the phase
    const surfaceScalarField& alphaPhi() const
    {
      return alphaPhi_;
    }
    //- Return non-const access to the volumetric flux of the phase
    surfaceScalarField& alphaPhi()
    {
      return alphaPhi_;
    }
    //- Return the mass flux of the phase
    const surfaceScalarField& alphaRhoPhi() const
    {
      return alphaRhoPhi_;
    }
    //- Return non-const access to the mass flux of the phase
    surfaceScalarField& alphaRhoPhi()
    {
      return alphaRhoPhi_;
    }
    //- Correct the phase properties
    //  other than the thermodynamics and turbulence
    //  which have special treatment
    void correct();
    //- Read phaseProperties dictionary
    virtual bool read(const dictionary& phaseProperties);
    //- Dummy Read for transportModel
    virtual bool read()
    {
      return true;
    }
};
}  // namespace mousse
#endif
