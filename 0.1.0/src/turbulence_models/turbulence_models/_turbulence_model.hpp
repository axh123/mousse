#ifndef TURBULENCE_MODELS_TURBULENCE_MODELS_TTURBULENCE_MODEL_HPP_
#define TURBULENCE_MODELS_TURBULENCE_MODELS_TTURBULENCE_MODEL_HPP_

// mousse: CFD toolbox
// Copyright (C) 2013-2014 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::TurbulenceModel
// Description
//   Templated abstract base class for turbulence models

#include "turbulence_model.hpp"
#include "auto_ptr.hpp"
#include "run_time_selection_tables.hpp"


namespace mousse {

template
<
  class Alpha,
  class Rho,
  class BasicTurbulenceModel,
  class TransportModel
>
class TurbulenceModel
:
  public BasicTurbulenceModel
{
public:
  typedef Alpha alphaField;
  typedef Rho rhoField;
  typedef TransportModel transportModel;
protected:
  // Protected data
    const alphaField& alpha_;
    const transportModel& transport_;
public:
  // Declare run-time constructor selection table
    DECLARE_RUN_TIME_NEW_SELECTION_TABLE
    (
      autoPtr,
      TurbulenceModel,
      dictionary,
      (
        const alphaField& alpha,
        const rhoField& rho,
        const volVectorField& U,
        const surfaceScalarField& alphaRhoPhi,
        const surfaceScalarField& phi,
        const transportModel& transport,
        const word& propertiesName
      ),
      (alpha, rho, U, alphaRhoPhi, phi, transport, propertiesName)
    );
  // Constructors
    //- Construct
    TurbulenceModel
    (
      const alphaField& alpha,
      const rhoField& rho,
      const volVectorField& U,
      const surfaceScalarField& alphaRhoPhi,
      const surfaceScalarField& phi,
      const transportModel& transport,
      const word& propertiesName
    );
    //- Disallow default bitwise copy construct
    TurbulenceModel(const TurbulenceModel&) = delete;
    //- Disallow default bitwise assignment
    TurbulenceModel& operator=(const TurbulenceModel&) = delete;
  // Selectors
    //- Return a reference to the selected turbulence model
    static autoPtr<TurbulenceModel> New
    (
      const alphaField& alpha,
      const rhoField& rho,
      const volVectorField& U,
      const surfaceScalarField& alphaRhoPhi,
      const surfaceScalarField& phi,
      const transportModel& transport,
      const word& propertiesName = turbulenceModel::propertiesName
    );
  //- Destructor
  virtual ~TurbulenceModel()
  {}
  // Member Functions
    //- Access function to phase fraction
    const alphaField& alpha() const { return alpha_; }
    //- Access function to incompressible transport model
    const transportModel& transport() const { return transport_; }
    //- Return the laminar viscosity
    tmp<volScalarField> nu() const { return transport_.nu(); }
    //- Return the laminar viscosity on patchi
    tmp<scalarField> nu(const label patchi) const
    {
      return transport_.nu(patchi);
    }
};

}  // namespace mousse

#include "_turbulence_model.ipp"

#endif

