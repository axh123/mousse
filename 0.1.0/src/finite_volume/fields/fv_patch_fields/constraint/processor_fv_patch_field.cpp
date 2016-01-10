// mousse: CFD toolbox
// Copyright (C) 2011-2013 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "processor_fv_patch_field.hpp"
#include "processor_fv_patch.hpp"
#include "demand_driven_data.hpp"
#include "transform_field.hpp"

// Constructors
template<class Type>
mousse::processorFvPatchField<Type>::processorFvPatchField
(
  const fvPatch& p,
  const DimensionedField<Type, volMesh>& iF
)
:
  coupledFvPatchField<Type>{p, iF},
  procPatch_{refCast<const processorFvPatch>(p)},
  sendBuf_{0},
  receiveBuf_{0},
  outstandingSendRequest_{-1},
  outstandingRecvRequest_{-1},
  scalarSendBuf_{0},
  scalarReceiveBuf_{0}
{}


template<class Type>
mousse::processorFvPatchField<Type>::processorFvPatchField
(
  const fvPatch& p,
  const DimensionedField<Type, volMesh>& iF,
  const Field<Type>& f
)
:
  coupledFvPatchField<Type>{p, iF, f},
  procPatch_{refCast<const processorFvPatch>(p)},
  sendBuf_{0},
  receiveBuf_{0},
  outstandingSendRequest_{-1},
  outstandingRecvRequest_{-1},
  scalarSendBuf_{0},
  scalarReceiveBuf_{0}
{}


// Construct by mapping given processorFvPatchField<Type>
template<class Type>
mousse::processorFvPatchField<Type>::processorFvPatchField
(
  const processorFvPatchField<Type>& ptf,
  const fvPatch& p,
  const DimensionedField<Type, volMesh>& iF,
  const fvPatchFieldMapper& mapper
)
:
  coupledFvPatchField<Type>{ptf, p, iF, mapper},
  procPatch_{refCast<const processorFvPatch>(p)},
  sendBuf_{0},
  receiveBuf_{0},
  outstandingSendRequest_{-1},
  outstandingRecvRequest_{-1},
  scalarSendBuf_{0},
  scalarReceiveBuf_{0}
{
  if (!isA<processorFvPatch>(this->patch()))
  {
    FATAL_ERROR_IN
    (
      "processorFvPatchField<Type>::processorFvPatchField\n"
      "(\n"
      "    const processorFvPatchField<Type>& ptf,\n"
      "    const fvPatch& p,\n"
      "    const DimensionedField<Type, volMesh>& iF,\n"
      "    const fvPatchFieldMapper& mapper\n"
      ")\n"
    )
    << "\n    patch type '" << p.type()
    << "' not constraint type '" << typeName << "'"
    << "\n    for patch " << p.name()
    << " of field " << this->dimensionedInternalField().name()
    << " in file " << this->dimensionedInternalField().objectPath()
    << exit(FatalIOError);
  }
  if (debug && !ptf.ready())
  {
    FATAL_ERROR_IN("processorFvPatchField<Type>::processorFvPatchField(..)")
      << "On patch " << procPatch_.name() << " outstanding request."
      << abort(FatalError);
  }
}


template<class Type>
mousse::processorFvPatchField<Type>::processorFvPatchField
(
  const fvPatch& p,
  const DimensionedField<Type, volMesh>& iF,
  const dictionary& dict
)
:
  coupledFvPatchField<Type>{p, iF, dict},
  procPatch_{refCast<const processorFvPatch>(p)},
  sendBuf_{0},
  receiveBuf_{0},
  outstandingSendRequest_{-1},
  outstandingRecvRequest_{-1},
  scalarSendBuf_{0},
  scalarReceiveBuf_{0}
{
  if (!isA<processorFvPatch>(p))
  {
    FATAL_IO_ERROR_IN
    (
      "processorFvPatchField<Type>::processorFvPatchField\n"
      "(\n"
      "    const fvPatch& p,\n"
      "    const Field<Type>& field,\n"
      "    const dictionary& dict\n"
      ")\n",
      dict
    )
    << "\n    patch type '" << p.type()
    << "' not constraint type '" << typeName << "'"
    << "\n    for patch " << p.name()
    << " of field " << this->dimensionedInternalField().name()
    << " in file " << this->dimensionedInternalField().objectPath()
    << exit(FatalIOError);
  }
}


template<class Type>
mousse::processorFvPatchField<Type>::processorFvPatchField
(
  const processorFvPatchField<Type>& ptf
)
:
  processorLduInterfaceField{},
  coupledFvPatchField<Type>{ptf},
  procPatch_{refCast<const processorFvPatch>(ptf.patch())},
  sendBuf_{ptf.sendBuf_.xfer()},
  receiveBuf_{ptf.receiveBuf_.xfer()},
  outstandingSendRequest_{-1},
  outstandingRecvRequest_{-1},
  scalarSendBuf_{ptf.scalarSendBuf_.xfer()},
  scalarReceiveBuf_{ptf.scalarReceiveBuf_.xfer()}
{
  if (debug && !ptf.ready())
  {
    FATAL_ERROR_IN("processorFvPatchField<Type>::processorFvPatchField(..)")
      << "On patch " << procPatch_.name() << " outstanding request."
      << abort(FatalError);
  }
}


template<class Type>
mousse::processorFvPatchField<Type>::processorFvPatchField
(
  const processorFvPatchField<Type>& ptf,
  const DimensionedField<Type, volMesh>& iF
)
:
  coupledFvPatchField<Type>{ptf, iF},
  procPatch_{refCast<const processorFvPatch>(ptf.patch())},
  sendBuf_{0},
  receiveBuf_{0},
  outstandingSendRequest_{-1},
  outstandingRecvRequest_{-1},
  scalarSendBuf_{0},
  scalarReceiveBuf_{0}
{
  if (debug && !ptf.ready())
  {
    FATAL_ERROR_IN("processorFvPatchField<Type>::processorFvPatchField(..)")
      << "On patch " << procPatch_.name() << " outstanding request."
      << abort(FatalError);
  }
}


// Destructor 
template<class Type>
mousse::processorFvPatchField<Type>::~processorFvPatchField()
{}


// Member Functions 
template<class Type>
mousse::tmp<mousse::Field<Type> >
mousse::processorFvPatchField<Type>::patchNeighbourField() const
{
  if (debug && !this->ready())
  {
    FATAL_ERROR_IN("processorFvPatchField<Type>::patchNeighbourField()")
      << "On patch " << procPatch_.name()
      << " outstanding request."
      << abort(FatalError);
  }
  return *this;
}


template<class Type>
void mousse::processorFvPatchField<Type>::initEvaluate
(
  const Pstream::commsTypes commsType
)
{
  if (Pstream::parRun())
  {
    this->patchInternalField(sendBuf_);
    if (commsType == Pstream::nonBlocking && !Pstream::floatTransfer)
    {
      // Fast path. Receive into *this
      this->setSize(sendBuf_.size());
      outstandingRecvRequest_ = UPstream::nRequests();
      UIPstream::read
      (
        Pstream::nonBlocking,
        procPatch_.neighbProcNo(),
        reinterpret_cast<char*>(this->begin()),
        this->byteSize(),
        procPatch_.tag(),
        procPatch_.comm()
      );
      outstandingSendRequest_ = UPstream::nRequests();
      UOPstream::write
      (
        Pstream::nonBlocking,
        procPatch_.neighbProcNo(),
        reinterpret_cast<const char*>(sendBuf_.begin()),
        this->byteSize(),
        procPatch_.tag(),
        procPatch_.comm()
      );
    }
    else
    {
      procPatch_.compressedSend(commsType, sendBuf_);
    }
  }
}


template<class Type>
void mousse::processorFvPatchField<Type>::evaluate
(
  const Pstream::commsTypes commsType
)
{
  if (Pstream::parRun())
  {
    if (commsType == Pstream::nonBlocking && !Pstream::floatTransfer)
    {
      // Fast path. Received into *this
      if
      (
        outstandingRecvRequest_ >= 0
      && outstandingRecvRequest_ < Pstream::nRequests()
      )
      {
        UPstream::waitRequest(outstandingRecvRequest_);
      }
      outstandingSendRequest_ = -1;
      outstandingRecvRequest_ = -1;
    }
    else
    {
      procPatch_.compressedReceive<Type>(commsType, *this);
    }
    if (doTransform())
    {
      transform(*this, procPatch_.forwardT(), *this);
    }
  }
}


template<class Type>
mousse::tmp<mousse::Field<Type> >
mousse::processorFvPatchField<Type>::snGrad
(
  const scalarField& deltaCoeffs
) const
{
  return deltaCoeffs*(*this - this->patchInternalField());
}


template<class Type>
void mousse::processorFvPatchField<Type>::initInterfaceMatrixUpdate
(
  scalarField&,
  const scalarField& psiInternal,
  const scalarField&,
  const direction,
  const Pstream::commsTypes commsType
) const
{
  this->patch().patchInternalField(psiInternal, scalarSendBuf_);
  if (commsType == Pstream::nonBlocking && !Pstream::floatTransfer)
  {
    // Fast path.
    if (debug && !this->ready())
    {
      FATAL_ERROR_IN
      (
        "processorFvPatchField<Type>::initInterfaceMatrixUpdate(..)"
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
  }
  else
  {
    procPatch_.compressedSend(commsType, scalarSendBuf_);
  }
  const_cast<processorFvPatchField<Type>&>(*this).updatedMatrix() = false;
}


template<class Type>
void mousse::processorFvPatchField<Type>::updateInterfaceMatrix
(
  scalarField& result,
  const scalarField&,
  const scalarField& coeffs,
  const direction cmpt,
  const Pstream::commsTypes commsType
) const
{
  if (this->updatedMatrix())
  {
    return;
  }
  const labelUList& faceCells = this->patch().faceCells();
  if (commsType == Pstream::nonBlocking && !Pstream::floatTransfer)
  {
    // Fast path.
    if (outstandingRecvRequest_ >= 0
        && outstandingRecvRequest_ < Pstream::nRequests())
    {
      UPstream::waitRequest(outstandingRecvRequest_);
    }
    // Recv finished so assume sending finished as well.
    outstandingSendRequest_ = -1;
    outstandingRecvRequest_ = -1;
    // Consume straight from scalarReceiveBuf_
    // Transform according to the transformation tensor
    transformCoupleField(scalarReceiveBuf_, cmpt);
    // Multiply the field by coefficients and add into the result
    FOR_ALL(faceCells, elemI)
    {
      result[faceCells[elemI]] -= coeffs[elemI]*scalarReceiveBuf_[elemI];
    }
  }
  else
  {
    scalarField pnf
    {
      procPatch_.compressedReceive<scalar>(commsType, this->size())()
    };
    // Transform according to the transformation tensor
    transformCoupleField(pnf, cmpt);
    // Multiply the field by coefficients and add into the result
    FOR_ALL(faceCells, elemI)
    {
      result[faceCells[elemI]] -= coeffs[elemI]*pnf[elemI];
    }
  }
  const_cast<processorFvPatchField<Type>&>(*this).updatedMatrix() = true;
}


template<class Type>
void mousse::processorFvPatchField<Type>::initInterfaceMatrixUpdate
(
  Field<Type>&,
  const Field<Type>& psiInternal,
  const scalarField&,
  const Pstream::commsTypes commsType
) const
{
  this->patch().patchInternalField(psiInternal, sendBuf_);
  if (commsType == Pstream::nonBlocking && !Pstream::floatTransfer)
  {
    // Fast path.
    if (debug && !this->ready())
    {
      FATAL_ERROR_IN
      (
        "processorFvPatchField<Type>::initInterfaceMatrixUpdate(..)"
      )
      << "On patch " << procPatch_.name()
      << " outstanding request."
      << abort(FatalError);
    }
    receiveBuf_.setSize(sendBuf_.size());
    outstandingRecvRequest_ = UPstream::nRequests();
    IPstream::read
    (
      Pstream::nonBlocking,
      procPatch_.neighbProcNo(),
      reinterpret_cast<char*>(receiveBuf_.begin()),
      receiveBuf_.byteSize(),
      procPatch_.tag(),
      procPatch_.comm()
    );
    outstandingSendRequest_ = UPstream::nRequests();
    OPstream::write
    (
      Pstream::nonBlocking,
      procPatch_.neighbProcNo(),
      reinterpret_cast<const char*>(sendBuf_.begin()),
      sendBuf_.byteSize(),
      procPatch_.tag(),
      procPatch_.comm()
    );
  }
  else
  {
    procPatch_.compressedSend(commsType, sendBuf_);
  }
  const_cast<processorFvPatchField<Type>&>(*this).updatedMatrix() = false;
}


template<class Type>
void mousse::processorFvPatchField<Type>::updateInterfaceMatrix
(
  Field<Type>& result,
  const Field<Type>&,
  const scalarField& coeffs,
  const Pstream::commsTypes commsType
) const
{
  if (this->updatedMatrix())
  {
    return;
  }
  const labelUList& faceCells = this->patch().faceCells();
  if (commsType == Pstream::nonBlocking && !Pstream::floatTransfer)
  {
    // Fast path.
    if (outstandingRecvRequest_ >= 0
        && outstandingRecvRequest_ < Pstream::nRequests())
    {
      UPstream::waitRequest(outstandingRecvRequest_);
    }
    // Recv finished so assume sending finished as well.
    outstandingSendRequest_ = -1;
    outstandingRecvRequest_ = -1;
    // Consume straight from receiveBuf_
    // Transform according to the transformation tensor
    transformCoupleField(receiveBuf_);
    // Multiply the field by coefficients and add into the result
    FOR_ALL(faceCells, elemI)
    {
      result[faceCells[elemI]] -= coeffs[elemI]*receiveBuf_[elemI];
    }
  }
  else
  {
    Field<Type> pnf
    {
      procPatch_.compressedReceive<Type>(commsType, this->size())()
    };
    // Transform according to the transformation tensor
    transformCoupleField(pnf);
    // Multiply the field by coefficients and add into the result
    FOR_ALL(faceCells, elemI)
    {
      result[faceCells[elemI]] -= coeffs[elemI]*pnf[elemI];
    }
  }
  const_cast<processorFvPatchField<Type>&>(*this).updatedMatrix() = true;
}


template<class Type>
bool mousse::processorFvPatchField<Type>::ready() const
{
  if (outstandingSendRequest_ >= 0
      && outstandingSendRequest_ < Pstream::nRequests())
  {
    bool finished = UPstream::finishedRequest(outstandingSendRequest_);
    if (!finished)
    {
      return false;
    }
  }
  outstandingSendRequest_ = -1;
  if (outstandingRecvRequest_ >= 0
      && outstandingRecvRequest_ < Pstream::nRequests())
  {
    bool finished = UPstream::finishedRequest(outstandingRecvRequest_);
    if (!finished)
    {
      return false;
    }
  }
  outstandingRecvRequest_ = -1;
  return true;
}
