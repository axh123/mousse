#ifndef CORE_MATRICES_LDU_MATRIX_SOLVERS_GAMG_GAMG_AGGLOMERATIONS_PAIR_GAMG_AGGLOMERATION_HPP_
#define CORE_MATRICES_LDU_MATRIX_SOLVERS_GAMG_GAMG_AGGLOMERATIONS_PAIR_GAMG_AGGLOMERATION_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011-2013 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::pairGAMGAgglomeration
// Description
//   Agglomerate using the pair algorithm.

#include "gamg_agglomeration.hpp"


namespace mousse {

class pairGAMGAgglomeration
:
  public GAMGAgglomeration
{

  // Private data

    //- Number of levels to merge, 1 = don't merge, 2 = merge pairs etc.
    label mergeLevels_;

    //- Direction of cell loop for the current level
    static bool forward_;

protected:

  // Protected Member Functions

    //- Agglomerate all levels starting from the given face weights
    void agglomerate
    (
      const lduMesh& mesh,
      const scalarField& faceWeights
    );

public:

  //- Runtime type information
  TYPE_NAME("pair");

  // Constructors

    //- Construct given mesh and controls
    pairGAMGAgglomeration
    (
      const lduMesh& mesh,
      const dictionary& controlDict
    );

    //- Disallow default bitwise copy construct
    pairGAMGAgglomeration(const pairGAMGAgglomeration&) = delete;

    //- Disallow default bitwise assignment
    pairGAMGAgglomeration& operator=(const pairGAMGAgglomeration&) = delete;

    //- Calculate and return agglomeration
    static tmp<labelField> agglomerate
    (
      label& nCoarseCells,
      const lduAddressing& fineMatrixAddressing,
      const scalarField& faceWeights
    );

};

}  // namespace mousse

#endif
