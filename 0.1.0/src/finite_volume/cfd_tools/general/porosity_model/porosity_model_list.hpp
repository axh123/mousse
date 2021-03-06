#ifndef FINITE_VOLUME_CFD_TOOLS_GENERAL_POROSITY_MODEL_POROSITY_MODEL_LIST_HPP_
#define FINITE_VOLUME_CFD_TOOLS_GENERAL_POROSITY_MODEL_POROSITY_MODEL_LIST_HPP_

// mousse: CFD toolbox
// Copyright (C) 2012-2014 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::porosityModelList
// Description
//   List container for porosity models

#include "fv_mesh.hpp"
#include "dictionary.hpp"
#include "fv_matrices_fwd.hpp"
#include "porosity_model.hpp"


namespace mousse {

// Forward declaration of friend functions and operators
class porosityModelList;
Ostream& operator<<(Ostream& os, const porosityModelList& models);


class porosityModelList
:
  public PtrList<porosityModel>
{
protected:
  // Protected data
    //- Reference to the mesh database
    const fvMesh& mesh_;
public:
  //- Constructor
    porosityModelList(const fvMesh& mesh, const dictionary& dict);
    //- Disallow default bitwise copy construct
    porosityModelList(const porosityModelList&) = delete;
    //- Disallow default bitwise assignment
    porosityModelList& operator=(const porosityModelList&) = delete;
  //- Destructor
  ~porosityModelList();
  // Member Functions
    //- Return active status
    bool active(const bool active = false) const;
    //- Reset the source list
    void reset(const dictionary& dict);
    //- Add resistance
    void addResistance(fvVectorMatrix& UEqn);
    //- Add resistance
    void addResistance
    (
      fvVectorMatrix& UEqn,
      const volScalarField& rho,
      const volScalarField& mu
    );
    //- Add resistance
    void addResistance
    (
      const fvVectorMatrix& UEqn,
      volTensorField& AU,
      bool correctAUprocBC = true
    );
    // I-O
      //- Read dictionary
      bool read(const dictionary& dict);
      //- Write data to Ostream
      bool writeData(Ostream& os) const;
      //- Ostream operator
      friend Ostream& operator<<
      (
        Ostream& os,
        const porosityModelList& models
      );
};
}  // namespace mousse
#endif
