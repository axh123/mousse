// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "smoluchowski_jump_t_fv_patch_scalar_field.hpp"
#include "add_to_run_time_selection_table.hpp"
#include "fv_patch_field_mapper.hpp"
#include "vol_fields.hpp"
#include "basic_thermo.hpp"
#include "mathematical_constants.hpp"


// Constructors 
mousse::smoluchowskiJumpTFvPatchScalarField::smoluchowskiJumpTFvPatchScalarField
(
  const fvPatch& p,
  const DimensionedField<scalar, volMesh>& iF
)
:
  mixedFvPatchScalarField{p, iF},
  UName_{"U"},
  rhoName_{"rho"},
  psiName_{"thermo:psi"},
  muName_{"thermo:mu"},
  accommodationCoeff_{1.0},
  Twall_{p.size(), 0.0},
  gamma_{1.4}
{
  refValue() = 0.0;
  refGrad() = 0.0;
  valueFraction() = 0.0;
}


mousse::smoluchowskiJumpTFvPatchScalarField::smoluchowskiJumpTFvPatchScalarField
(
  const smoluchowskiJumpTFvPatchScalarField& ptf,
  const fvPatch& p,
  const DimensionedField<scalar, volMesh>& iF,
  const fvPatchFieldMapper& mapper
)
:
  mixedFvPatchScalarField{ptf, p, iF, mapper},
  UName_{ptf.UName_},
  rhoName_{ptf.rhoName_},
  psiName_{ptf.psiName_},
  muName_{ptf.muName_},
  accommodationCoeff_{ptf.accommodationCoeff_},
  Twall_{ptf.Twall_},
  gamma_{ptf.gamma_}
{}


mousse::smoluchowskiJumpTFvPatchScalarField::smoluchowskiJumpTFvPatchScalarField
(
  const fvPatch& p,
  const DimensionedField<scalar, volMesh>& iF,
  const dictionary& dict
)
:
  mixedFvPatchScalarField{p, iF},
  UName_{dict.lookupOrDefault<word>("U", "U")},
  rhoName_{dict.lookupOrDefault<word>("rho", "rho")},
  psiName_{dict.lookupOrDefault<word>("psi", "thermo:psi")},
  muName_{dict.lookupOrDefault<word>("mu", "thermo:mu")},
  accommodationCoeff_{readScalar(dict.lookup("accommodationCoeff"))},
  Twall_{"Twall", dict, p.size()},
  gamma_{dict.lookupOrDefault<scalar>("gamma", 1.4)}
{
  if (mag(accommodationCoeff_) < SMALL || mag(accommodationCoeff_) > 2.0) {
    FATAL_IO_ERROR_IN
    (
      "smoluchowskiJumpTFvPatchScalarField::"
      "smoluchowskiJumpTFvPatchScalarField"
      "("
      "  const fvPatch&,"
      "  const DimensionedField<scalar, volMesh>&,"
      "  const dictionary&"
      ")",
      dict
    )
    << "unphysical accommodationCoeff specified"
    << "(0 < accommodationCoeff <= 1)" << endl
    << exit(FatalIOError);
  }
  if (dict.found("value")) {
    fvPatchField<scalar>::operator=
    (
      scalarField{"value", dict, p.size()}
    );
  } else {
    fvPatchField<scalar>::operator=(patchInternalField());
  }
  refValue() = *this;
  refGrad() = 0.0;
  valueFraction() = 0.0;
}


mousse::smoluchowskiJumpTFvPatchScalarField::smoluchowskiJumpTFvPatchScalarField
(
  const smoluchowskiJumpTFvPatchScalarField& ptpsf,
  const DimensionedField<scalar, volMesh>& iF
)
:
  mixedFvPatchScalarField{ptpsf, iF},
  accommodationCoeff_{ptpsf.accommodationCoeff_},
  Twall_{ptpsf.Twall_},
  gamma_{ptpsf.gamma_}
{}


// Member Functions 
// Map from self
void mousse::smoluchowskiJumpTFvPatchScalarField::autoMap
(
  const fvPatchFieldMapper& m
)
{
  mixedFvPatchScalarField::autoMap(m);
}


// Reverse-map the given fvPatchField onto this fvPatchField
void mousse::smoluchowskiJumpTFvPatchScalarField::rmap
(
  const fvPatchField<scalar>& ptf,
  const labelList& addr
)
{
  mixedFvPatchField<scalar>::rmap(ptf, addr);
}


// Update the coefficients associated with the patch field
void mousse::smoluchowskiJumpTFvPatchScalarField::updateCoeffs()
{
  if (updated()) {
    return;
  }
  const fvPatchScalarField& pmu =
    patch().lookupPatchField<volScalarField, scalar>(muName_);
  const fvPatchScalarField& prho =
    patch().lookupPatchField<volScalarField, scalar>(rhoName_);
  const fvPatchField<scalar>& ppsi =
    patch().lookupPatchField<volScalarField, scalar>(psiName_);
  const fvPatchVectorField& pU =
    patch().lookupPatchField<volVectorField, vector>(UName_);
  // Prandtl number reading consistent with rhoCentralFoam
  const dictionary& thermophysicalProperties =
    db().lookupObject<IOdictionary>(basicThermo::dictName);
  dimensionedScalar Pr
  {
    "Pr",
    dimless,
    thermophysicalProperties.subDict("mixture").subDict("transport").lookup("Pr")
  };
  Field<scalar> C2
  {
    pmu/prho*sqrt(ppsi*constant::mathematical::piByTwo)
      *2.0*gamma_/Pr.value()/(gamma_ + 1.0)
      *(2.0 - accommodationCoeff_)/accommodationCoeff_
  };
  Field<scalar> aCoeff{prho.snGrad() - prho/C2};
  Field<scalar> KEbyRho{0.5*magSqr(pU)};
  valueFraction() = (1.0/(1.0 + patch().deltaCoeffs()*C2));
  refValue() = Twall_;
  refGrad() = 0.0;
  mixedFvPatchScalarField::updateCoeffs();
}


// Write
void mousse::smoluchowskiJumpTFvPatchScalarField::write(Ostream& os) const
{
  fvPatchScalarField::write(os);
  writeEntryIfDifferent<word>(os, "U", "U", UName_);
  writeEntryIfDifferent<word>(os, "rho", "rho", rhoName_);
  writeEntryIfDifferent<word>(os, "psi", "thermo:psi", psiName_);
  writeEntryIfDifferent<word>(os, "mu", "thermo:mu", muName_);
  os.writeKeyword("accommodationCoeff")
    << accommodationCoeff_ << token::END_STATEMENT << nl;
  Twall_.writeEntry("Twall", os);
  os.writeKeyword("gamma")
    << gamma_ << token::END_STATEMENT << nl;
  writeEntry("value", os);
}


namespace mousse {

MAKE_PATCH_TYPE_FIELD
(
  fvPatchScalarField,
  smoluchowskiJumpTFvPatchScalarField
);

}

