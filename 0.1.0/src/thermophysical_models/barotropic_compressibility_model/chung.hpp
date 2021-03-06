#ifndef THERMOPHYSICAL_MODELS_BAROTROPIC_COMPRESSIBILITY_MODEL_CHUNG_HPP_
#define THERMOPHYSICAL_MODELS_BAROTROPIC_COMPRESSIBILITY_MODEL_CHUNG_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::compressibilityModels::Chung
// Description
//   Chung compressibility model.

#include "barotropic_compressibility_model.hpp"
#include "dimensioned_scalar.hpp"


namespace mousse {

namespace compressibilityModels
{
class Chung
:
  public barotropicCompressibilityModel
{
  // Private data
    dimensionedScalar psiv_;
    dimensionedScalar psil_;
    dimensionedScalar rhovSat_;
    dimensionedScalar rholSat_;
public:
  //- Runtime type information
  TYPE_NAME("Chung");
  // Constructors
    //- Construct from components
    Chung
    (
      const dictionary& compressibilityProperties,
      const volScalarField& gamma,
      const word& psiName = "psi"
    );
  //- Destructor
  ~Chung()
  {}
  // Member Functions
    //- Correct the Chung compressibility
    void correct();
    //- Read transportProperties dictionary
    bool read(const dictionary& compressibilityProperties);
};

}  // namespace compressibilityModels
}  // namespace mousse

#endif

