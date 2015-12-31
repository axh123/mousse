// mousse: CFD toolbox
// Copyright (C) 2011-2014 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "mixed_fv_patch_field.hpp"
namespace mousse
{
// Member Functions 
template<class Type>
mixedFvPatchField<Type>::mixedFvPatchField
(
  const fvPatch& p,
  const DimensionedField<Type, volMesh>& iF
)
:
  fvPatchField<Type>(p, iF),
  refValue_(p.size()),
  refGrad_(p.size()),
  valueFraction_(p.size())
{}
template<class Type>
mixedFvPatchField<Type>::mixedFvPatchField
(
  const mixedFvPatchField<Type>& ptf,
  const fvPatch& p,
  const DimensionedField<Type, volMesh>& iF,
  const fvPatchFieldMapper& mapper
)
:
  fvPatchField<Type>(ptf, p, iF, mapper),
  refValue_(ptf.refValue_, mapper),
  refGrad_(ptf.refGrad_, mapper),
  valueFraction_(ptf.valueFraction_, mapper)
{
  if (notNull(iF) && mapper.hasUnmapped())
  {
    WarningIn
    (
      "mixedFvPatchField<Type>::mixedFvPatchField\n"
      "(\n"
      "    const mixedFvPatchField<Type>&,\n"
      "    const fvPatch&,\n"
      "    const DimensionedField<Type, volMesh>&,\n"
      "    const fvPatchFieldMapper&\n"
      ")\n"
    )   << "On field " << iF.name() << " patch " << p.name()
      << " patchField " << this->type()
      << " : mapper does not map all values." << nl
      << "    To avoid this warning fully specify the mapping in derived"
      << " patch fields." << endl;
  }
}
template<class Type>
mixedFvPatchField<Type>::mixedFvPatchField
(
  const fvPatch& p,
  const DimensionedField<Type, volMesh>& iF,
  const dictionary& dict
)
:
  fvPatchField<Type>(p, iF, dict),
  refValue_("refValue", dict, p.size()),
  refGrad_("refGradient", dict, p.size()),
  valueFraction_("valueFraction", dict, p.size())
{
  evaluate();
}
template<class Type>
mixedFvPatchField<Type>::mixedFvPatchField
(
  const mixedFvPatchField<Type>& ptf
)
:
  fvPatchField<Type>(ptf),
  refValue_(ptf.refValue_),
  refGrad_(ptf.refGrad_),
  valueFraction_(ptf.valueFraction_)
{}
template<class Type>
mixedFvPatchField<Type>::mixedFvPatchField
(
  const mixedFvPatchField<Type>& ptf,
  const DimensionedField<Type, volMesh>& iF
)
:
  fvPatchField<Type>(ptf, iF),
  refValue_(ptf.refValue_),
  refGrad_(ptf.refGrad_),
  valueFraction_(ptf.valueFraction_)
{}
// Member Functions 
template<class Type>
void mixedFvPatchField<Type>::autoMap
(
  const fvPatchFieldMapper& m
)
{
  fvPatchField<Type>::autoMap(m);
  refValue_.autoMap(m);
  refGrad_.autoMap(m);
  valueFraction_.autoMap(m);
}
template<class Type>
void mixedFvPatchField<Type>::rmap
(
  const fvPatchField<Type>& ptf,
  const labelList& addr
)
{
  fvPatchField<Type>::rmap(ptf, addr);
  const mixedFvPatchField<Type>& mptf =
    refCast<const mixedFvPatchField<Type> >(ptf);
  refValue_.rmap(mptf.refValue_, addr);
  refGrad_.rmap(mptf.refGrad_, addr);
  valueFraction_.rmap(mptf.valueFraction_, addr);
}
template<class Type>
void mixedFvPatchField<Type>::evaluate(const Pstream::commsTypes)
{
  if (!this->updated())
  {
    this->updateCoeffs();
  }
  Field<Type>::operator=
  (
    valueFraction_*refValue_
   +
    (1.0 - valueFraction_)*
    (
      this->patchInternalField()
     + refGrad_/this->patch().deltaCoeffs()
    )
  );
  fvPatchField<Type>::evaluate();
}
template<class Type>
tmp<Field<Type> > mixedFvPatchField<Type>::snGrad() const
{
  return
    valueFraction_
   *(refValue_ - this->patchInternalField())
   *this->patch().deltaCoeffs()
   + (1.0 - valueFraction_)*refGrad_;
}
template<class Type>
tmp<Field<Type> > mixedFvPatchField<Type>::valueInternalCoeffs
(
  const tmp<scalarField>&
) const
{
  return Type(pTraits<Type>::one)*(1.0 - valueFraction_);
}
template<class Type>
tmp<Field<Type> > mixedFvPatchField<Type>::valueBoundaryCoeffs
(
  const tmp<scalarField>&
) const
{
  return
    valueFraction_*refValue_
   + (1.0 - valueFraction_)*refGrad_/this->patch().deltaCoeffs();
}
template<class Type>
tmp<Field<Type> > mixedFvPatchField<Type>::gradientInternalCoeffs() const
{
  return -Type(pTraits<Type>::one)*valueFraction_*this->patch().deltaCoeffs();
}
template<class Type>
tmp<Field<Type> > mixedFvPatchField<Type>::gradientBoundaryCoeffs() const
{
  return
    valueFraction_*this->patch().deltaCoeffs()*refValue_
   + (1.0 - valueFraction_)*refGrad_;
}
template<class Type>
void mixedFvPatchField<Type>::write(Ostream& os) const
{
  fvPatchField<Type>::write(os);
  refValue_.writeEntry("refValue", os);
  refGrad_.writeEntry("refGradient", os);
  valueFraction_.writeEntry("valueFraction", os);
  this->writeEntry("value", os);
}
}  // namespace mousse
