// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::kineticTheoryModels::conductivityModels::Gidaspow
// Description
// SourceFiles
//   gidaspow.cpp
#ifndef Gidaspow_H
#define Gidaspow_H
#include "conductivity_model.hpp"
namespace mousse
{
namespace kineticTheoryModels
{
namespace conductivityModels
{
class Gidaspow
:
  public conductivityModel
{
public:
    //- Runtime type information
    TYPE_NAME("Gidaspow");
  // Constructors
    //- Construct from components
    Gidaspow(const dictionary& dict);
  //- Destructor
  virtual ~Gidaspow();
  // Member Functions
    tmp<volScalarField> kappa
    (
      const volScalarField& alpha1,
      const volScalarField& Theta,
      const volScalarField& g0,
      const volScalarField& rho1,
      const volScalarField& da,
      const dimensionedScalar& e
    ) const;
};
}  // namespace conductivityModels
}  // namespace kineticTheoryModels
}  // namespace mousse
#endif
