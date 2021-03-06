#ifndef FINITE_VOLUME_FIELDS_FV_PATCH_FIELDS_DERIVED_PRGH_PRESSURE_FV_PATCH_SCALAR_FIELD_HPP_
#define FINITE_VOLUME_FIELDS_FV_PATCH_FIELDS_DERIVED_PRGH_PRESSURE_FV_PATCH_SCALAR_FIELD_HPP_

// mousse: CFD toolbox
// Copyright (C) 2013-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::prghPressureFvPatchScalarField
// Group
//   grpGenericBoundaryConditions
// Description
//   This boundary condition provides static pressure condition for p_rgh,
//   calculated as:
//     \f[
//       p_rgh = p - \rho g (h - hRef)
//     \f]
//   where
//   \vartable
//     p_rgh   | Pseudo hydrostatic pressure [Pa]
//     p       | Static pressure [Pa]
//     h       | Height in the opposite direction to gravity
//     hRef    | Reference height in the opposite direction to gravity
//     \rho    | density
//     g       | acceleration due to gravity [m/s^2]
//   \endtable
//   \heading Patch usage
//   \table
//     Property     | Description             | Required    | Default value
//     rho          | rho field name          | no          | rho
//     p            | static pressure         | yes         |
//   \endtable
//   Example of the boundary condition specification:
//   \verbatim
//   myPatch
//   {
//     type            prghPressure;
//     rho             rho;
//     p               uniform 0;
//     value           uniform 0; // optional initial value
//   }
//   \endverbatim
// SeeAlso
//   mousse::fixedValueFvPatchScalarField

#include "fixed_value_fv_patch_fields.hpp"


namespace mousse {

class prghPressureFvPatchScalarField
:
  public fixedValueFvPatchScalarField
{
protected:
  // Protected data
    //- Name of phase-fraction field
    word rhoName_;
    //- Static pressure
    scalarField p_;
public:
  //- Runtime type information
  TYPE_NAME("prghPressure");
  // Constructors
    //- Construct from patch and internal field
    prghPressureFvPatchScalarField
    (
      const fvPatch&,
      const DimensionedField<scalar, volMesh>&
    );
    //- Construct from patch, internal field and dictionary
    prghPressureFvPatchScalarField
    (
      const fvPatch&,
      const DimensionedField<scalar, volMesh>&,
      const dictionary&
    );
    //- Construct by mapping given
    //  prghPressureFvPatchScalarField onto a new patch
    prghPressureFvPatchScalarField
    (
      const prghPressureFvPatchScalarField&,
      const fvPatch&,
      const DimensionedField<scalar, volMesh>&,
      const fvPatchFieldMapper&
    );
    //- Construct as copy
    prghPressureFvPatchScalarField
    (
      const prghPressureFvPatchScalarField&
    );
    //- Construct and return a clone
    virtual tmp<fvPatchScalarField> clone() const
    {
      return tmp<fvPatchScalarField>
      {
        new prghPressureFvPatchScalarField{*this}
      };
    }
    //- Construct as copy setting internal field reference
    prghPressureFvPatchScalarField
    (
      const prghPressureFvPatchScalarField&,
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
        new prghPressureFvPatchScalarField{*this, iF}
      };
    }
  // Member functions
    // Access
      //- Return the static pressure
      const scalarField& p() const
      {
        return p_;
      }
      //- Return reference to the static pressure to allow adjustment
      scalarField& p()
      {
        return p_;
      }
    // Mapping functions
      //- Map (and resize as needed) from self given a mapping object
      virtual void autoMap
      (
        const fvPatchFieldMapper&
      );
      //- Reverse map the given fvPatchField onto this fvPatchField
      virtual void rmap
      (
        const fvPatchScalarField&,
        const labelList&
      );
    // Evaluation functions
      //- Update the coefficients associated with the patch field
      virtual void updateCoeffs();
    //- Write
    virtual void write(Ostream&) const;
};
}  // namespace mousse
#endif
