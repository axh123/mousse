// mousse: CFD toolbox
// Copyright (C) 2011-2014 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "fixed_gradient_fv_patch_field.hpp"
#include "dictionary.hpp"

namespace mousse
{

// Member Functions 
template<class Type>
fixedGradientFvPatchField<Type>::fixedGradientFvPatchField
(
  const fvPatch& p,
  const DimensionedField<Type, volMesh>& iF
)
:
  fvPatchField<Type>{p, iF},
  gradient_{p.size(), pTraits<Type>::zero}
{}


template<class Type>
fixedGradientFvPatchField<Type>::fixedGradientFvPatchField
(
  const fixedGradientFvPatchField<Type>& ptf,
  const fvPatch& p,
  const DimensionedField<Type, volMesh>& iF,
  const fvPatchFieldMapper& mapper
)
:
  fvPatchField<Type>{ptf, p, iF, mapper},
  gradient_{ptf.gradient_, mapper}
{
  if (notNull(iF) && mapper.hasUnmapped())
  {
    WARNING_IN
    (
      "fixedGradientFvPatchField<Type>::fixedGradientFvPatchField\n"
      "(\n"
      "    const fixedGradientFvPatchField<Type>&,\n"
      "    const fvPatch&,\n"
      "    const DimensionedField<Type, volMesh>&,\n"
      "    const fvPatchFieldMapper&\n"
      ")\n"
    )
    << "On field " << iF.name() << " patch " << p.name()
    << " patchField " << this->type()
    << " : mapper does not map all values." << nl
    << "    To avoid this warning fully specify the mapping in derived"
    << " patch fields." << endl;
  }
}


template<class Type>
fixedGradientFvPatchField<Type>::fixedGradientFvPatchField
(
  const fvPatch& p,
  const DimensionedField<Type, volMesh>& iF,
  const dictionary& dict
)
:
  fvPatchField<Type>{p, iF, dict},
  gradient_{"gradient", dict, p.size()}
{
  evaluate();
}


template<class Type>
fixedGradientFvPatchField<Type>::fixedGradientFvPatchField
(
  const fixedGradientFvPatchField<Type>& ptf
)
:
  fvPatchField<Type>(ptf),
  gradient_(ptf.gradient_)
{}


template<class Type>
fixedGradientFvPatchField<Type>::fixedGradientFvPatchField
(
  const fixedGradientFvPatchField<Type>& ptf,
  const DimensionedField<Type, volMesh>& iF
)
:
  fvPatchField<Type>(ptf, iF),
  gradient_(ptf.gradient_)
{}


// Member Functions 
template<class Type>
void fixedGradientFvPatchField<Type>::autoMap
(
  const fvPatchFieldMapper& m
)
{
  fvPatchField<Type>::autoMap(m);
  gradient_.autoMap(m);
}


template<class Type>
void fixedGradientFvPatchField<Type>::rmap
(
  const fvPatchField<Type>& ptf,
  const labelList& addr
)
{
  fvPatchField<Type>::rmap(ptf, addr);
  const fixedGradientFvPatchField<Type>& fgptf =
    refCast<const fixedGradientFvPatchField<Type>>(ptf);
  gradient_.rmap(fgptf.gradient_, addr);
}


template<class Type>
void fixedGradientFvPatchField<Type>::evaluate(const Pstream::commsTypes)
{
  if (!this->updated())
  {
    this->updateCoeffs();
  }
  Field<Type>::operator=
  (
    this->patchInternalField() + gradient_/this->patch().deltaCoeffs()
  );
  fvPatchField<Type>::evaluate();
}


template<class Type>
tmp<Field<Type>> fixedGradientFvPatchField<Type>::valueInternalCoeffs
(
  const tmp<scalarField>&
) const
{
  return tmp<Field<Type>>{new Field<Type>{this->size(), pTraits<Type>::one}};
}


template<class Type>
tmp<Field<Type>> fixedGradientFvPatchField<Type>::valueBoundaryCoeffs
(
  const tmp<scalarField>&
) const
{
  return gradient()/this->patch().deltaCoeffs();
}


template<class Type>
tmp<Field<Type>> fixedGradientFvPatchField<Type>::
gradientInternalCoeffs() const
{
  return tmp<Field<Type>>
  {
    new Field<Type>{this->size(), pTraits<Type>::zero}
  };
}
template<class Type>
tmp<Field<Type>> fixedGradientFvPatchField<Type>::
gradientBoundaryCoeffs() const
{
  return gradient();
}


template<class Type>
void fixedGradientFvPatchField<Type>::write(Ostream& os) const
{
  fvPatchField<Type>::write(os);
  gradient_.writeEntry("gradient", os);
}

}  // namespace mousse
