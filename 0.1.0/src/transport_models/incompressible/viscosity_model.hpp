#ifndef TRANSPORT_MODELS_INCOMPRESSIBLE_VISCOSITY_MODEL_HPP_
#define TRANSPORT_MODELS_INCOMPRESSIBLE_VISCOSITY_MODEL_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::viscosityModel
// Description
//   An abstract base class for incompressible viscosityModels.
//   The strain rate is defined by:
//     mag(symm(grad(U)))

#include "dictionary.hpp"
#include "vol_fields_fwd.hpp"
#include "surface_fields_fwd.hpp"
#include "dimensioned_scalar.hpp"
#include "run_time_selection_tables.hpp"


namespace mousse {

class viscosityModel
{
protected:
  // Protected data
    word name_;
    dictionary viscosityProperties_;
    const volVectorField& U_;
    const surfaceScalarField& phi_;
public:
  //- Runtime type information
  TYPE_NAME("viscosityModel");
  // Declare run-time constructor selection table
    DECLARE_RUN_TIME_SELECTION_TABLE
    (
      autoPtr,
      viscosityModel,
      dictionary,
      (
        const word& name,
        const dictionary& viscosityProperties,
        const volVectorField& U,
        const surfaceScalarField& phi
      ),
      (name, viscosityProperties, U, phi)
    );
  // Selectors
    //- Return a reference to the selected viscosity model
    static autoPtr<viscosityModel> New
    (
      const word& name,
      const dictionary& viscosityProperties,
      const volVectorField& U,
      const surfaceScalarField& phi
    );
  // Constructors
    //- Construct from components
    viscosityModel
    (
      const word& name,
      const dictionary& viscosityProperties,
      const volVectorField& U,
      const surfaceScalarField& phi
    );
    //- Disallow copy construct
    viscosityModel(const viscosityModel&) = delete;
    //- Disallow default bitwise assignment
    viscosityModel& operator=(const viscosityModel&) = delete;
  //- Destructor
  virtual ~viscosityModel()
  {}
  // Member Functions
    //- Return the phase transport properties dictionary
    const dictionary& viscosityProperties() const
    {
      return viscosityProperties_;
    }
    //- Return the strain rate
    tmp<volScalarField> strainRate() const;
    //- Return the laminar viscosity
    virtual tmp<volScalarField> nu() const = 0;
    //- Return the laminar viscosity for patch
    virtual tmp<scalarField> nu(const label patchi) const = 0;
    //- Correct the laminar viscosity
    virtual void correct() = 0;
    //- Read transportProperties dictionary
    virtual bool read(const dictionary& viscosityProperties) = 0;
};

}  // namespace mousse

#endif

