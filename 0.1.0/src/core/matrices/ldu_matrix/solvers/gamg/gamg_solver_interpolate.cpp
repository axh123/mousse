// mousse: CFD toolbox
// Copyright (C) 2013-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "gamg_solver.hpp"


// Private Member Functions 
void mousse::GAMGSolver::interpolate
(
  scalarField& psi,
  scalarField& Apsi,
  const lduMatrix& m,
  const FieldField<Field, scalar>& interfaceBouCoeffs,
  const lduInterfaceFieldPtrsList& interfaces,
  const direction cmpt
) const
{
  scalar* __restrict__ psiPtr = psi.begin();
  const label* const __restrict__ uPtr = m.lduAddr().upperAddr().begin();
  const label* const __restrict__ lPtr = m.lduAddr().lowerAddr().begin();
  const scalar* const __restrict__ diagPtr = m.diag().begin();
  const scalar* const __restrict__ upperPtr = m.upper().begin();
  const scalar* const __restrict__ lowerPtr = m.lower().begin();
  Apsi = 0;
  scalar* __restrict__ ApsiPtr = Apsi.begin();
  m.initMatrixInterfaces
  (
    interfaceBouCoeffs,
    interfaces,
    psi,
    Apsi,
    cmpt
  );
  const label nFaces = m.upper().size();
  for (label face=0; face<nFaces; face++) {
    ApsiPtr[uPtr[face]] += lowerPtr[face]*psiPtr[lPtr[face]];
    ApsiPtr[lPtr[face]] += upperPtr[face]*psiPtr[uPtr[face]];
  }
  m.updateMatrixInterfaces
  (
    interfaceBouCoeffs,
    interfaces,
    psi,
    Apsi,
    cmpt
  );
  const label nCells = m.diag().size();
  for (label celli=0; celli<nCells; celli++) {
    psiPtr[celli] = -ApsiPtr[celli]/(diagPtr[celli]);
  }
}


void mousse::GAMGSolver::interpolate
(
  scalarField& psi,
  scalarField& Apsi,
  const lduMatrix& m,
  const FieldField<Field, scalar>& interfaceBouCoeffs,
  const lduInterfaceFieldPtrsList& interfaces,
  const labelList& restrictAddressing,
  const scalarField& psiC,
  const direction cmpt
) const
{
  interpolate
  (
    psi,
    Apsi,
    m,
    interfaceBouCoeffs,
    interfaces,
    cmpt
  );
  const label nCells = m.diag().size();
  scalar* __restrict__ psiPtr = psi.begin();
  const scalar* const __restrict__ diagPtr = m.diag().begin();
  const label nCCells = psiC.size();
  scalarField corrC{nCCells, 0};
  scalarField diagC{nCCells, 0};
  for (label celli=0; celli<nCells; celli++) {
    corrC[restrictAddressing[celli]] += diagPtr[celli]*psiPtr[celli];
    diagC[restrictAddressing[celli]] += diagPtr[celli];
  }
  for (label ccelli=0; ccelli<nCCells; ccelli++) {
    corrC[ccelli] = psiC[ccelli] - corrC[ccelli]/diagC[ccelli];
  }
  for (label celli=0; celli<nCells; celli++) {
    psiPtr[celli] += corrC[restrictAddressing[celli]];
  }
}
