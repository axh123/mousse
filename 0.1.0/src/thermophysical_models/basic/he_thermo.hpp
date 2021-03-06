#ifndef THERMOPHYSICAL_MODELS_BASIC_HE_THERMO_HPP_
#define THERMOPHYSICAL_MODELS_BASIC_HE_THERMO_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::heThermo
// Description
//   Enthalpy/Internal energy for a mixture

#include "basic_mixture.hpp"


namespace mousse {

template<class BasicThermo, class MixtureType>
class heThermo
:
  public BasicThermo,
  public MixtureType
{
protected:
  // Protected data
    //- Energy field
    volScalarField he_;
  // Protected Member Functions
    // Enthalpy/Internal energy
      //- Correct the enthalpy/internal energy field boundaries
      void heBoundaryCorrection(volScalarField& he);
private:
  // Private Member Functions
    //- Initialize heThermo
    void init();
public:
  // Constructors
    //- Construct from mesh
    heThermo
    (
      const fvMesh&,
      const word& phaseName
    );
    //- Construct from mesh and dictionary
    heThermo
    (
      const fvMesh&,
      const dictionary&,
      const word& phaseName
    );
    //- Disable construct as copy
    heThermo(const heThermo<BasicThermo, MixtureType>&) = delete;
  //- Destructor
  virtual ~heThermo();
  // Member functions
    //- Return the compostion of the mixture
    virtual typename MixtureType::basicMixtureType&
    composition()
    {
      return *this;
    }
    //- Return the compostion of the mixture
    virtual const typename MixtureType::basicMixtureType&
    composition() const
    {
      return *this;
    }
    //- Return true if the equation of state is incompressible
    //  i.e. rho != f(p)
    virtual bool incompressible() const
    {
      return MixtureType::thermoType::incompressible;
    }
    //- Return true if the equation of state is isochoric
    //  i.e. rho = const
    virtual bool isochoric() const
    {
      return MixtureType::thermoType::isochoric;
    }
    // Access to thermodynamic state variables
      //- Enthalpy/Internal energy [J/kg]
      //  Non-const access allowed for transport equations
      virtual volScalarField& he()
      {
        return he_;
      }
      //- Enthalpy/Internal energy [J/kg]
      virtual const volScalarField& he() const
      {
        return he_;
      }
    // Fields derived from thermodynamic state variables
      //- Enthalpy/Internal energy
      //  for given pressure and temperature [J/kg]
      virtual tmp<volScalarField> he
      (
        const volScalarField& p,
        const volScalarField& T
      ) const;
      //- Enthalpy/Internal energy for cell-set [J/kg]
      virtual tmp<scalarField> he
      (
        const scalarField& p,
        const scalarField& T,
        const labelList& cells
      ) const;
      //- Enthalpy/Internal energy for patch [J/kg]
      virtual tmp<scalarField> he
      (
        const scalarField& p,
        const scalarField& T,
        const label patchi
      ) const;
      //- Chemical enthalpy [J/kg]
      virtual tmp<volScalarField> hc() const;
      //- Temperature from enthalpy/internal energy for cell-set
      virtual tmp<scalarField> THE
      (
        const scalarField& he,
        const scalarField& p,
        const scalarField& T0,      // starting temperature
        const labelList& cells
      ) const;
      //- Temperature from enthalpy/internal energy for patch
      virtual tmp<scalarField> THE
      (
        const scalarField& he,
        const scalarField& p,
        const scalarField& T0,      // starting temperature
        const label patchi
      ) const;
      //- Heat capacity at constant pressure for patch [J/kg/K]
      virtual tmp<scalarField> Cp
      (
        const scalarField& p,
        const scalarField& T,
        const label patchi
      ) const;
      //- Heat capacity at constant pressure [J/kg/K]
      virtual tmp<volScalarField> Cp() const;
      //- Heat capacity at constant volume for patch [J/kg/K]
      virtual tmp<scalarField> Cv
      (
        const scalarField& p,
        const scalarField& T,
        const label patchi
      ) const;
      //- Heat capacity at constant volume [J/kg/K]
      virtual tmp<volScalarField> Cv() const;
      //- Gamma = Cp/Cv []
      virtual tmp<volScalarField> gamma() const;
      //- Gamma = Cp/Cv for patch []
      virtual tmp<scalarField> gamma
      (
        const scalarField& p,
        const scalarField& T,
        const label patchi
      ) const;
      //- Heat capacity at constant pressure/volume for patch [J/kg/K]
      virtual tmp<scalarField> Cpv
      (
        const scalarField& p,
        const scalarField& T,
        const label patchi
      ) const;
      //- Heat capacity at constant pressure/volume [J/kg/K]
      virtual tmp<volScalarField> Cpv() const;
      //- Heat capacity ratio []
      virtual tmp<volScalarField> CpByCpv() const;
      //- Heat capacity ratio for patch []
      virtual tmp<scalarField> CpByCpv
      (
        const scalarField& p,
        const scalarField& T,
        const label patchi
      ) const;
    // Fields derived from transport state variables
      //- Thermal diffusivity for temperature of mixture [J/m/s/K]
      virtual tmp<volScalarField> kappa() const;
      //- Thermal diffusivity for temperature
      //  of mixture for patch [J/m/s/K]
      virtual tmp<scalarField> kappa
      (
        const label patchi
      ) const;
      //- Effective thermal diffusivity for temperature
      //  of mixture [J/m/s/K]
      virtual tmp<volScalarField> kappaEff(const volScalarField&) const;
      //- Effective thermal diffusivity for temperature
      //  of mixture for patch [J/m/s/K]
      virtual tmp<scalarField> kappaEff
      (
        const scalarField& alphat,
        const label patchi
      ) const;
      //- Effective thermal diffusivity of mixture [kg/m/s]
      virtual tmp<volScalarField> alphaEff
      (
        const volScalarField& alphat
      ) const;
      //- Effective thermal diffusivity of mixture for patch [kg/m/s]
      virtual tmp<scalarField> alphaEff
      (
        const scalarField& alphat,
        const label patchi
      ) const;
    //- Read thermophysical properties dictionary
    virtual bool read();
};

}  // namespace mousse

#include "he_thermo.ipp"

#endif
