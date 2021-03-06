// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "marshak_radiation_fv_patch_scalar_field.hpp"
#include "add_to_run_time_selection_table.hpp"
#include "fv_patch_field_mapper.hpp"
#include "vol_fields.hpp"
#include "radiation_model.hpp"
#include "physico_chemical_constants.hpp"


// Constructors 
mousse::MarshakRadiationFvPatchScalarField::MarshakRadiationFvPatchScalarField
(
  const fvPatch& p,
  const DimensionedField<scalar, volMesh>& iF
)
:
  mixedFvPatchScalarField{p, iF},
  radiationCoupledBase{p, "undefined", scalarField::null()},
  TName_{"T"}
{
  refValue() = 0.0;
  refGrad() = 0.0;
  valueFraction() = 0.0;
}


mousse::MarshakRadiationFvPatchScalarField::MarshakRadiationFvPatchScalarField
(
  const MarshakRadiationFvPatchScalarField& ptf,
  const fvPatch& p,
  const DimensionedField<scalar, volMesh>& iF,
  const fvPatchFieldMapper& mapper
)
:
  mixedFvPatchScalarField{ptf, p, iF, mapper},
  radiationCoupledBase{p, ptf.emissivityMethod(), ptf.emissivity_, mapper},
  TName_{ptf.TName_}
{}


mousse::MarshakRadiationFvPatchScalarField::MarshakRadiationFvPatchScalarField
(
  const fvPatch& p,
  const DimensionedField<scalar, volMesh>& iF,
  const dictionary& dict
)
:
  mixedFvPatchScalarField{p, iF},
  radiationCoupledBase{p, dict},
  TName_{dict.lookupOrDefault<word>("T", "T")}
{
  if (dict.found("value")) {
    refValue() = scalarField{"value", dict, p.size()};
  } else {
    refValue() = 0.0;
  }
  // zero gradient
  refGrad() = 0.0;
  valueFraction() = 1.0;
  fvPatchScalarField::operator=(refValue());
}


mousse::MarshakRadiationFvPatchScalarField::MarshakRadiationFvPatchScalarField
(
  const MarshakRadiationFvPatchScalarField& ptf
)
:
  mixedFvPatchScalarField{ptf},
  radiationCoupledBase{ptf.patch(), ptf.emissivityMethod(), ptf.emissivity_},
  TName_{ptf.TName_}
{}


mousse::MarshakRadiationFvPatchScalarField::MarshakRadiationFvPatchScalarField
(
  const MarshakRadiationFvPatchScalarField& ptf,
  const DimensionedField<scalar, volMesh>& iF
)
:
  mixedFvPatchScalarField{ptf, iF},
  radiationCoupledBase{ptf.patch(), ptf.emissivityMethod(), ptf.emissivity_},
  TName_{ptf.TName_}
{}


// Member Functions 
void mousse::MarshakRadiationFvPatchScalarField::autoMap
(
  const fvPatchFieldMapper& m
)
{
  mixedFvPatchScalarField::autoMap(m);
  radiationCoupledBase::autoMap(m);
}


void mousse::MarshakRadiationFvPatchScalarField::rmap
(
  const fvPatchScalarField& ptf,
  const labelList& addr
)
{
  mixedFvPatchScalarField::rmap(ptf, addr);
  radiationCoupledBase::rmap(ptf, addr);
}


void mousse::MarshakRadiationFvPatchScalarField::updateCoeffs()
{
  if (this->updated()) {
    return;
  }
  // Since we're inside initEvaluate/evaluate there might be processor
  // comms underway. Change the tag we use.
  int oldTag = UPstream::msgType();
  UPstream::msgType() = oldTag+1;
  // Temperature field
  const scalarField& Tp =
    patch().lookupPatchField<volScalarField, scalar>(TName_);
  // Re-calc reference value
  refValue() = 4.0*constant::physicoChemical::sigma.value()*pow4(Tp);
  // Diffusion coefficient - created by radiation model's ::updateCoeffs()
  const scalarField& gamma =
    patch().lookupPatchField<volScalarField, scalar>("gammaRad");
  const scalarField temissivity = emissivity();
  const scalarField Ep{temissivity/(2.0*(2.0 - temissivity))};
  // Set value fraction
  valueFraction() = 1.0/(1.0 + gamma*patch().deltaCoeffs()/Ep);
  // Restore tag
  UPstream::msgType() = oldTag;
  mixedFvPatchScalarField::updateCoeffs();
}


void mousse::MarshakRadiationFvPatchScalarField::write(Ostream& os) const
{
  mixedFvPatchScalarField::write(os);
  radiationCoupledBase::write(os);
  writeEntryIfDifferent<word>(os, "T", "T", TName_);
}


namespace mousse {

MAKE_PATCH_TYPE_FIELD
(
  fvPatchScalarField,
  MarshakRadiationFvPatchScalarField
);

}

