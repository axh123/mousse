// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "_luscalar_matrix.hpp"
#include "ldu_matrix.hpp"
#include "proc_ldu_matrix.hpp"
#include "proc_ldu_interface.hpp"
#include "cyclic_ldu_interface.hpp"
// Static Data Members
namespace mousse
{
  DEFINE_TYPE_NAME_AND_DEBUG(LUscalarMatrix, 0);
}
// Constructors 
mousse::LUscalarMatrix::LUscalarMatrix(const scalarSquareMatrix& matrix)
:
  scalarSquareMatrix(matrix),
  comm_(Pstream::worldComm),
  pivotIndices_(n())
{
  LUDecompose(*this, pivotIndices_);
}
mousse::LUscalarMatrix::LUscalarMatrix
(
  const lduMatrix& ldum,
  const FieldField<Field, scalar>& interfaceCoeffs,
  const lduInterfaceFieldPtrsList& interfaces
)
:
  comm_(ldum.mesh().comm())
{
  if (Pstream::parRun())
  {
    PtrList<procLduMatrix> lduMatrices(Pstream::nProcs(comm_));
    label lduMatrixi = 0;
    lduMatrices.set
    (
      lduMatrixi++,
      new procLduMatrix
      (
        ldum,
        interfaceCoeffs,
        interfaces
      )
    );
    if (Pstream::master(comm_))
    {
      for
      (
        int slave=Pstream::firstSlave();
        slave<=Pstream::lastSlave(comm_);
        slave++
      )
      {
        lduMatrices.set
        (
          lduMatrixi++,
          new procLduMatrix
          (
            IPstream
            (
              Pstream::scheduled,
              slave,
              0,          // bufSize
              Pstream::msgType(),
              comm_
            )()
          )
        );
      }
    }
    else
    {
      OPstream toMaster
      (
        Pstream::scheduled,
        Pstream::masterNo(),
        0,              // bufSize
        Pstream::msgType(),
        comm_
      );
      procLduMatrix cldum
      (
        ldum,
        interfaceCoeffs,
        interfaces
      );
      toMaster<< cldum;
    }
    if (Pstream::master(comm_))
    {
      label nCells = 0;
      FOR_ALL(lduMatrices, i)
      {
        nCells += lduMatrices[i].size();
      }
      scalarSquareMatrix m(nCells, nCells, 0.0);
      transfer(m);
      convert(lduMatrices);
    }
  }
  else
  {
    label nCells = ldum.lduAddr().size();
    scalarSquareMatrix m(nCells, nCells, 0.0);
    transfer(m);
    convert(ldum, interfaceCoeffs, interfaces);
  }
  if (Pstream::master(comm_))
  {
    label nRows = n();
    label nColumns = m();
    if (debug)
    {
      Pout<< "LUscalarMatrix : size:" << nRows << endl;
      for (label rowI = 0; rowI < nRows; rowI++)
      {
        const scalar* row = operator[](rowI);
        Pout<< "cell:" << rowI << " diagCoeff:" << row[rowI] << endl;
        Pout<< "    connects to upper cells :";
        for (label columnI = rowI+1; columnI < nColumns; columnI++)
        {
          if (mag(row[columnI]) > SMALL)
          {
            Pout<< ' ' << columnI << " (coeff:" << row[columnI]
              << ")";
          }
        }
        Pout<< endl;
        Pout<< "    connects to lower cells :";
        for (label columnI = 0; columnI < rowI; columnI++)
        {
          if (mag(row[columnI]) > SMALL)
          {
            Pout<< ' ' << columnI << " (coeff:" << row[columnI]
              << ")";
          }
        }
        Pout<< endl;
      }
      Pout<< endl;
    }
    pivotIndices_.setSize(n());
    LUDecompose(*this, pivotIndices_);
  }
}
// Member Functions 
void mousse::LUscalarMatrix::convert
(
  const lduMatrix& ldum,
  const FieldField<Field, scalar>& interfaceCoeffs,
  const lduInterfaceFieldPtrsList& interfaces
)
{
  const label* __restrict__ uPtr = ldum.lduAddr().upperAddr().begin();
  const label* __restrict__ lPtr = ldum.lduAddr().lowerAddr().begin();
  const scalar* __restrict__ diagPtr = ldum.diag().begin();
  const scalar* __restrict__ upperPtr = ldum.upper().begin();
  const scalar* __restrict__ lowerPtr = ldum.lower().begin();
  const label nCells = ldum.diag().size();
  const label nFaces = ldum.upper().size();
  for (label cell=0; cell<nCells; cell++)
  {
    operator[](cell)[cell] = diagPtr[cell];
  }
  for (label face=0; face<nFaces; face++)
  {
    label uCell = uPtr[face];
    label lCell = lPtr[face];
    operator[](uCell)[lCell] = lowerPtr[face];
    operator[](lCell)[uCell] = upperPtr[face];
  }
  FOR_ALL(interfaces, inti)
  {
    if (interfaces.set(inti))
    {
      const lduInterface& interface = interfaces[inti].interface();
      // Assume any interfaces are cyclic ones
      const label* __restrict__ lPtr = interface.faceCells().begin();
      const cyclicLduInterface& cycInterface =
        refCast<const cyclicLduInterface>(interface);
      label nbrInt = cycInterface.neighbPatchID();
      const label* __restrict__ uPtr =
        interfaces[nbrInt].interface().faceCells().begin();
      const scalar* __restrict__ nbrUpperLowerPtr =
        interfaceCoeffs[nbrInt].begin();
      label inFaces = interface.faceCells().size();
      for (label face=0; face<inFaces; face++)
      {
        label uCell = lPtr[face];
        label lCell = uPtr[face];
        operator[](uCell)[lCell] -= nbrUpperLowerPtr[face];
      }
    }
  }
  //printDiagonalDominance();
}
void mousse::LUscalarMatrix::convert
(
  const PtrList<procLduMatrix>& lduMatrices
)
{
  procOffsets_.setSize(lduMatrices.size() + 1);
  procOffsets_[0] = 0;
  FOR_ALL(lduMatrices, ldumi)
  {
    procOffsets_[ldumi+1] = procOffsets_[ldumi] + lduMatrices[ldumi].size();
  }
  FOR_ALL(lduMatrices, ldumi)
  {
    const procLduMatrix& lduMatrixi = lduMatrices[ldumi];
    label offset = procOffsets_[ldumi];
    const label* __restrict__ uPtr = lduMatrixi.upperAddr_.begin();
    const label* __restrict__ lPtr = lduMatrixi.lowerAddr_.begin();
    const scalar* __restrict__ diagPtr = lduMatrixi.diag_.begin();
    const scalar* __restrict__ upperPtr = lduMatrixi.upper_.begin();
    const scalar* __restrict__ lowerPtr = lduMatrixi.lower_.begin();
    const label nCells = lduMatrixi.size();
    const label nFaces = lduMatrixi.upper_.size();
    for (label cell=0; cell<nCells; cell++)
    {
      label globalCell = cell + offset;
      operator[](globalCell)[globalCell] = diagPtr[cell];
    }
    for (label face=0; face<nFaces; face++)
    {
      label uCell = uPtr[face] + offset;
      label lCell = lPtr[face] + offset;
      operator[](uCell)[lCell] = lowerPtr[face];
      operator[](lCell)[uCell] = upperPtr[face];
    }
    const PtrList<procLduInterface>& interfaces =
      lduMatrixi.interfaces_;
    FOR_ALL(interfaces, inti)
    {
      const procLduInterface& interface = interfaces[inti];
      if (interface.myProcNo_ == interface.neighbProcNo_)
      {
        const label* __restrict__ ulPtr = interface.faceCells_.begin();
        const scalar* __restrict__ upperLowerPtr =
          interface.coeffs_.begin();
        label inFaces = interface.faceCells_.size()/2;
        for (label face=0; face<inFaces; face++)
        {
          label uCell = ulPtr[face] + offset;
          label lCell = ulPtr[face + inFaces] + offset;
          operator[](uCell)[lCell] -= upperLowerPtr[face + inFaces];
          operator[](lCell)[uCell] -= upperLowerPtr[face];
        }
      }
      else if (interface.myProcNo_ < interface.neighbProcNo_)
      {
        // Interface to neighbour proc. Find on neighbour proc the
        // corresponding interface. The problem is that there can
        // be multiple interfaces between two processors (from
        // processorCyclics) so also compare the communication tag
        const PtrList<procLduInterface>& neiInterfaces =
          lduMatrices[interface.neighbProcNo_].interfaces_;
        label neiInterfacei = -1;
        FOR_ALL(neiInterfaces, ninti)
        {
          if
          (
            (neiInterfaces[ninti].neighbProcNo_ == interface.myProcNo_)
            && (neiInterfaces[ninti].tag_ ==  interface.tag_)
          )
          {
            neiInterfacei = ninti;
            break;
          }
        }
        if (neiInterfacei == -1)
        {
          FATAL_ERROR_IN("LUscalarMatrix::convert") << exit(FatalError);
        }
        const procLduInterface& neiInterface =
          neiInterfaces[neiInterfacei];
        const label* __restrict__ uPtr = interface.faceCells_.begin();
        const label* __restrict__ lPtr =
          neiInterface.faceCells_.begin();
        const scalar* __restrict__ upperPtr = interface.coeffs_.begin();
        const scalar* __restrict__ lowerPtr =
          neiInterface.coeffs_.begin();
        label inFaces = interface.faceCells_.size();
        label neiOffset = procOffsets_[interface.neighbProcNo_];
        for (label face=0; face<inFaces; face++)
        {
          label uCell = uPtr[face] + offset;
          label lCell = lPtr[face] + neiOffset;
          operator[](uCell)[lCell] -= lowerPtr[face];
          operator[](lCell)[uCell] -= upperPtr[face];
        }
      }
    }
  }
  //printDiagonalDominance();
}
void mousse::LUscalarMatrix::printDiagonalDominance() const
{
  for (label i=0; i<n(); i++)
  {
    scalar sum = 0.0;
    for (label j=0; j<n(); j++)
    {
      if (i != j)
      {
        sum += operator[](i)[j];
      }
    }
    Info<< mag(sum)/mag(operator[](i)[i]) << endl;
  }
}