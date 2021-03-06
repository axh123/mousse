#ifndef SOLVERS_MULTIPHASE_REACTING_EULER_INTERFACIAL_MODELS_ASPECT_RATIO_MODELS_ASPECT_RATIO_MODEL_HPP_
#define SOLVERS_MULTIPHASE_REACTING_EULER_INTERFACIAL_MODELS_ASPECT_RATIO_MODELS_ASPECT_RATIO_MODEL_HPP_

// mousse: CFD toolbox
// Copyright (C) 2014-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::aspectRatioModel
// Description
// SourceFiles
//   aspect_ratio_model.cpp
//   new_aspect_ratio_model.cpp
#include "vol_fields.hpp"
#include "dictionary.hpp"
#include "run_time_selection_tables.hpp"
namespace mousse
{
class phasePair;
class aspectRatioModel
{
protected:
  // Protected data
    //- Phase pair
    const phasePair& pair_;
public:
  //- Runtime type information
  TYPE_NAME("aspectRatioModel");
  // Declare runtime construction
  DECLARE_RUN_TIME_SELECTION_TABLE
  (
    autoPtr,
    aspectRatioModel,
    dictionary,
    (
      const dictionary& dict,
      const phasePair& pair
    ),
    (dict, pair)
  );
  // Constructors
    //- Construct from a dictionary and an ordered phase pair
    aspectRatioModel
    (
      const dictionary& dict,
      const phasePair& pair
    );
  //- Destructor
  virtual ~aspectRatioModel();
  // Selectors
    static autoPtr<aspectRatioModel> New
    (
      const dictionary& dict,
      const phasePair& pair
    );
  // Member Functions
    //- Aspect ratio
    virtual tmp<volScalarField> E() const = 0;
};
}  // namespace mousse
#endif
