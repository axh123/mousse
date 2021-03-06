#ifndef TURBULENCE_MODELS_TURBULENCE_MODELS_LINEAR_VISCOUS_STRESS_HPP_
#define TURBULENCE_MODELS_TURBULENCE_MODELS_LINEAR_VISCOUS_STRESS_HPP_

// mousse: CFD toolbox
// Copyright (C) 2013-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::linearViscousStress
// Group
//   grpTurbulence
// Description
//   Linear viscous stress turbulence model base class

namespace mousse {

template<class BasicTurbulenceModel>
class linearViscousStress
:
  public BasicTurbulenceModel
{
public:
  typedef typename BasicTurbulenceModel::alphaField alphaField;
  typedef typename BasicTurbulenceModel::rhoField rhoField;
  typedef typename BasicTurbulenceModel::transportModel transportModel;
  // Constructors
    //- Construct from components
    linearViscousStress
    (
      const word& modelName,
      const alphaField& alpha,
      const rhoField& rho,
      const volVectorField& U,
      const surfaceScalarField& alphaRhoPhi,
      const surfaceScalarField& phi,
      const transportModel& transport,
      const word& propertiesName
    );
  //- Destructor
  virtual ~linearViscousStress()
  {}
  // Member Functions
    //- Re-read model coefficients if they have changed
    virtual bool read() = 0;
    //- Return the effective stress tensor
    virtual tmp<volSymmTensorField> devRhoReff() const;
    //- Return the source term for the momentum equation
    virtual tmp<fvVectorMatrix> divDevRhoReff(volVectorField& U) const;
    //- Return the source term for the momentum equation
    virtual tmp<fvVectorMatrix> divDevRhoReff
    (
      const volScalarField& rho,
      volVectorField& U
    ) const;
    //- Solve the turbulence equations and correct the turbulence viscosity
    virtual void correct() = 0;
};

}  // namespace mousse

#include "linear_viscous_stress.ipp"

#endif
