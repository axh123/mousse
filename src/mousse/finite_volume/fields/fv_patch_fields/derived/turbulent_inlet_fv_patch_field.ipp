// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "turbulent_inlet_fv_patch_field.hpp"


namespace mousse {

// Constructors 
template<class Type>
turbulentInletFvPatchField<Type>::turbulentInletFvPatchField
(
  const fvPatch& p,
  const DimensionedField<Type, volMesh>& iF
)
:
  fixedValueFvPatchField<Type>{p, iF},
  ranGen_{label(0)},
  fluctuationScale_{pTraits<Type>::zero},
  referenceField_{p.size()},
  alpha_{0.1},
  curTimeIndex_{-1}
{}


template<class Type>
turbulentInletFvPatchField<Type>::turbulentInletFvPatchField
(
  const turbulentInletFvPatchField<Type>& ptf,
  const fvPatch& p,
  const DimensionedField<Type, volMesh>& iF,
  const fvPatchFieldMapper& mapper
)
:
  fixedValueFvPatchField<Type>{ptf, p, iF, mapper},
  ranGen_{label(0)},
  fluctuationScale_{ptf.fluctuationScale_},
  referenceField_{ptf.referenceField_, mapper},
  alpha_{ptf.alpha_},
  curTimeIndex_{-1}
{}


template<class Type>
turbulentInletFvPatchField<Type>::turbulentInletFvPatchField
(
  const fvPatch& p,
  const DimensionedField<Type, volMesh>& iF,
  const dictionary& dict
)
:
  fixedValueFvPatchField<Type>{p, iF},
  ranGen_{label(0)},
  fluctuationScale_{pTraits<Type>(dict.lookup("fluctuationScale"))},
  referenceField_{"referenceField", dict, p.size()},
  alpha_{dict.lookupOrDefault<scalar>("alpha", 0.1)},
  curTimeIndex_{-1}
{
  if (dict.found("value")) {
    fixedValueFvPatchField<Type>::operator==
    (
      Field<Type>{"value", dict, p.size()}
    );
  } else {
    fixedValueFvPatchField<Type>::operator==(referenceField_);
  }
}


template<class Type>
turbulentInletFvPatchField<Type>::turbulentInletFvPatchField
(
  const turbulentInletFvPatchField<Type>& ptf
)
:
  fixedValueFvPatchField<Type>{ptf},
  ranGen_{ptf.ranGen_},
  fluctuationScale_{ptf.fluctuationScale_},
  referenceField_{ptf.referenceField_},
  alpha_{ptf.alpha_},
  curTimeIndex_{-1}
{}


template<class Type>
turbulentInletFvPatchField<Type>::turbulentInletFvPatchField
(
  const turbulentInletFvPatchField<Type>& ptf,
  const DimensionedField<Type, volMesh>& iF
)
:
  fixedValueFvPatchField<Type>{ptf, iF},
  ranGen_{ptf.ranGen_},
  fluctuationScale_{ptf.fluctuationScale_},
  referenceField_{ptf.referenceField_},
  alpha_{ptf.alpha_},
  curTimeIndex_{-1}
{}


// Member Functions 
template<class Type>
void turbulentInletFvPatchField<Type>::autoMap
(
  const fvPatchFieldMapper& m
)
{
  fixedValueFvPatchField<Type>::autoMap(m);
  referenceField_.autoMap(m);
}


template<class Type>
void turbulentInletFvPatchField<Type>::rmap
(
  const fvPatchField<Type>& ptf,
  const labelList& addr
)
{
  fixedValueFvPatchField<Type>::rmap(ptf, addr);
  const turbulentInletFvPatchField<Type>& tiptf =
    refCast<const turbulentInletFvPatchField<Type> >(ptf);
  referenceField_.rmap(tiptf.referenceField_, addr);
}


template<class Type>
void turbulentInletFvPatchField<Type>::updateCoeffs()
{
  if (this->updated()) {
    return;
  }
  if (curTimeIndex_ != this->db().time().timeIndex()) {
    Field<Type>& patchField = *this;
    Field<Type> randomField{this->size()};
    FOR_ALL(patchField, facei) {
      ranGen_.randomise(randomField[facei]);
    }
    // Correction-factor to compensate for the loss of RMS fluctuation
    // due to the temporal correlation introduced by the alpha parameter.
    scalar rmsCorr = sqrt(12*(2*alpha_ - sqr(alpha_)))/alpha_;
    patchField =
      (1 - alpha_)*patchField
      + alpha_*(referenceField_
                + rmsCorr*cmptMultiply(randomField - 0.5*pTraits<Type>::one,
                                       fluctuationScale_)*mag(referenceField_));
    curTimeIndex_ = this->db().time().timeIndex();
  }
  fixedValueFvPatchField<Type>::updateCoeffs();
}


template<class Type>
void turbulentInletFvPatchField<Type>::write(Ostream& os) const
{
  fvPatchField<Type>::write(os);
  os.writeKeyword("fluctuationScale")
    << fluctuationScale_ << token::END_STATEMENT << nl;
  referenceField_.writeEntry("referenceField", os);
  os.writeKeyword("alpha") << alpha_ << token::END_STATEMENT << nl;
  this->writeEntry("value", os);
}

}  // namespace mousse
