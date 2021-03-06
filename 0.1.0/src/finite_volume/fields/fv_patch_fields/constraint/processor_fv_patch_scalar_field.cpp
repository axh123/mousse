// mousse: CFD toolbox
// Copyright (C) 2011-2013 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "processor_fv_patch_scalar_field.hpp"
#include "uipstream.hpp"
#include "uopstream.hpp"


namespace mousse {

// Member Functions 
template<>
void processorFvPatchField<scalar>::initInterfaceMatrixUpdate
(
  scalarField&,
  const scalarField& psiInternal,
  const scalarField&,
  const direction,
  const Pstream::commsTypes commsType
) const
{
  this->patch().patchInternalField(psiInternal, scalarSendBuf_);
  if (commsType == Pstream::nonBlocking && !Pstream::floatTransfer) {
    // Fast path.
    if (debug && !this->ready()) {
      FATAL_ERROR_IN
      (
        "processorFvPatchField<scalar>::initInterfaceMatrixUpdate(..)"
      )
      << "On patch " << procPatch_.name()
      << " outstanding request."
      << abort(FatalError);
    }
    scalarReceiveBuf_.setSize(scalarSendBuf_.size());
    outstandingRecvRequest_ = UPstream::nRequests();
    UIPstream::read
    (
      Pstream::nonBlocking,
      procPatch_.neighbProcNo(),
      reinterpret_cast<char*>(scalarReceiveBuf_.begin()),
      scalarReceiveBuf_.byteSize(),
      procPatch_.tag(),
      procPatch_.comm()
    );
    outstandingSendRequest_ = UPstream::nRequests();
    UOPstream::write
    (
      Pstream::nonBlocking,
      procPatch_.neighbProcNo(),
      reinterpret_cast<const char*>(scalarSendBuf_.begin()),
      scalarSendBuf_.byteSize(),
      procPatch_.tag(),
      procPatch_.comm()
    );
  } else {
    procPatch_.compressedSend(commsType, scalarSendBuf_);
  }
  const_cast<processorFvPatchField<scalar>&>(*this).updatedMatrix() = false;
}


template<>
void processorFvPatchField<scalar>::updateInterfaceMatrix
(
  scalarField& result,
  const scalarField&,
  const scalarField& coeffs,
  const direction,
  const Pstream::commsTypes commsType
) const
{
  if (this->updatedMatrix()) {
    return;
  }
  const labelUList& faceCells = this->patch().faceCells();
  if (commsType == Pstream::nonBlocking && !Pstream::floatTransfer) {
    // Fast path.
    if (outstandingRecvRequest_ >= 0
        && outstandingRecvRequest_ < Pstream::nRequests()) {
      UPstream::waitRequest(outstandingRecvRequest_);
    }
    // Recv finished so assume sending finished as well.
    outstandingSendRequest_ = -1;
    outstandingRecvRequest_ = -1;
    // Consume straight from scalarReceiveBuf_
    FOR_ALL(faceCells, elemI) {
      result[faceCells[elemI]] -= coeffs[elemI]*scalarReceiveBuf_[elemI];
    }
  } else {
    scalarField pnf
    {
      procPatch_.compressedReceive<scalar>(commsType, this->size())()
    };
    FOR_ALL(faceCells, elemI) {
      result[faceCells[elemI]] -= coeffs[elemI]*pnf[elemI];
    }
  }
  const_cast<processorFvPatchField<scalar>&>(*this).updatedMatrix() = true;
}

}  // namespace mousse

