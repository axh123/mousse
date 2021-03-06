#ifndef THERMOPHYSICAL_MODELS_RADIATION_RADIATION_MODELS_P1_HPP_
#define THERMOPHYSICAL_MODELS_RADIATION_RADIATION_MODELS_P1_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::radiation::P1
// Description
//   Works well for combustion applications where optical thickness, tau is
//   large, i.e. tau = a*L > 3 (L = distance between objects)
//   Assumes
//   - all surfaces are diffuse
//   - tends to over predict radiative fluxes from sources/sinks
//    *** SOURCES NOT CURRENTLY INCLUDED ***

#include "radiation_model.hpp"
#include "vol_fields.hpp"


namespace mousse {
namespace radiation {

class P1
:
  public radiationModel
{
  // Private data
    //- Incident radiation / [W/m2]
    volScalarField G_;
    //- Total radiative heat flux [W/m2]
    volScalarField Qr_;
    //- Absorption coefficient
    volScalarField a_;
    //- Emission coefficient
    volScalarField e_;
    //- Emission contribution
    volScalarField E_;
public:
  //- Runtime type information
  TYPE_NAME("P1");
  // Constructors
    //- Construct from components
    P1(const volScalarField& T);
    //- Construct from components
    P1(const dictionary& dict, const volScalarField& T);
    //- Disallow default bitwise copy construct
    P1(const P1&) = delete;
    //- Disallow default bitwise assignment
    P1& operator=(const P1&) = delete;
  //- Destructor
  virtual ~P1();
  // Member functions
    // Edit
      //- Solve radiation equation(s)
      void calculate();
      //- Read radiation properties dictionary
      bool read();
    // Access
      //- Source term component (for power of T^4)
      virtual tmp<volScalarField> Rp() const;
      //- Source term component (constant)
      virtual tmp<DimensionedField<scalar, volMesh>> Ru() const;
};

}  // namespace radiation
}  // namespace mousse

#endif

