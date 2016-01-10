// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "processor_cyclic_fvs_patch_field.hpp"

namespace mousse
{

// Constructors
template<class Type>
processorCyclicFvsPatchField<Type>::processorCyclicFvsPatchField
(
  const fvPatch& p,
  const DimensionedField<Type, surfaceMesh>& iF
)
:
  coupledFvsPatchField<Type>{p, iF},
  procPatch_{refCast<const processorCyclicFvPatch>(p)}
{}

template<class Type>
processorCyclicFvsPatchField<Type>::processorCyclicFvsPatchField
(
  const fvPatch& p,
  const DimensionedField<Type, surfaceMesh>& iF,
  const Field<Type>& f
)
:
  coupledFvsPatchField<Type>{p, iF, f},
  procPatch_{refCast<const processorCyclicFvPatch>(p)}
{}

// Construct by mapping given processorCyclicFvsPatchField<Type>
template<class Type>
processorCyclicFvsPatchField<Type>::processorCyclicFvsPatchField
(
  const processorCyclicFvsPatchField<Type>& ptf,
  const fvPatch& p,
  const DimensionedField<Type, surfaceMesh>& iF,
  const fvPatchFieldMapper& mapper
)
:
  coupledFvsPatchField<Type>{ptf, p, iF, mapper},
  procPatch_{refCast<const processorCyclicFvPatch>(p)}
{
  if (!isType<processorCyclicFvPatch>(this->patch()))
  {
    FATAL_ERROR_IN
    (
      "processorCyclicFvsPatchField<Type>::processorCyclicFvsPatchField\n"
      "(\n"
      "    const processorCyclicFvsPatchField<Type>& ptf,\n"
      "    const fvPatch& p,\n"
      "    const DimensionedField<Type, surfaceMesh>& iF,\n"
      "    const fvPatchFieldMapper& mapper\n"
      ")\n"
    )
    << "Field type does not correspond to patch type for patch "
    << this->patch().index() << "." << endl
    << "Field type: " << typeName << endl
    << "Patch type: " << this->patch().type()
    << exit(FatalError);
  }
}

template<class Type>
processorCyclicFvsPatchField<Type>::processorCyclicFvsPatchField
(
  const fvPatch& p,
  const DimensionedField<Type, surfaceMesh>& iF,
  const dictionary& dict
)
:
  coupledFvsPatchField<Type>{p, iF, dict},
  procPatch_{refCast<const processorCyclicFvPatch>(p)}
{
  if (!isType<processorCyclicFvPatch>(p))
  {
    FATAL_IO_ERROR_IN
    (
      "processorCyclicFvsPatchField<Type>::processorCyclicFvsPatchField\n"
      "(\n"
      "    const fvPatch& p,\n"
      "    const Field<Type>& field,\n"
      "    const dictionary& dict\n"
      ")\n",
      dict
    )
    << "patch " << this->patch().index() << " not processor type. "
    << "Patch type = " << p.type()
    << exit(FatalIOError);
  }
}

template<class Type>
processorCyclicFvsPatchField<Type>::processorCyclicFvsPatchField
(
  const processorCyclicFvsPatchField<Type>& ptf
)
:
  coupledFvsPatchField<Type>{ptf},
  procPatch_{refCast<const processorCyclicFvPatch>(ptf.patch())}
{}

template<class Type>
processorCyclicFvsPatchField<Type>::processorCyclicFvsPatchField
(
  const processorCyclicFvsPatchField<Type>& ptf,
  const DimensionedField<Type, surfaceMesh>& iF
)
:
  coupledFvsPatchField<Type>{ptf, iF},
  procPatch_{refCast<const processorCyclicFvPatch>(ptf.patch())}
{}

// Destructor 
template<class Type>
processorCyclicFvsPatchField<Type>::~processorCyclicFvsPatchField()
{}

}  // namespace mousse
