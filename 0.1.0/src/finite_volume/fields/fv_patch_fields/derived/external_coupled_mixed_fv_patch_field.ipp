// mousse: CFD toolbox
// Copyright (C) 2013 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "external_coupled_mixed_fv_patch_field.hpp"
#include "fv_patch_field_mapper.hpp"
#include "vol_fields.hpp"
#include "ifstream.hpp"
#include "global_index.hpp"


// Static Data Members
template<class Type>
mousse::word mousse::externalCoupledMixedFvPatchField<Type>::lockName = "mousse";

template<class Type>
mousse::string
mousse::externalCoupledMixedFvPatchField<Type>::patchKey = "# Patch: ";


// Private Member Functions 
template<class Type>
mousse::fileName mousse::externalCoupledMixedFvPatchField<Type>::baseDir() const
{
  word regionName(this->dimensionedInternalField().mesh().name());
  if (regionName == polyMesh::defaultRegion) {
    regionName = ".";
  }
  fileName result{commsDir_/regionName};
  result.clean();
  return result;
}


template<class Type>
void mousse::externalCoupledMixedFvPatchField<Type>::setMaster
(
  const labelList& patchIDs
)
{
  const volFieldType& cvf =
    static_cast<const volFieldType&>(this->dimensionedInternalField());
  volFieldType& vf = const_cast<volFieldType&>(cvf);
  typename volFieldType::GeometricBoundaryField& bf = vf.boundaryField();
  // number of patches can be different in parallel...
  label nPatch = bf.size();
  reduce(nPatch, maxOp<label>());
  offsets_.setSize(nPatch);
  FOR_ALL(offsets_, i) {
    offsets_[i].setSize(Pstream::nProcs());
    offsets_[i] = 0;
  }
  // set the master patch
  FOR_ALL(patchIDs, i) {
    label patchI = patchIDs[i];
    patchType& pf = refCast<patchType>(bf[patchI]);
    offsets_[patchI][Pstream::myProcNo()] = pf.size();
    if (i == 0) {
      pf.master() = true;
    } else {
      pf.master() = false;
    }
  }
  // set the patch offsets
  int tag = Pstream::msgType() + 1;
  FOR_ALL(offsets_, i) {
    Pstream::gatherList(offsets_[i], tag);
    Pstream::scatterList(offsets_[i], tag);
  }
  label patchOffset = 0;
  FOR_ALL(offsets_, patchI) {
    label sumOffset = 0;
    List<label>& procOffsets = offsets_[patchI];
    FOR_ALL(procOffsets, procI) {
      label o = procOffsets[procI];
      if (o > 0) {
        procOffsets[procI] = patchOffset + sumOffset;
        sumOffset += o;
      }
    }
    patchOffset += sumOffset;
  }
}


template<class Type>
void mousse::externalCoupledMixedFvPatchField<Type>::writeGeometry
(
  OFstream& osPoints,
  OFstream& osFaces
) const
{
  int tag = Pstream::msgType() + 1;
  const label procI = Pstream::myProcNo();
  const polyPatch& p = this->patch().patch();
  const polyMesh& mesh = p.boundaryMesh().mesh();
  labelList pointToGlobal;
  labelList uniquePointIDs;
  (void)mesh.globalData().mergePoints
  (
    p.meshPoints(),
    p.meshPointMap(),
    pointToGlobal,
    uniquePointIDs
  );
  List<pointField> allPoints{Pstream::nProcs()};
  allPoints[procI] = pointField{mesh.points(), uniquePointIDs};
  Pstream::gatherList(allPoints, tag);
  List<faceList> allFaces{Pstream::nProcs()};
  faceList& patchFaces = allFaces[procI];
  patchFaces = p.localFaces();
  FOR_ALL(patchFaces, faceI) {
    inplaceRenumber(pointToGlobal, patchFaces[faceI]);
  }
  Pstream::gatherList(allFaces, tag);
  if (Pstream::master()) {
    pointField pts
    {
      ListListOps::combine<pointField>(allPoints, accessOp<pointField>())
    };
    // write points
    osPoints << patchKey.c_str() << this->patch().name() << pts << endl;
    faceList fcs
    {
      ListListOps::combine<faceList>(allFaces, accessOp<faceList>())
    };
    // write faces
    osFaces<< patchKey.c_str() << this->patch().name() << fcs << endl;
  }
}


template<class Type>
mousse::fileName mousse::externalCoupledMixedFvPatchField<Type>::lockFile() const
{
  return fileName{baseDir()/(lockName + ".lock")};
}


template<class Type>
void mousse::externalCoupledMixedFvPatchField<Type>::createLockFile() const
{
  if (!master_ || !Pstream::master()) {
    return;
  }
  const fileName fName{lockFile()};
  IFstream is{fName};
  // only create lock file if it doesn't already exist
  if (!is.good()) {
    if (log_) {
      Info << type() << ": creating lock file" << endl;
    }
    OFstream os{fName};
    os << "lock file";
    os.flush();
  }
}


template<class Type>
void mousse::externalCoupledMixedFvPatchField<Type>::removeLockFile() const
{
  if (!master_ || !Pstream::master()) {
    return;
  }
  if (log_) {
    Info << type() << ": removing lock file" << endl;
  }
  rm(lockFile());
}


template<class Type>
void mousse::externalCoupledMixedFvPatchField<Type>::startWait() const
{
  // only wait on master patch
  const volFieldType& cvf =
    static_cast<const volFieldType&>(this->dimensionedInternalField());
  const typename volFieldType::GeometricBoundaryField& bf =
    cvf.boundaryField();
  FOR_ALL(coupledPatchIDs_, i) {
    label patchI = coupledPatchIDs_[i];
    const patchType& pf = refCast<const patchType>(bf[patchI]);
    if (pf.master()) {
      pf.wait();
      break;
    }
  }
}


template<class Type>
void mousse::externalCoupledMixedFvPatchField<Type>::wait() const
{
  const fileName fName{lockFile()};
  label found = 0;
  label totalTime = 0;
  if (log_) {
    Info << type() << ": beginning wait for lock file " << fName << endl;
  }
  while (found == 0) {
    if (Pstream::master()) {
      if (totalTime > timeOut_) {
        FATAL_ERROR_IN
        (
          "void "
          "mousse::externalCoupledMixedFvPatchField<Type>::wait() "
          "const"
        )
        << "Wait time exceeded time out time of " << timeOut_
        << " s" << abort(FatalError);
      }
      IFstream is{fName};
      if (is.good()) {
        found++;
        if (log_) {
          Info << type() << ": found lock file " << fName << endl;
        }
      } else {
        sleep(waitInterval_);
        totalTime += waitInterval_;
        if (log_) {
          Info << type() << ": wait time = " << totalTime << endl;
        }
      }
    }
    // prevent other procs from racing ahead
    reduce(found, sumOp<label>());
  }
}


template<class Type>
void mousse::externalCoupledMixedFvPatchField<Type>::initialiseRead
(
  IFstream& is
) const
{
  if (!is.good()) {
    FATAL_ERROR_IN
    (
      "void mousse::externalCoupledMixedFvPatchField<Type>::"
      "initialiseRead"
      "("
        "IFstream&"
      ") const"
    )
    << "Unable to open data transfer file " << is.name()
    << " for patch " << this->patch().name()
    << exit(FatalError);
  }
  label offset = offsets_[this->patch().index()][Pstream::myProcNo()];
  // scan forward to start reading at relevant line/position
  string line;
  for (label i = 0; i < offset; i++) {
    if (is.good()) {
      is.getLine(line);
    } else {
      FATAL_ERROR_IN
      (
        "void mousse::externalCoupledMixedFvPatchField<Type>::"
        "initialiseRead"
        "("
          "IFstream&"
        ") const"
      )
      << "Unable to scan forward to appropriate read position for "
      << "data transfer file " << is.name()
      << " for patch " << this->patch().name()
      << exit(FatalError);
    }
  }
}


// Protected Member Functions 
template<class Type>
void mousse::externalCoupledMixedFvPatchField<Type>::readData
(
  const fileName& transferFile
)
{
  // read data passed back from external source
  IFstream is{transferFile + ".in"};
  // pre-process the input transfer file
  initialiseRead(is);
  // read data from file
  FOR_ALL(this->patch(), faceI) {
    if (is.good()) {
      is >> this->refValue()[faceI] >> this->refGrad()[faceI]
        >> this->valueFraction()[faceI];
    } else {
      FATAL_ERROR_IN
      (
        "void mousse::externalCoupledMixedFvPatchField<Type>::readData"
        "("
          "const fileName&"
        ")"
      )
      << "Insufficient data for patch "
      << this->patch().name()
      << " in file " << is.name()
      << exit(FatalError);
    }
  }
  initialised_ = true;
  // update the value from the mixed condition
  mixedFvPatchField<Type>::evaluate();
}


template<class Type>
void mousse::externalCoupledMixedFvPatchField<Type>::writeData
(
  const fileName& transferFile
) const
{
  if (!master_) {
    return;
  }
  OFstream os{transferFile};
  writeHeader(os);
  const volFieldType& cvf =
    static_cast<const volFieldType&>(this->dimensionedInternalField());
  const typename volFieldType::GeometricBoundaryField& bf =
    cvf.boundaryField();
  FOR_ALL(coupledPatchIDs_, i) {
    label patchI = coupledPatchIDs_[i];
    const patchType& pf = refCast<const patchType>(bf[patchI]);
    pf.transferData(os);
  }
}


template<class Type>
void mousse::externalCoupledMixedFvPatchField<Type>::writeHeader
(
  OFstream& os
) const
{
  os << "# Values: magSf value snGrad" << endl;
}


// Constructors 
template<class Type>
mousse::externalCoupledMixedFvPatchField<Type>::
externalCoupledMixedFvPatchField
(
  const fvPatch& p,
  const DimensionedField<Type, volMesh>& iF
)
:
  mixedFvPatchField<Type>{p, iF},
  commsDir_{"unknown-commsDir"},
  fName_{"unknown-fName"},
  waitInterval_{0},
  timeOut_{0},
  calcFrequency_{0},
  initByExternal_{false},
  log_{false},
  master_{false},
  offsets_{},
  initialised_{false},
  coupledPatchIDs_{}
{
  this->refValue() = pTraits<Type>::zero;
  this->refGrad() = pTraits<Type>::zero;
  this->valueFraction() = 0.0;
}


template<class Type>
mousse::externalCoupledMixedFvPatchField<Type>::
externalCoupledMixedFvPatchField
(
  const externalCoupledMixedFvPatchField& ptf,
  const fvPatch& p,
  const DimensionedField<Type, volMesh>& iF,
  const fvPatchFieldMapper& mapper
)
:
  mixedFvPatchField<Type>{ptf, p, iF, mapper},
  commsDir_{ptf.commsDir_},
  fName_{ptf.fName_},
  waitInterval_{ptf.waitInterval_},
  timeOut_{ptf.timeOut_},
  calcFrequency_{ptf.calcFrequency_},
  initByExternal_{ptf.initByExternal_},
  log_{ptf.log_},
  master_{ptf.master_},
  offsets_{ptf.offsets_},
  initialised_{ptf.initialised_},
  coupledPatchIDs_{ptf.coupledPatchIDs_}
{}


template<class Type>
mousse::externalCoupledMixedFvPatchField<Type>::
externalCoupledMixedFvPatchField
(
  const fvPatch& p,
  const DimensionedField<Type, volMesh>& iF,
  const dictionary& dict
)
:
  mixedFvPatchField<Type>{p, iF},
  commsDir_{dict.lookup("commsDir")},
  fName_{dict.lookup("fileName")},
  waitInterval_{dict.lookupOrDefault("waitInterval", 1)},
  timeOut_{dict.lookupOrDefault("timeOut", 100*waitInterval_)},
  calcFrequency_{dict.lookupOrDefault("calcFrequency", 1)},
  initByExternal_{readBool(dict.lookup("initByExternal"))},
  log_{dict.lookupOrDefault("log", false)},
  master_{true},
  offsets_{},
  initialised_{false},
  coupledPatchIDs_{}
{
  if (dict.found("value")) {
    fvPatchField<Type>::operator=
    (
      Field<Type>("value", dict, p.size())
    );
  } else {
    fvPatchField<Type>::operator=(this->patchInternalField());
  }
  commsDir_.expand();
  if (Pstream::master()) {
    mkDir(baseDir());
  }
  if (!initByExternal_) {
    createLockFile();
  }
  // initialise as a fixed value
  this->refValue() = *this;
  this->refGrad() = pTraits<Type>::zero;
  this->valueFraction() = 1.0;
}


template<class Type>
mousse::externalCoupledMixedFvPatchField<Type>::
externalCoupledMixedFvPatchField
(
  const externalCoupledMixedFvPatchField& ecmpf
)
:
  mixedFvPatchField<Type>{ecmpf},
  commsDir_{ecmpf.commsDir_},
  fName_{ecmpf.fName_},
  waitInterval_{ecmpf.waitInterval_},
  timeOut_{ecmpf.timeOut_},
  calcFrequency_{ecmpf.calcFrequency_},
  initByExternal_{ecmpf.initByExternal_},
  log_{ecmpf.log_},
  master_{ecmpf.master_},
  offsets_{ecmpf.offsets_},
  initialised_{ecmpf.initialised_},
  coupledPatchIDs_{ecmpf.coupledPatchIDs_}
{}


template<class Type>
mousse::externalCoupledMixedFvPatchField<Type>::
externalCoupledMixedFvPatchField
(
  const externalCoupledMixedFvPatchField& ecmpf,
  const DimensionedField<Type, volMesh>& iF
)
:
  mixedFvPatchField<Type>{ecmpf, iF},
  commsDir_{ecmpf.commsDir_},
  fName_{ecmpf.fName_},
  waitInterval_{ecmpf.waitInterval_},
  timeOut_{ecmpf.timeOut_},
  calcFrequency_{ecmpf.calcFrequency_},
  initByExternal_{ecmpf.initByExternal_},
  log_{ecmpf.log_},
  master_{ecmpf.master_},
  offsets_{ecmpf.offsets_},
  initialised_{ecmpf.initialised_},
  coupledPatchIDs_{ecmpf.coupledPatchIDs_}
{}


// Destructor 
template<class Type>
mousse::externalCoupledMixedFvPatchField<Type>::
~externalCoupledMixedFvPatchField()
{
  removeLockFile();
}


// Member Functions 
template<class Type>
void mousse::externalCoupledMixedFvPatchField<Type>::initialise
(
  const fileName& transferFile
)
{
  if (initialised_) {
    return;
  }
  const volFieldType& cvf =
    static_cast<const volFieldType&>(this->dimensionedInternalField());
  volFieldType& vf = const_cast<volFieldType&>(cvf);
  typename volFieldType::GeometricBoundaryField& bf = vf.boundaryField();
  // identify all coupled patches
  DynamicList<label> coupledPatchIDs{bf.size()};
  FOR_ALL(bf, patchI) {
    if (isA<patchType>(bf[patchI])) {
      coupledPatchIDs.append(patchI);
    }
  }
  coupledPatchIDs_.transfer(coupledPatchIDs);
  // initialise by external solver, or just set the master patch
  if (initByExternal_) {
    FOR_ALL(coupledPatchIDs_, i) {
      label patchI = coupledPatchIDs_[i];
      patchType& pf = refCast<patchType>(bf[patchI]);
      pf.setMaster(coupledPatchIDs_);
    }
    // wait for initial data to be made available
    startWait();
    // read the initial data
    if (master_) {
      FOR_ALL(coupledPatchIDs_, i) {
        label patchI = coupledPatchIDs_[i];
        patchType& pf = refCast<patchType>(bf[patchI]);
        pf.readData(transferFile);
      }
    }
  } else {
    setMaster(coupledPatchIDs_);
  }
  initialised_ = true;
}


template<class Type>
void mousse::externalCoupledMixedFvPatchField<Type>::evaluate
(
  const Pstream::commsTypes
)
{
  if (!initialised_ || this->db().time().timeIndex() % calcFrequency_ == 0) {
    const fileName transferFile(baseDir()/fName_);
    // initialise the coupling
    initialise(transferFile);
    // write data for external source
    writeData(transferFile + ".out");
    // remove lock file, signalling external source to execute
    removeLockFile();
    // wait for response
    startWait();
    if (master_ && Pstream::master()) {
      // remove old data file from OpenFOAM
      rm(transferFile + ".out");
    }
    // read data passed back from external source
    readData(transferFile);
    // create lock file for external source
    createLockFile();
  }
}


template<class Type>
void mousse::externalCoupledMixedFvPatchField<Type>::transferData
(
  OFstream& os
) const
{
  if (log_) {
    Info << type() << ": writing data to " << os.name()  << endl;
  }
  if (Pstream::parRun()) {
    int tag = Pstream::msgType() + 1;
    List<Field<scalar>> magSfs{Pstream::nProcs()};
    magSfs[Pstream::myProcNo()].setSize(this->patch().size());
    magSfs[Pstream::myProcNo()] = this->patch().magSf();
    Pstream::gatherList(magSfs, tag);
    List<Field<Type>> values{Pstream::nProcs()};
    values[Pstream::myProcNo()].setSize(this->patch().size());
    values[Pstream::myProcNo()] = this->refValue();
    Pstream::gatherList(values, tag);
    List<Field<Type>> snGrads{Pstream::nProcs()};
    snGrads[Pstream::myProcNo()].setSize(this->patch().size());
    snGrads[Pstream::myProcNo()] = this->snGrad();
    Pstream::gatherList(snGrads, tag);
    if (Pstream::master()) {
      FOR_ALL(values, procI) {
        const Field<scalar>& magSf = magSfs[procI];
        const Field<Type>& value = values[procI];
        const Field<Type>& snGrad = snGrads[procI];
        FOR_ALL(magSf, faceI) {
          os << magSf[faceI] << token::SPACE
            << value[faceI] << token::SPACE
            << snGrad[faceI] << nl;
        }
      }
      os.flush();
    }
  } else {
    const Field<scalar>& magSf{this->patch().magSf()};
    const Field<Type>& value{this->refValue()};
    const Field<Type> snGrad{this->snGrad()};
    FOR_ALL(magSf, faceI) {
      os << magSf[faceI] << token::SPACE
        << value[faceI] << token::SPACE
        << snGrad[faceI] << nl;
    }
    os.flush();
  }
}


template<class Type>
void mousse::externalCoupledMixedFvPatchField<Type>::writeGeometry() const
{
  const volFieldType& cvf =
    static_cast<const volFieldType&>(this->dimensionedInternalField());
  const typename volFieldType::GeometricBoundaryField& bf =
    cvf.boundaryField();
  OFstream osPoints{baseDir()/"patchPoints"};
  OFstream osFaces{baseDir()/"patchFaces"};
  if (log_) {
    Info << "writing collated patch points to: " << osPoints.name() << nl
      << "writing collated patch faces to: " << osFaces.name() << endl;
  }
  FOR_ALL(bf, patchI) {
    if (isA<patchType>(bf[patchI])) {
      const patchType& pf = refCast<const patchType>(bf[patchI]);
      pf.writeGeometry(osPoints, osFaces);
    }
  }
}


template<class Type>
void mousse::externalCoupledMixedFvPatchField<Type>::write(Ostream& os) const
{
  mixedFvPatchField<Type>::write(os);
  os.writeKeyword("commsDir") << commsDir_ << token::END_STATEMENT << nl;
  os.writeKeyword("fileName") << fName_ << token::END_STATEMENT << nl;
  os.writeKeyword("waitInterval") << waitInterval_ << token::END_STATEMENT
    << nl;
  os.writeKeyword("timeOut") << timeOut_ << token::END_STATEMENT << nl;
  os.writeKeyword("calcFrequency") << calcFrequency_ << token::END_STATEMENT
    << nl;
  os.writeKeyword("initByExternal") << initByExternal_ << token::END_STATEMENT
    << nl;
  os.writeKeyword("log") << log_ << token::END_STATEMENT << nl;
  this->writeEntry("value", os);
}

