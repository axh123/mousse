#ifndef FINITE_VOLUME_FV_MATRICES_FV_MATRIX_HPP_
#define FINITE_VOLUME_FV_MATRICES_FV_MATRIX_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::fvMatrix
// Description
//   A special matrix type and solver, designed for finite volume
//   solutions of scalar equations.
//   Face addressing is used to make all matrix assembly
//   and solution loops vectorise.
// SourceFiles
//   fv_matrix.cpp
//   fv_matrix_solve.cpp
//   fv_scalar_matrix.cpp
#include "vol_fields.hpp"
#include "surface_fields.hpp"
#include "ldu_matrix.hpp"
#include "tmp.hpp"
#include "auto_ptr.hpp"
#include "dimensioned_types.hpp"
#include "zero.hpp"
#include "class_name.hpp"
namespace mousse
{
// Forward declaration of friend functions and operators
template<class Type>
class fvMatrix;
template<class Type>
tmp<GeometricField<Type, fvPatchField, volMesh> > operator&
(
  const fvMatrix<Type>&,
  const DimensionedField<Type, volMesh>&
);
template<class Type>
tmp<GeometricField<Type, fvPatchField, volMesh> > operator&
(
  const fvMatrix<Type>&,
  const tmp<DimensionedField<Type, volMesh> >&
);
template<class Type>
tmp<GeometricField<Type, fvPatchField, volMesh> > operator&
(
  const fvMatrix<Type>&,
  const tmp<GeometricField<Type, fvPatchField, volMesh> >&
);
template<class Type>
tmp<GeometricField<Type, fvPatchField, volMesh> > operator&
(
  const tmp<fvMatrix<Type> >&,
  const DimensionedField<Type, volMesh>&
);
template<class Type>
tmp<GeometricField<Type, fvPatchField, volMesh> > operator&
(
  const tmp<fvMatrix<Type> >&,
  const tmp<DimensionedField<Type, volMesh> >&
);
template<class Type>
tmp<GeometricField<Type, fvPatchField, volMesh> > operator&
(
  const tmp<fvMatrix<Type> >&,
  const tmp<GeometricField<Type, fvPatchField, volMesh> >&
);
template<class Type>
Ostream& operator<<(Ostream&, const fvMatrix<Type>&);
template<class T> class UIndirectList;
template<class Type>
class fvMatrix
:
  public refCount,
  public lduMatrix
{
  // Private data
    //- Const reference to GeometricField<Type, fvPatchField, volMesh>
    //  Converted into a non-const reference at the point of solution.
    const GeometricField<Type, fvPatchField, volMesh>& psi_;
    //- Dimension set
    dimensionSet dimensions_;
    //- Source term
    Field<Type> source_;
    //- Boundary scalar field containing pseudo-matrix coeffs
    //  for internal cells
    FieldField<Field, Type> internalCoeffs_;
    //- Boundary scalar field containing pseudo-matrix coeffs
    //  for boundary cells
    FieldField<Field, Type> boundaryCoeffs_;
    //- Face flux field for non-orthogonal correction
    mutable GeometricField<Type, fvsPatchField, surfaceMesh>
      *faceFluxCorrectionPtr_;
protected:
  //- Declare friendship with the fvSolver class
  friend class fvSolver;
  // Protected Member Functions
    //- Add patch contribution to internal field
    template<class Type2>
    void addToInternalField
    (
      const labelUList& addr,
      const Field<Type2>& pf,
      Field<Type2>& intf
    ) const;
    template<class Type2>
    void addToInternalField
    (
      const labelUList& addr,
      const tmp<Field<Type2> >& tpf,
      Field<Type2>& intf
    ) const;
    //- Subtract patch contribution from internal field
    template<class Type2>
    void subtractFromInternalField
    (
      const labelUList& addr,
      const Field<Type2>& pf,
      Field<Type2>& intf
    ) const;
    template<class Type2>
    void subtractFromInternalField
    (
      const labelUList& addr,
      const tmp<Field<Type2> >& tpf,
      Field<Type2>& intf
    ) const;
    // Matrix completion functionality
      void addBoundaryDiag
      (
        scalarField& diag,
        const direction cmpt
      ) const;
      void addCmptAvBoundaryDiag(scalarField& diag) const;
      void addBoundarySource
      (
        Field<Type>& source,
        const bool couples=true
      ) const;
    // Matrix manipulation functionality
      //- Set solution in given cells to the specified values
      template<template<class> class ListType>
      void setValuesFromList
      (
        const labelUList& cells,
        const ListType<Type>& values
      );
public:
  //- Solver class returned by the solver function
  //  used for systems in which it is useful to cache the solver for reuse
  //  e.g. if the solver is potentially expensive to construct (AMG) and can
  //  be used several times (PISO)
  class fvSolver
  {
    fvMatrix<Type>& fvMat_;
    autoPtr<lduMatrix::solver> solver_;
  public:
    // Constructors
      fvSolver(fvMatrix<Type>& fvMat, autoPtr<lduMatrix::solver> sol)
      :
        fvMat_(fvMat),
        solver_(sol)
      {}
    // Member functions
      //- Solve returning the solution statistics.
      //  Use the given solver controls
      solverPerformance solve(const dictionary&);
      //- Solve returning the solution statistics.
      //  Solver controls read from fvSolution
      solverPerformance solve();
  };
  CLASS_NAME("fvMatrix");
  // Constructors
    //- Construct given a field to solve for
    fvMatrix
    (
      const GeometricField<Type, fvPatchField, volMesh>&,
      const dimensionSet&
    );
    //- Construct as copy
    fvMatrix(const fvMatrix<Type>&);
    //- Construct as copy of tmp<fvMatrix<Type> > deleting argument
    #ifndef NoConstructFromTmp
    fvMatrix(const tmp<fvMatrix<Type> >&);
    #endif
    //- Construct from Istream given field to solve for
    fvMatrix(const GeometricField<Type, fvPatchField, volMesh>&, Istream&);
  //- Destructor
  virtual ~fvMatrix();
  // Member functions
    // Access
      const GeometricField<Type, fvPatchField, volMesh>& psi() const
      {
        return psi_;
      }
      const dimensionSet& dimensions() const
      {
        return dimensions_;
      }
      Field<Type>& source()
      {
        return source_;
      }
      const Field<Type>& source() const
      {
        return source_;
      }
      //- fvBoundary scalar field containing pseudo-matrix coeffs
      //  for internal cells
      FieldField<Field, Type>& internalCoeffs()
      {
        return internalCoeffs_;
      }
      //- fvBoundary scalar field containing pseudo-matrix coeffs
      //  for boundary cells
      FieldField<Field, Type>& boundaryCoeffs()
      {
        return boundaryCoeffs_;
      }
      //- Declare return type of the faceFluxCorrectionPtr() function
      typedef GeometricField<Type, fvsPatchField, surfaceMesh>
        *surfaceTypeFieldPtr;
      //- Return pointer to face-flux non-orthogonal correction field
      surfaceTypeFieldPtr& faceFluxCorrectionPtr()
      {
        return faceFluxCorrectionPtr_;
      }
    // Operations
      //- Set solution in given cells to the specified values
      //  and eliminate the corresponding equations from the matrix.
      void setValues
      (
        const labelUList& cells,
        const UList<Type>& values
      );
      //- Set solution in given cells to the specified values
      //  and eliminate the corresponding equations from the matrix.
      void setValues
      (
        const labelUList& cells,
        const UIndirectList<Type>& values
      );
      //- Set reference level for solution
      void setReference
      (
        const label celli,
        const Type& value,
        const bool forceReference = false
      );
      //- Set reference level for a component of the solution
      //  on a given patch face
      void setComponentReference
      (
        const label patchi,
        const label facei,
        const direction cmpt,
        const scalar value
      );
      //- Relax matrix (for steady-state solution).
      //  alpha = 1 : diagonally equal
      //  alpha < 1 : diagonally dominant
      //  alpha = 0 : do nothing
      //  Note: Requires positive diagonal.
      void relax(const scalar alpha);
      //- Relax matrix (for steady-state solution).
      //  alpha is read from controlDict
      void relax();
      //- Manipulate based on a boundary field
      void boundaryManipulate
      (
        typename GeometricField<Type, fvPatchField, volMesh>::
          GeometricBoundaryField& values
      );
      //- Construct and return the solver
      //  Use the given solver controls
      autoPtr<fvSolver> solver(const dictionary&);
      //- Construct and return the solver
      //  Solver controls read from fvSolution
      autoPtr<fvSolver> solver();
      //- Solve segregated or coupled returning the solution statistics.
      //  Use the given solver controls
      solverPerformance solve(const dictionary&);
      //- Solve segregated returning the solution statistics.
      //  Use the given solver controls
      solverPerformance solveSegregated(const dictionary&);
      //- Solve coupled returning the solution statistics.
      //  Use the given solver controls
      solverPerformance solveCoupled(const dictionary&);
      //- Solve returning the solution statistics.
      //  Solver controls read from fvSolution
      solverPerformance solve();
      //- Return the matrix residual
      tmp<Field<Type> > residual() const;
      //- Return the matrix scalar diagonal
      tmp<scalarField> D() const;
      //- Return the matrix Type diagonal
      tmp<Field<Type> > DD() const;
      //- Return the central coefficient
      tmp<volScalarField> A() const;
      //- Return the H operation source
      tmp<GeometricField<Type, fvPatchField, volMesh> > H() const;
      //- Return H(1)
      tmp<volScalarField> H1() const;
      //- Return the face-flux field from the matrix
      tmp<GeometricField<Type, fvsPatchField, surfaceMesh> >
        flux() const;
  // Member operators
    void operator=(const fvMatrix<Type>&);
    void operator=(const tmp<fvMatrix<Type> >&);
    void negate();
    void operator+=(const fvMatrix<Type>&);
    void operator+=(const tmp<fvMatrix<Type> >&);
    void operator-=(const fvMatrix<Type>&);
    void operator-=(const tmp<fvMatrix<Type> >&);
    void operator+=
    (
      const DimensionedField<Type, volMesh>&
    );
    void operator+=
    (
      const tmp<DimensionedField<Type, volMesh> >&
    );
    void operator+=
    (
      const tmp<GeometricField<Type, fvPatchField, volMesh> >&
    );
    void operator-=
    (
      const DimensionedField<Type, volMesh>&
    );
    void operator-=
    (
      const tmp<DimensionedField<Type, volMesh> >&
    );
    void operator-=
    (
      const tmp<GeometricField<Type, fvPatchField, volMesh> >&
    );
    void operator+=(const dimensioned<Type>&);
    void operator-=(const dimensioned<Type>&);
    void operator+=(const zero&);
    void operator-=(const zero&);
    void operator*=(const DimensionedField<scalar, volMesh>&);
    void operator*=(const tmp<DimensionedField<scalar, volMesh> >&);
    void operator*=(const tmp<volScalarField>&);
    void operator*=(const dimensioned<scalar>&);
  // Friend operators
    friend tmp<GeometricField<Type, fvPatchField, volMesh> >
    operator& <Type>
    (
      const fvMatrix<Type>&,
      const DimensionedField<Type, volMesh>&
    );
    friend tmp<GeometricField<Type, fvPatchField, volMesh> >
    operator& <Type>
    (
      const fvMatrix<Type>&,
      const tmp<GeometricField<Type, fvPatchField, volMesh> >&
    );
    friend tmp<GeometricField<Type, fvPatchField, volMesh> >
    operator& <Type>
    (
      const tmp<fvMatrix<Type> >&,
      const DimensionedField<Type, volMesh>&
    );
    friend tmp<GeometricField<Type, fvPatchField, volMesh> >
    operator& <Type>
    (
      const tmp<fvMatrix<Type> >&,
      const tmp<GeometricField<Type, fvPatchField, volMesh> >&
    );
  // Ostream operator
    friend Ostream& operator<< <Type>
    (
      Ostream&,
      const fvMatrix<Type>&
    );
};
// Global functions 
template<class Type>
void checkMethod
(
  const fvMatrix<Type>&,
  const fvMatrix<Type>&,
  const char*
);
template<class Type>
void checkMethod
(
  const fvMatrix<Type>&,
  const DimensionedField<Type, volMesh>&,
  const char*
);
template<class Type>
void checkMethod
(
  const fvMatrix<Type>&,
  const dimensioned<Type>&,
  const char*
);
//- Solve returning the solution statistics given convergence tolerance
//  Use the given solver controls
template<class Type>
solverPerformance solve(fvMatrix<Type>&, const dictionary&);
//- Solve returning the solution statistics given convergence tolerance,
//  deleting temporary matrix after solution.
//  Use the given solver controls
template<class Type>
solverPerformance solve
(
  const tmp<fvMatrix<Type> >&,
  const dictionary&
);
//- Solve returning the solution statistics given convergence tolerance
//  Solver controls read fvSolution
template<class Type>
solverPerformance solve(fvMatrix<Type>&);
//- Solve returning the solution statistics given convergence tolerance,
//  deleting temporary matrix after solution.
//  Solver controls read fvSolution
template<class Type>
solverPerformance solve(const tmp<fvMatrix<Type> >&);
//- Return the correction form of the given matrix
//  by subtracting the matrix multiplied by the current field
template<class Type>
tmp<fvMatrix<Type> > correction(const fvMatrix<Type>&);
//- Return the correction form of the given temporary matrix
//  by subtracting the matrix multiplied by the current field
template<class Type>
tmp<fvMatrix<Type> > correction(const tmp<fvMatrix<Type> >&);
// Global operators 
template<class Type>
tmp<fvMatrix<Type> > operator==
(
  const fvMatrix<Type>&,
  const fvMatrix<Type>&
);
template<class Type>
tmp<fvMatrix<Type> > operator==
(
  const tmp<fvMatrix<Type> >&,
  const fvMatrix<Type>&
);
template<class Type>
tmp<fvMatrix<Type> > operator==
(
  const fvMatrix<Type>&,
  const tmp<fvMatrix<Type> >&
);
template<class Type>
tmp<fvMatrix<Type> > operator==
(
  const tmp<fvMatrix<Type> >&,
  const tmp<fvMatrix<Type> >&
);
template<class Type>
tmp<fvMatrix<Type> > operator==
(
  const fvMatrix<Type>&,
  const DimensionedField<Type, volMesh>&
);
template<class Type>
tmp<fvMatrix<Type> > operator==
(
  const fvMatrix<Type>&,
  const tmp<DimensionedField<Type, volMesh> >&
);
template<class Type>
tmp<fvMatrix<Type> > operator==
(
  const fvMatrix<Type>&,
  const tmp<GeometricField<Type, fvPatchField, volMesh> >&
);
template<class Type>
tmp<fvMatrix<Type> > operator==
(
  const tmp<fvMatrix<Type> >&,
  const DimensionedField<Type, volMesh>&
);
template<class Type>
tmp<fvMatrix<Type> > operator==
(
  const tmp<fvMatrix<Type> >&,
  const tmp<DimensionedField<Type, volMesh> >&
);
template<class Type>
tmp<fvMatrix<Type> > operator==
(
  const tmp<fvMatrix<Type> >&,
  const tmp<GeometricField<Type, fvPatchField, volMesh> >&
);
template<class Type>
tmp<fvMatrix<Type> > operator==
(
  const fvMatrix<Type>&,
  const dimensioned<Type>&
);
template<class Type>
tmp<fvMatrix<Type> > operator==
(
  const tmp<fvMatrix<Type> >&,
  const dimensioned<Type>&
);
template<class Type>
tmp<fvMatrix<Type> > operator==
(
  const fvMatrix<Type>&,
  const zero&
);
template<class Type>
tmp<fvMatrix<Type> > operator==
(
  const tmp<fvMatrix<Type> >&,
  const zero&
);
template<class Type>
tmp<fvMatrix<Type> > operator-
(
  const fvMatrix<Type>&
);
template<class Type>
tmp<fvMatrix<Type> > operator-
(
  const tmp<fvMatrix<Type> >&
);
template<class Type>
tmp<fvMatrix<Type> > operator+
(
  const fvMatrix<Type>&,
  const fvMatrix<Type>&
);
template<class Type>
tmp<fvMatrix<Type> > operator+
(
  const tmp<fvMatrix<Type> >&,
  const fvMatrix<Type>&
);
template<class Type>
tmp<fvMatrix<Type> > operator+
(
  const fvMatrix<Type>&,
  const tmp<fvMatrix<Type> >&
);
template<class Type>
tmp<fvMatrix<Type> > operator+
(
  const tmp<fvMatrix<Type> >&,
  const tmp<fvMatrix<Type> >&
);
template<class Type>
tmp<fvMatrix<Type> > operator+
(
  const fvMatrix<Type>&,
  const DimensionedField<Type, volMesh>&
);
template<class Type>
tmp<fvMatrix<Type> > operator+
(
  const fvMatrix<Type>&,
  const tmp<DimensionedField<Type, volMesh> >&
);
template<class Type>
tmp<fvMatrix<Type> > operator+
(
  const fvMatrix<Type>&,
  const tmp<GeometricField<Type, fvPatchField, volMesh> >&
);
template<class Type>
tmp<fvMatrix<Type> > operator+
(
  const tmp<fvMatrix<Type> >&,
  const DimensionedField<Type, volMesh>&
);
template<class Type>
tmp<fvMatrix<Type> > operator+
(
  const tmp<fvMatrix<Type> >&,
  const tmp<DimensionedField<Type, volMesh> >&
);
template<class Type>
tmp<fvMatrix<Type> > operator+
(
  const tmp<fvMatrix<Type> >&,
  const tmp<GeometricField<Type, fvPatchField, volMesh> >&
);
template<class Type>
tmp<fvMatrix<Type> > operator+
(
  const DimensionedField<Type, volMesh>&,
  const fvMatrix<Type>&
);
template<class Type>
tmp<fvMatrix<Type> > operator+
(
  const tmp<DimensionedField<Type, volMesh> >&,
  const fvMatrix<Type>&
);
template<class Type>
tmp<fvMatrix<Type> > operator+
(
  const tmp<GeometricField<Type, fvPatchField, volMesh> >&,
  const fvMatrix<Type>&
);
template<class Type>
tmp<fvMatrix<Type> > operator+
(
  const DimensionedField<Type, volMesh>&,
  const tmp<fvMatrix<Type> >&
);
template<class Type>
tmp<fvMatrix<Type> > operator+
(
  const tmp<DimensionedField<Type, volMesh> >&,
  const tmp<fvMatrix<Type> >&
);
template<class Type>
tmp<fvMatrix<Type> > operator+
(
  const tmp<GeometricField<Type, fvPatchField, volMesh> >&,
  const tmp<fvMatrix<Type> >&
);
template<class Type>
tmp<fvMatrix<Type> > operator+
(
  const fvMatrix<Type>&,
  const dimensioned<Type>&
);
template<class Type>
tmp<fvMatrix<Type> > operator+
(
  const tmp<fvMatrix<Type> >&,
  const dimensioned<Type>&
);
template<class Type>
tmp<fvMatrix<Type> > operator+
(
  const dimensioned<Type>&,
  const fvMatrix<Type>&
);
template<class Type>
tmp<fvMatrix<Type> > operator+
(
  const dimensioned<Type>&,
  const tmp<fvMatrix<Type> >&
);
template<class Type>
tmp<fvMatrix<Type> > operator-
(
  const fvMatrix<Type>&,
  const fvMatrix<Type>&
);
template<class Type>
tmp<fvMatrix<Type> > operator-
(
  const tmp<fvMatrix<Type> >&,
  const fvMatrix<Type>&
);
template<class Type>
tmp<fvMatrix<Type> > operator-
(
  const fvMatrix<Type>&,
  const tmp<fvMatrix<Type> >&
);
template<class Type>
tmp<fvMatrix<Type> > operator-
(
  const tmp<fvMatrix<Type> >&,
  const tmp<fvMatrix<Type> >&
);
template<class Type>
tmp<fvMatrix<Type> > operator-
(
  const fvMatrix<Type>&,
  const DimensionedField<Type, volMesh>&
);
template<class Type>
tmp<fvMatrix<Type> > operator-
(
  const fvMatrix<Type>&,
  const tmp<DimensionedField<Type, volMesh> >&
);
template<class Type>
tmp<fvMatrix<Type> > operator-
(
  const fvMatrix<Type>&,
  const tmp<GeometricField<Type, fvPatchField, volMesh> >&
);
template<class Type>
tmp<fvMatrix<Type> > operator-
(
  const tmp<fvMatrix<Type> >&,
  const DimensionedField<Type, volMesh>&
);
template<class Type>
tmp<fvMatrix<Type> > operator-
(
  const tmp<fvMatrix<Type> >&,
  const tmp<DimensionedField<Type, volMesh> >&
);
template<class Type>
tmp<fvMatrix<Type> > operator-
(
  const tmp<fvMatrix<Type> >&,
  const tmp<GeometricField<Type, fvPatchField, volMesh> >&
);
template<class Type>
tmp<fvMatrix<Type> > operator-
(
  const DimensionedField<Type, volMesh>&,
  const fvMatrix<Type>&
);
template<class Type>
tmp<fvMatrix<Type> > operator-
(
  const tmp<DimensionedField<Type, volMesh> >&,
  const fvMatrix<Type>&
);
template<class Type>
tmp<fvMatrix<Type> > operator-
(
  const tmp<GeometricField<Type, fvPatchField, volMesh> >&,
  const fvMatrix<Type>&
);
template<class Type>
tmp<fvMatrix<Type> > operator-
(
  const DimensionedField<Type, volMesh>&,
  const tmp<fvMatrix<Type> >&
);
template<class Type>
tmp<fvMatrix<Type> > operator-
(
  const tmp<DimensionedField<Type, volMesh> >&,
  const tmp<fvMatrix<Type> >&
);
template<class Type>
tmp<fvMatrix<Type> > operator-
(
  const tmp<GeometricField<Type, fvPatchField, volMesh> >&,
  const tmp<fvMatrix<Type> >&
);
template<class Type>
tmp<fvMatrix<Type> > operator-
(
  const fvMatrix<Type>&,
  const dimensioned<Type>&
);
template<class Type>
tmp<fvMatrix<Type> > operator-
(
  const tmp<fvMatrix<Type> >&,
  const dimensioned<Type>&
);
template<class Type>
tmp<fvMatrix<Type> > operator-
(
  const dimensioned<Type>&,
  const fvMatrix<Type>&
);
template<class Type>
tmp<fvMatrix<Type> > operator-
(
  const dimensioned<Type>&,
  const tmp<fvMatrix<Type> >&
);
template<class Type>
tmp<fvMatrix<Type> > operator*
(
  const DimensionedField<scalar, volMesh>&,
  const fvMatrix<Type>&
);
template<class Type>
tmp<fvMatrix<Type> > operator*
(
  const tmp<DimensionedField<scalar, volMesh> >&,
  const fvMatrix<Type>&
);
template<class Type>
tmp<fvMatrix<Type> > operator*
(
  const tmp<volScalarField>&,
  const fvMatrix<Type>&
);
template<class Type>
tmp<fvMatrix<Type> > operator*
(
  const DimensionedField<scalar, volMesh>&,
  const tmp<fvMatrix<Type> >&
);
template<class Type>
tmp<fvMatrix<Type> > operator*
(
  const tmp<DimensionedField<scalar, volMesh> >&,
  const tmp<fvMatrix<Type> >&
);
template<class Type>
tmp<fvMatrix<Type> > operator*
(
  const tmp<volScalarField>&,
  const tmp<fvMatrix<Type> >&
);
template<class Type>
tmp<fvMatrix<Type> > operator*
(
  const dimensioned<scalar>&,
  const fvMatrix<Type>&
);
template<class Type>
tmp<fvMatrix<Type> > operator*
(
  const dimensioned<scalar>&,
  const tmp<fvMatrix<Type> >&
);
}  // namespace mousse
#ifdef NoRepository
#   include "fv_matrix.cpp"
#endif
// Specialisation for scalars
#include "fv_scalar_matrix.hpp"
#endif
