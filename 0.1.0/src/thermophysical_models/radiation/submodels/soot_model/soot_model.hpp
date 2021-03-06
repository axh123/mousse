#ifndef THERMOPHYSICAL_MODELS_RADIATION_SUBMODELS_SOOT_MODEL_SOOT_MODEL_HPP_
#define THERMOPHYSICAL_MODELS_RADIATION_SUBMODELS_SOOT_MODEL_SOOT_MODEL_HPP_

// mousse: CFD toolbox
// Copyright (C) 2013-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::radiation::sootModel
// Description
//   Base class for soor models

#include "iodictionary.hpp"
#include "auto_ptr.hpp"
#include "run_time_selection_tables.hpp"
#include "vol_fields.hpp"


namespace mousse {
namespace radiation {

class sootModel
{
protected:
  // Protected data
    //- Radiation model dictionary
    const dictionary dict_;
    //- Reference to the fvMesh
    const fvMesh& mesh_;
public:
  //- Runtime type information
  TYPE_NAME("sootModel");
  //- Declare runtime constructor selection table
    DECLARE_RUN_TIME_SELECTION_TABLE
    (
      autoPtr,
      sootModel,
      dictionary,
      (
        const dictionary& dict,
        const fvMesh& mesh,
        const word& modelType
      ),
      (dict, mesh, modelType)
    );
  // Constructors
    //- Construct from components
    sootModel
    (
      const dictionary& dict,
      const fvMesh& mesh,
      const word& modelType
    );
  //- Selector
  static autoPtr<sootModel> New
  (
    const dictionary& dict,
    const fvMesh& mesh
  );
  //- Destructor
  virtual ~sootModel();
  // Member Functions
    // Access
      //- Reference to the mesh
      inline const fvMesh& mesh() const { return mesh_; }
      //- Reference to the dictionary
      inline const dictionary& dict() const { return dict_; }
      // Member Functions
        // Edit
          //- Main update/correction routine
          virtual void correct() = 0;
        // Access
          //- Return const reference to soot
          virtual const volScalarField& soot() const = 0;
};

}  // namespace mousse
}  // namespace radiation

#endif

