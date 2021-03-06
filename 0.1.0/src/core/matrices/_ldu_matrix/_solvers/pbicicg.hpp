#ifndef CORE_MATRICES_TLDU_MATRIX_TSOLVERS_PBICICG_HPP_
#define CORE_MATRICES_TLDU_MATRIX_TSOLVERS_PBICICG_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011-2012 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::PBiCICG
// Description
//   Preconditioned bi-conjugate gradient solver for asymmetric lduMatrices
//   using a run-time selectable preconditiioner.

#include "_ldu_matrix.hpp"


namespace mousse {

template<class Type, class DType, class LUType>
class PBiCICG
:
  public LduMatrix<Type, DType, LUType>::solver
{
public:

  //- Runtime type information
  TYPE_NAME("PBiCICG");

  // Constructors
    //- Construct from matrix components and solver data dictionary
    PBiCICG
    (
      const word& fieldName,
      const LduMatrix<Type, DType, LUType>& matrix,
      const dictionary& solverDict
    );

    //- Disallow default bitwise copy construct
    PBiCICG(const PBiCICG&) = delete;

    //- Disallow default bitwise assignment
    PBiCICG& operator=(const PBiCICG&) = delete;

  // Destructor
    virtual ~PBiCICG()
    {}

  // Member Functions

    //- Solve the matrix with this solver
    virtual SolverPerformance<Type> solve(Field<Type>& psi) const;

};

}  // namespace mousse

#include "pbicicg.ipp"

#endif
