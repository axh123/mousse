// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "symmetry_fvs_patch_field.hpp"

namespace mousse
{

// Constructors 
template<class Type>
symmetryFvsPatchField<Type>::symmetryFvsPatchField
(
  const fvPatch& p,
  const DimensionedField<Type, surfaceMesh>& iF
)
:
  fvsPatchField<Type>{p, iF}
{}

template<class Type>
symmetryFvsPatchField<Type>::symmetryFvsPatchField
(
  const symmetryFvsPatchField<Type>& ptf,
  const fvPatch& p,
  const DimensionedField<Type, surfaceMesh>& iF,
  const fvPatchFieldMapper& mapper
)
:
  fvsPatchField<Type>{ptf, p, iF, mapper}
{
  if (!isType<symmetryFvPatch>(this->patch()))
  {
    FATAL_ERROR_IN
    (
      "symmetryFvsPatchField<Type>::symmetryFvsPatchField\n"
      "(\n"
      "    const symmetryFvsPatchField<Type>& ptf,\n"
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
symmetryFvsPatchField<Type>::symmetryFvsPatchField
(
  const fvPatch& p,
  const DimensionedField<Type, surfaceMesh>& iF,
  const dictionary& dict
)
:
  fvsPatchField<Type>{p, iF, dict}
{
  if (!isType<symmetryFvPatch>(p))
  {
    FATAL_IO_ERROR_IN
    (
      "symmetryFvsPatchField<Type>::symmetryFvsPatchField\n"
      "(\n"
      "    const fvPatch& p,\n"
      "    const Field<Type>& field,\n"
      "    const dictionary& dict\n"
      ")\n",
      dict
    )
    << "patch " << this->patch().index() << " not symmetry type. "
    << "Patch type = " << p.type()
    << exit(FatalIOError);
  }
}

template<class Type>
symmetryFvsPatchField<Type>::symmetryFvsPatchField
(
  const symmetryFvsPatchField<Type>& ptf
)
:
  fvsPatchField<Type>{ptf}
{}

template<class Type>
symmetryFvsPatchField<Type>::symmetryFvsPatchField
(
  const symmetryFvsPatchField<Type>& ptf,
  const DimensionedField<Type, surfaceMesh>& iF
)
:
  fvsPatchField<Type>{ptf, iF}
{}

}  // namespace mousse
