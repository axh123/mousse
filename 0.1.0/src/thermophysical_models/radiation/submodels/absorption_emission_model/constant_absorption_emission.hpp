#ifndef THERMOPHYSICAL_MODELS_RADIATION_SUBMODELS_ABSORPTION_EMISSION_MODEL_CONSTANT_ABSORPTION_EMISSION_HPP_
#define THERMOPHYSICAL_MODELS_RADIATION_SUBMODELS_ABSORPTION_EMISSION_MODEL_CONSTANT_ABSORPTION_EMISSION_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::radiation::constantAbsorptionEmission
// Description
//   Constant radiation absorption and emission coefficients for continuous
//   phase

#include "absorption_emission_model.hpp"


namespace mousse {
namespace radiation {

class constantAbsorptionEmission
:
  public absorptionEmissionModel
{
  // Private data
    //- Absorption model dictionary
    dictionary coeffsDict_;
    //- Absorption coefficient / [1/m]
    dimensionedScalar a_;
    //- Emission coefficient / [1/m]
    dimensionedScalar e_;
    //- Emission contribution / [kg/(m s^3)]
    dimensionedScalar E_;
public:
  //- Runtime type information
  TYPE_NAME("constantAbsorptionEmission");
  // Constructors
    //- Construct from components
    constantAbsorptionEmission(const dictionary& dict, const fvMesh& mesh);
  //- Destructor
  virtual ~constantAbsorptionEmission();
  // Member Functions
    // Access
      // Absorption coefficient
        //- Absorption coefficient for continuous phase
        tmp<volScalarField> aCont(const label bandI = 0) const;
      // Emission coefficient
        //- Emission coefficient for continuous phase
        tmp<volScalarField> eCont(const label bandI = 0) const;
      // Emission contribution
        //- Emission contribution for continuous phase
        tmp<volScalarField> ECont(const label bandI = 0) const;
  // Member Functions
    inline bool isGrey() const
    {
      return true;
    }
};

}  // namespace radiation
}  // namespace mousse

#endif

