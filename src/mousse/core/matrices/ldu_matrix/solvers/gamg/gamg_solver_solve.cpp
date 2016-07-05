// mousse: CFD toolbox
// Copyright (C) 2011-2014 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "gamg_solver.hpp"
#include "iccg.hpp"
#include "biccg.hpp"
#include "sub_field.hpp"


// Member Functions 
mousse::solverPerformance mousse::GAMGSolver::solve
(
  scalarField& psi,
  const scalarField& source,
  const direction cmpt
) const
{
  // Setup class containing solver performance data
  solverPerformance solverPerf{typeName, fieldName_};
  // Calculate A.psi used to calculate the initial residual
  scalarField Apsi{psi.size()};
  matrix_.Amul(Apsi, psi, interfaceBouCoeffs_, interfaces_, cmpt);
  // Create the storage for the finestCorrection which may be used as a
  // temporary in normFactor
  scalarField finestCorrection{psi.size()};
  // Calculate normalisation factor
  scalar normFactor = this->normFactor(psi, source, Apsi, finestCorrection);
  if (debug >= 2) {
    Pout << "   Normalisation factor = " << normFactor << endl;
  }
  // Calculate initial finest-grid residual field
  scalarField finestResidual{source - Apsi};
  // Calculate normalised residual for convergence test
  solverPerf.initialResidual() = gSumMag
  (
    finestResidual,
    matrix().mesh().comm()
  )/normFactor;
  solverPerf.finalResidual() = solverPerf.initialResidual();
  // Check convergence, solve if not converged
  if (minIter_ > 0
      || !solverPerf.checkConvergence(tolerance_, relTol_)) {
    // Create coarse grid correction fields
    PtrList<scalarField> coarseCorrFields;
    // Create coarse grid sources
    PtrList<scalarField> coarseSources;
    // Create the smoothers for all levels
    PtrList<lduMatrix::smoother> smoothers;
    // Scratch fields if processor-agglomerated coarse level meshes
    // are bigger than original. Usually not needed
    scalarField scratch1;
    scalarField scratch2;
    // Initialise the above data structures
    initVcycle
    (
      coarseCorrFields,
      coarseSources,
      smoothers,
      scratch1,
      scratch2
    );
    do {
      Vcycle
      (
        smoothers,
        psi,
        source,
        Apsi,
        finestCorrection,
        finestResidual,
        (scratch1.size() ? scratch1 : Apsi),
        (scratch2.size() ? scratch2 : finestCorrection),
        coarseCorrFields,
        coarseSources,
        cmpt
      );
      // Calculate finest level residual field
      matrix_.Amul(Apsi, psi, interfaceBouCoeffs_, interfaces_, cmpt);
      finestResidual = source;
      finestResidual -= Apsi;
      solverPerf.finalResidual() = gSumMag
      (
        finestResidual,
        matrix().mesh().comm()
      )/normFactor;
      if (debug >= 2) {
        solverPerf.print(Info.masterStream(matrix().mesh().comm()));
      }
    } while ((++solverPerf.nIterations() < maxIter_
              && !solverPerf.checkConvergence(tolerance_, relTol_))
             || solverPerf.nIterations() < minIter_);
  }
  return solverPerf;
}


void mousse::GAMGSolver::Vcycle
(
  const PtrList<lduMatrix::smoother>& smoothers,
  scalarField& psi,
  const scalarField& source,
  scalarField& Apsi,
  scalarField& finestCorrection,
  scalarField& finestResidual,
  scalarField& scratch1,
  scalarField& scratch2,
  PtrList<scalarField>& coarseCorrFields,
  PtrList<scalarField>& coarseSources,
  const direction cmpt
) const
{
  //debug = 2;
  const label coarsestLevel = matrixLevels_.size() - 1;
  // Restrict finest grid residual for the next level up.
  agglomeration_.restrictField(coarseSources[0], finestResidual, 0, true);
  if (debug >= 2 && nPreSweeps_) {
    Pout << "Pre-smoothing scaling factors: ";
  }
  // Residual restriction (going to coarser levels)
  for (label leveli = 0; leveli < coarsestLevel; leveli++) {
    if (coarseSources.set(leveli + 1)) {
      // If the optional pre-smoothing sweeps are selected
      // smooth the coarse-grid field for the restriced source
      if (nPreSweeps_) {
        coarseCorrFields[leveli] = 0.0;
        smoothers[leveli + 1].smooth
        (
          coarseCorrFields[leveli],
          coarseSources[leveli],
          cmpt,
          min
          (
            nPreSweeps_ +  preSweepsLevelMultiplier_*leveli,
            maxPreSweeps_
          )
        );
        scalarField::subField ACf
        {
          scratch1,
          coarseCorrFields[leveli].size()
        };
        // Scale coarse-grid correction field
        // but not on the coarsest level because it evaluates to 1
        if (scaleCorrection_ && leveli < coarsestLevel - 1) {
          scale
          (
            coarseCorrFields[leveli],
            const_cast<scalarField&>
            (
              ACf.operator const scalarField&()
            ),
            matrixLevels_[leveli],
            interfaceLevelsBouCoeffs_[leveli],
            interfaceLevels_[leveli],
            coarseSources[leveli],
            cmpt
          );
        }
        // Correct the residual with the new solution
        matrixLevels_[leveli].Amul
        (
          const_cast<scalarField&>
          (
            ACf.operator const scalarField&()
          ),
          coarseCorrFields[leveli],
          interfaceLevelsBouCoeffs_[leveli],
          interfaceLevels_[leveli],
          cmpt
        );
        coarseSources[leveli] -= ACf;
      }
      // Residual is equal to source
      agglomeration_.restrictField
      (
        coarseSources[leveli + 1],
        coarseSources[leveli],
        leveli + 1,
        true
      );
    }
  }
  if (debug >= 2 && nPreSweeps_) {
    Pout << endl;
  }
  // Solve Coarsest level with either an iterative or direct solver
  if (coarseCorrFields.set(coarsestLevel)) {
    solveCoarsestLevel
    (
      coarseCorrFields[coarsestLevel],
      coarseSources[coarsestLevel]
    );
  }
  if (debug >= 2) {
    Pout << "Post-smoothing scaling factors: ";
  }
  // Smoothing and prolongation of the coarse correction fields
  // (going to finer levels)
  scalarField dummyField{0};
  for (label leveli = coarsestLevel - 1; leveli >= 0; leveli--) {
    if (coarseCorrFields.set(leveli)) {
      // Create a field for the pre-smoothed correction field
      // as a sub-field of the finestCorrection which is not
      // currently being used
      scalarField::subField preSmoothedCoarseCorrField
      {
        scratch2,
        coarseCorrFields[leveli].size()
      };
      // Only store the preSmoothedCoarseCorrField if pre-smoothing is
      // used
      if (nPreSweeps_) {
        preSmoothedCoarseCorrField.assign(coarseCorrFields[leveli]);
      }
      agglomeration_.prolongField
      (
        coarseCorrFields[leveli],
        (
          coarseCorrFields.set(leveli + 1)
          ? coarseCorrFields[leveli + 1]
          : dummyField              // dummy value
        ),
        leveli + 1,
        true
      );
      // Create A.psi for this coarse level as a sub-field of Apsi
      scalarField::subField ACf
      {
        scratch1,
        coarseCorrFields[leveli].size()
      };
      scalarField& ACfRef =
        const_cast<scalarField&>(ACf.operator const scalarField&());
      if (interpolateCorrection_) { //&& leveli < coarsestLevel - 2)
        if (coarseCorrFields.set(leveli+1)) {
          interpolate
          (
            coarseCorrFields[leveli],
            ACfRef,
            matrixLevels_[leveli],
            interfaceLevelsBouCoeffs_[leveli],
            interfaceLevels_[leveli],
            agglomeration_.restrictAddressing(leveli + 1),
            coarseCorrFields[leveli + 1],
            cmpt
          );
        } else {
          interpolate
          (
            coarseCorrFields[leveli],
            ACfRef,
            matrixLevels_[leveli],
            interfaceLevelsBouCoeffs_[leveli],
            interfaceLevels_[leveli],
            cmpt
          );
        }
      }
      // Scale coarse-grid correction field
      // but not on the coarsest level because it evaluates to 1
      if (scaleCorrection_
          && (interpolateCorrection_ || leveli < coarsestLevel - 1)) {
        scale
        (
          coarseCorrFields[leveli],
          ACfRef,
          matrixLevels_[leveli],
          interfaceLevelsBouCoeffs_[leveli],
          interfaceLevels_[leveli],
          coarseSources[leveli],
          cmpt
        );
      }
      // Only add the preSmoothedCoarseCorrField if pre-smoothing is
      // used
      if (nPreSweeps_) {
        coarseCorrFields[leveli] += preSmoothedCoarseCorrField;
      }
      smoothers[leveli + 1].smooth
      (
        coarseCorrFields[leveli],
        coarseSources[leveli],
        cmpt,
        min
        (
          nPostSweeps_ + postSweepsLevelMultiplier_*leveli,
          maxPostSweeps_
        )
      );
    }
  }
  // Prolong the finest level correction
  agglomeration_.prolongField
  (
    finestCorrection,
    coarseCorrFields[0],
    0,
    true
  );
  if (interpolateCorrection_) {
    interpolate
    (
      finestCorrection,
      Apsi,
      matrix_,
      interfaceBouCoeffs_,
      interfaces_,
      agglomeration_.restrictAddressing(0),
      coarseCorrFields[0],
      cmpt
    );
  }
  if (scaleCorrection_) {
    // Scale the finest level correction
    scale
    (
      finestCorrection,
      Apsi,
      matrix_,
      interfaceBouCoeffs_,
      interfaces_,
      finestResidual,
      cmpt
    );
  }
  FOR_ALL(psi, i) {
    psi[i] += finestCorrection[i];
  }
  smoothers[0].smooth
  (
    psi,
    source,
    cmpt,
    nFinestSweeps_
  );
}


void mousse::GAMGSolver::initVcycle
(
  PtrList<scalarField>& coarseCorrFields,
  PtrList<scalarField>& coarseSources,
  PtrList<lduMatrix::smoother>& smoothers,
  scalarField& scratch1,
  scalarField& scratch2
) const
{
  label maxSize = matrix_.diag().size();
  coarseCorrFields.setSize(matrixLevels_.size());
  coarseSources.setSize(matrixLevels_.size());
  smoothers.setSize(matrixLevels_.size() + 1);
  // Create the smoother for the finest level
  smoothers.set
  (
    0,
    lduMatrix::smoother::New
    (
      fieldName_,
      matrix_,
      interfaceBouCoeffs_,
      interfaceIntCoeffs_,
      interfaces_,
      controlDict_
    )
  );
  FOR_ALL(matrixLevels_, leveli) {
    if (agglomeration_.nCells(leveli) >= 0) {
      label nCoarseCells = agglomeration_.nCells(leveli);
      coarseSources.set(leveli, new scalarField{nCoarseCells});
    }
    if (matrixLevels_.set(leveli)) {
      const lduMatrix& mat = matrixLevels_[leveli];
      label nCoarseCells = mat.diag().size();
      maxSize = max(maxSize, nCoarseCells);
      coarseCorrFields.set(leveli, new scalarField{nCoarseCells});
      smoothers.set
      (
        leveli + 1,
        lduMatrix::smoother::New
        (
          fieldName_,
          matrixLevels_[leveli],
          interfaceLevelsBouCoeffs_[leveli],
          interfaceLevelsIntCoeffs_[leveli],
          interfaceLevels_[leveli],
          controlDict_
        )
      );
    }
  }
  if (maxSize > matrix_.diag().size()) {
    // Allocate some scratch storage
    scratch1.setSize(maxSize);
    scratch2.setSize(maxSize);
  }
}


void mousse::GAMGSolver::solveCoarsestLevel
(
  scalarField& coarsestCorrField,
  const scalarField& coarsestSource
) const
{
  const label coarsestLevel = matrixLevels_.size() - 1;
  label coarseComm = matrixLevels_[coarsestLevel].mesh().comm();
  label oldWarn = UPstream::warnComm;
  UPstream::warnComm = coarseComm;
  if (directSolveCoarsest_) {
    coarsestCorrField = coarsestSource;
    coarsestLUMatrixPtr_->solve(coarsestCorrField);
  }
  else
  {
    coarsestCorrField = 0;
    solverPerformance coarseSolverPerf;
    if (matrixLevels_[coarsestLevel].asymmetric()) {
      coarseSolverPerf = BICCG
      (
        "coarsestLevelCorr",
        matrixLevels_[coarsestLevel],
        interfaceLevelsBouCoeffs_[coarsestLevel],
        interfaceLevelsIntCoeffs_[coarsestLevel],
        interfaceLevels_[coarsestLevel],
        tolerance_,
        relTol_
      ).solve
      (
        coarsestCorrField,
        coarsestSource
      );
    } else {
      coarseSolverPerf = ICCG
      (
        "coarsestLevelCorr",
        matrixLevels_[coarsestLevel],
        interfaceLevelsBouCoeffs_[coarsestLevel],
        interfaceLevelsIntCoeffs_[coarsestLevel],
        interfaceLevels_[coarsestLevel],
        tolerance_,
        relTol_
      ).solve
      (
        coarsestCorrField,
        coarsestSource
      );
    }
    if (debug >= 2) {
      coarseSolverPerf.print(Info.masterStream(coarseComm));
    }
  }
  UPstream::warnComm = oldWarn;
}
