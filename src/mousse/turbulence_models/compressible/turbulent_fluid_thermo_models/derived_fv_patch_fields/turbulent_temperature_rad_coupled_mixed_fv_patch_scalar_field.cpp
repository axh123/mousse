// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "turbulent_temperature_rad_coupled_mixed_fv_patch_scalar_field.hpp"
#include "add_to_run_time_selection_table.hpp"
#include "fv_patch_field_mapper.hpp"
#include "vol_fields.hpp"
#include "mapped_patch_base.hpp"


namespace mousse {
namespace compressible {

// Constructors 
turbulentTemperatureRadCoupledMixedFvPatchScalarField::
turbulentTemperatureRadCoupledMixedFvPatchScalarField
(
  const fvPatch& p,
  const DimensionedField<scalar, volMesh>& iF
)
:
  mixedFvPatchScalarField{p, iF},
  temperatureCoupledBase{patch(), "undefined", "undefined", "undefined-K"},
  TnbrName_{"undefined-Tnbr"},
  QrNbrName_{"undefined-QrNbr"},
  QrName_{"undefined-Qr"},
  thicknessLayers_{0},
  kappaLayers_{0},
  contactRes_{0}
{
  this->refValue() = 0.0;
  this->refGrad() = 0.0;
  this->valueFraction() = 1.0;
}


turbulentTemperatureRadCoupledMixedFvPatchScalarField::
turbulentTemperatureRadCoupledMixedFvPatchScalarField
(
  const turbulentTemperatureRadCoupledMixedFvPatchScalarField& psf,
  const fvPatch& p,
  const DimensionedField<scalar, volMesh>& iF,
  const fvPatchFieldMapper& mapper
)
:
  mixedFvPatchScalarField{psf, p, iF, mapper},
  temperatureCoupledBase{patch(), psf},
  TnbrName_{psf.TnbrName_},
  QrNbrName_{psf.QrNbrName_},
  QrName_{psf.QrName_},
  thicknessLayers_{psf.thicknessLayers_},
  kappaLayers_{psf.kappaLayers_},
  contactRes_{psf.contactRes_}
{}


turbulentTemperatureRadCoupledMixedFvPatchScalarField::
turbulentTemperatureRadCoupledMixedFvPatchScalarField
(
  const fvPatch& p,
  const DimensionedField<scalar, volMesh>& iF,
  const dictionary& dict
)
:
  mixedFvPatchScalarField{p, iF},
  temperatureCoupledBase{patch(), dict},
  TnbrName_{dict.lookupOrDefault<word>("Tnbr", "T")},
  QrNbrName_{dict.lookupOrDefault<word>("QrNbr", "none")},
  QrName_{dict.lookupOrDefault<word>("Qr", "none")},
  thicknessLayers_{0},
  kappaLayers_{0},
  contactRes_{0.0}
{
  if (!isA<mappedPatchBase>(this->patch().patch())) {
    FATAL_ERROR_IN
    (
      "turbulentTemperatureRadCoupledMixedFvPatchScalarField::"
      "turbulentTemperatureRadCoupledMixedFvPatchScalarField\n"
      "(\n"
      "    const fvPatch& p,\n"
      "    const DimensionedField<scalar, volMesh>& iF,\n"
      "    const dictionary& dict\n"
      ")\n"
    )
    << "\n    patch type '" << p.type()
    << "' not type '" << mappedPatchBase::typeName << "'"
    << "\n    for patch " << p.name()
    << " of field " << dimensionedInternalField().name()
    << " in file " << dimensionedInternalField().objectPath()
    << exit(FatalError);
  }
  if (dict.found("thicknessLayers")) {
    dict.lookup("thicknessLayers") >> thicknessLayers_;
    dict.lookup("kappaLayers") >> kappaLayers_;
    if (thicknessLayers_.size() > 0) {
      // Calculate effective thermal resistance by harmonic averaging
      FOR_ALL(thicknessLayers_, iLayer) {
        contactRes_ += thicknessLayers_[iLayer]/kappaLayers_[iLayer];
      }
      contactRes_ = 1.0/contactRes_;
    }
  }
  fvPatchScalarField::operator=(scalarField{"value", dict, p.size()});
  if (dict.found("refValue")) {
    // Full restart
    refValue() = scalarField{"refValue", dict, p.size()};
    refGrad() = scalarField{"refGradient", dict, p.size()};
    valueFraction() = scalarField{"valueFraction", dict, p.size()};
  } else {
    // Start from user entered data. Assume fixedValue.
    refValue() = *this;
    refGrad() = 0.0;
    valueFraction() = 1.0;
  }
}


turbulentTemperatureRadCoupledMixedFvPatchScalarField::
turbulentTemperatureRadCoupledMixedFvPatchScalarField
(
  const turbulentTemperatureRadCoupledMixedFvPatchScalarField& psf,
  const DimensionedField<scalar, volMesh>& iF
)
:
  mixedFvPatchScalarField{psf, iF},
  temperatureCoupledBase{patch(), psf},
  TnbrName_{psf.TnbrName_},
  QrNbrName_{psf.QrNbrName_},
  QrName_{psf.QrName_},
  thicknessLayers_{psf.thicknessLayers_},
  kappaLayers_{psf.kappaLayers_},
  contactRes_{psf.contactRes_}
{}


// Member Functions 
void turbulentTemperatureRadCoupledMixedFvPatchScalarField::updateCoeffs()
{
  if (updated()) {
    return;
  }
  // Since we're inside initEvaluate/evaluate there might be processor
  // comms underway. Change the tag we use.
  int oldTag = UPstream::msgType();
  UPstream::msgType() = oldTag+1;
  // Get the coupling information from the mappedPatchBase
  const mappedPatchBase& mpp =
    refCast<const mappedPatchBase>(patch().patch());
  const polyMesh& nbrMesh = mpp.sampleMesh();
  const label samplePatchI = mpp.samplePolyPatch().index();
  const fvPatch& nbrPatch =
    refCast<const fvMesh>(nbrMesh).boundary()[samplePatchI];
  scalarField Tc(patchInternalField());
  scalarField& Tp = *this;
  const turbulentTemperatureRadCoupledMixedFvPatchScalarField&
    nbrField = refCast
      <const turbulentTemperatureRadCoupledMixedFvPatchScalarField>
      (
        nbrPatch.lookupPatchField<volScalarField, scalar>(TnbrName_)
      );
  // Swap to obtain full local values of neighbour internal field
  scalarField TcNbr{nbrField.patchInternalField()};
  mpp.distribute(TcNbr);
  // Swap to obtain full local values of neighbour K*delta
  scalarField KDeltaNbr;
  if (contactRes_ == 0.0) {
    KDeltaNbr = nbrField.kappa(nbrField)*nbrPatch.deltaCoeffs();
  } else {
    KDeltaNbr.setSize(nbrField.size(), contactRes_);
  }
  mpp.distribute(KDeltaNbr);
  scalarField KDelta{kappa(Tp)*patch().deltaCoeffs()};
  scalarField Qr{Tp.size(), 0.0};
  if (QrName_ != "none") {
    Qr = patch().lookupPatchField<volScalarField, scalar>(QrName_);
  }
  scalarField QrNbr{Tp.size(), 0.0};
  if (QrNbrName_ != "none") {
    QrNbr = nbrPatch.lookupPatchField<volScalarField, scalar>(QrNbrName_);
    mpp.distribute(QrNbr);
  }
  valueFraction() = KDeltaNbr/(KDeltaNbr + KDelta);
  refValue() = TcNbr;
  refGrad() = (Qr + QrNbr)/kappa(Tp);
  mixedFvPatchScalarField::updateCoeffs();
  if (debug) {
    scalar Q = gSum(kappa(Tp)*patch().magSf()*snGrad());
    Info
      << patch().boundaryMesh().mesh().name() << ':'
      << patch().name() << ':'
      << this->dimensionedInternalField().name() << " <- "
      << nbrMesh.name() << ':'
      << nbrPatch.name() << ':'
      << this->dimensionedInternalField().name() << " :"
      << " heat transfer rate:" << Q
      << " walltemperature "
      << " min:" << gMin(Tp)
      << " max:" << gMax(Tp)
      << " avg:" << gAverage(Tp)
      << endl;
  }
  // Restore tag
  UPstream::msgType() = oldTag;
}


void turbulentTemperatureRadCoupledMixedFvPatchScalarField::write
(
  Ostream& os
) const
{
  mixedFvPatchScalarField::write(os);
  os.writeKeyword("Tnbr")<< TnbrName_ << token::END_STATEMENT << nl;
  os.writeKeyword("QrNbr")<< QrNbrName_ << token::END_STATEMENT << nl;
  os.writeKeyword("Qr")<< QrName_ << token::END_STATEMENT << nl;
  thicknessLayers_.writeEntry("thicknessLayers", os);
  kappaLayers_.writeEntry("kappaLayers", os);
  temperatureCoupledBase::write(os);
}


MAKE_PATCH_TYPE_FIELD
(
  fvPatchScalarField,
  turbulentTemperatureRadCoupledMixedFvPatchScalarField
);

}  // namespace compressible
}  // namespace mousse
