#ifndef TRANSPORT_MODELS_INCOMPRESSIBLE_VISCOSITY_MODELS_POWER_LAW_HPP_
#define TRANSPORT_MODELS_INCOMPRESSIBLE_VISCOSITY_MODELS_POWER_LAW_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::viscosityModels::powerLaw
// Description
//   Standard power-law non-Newtonian viscosity model.

#include "viscosity_model.hpp"
#include "dimensioned_scalar.hpp"
#include "vol_fields.hpp"


namespace mousse {
namespace viscosityModels {

class powerLaw
:
  public viscosityModel
{
  // Private data
    dictionary coeffs_;
    dimensionedScalar k_;
    dimensionedScalar n_;
    dimensionedScalar nuMin_;
    dimensionedScalar nuMax_;
    volScalarField nu_;
  // Private Member Functions
    //- Calculate and return the laminar viscosity
    tmp<volScalarField> calcNu() const;
public:
  //- Runtime type information
  TYPE_NAME("powerLaw");
  // Constructors
    //- Construct from components
    powerLaw
    (
      const word& name,
      const dictionary& viscosityProperties,
      const volVectorField& U,
      const surfaceScalarField& phi
    );
  //- Destructor
  ~powerLaw()
  {}
  // Member Functions
    //- Return the laminar viscosity
    tmp<volScalarField> nu() const { return nu_; }
    //- Return the laminar viscosity for patch
    tmp<scalarField> nu(const label patchi) const
    {
      return nu_.boundaryField()[patchi];
    }
    //- Correct the laminar viscosity
    void correct()
    {
      nu_ = calcNu();
    }
    //- Read transportProperties dictionary
    bool read(const dictionary& viscosityProperties);
};

}  // namespace viscosityModels
}  // namespace mousse

#endif

