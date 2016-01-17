// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "sliced_fvs_patch_field.hpp"

namespace mousse
{
// Member Functions 
template<class Type>
slicedFvsPatchField<Type>::slicedFvsPatchField
(
  const fvPatch& p,
  const DimensionedField<Type, surfaceMesh>& iF,
  const Field<Type>& completeField
)
:
  fvsPatchField<Type>{p, iF, Field<Type>()}
{
  // Set the fvsPatchField to a slice of the given complete field
  UList<Type>::operator=(p.patchSlice(completeField));
}


template<class Type>
slicedFvsPatchField<Type>::slicedFvsPatchField
(
  const fvPatch& p,
  const DimensionedField<Type, surfaceMesh>& iF
)
:
  fvsPatchField<Type>{p, iF}
{}


template<class Type>
slicedFvsPatchField<Type>::slicedFvsPatchField
(
  const slicedFvsPatchField<Type>& ptf,
  const fvPatch& p,
  const DimensionedField<Type, surfaceMesh>& iF,
  const fvPatchFieldMapper& mapper
)
:
  fvsPatchField<Type>{ptf, p, iF, mapper}
{
  NOT_IMPLEMENTED
  (
    "slicedFvsPatchField<Type>::"
    "slicedFvsPatchField(const slicedFvsPatchField<Type>&, "
    "const fvPatch&, const Field<Type>&, const fvPatchFieldMapper&)"
  );
}


template<class Type>
slicedFvsPatchField<Type>::slicedFvsPatchField
(
  const fvPatch& p,
  const DimensionedField<Type, surfaceMesh>& iF,
  const dictionary& dict
)
:
  fvsPatchField<Type>{p, iF, Field<Type>("value", dict, p.size())}
{
  NOT_IMPLEMENTED
  (
    "slicedFvsPatchField<Type>::"
    "slicedFvsPatchField(const Field<Type>&, const dictionary&)"
  );
}


template<class Type>
slicedFvsPatchField<Type>::slicedFvsPatchField
(
  const slicedFvsPatchField<Type>& ptf,
  const DimensionedField<Type, surfaceMesh>& iF
)
:
  fvsPatchField<Type>{ptf.patch(), iF, Field<Type>()}
{
  // Transfer the slice from the argument
  UList<Type>::operator=(ptf);
}


template<class Type>
tmp<fvsPatchField<Type> > slicedFvsPatchField<Type>::clone() const
{
  return tmp<fvsPatchField<Type> >
  {
    new slicedFvsPatchField<Type>{*this}
  };
}


template<class Type>
slicedFvsPatchField<Type>::slicedFvsPatchField
(
  const slicedFvsPatchField<Type>& ptf
)
:
  fvsPatchField<Type>
  {
    ptf.patch(),
    ptf.dimensionedInternalField(),
    Field<Type>()
  }
{
  // Transfer the slice from the argument
  UList<Type>::operator=(ptf);
}


template<class Type>
tmp<fvsPatchField<Type> > slicedFvsPatchField<Type>::clone
(
  const DimensionedField<Type, surfaceMesh>& iF
) const
{
  return tmp<fvsPatchField<Type> >
  {
    new slicedFvsPatchField<Type>{*this, iF}
  };
}


template<class Type>
slicedFvsPatchField<Type>::~slicedFvsPatchField<Type>()
{
  // Set the fvsPatchField storage pointer to NULL before its destruction
  // to protect the field it a slice of.
  UList<Type>::operator=(UList<Type>(NULL, 0));
}

}  // namespace mousse
