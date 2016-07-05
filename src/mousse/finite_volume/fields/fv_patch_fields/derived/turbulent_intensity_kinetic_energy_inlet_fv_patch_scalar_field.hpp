#ifndef FINITE_VOLUME_FIELDS_FV_PATCH_FIELDS_DERIVED_TURBULENT_INTENSITY_KINETIC_ENERGY_INLET_FV_PATCH_SCALAR_FIELD_HPP_
#define FINITE_VOLUME_FIELDS_FV_PATCH_FIELDS_DERIVED_TURBULENT_INTENSITY_KINETIC_ENERGY_INLET_FV_PATCH_SCALAR_FIELD_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011-2013 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::turbulentIntensityKineticEnergyInletFvPatchScalarField
// Group
//   grpInletBoundaryConditions
// Description
//   This boundary condition provides a turbulent kinetic energy condition,
//   based on user-supplied turbulence intensity, defined as a fraction of the
//   mean velocity:
//     \f[
//       k_p = 1.5 (I |U|)^2
//     \f]
//   where
//   \vartable
//     k_p     | kinetic energy at the patch
//     I       | turbulence intensity
//     U       | velocity field
//   \endvartable
//   In the event of reverse flow, a zero-gradient condition is applied.
//   \heading Patch usage
//   \table
//     Property     | Description             | Required    | Default value
//     intensity    | fraction of mean field [0-1] | yes    |
//     U            | velocity field name     | no          | U
//     phi          | flux field name         | no          | phi
//   \endtable
//   Example of the boundary condition specification:
//   \verbatim
//   myPatch
//   {
//     type        turbulentIntensityKineticEnergyInlet;
//     intensity   0.05;           // 5% turbulence
//     value       uniform 1;      // placeholder
//   }
//   \endverbatim
// SeeAlso
//   mousse::inletOutletFvPatchField

#include "inlet_outlet_fv_patch_fields.hpp"


namespace mousse {

class turbulentIntensityKineticEnergyInletFvPatchScalarField
:
  public inletOutletFvPatchScalarField
{
  // Private data
    //- Turbulent intensity as fraction of mean velocity
    scalar intensity_;
    //- Name of the velocity field
    word UName_;
public:
  //- Runtime type information
  TYPE_NAME("turbulentIntensityKineticEnergyInlet");
  // Constructors
    //- Construct from patch and internal field
    turbulentIntensityKineticEnergyInletFvPatchScalarField
    (
      const fvPatch&,
      const DimensionedField<scalar, volMesh>&
    );
    //- Construct from patch, internal field and dictionary
    turbulentIntensityKineticEnergyInletFvPatchScalarField
    (
      const fvPatch&,
      const DimensionedField<scalar, volMesh>&,
      const dictionary&
    );
    //- Construct by mapping given
    //  turbulentIntensityKineticEnergyInletFvPatchScalarField
    //  onto a new patch
    turbulentIntensityKineticEnergyInletFvPatchScalarField
    (
      const turbulentIntensityKineticEnergyInletFvPatchScalarField&,
      const fvPatch&,
      const DimensionedField<scalar, volMesh>&,
      const fvPatchFieldMapper&
    );
    //- Construct as copy
    turbulentIntensityKineticEnergyInletFvPatchScalarField
    (
      const turbulentIntensityKineticEnergyInletFvPatchScalarField&
    );
    //- Construct and return a clone
    virtual tmp<fvPatchScalarField> clone() const
    {
      return tmp<fvPatchScalarField>
      {
        new turbulentIntensityKineticEnergyInletFvPatchScalarField{*this}
      };
    }
    //- Construct as copy setting internal field reference
    turbulentIntensityKineticEnergyInletFvPatchScalarField
    (
      const turbulentIntensityKineticEnergyInletFvPatchScalarField&,
      const DimensionedField<scalar, volMesh>&
    );
    //- Construct and return a clone setting internal field reference
    virtual tmp<fvPatchScalarField> clone
    (
      const DimensionedField<scalar, volMesh>& iF
    ) const
    {
      return tmp<fvPatchScalarField>
      {
        new turbulentIntensityKineticEnergyInletFvPatchScalarField{*this, iF}
      };
    }
  // Member functions
    //- Update the coefficients associated with the patch field
    virtual void updateCoeffs();
    //- Write
    virtual void write(Ostream&) const;
};
}  // namespace mousse
#endif