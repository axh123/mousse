#ifndef TURBULENCE_MODELS_TURBULENCE_MODELS_DERIVED_FV_PATCH_FIELDS_WALL_FUNCTIONS_KQ_R_WALL_FUNCTIONS_K_LOW_RE_WALL_FUNCTION_FV_PATCH_SCALAR_FIELD_HPP_
#define TURBULENCE_MODELS_TURBULENCE_MODELS_DERIVED_FV_PATCH_FIELDS_WALL_FUNCTIONS_KQ_R_WALL_FUNCTIONS_K_LOW_RE_WALL_FUNCTION_FV_PATCH_SCALAR_FIELD_HPP_

// mousse: CFD toolbox
// Copyright (C) 2012-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::kLowReWallFunctionFvPatchScalarField
// Group
//   grpWallFunctions
// Description
//   This boundary condition provides a turbulence kinetic energy wall function
//   condition for low- and high-Reynolds number turbulent flow cases.
//   The model operates in two modes, based on the computed laminar-to-turbulent
//   switch-over y+ value derived from kappa and E.
//   \heading Patch usage
//   \table
//     Property     | Description             | Required    | Default value
//     Cmu          | model coefficient       | no          | 0.09
//     kappa        | Von Karman constant     | no          | 0.41
//     E            | model coefficient       | no          | 9.8
//     Ceps2        | model coefficient       | no          | 1.9
//   \endtable
//   Example of the boundary condition specification:
//   \verbatim
//   myPatch
//   {
//     type            kLowReWallFunction;
//   }
//   \endverbatim
// SeeAlso
//   mousse::fixedValueFvPatchField

#include "fixed_value_fv_patch_field.hpp"


namespace mousse {

class kLowReWallFunctionFvPatchScalarField
:
  public fixedValueFvPatchField<scalar>
{
protected:
  // Protected data
    //- Cmu coefficient
    scalar Cmu_;
    //- Von Karman constant
    scalar kappa_;
    //- E coefficient
    scalar E_;
    //- Ceps2 coefficient
    scalar Ceps2_;
    //- Y+ at the edge of the laminar sublayer
    scalar yPlusLam_;
  // Protected Member Functions
    //- Check the type of the patch
    virtual void checkType();
    //- Calculate the Y+ at the edge of the laminar sublayer
    scalar yPlusLam(const scalar kappa, const scalar E);
public:
  //- Runtime type information
  TYPE_NAME("kLowReWallFunction");
  // Constructors
    //- Construct from patch and internal field
    kLowReWallFunctionFvPatchScalarField
    (
      const fvPatch&,
      const DimensionedField<scalar, volMesh>&
    );
    //- Construct from patch, internal field and dictionary
    kLowReWallFunctionFvPatchScalarField
    (
      const fvPatch&,
      const DimensionedField<scalar, volMesh>&,
      const dictionary&
    );
    //- Construct by mapping given kLowReWallFunctionFvPatchScalarField
    //  onto a new patch
    kLowReWallFunctionFvPatchScalarField
    (
      const kLowReWallFunctionFvPatchScalarField&,
      const fvPatch&,
      const DimensionedField<scalar, volMesh>&,
      const fvPatchFieldMapper&
    );
    //- Construct as copy
    kLowReWallFunctionFvPatchScalarField
    (
      const kLowReWallFunctionFvPatchScalarField&
    );
    //- Construct and return a clone
    virtual tmp<fvPatchScalarField> clone() const
    {
      return
        tmp<fvPatchScalarField>
        {
          new kLowReWallFunctionFvPatchScalarField{*this}
        };
    }
    //- Construct as copy setting internal field reference
    kLowReWallFunctionFvPatchScalarField
    (
      const kLowReWallFunctionFvPatchScalarField&,
      const DimensionedField<scalar, volMesh>&
    );
    //- Construct and return a clone setting internal field reference
    virtual tmp<fvPatchScalarField> clone
    (
      const DimensionedField<scalar, volMesh>& iF
    ) const
    {
      return
        tmp<fvPatchScalarField>
        {
          new kLowReWallFunctionFvPatchScalarField{*this, iF}
        };
    }
  // Member functions
    // Evaluation functions
      //- Update the coefficients associated with the patch field
      virtual void updateCoeffs();
      //- Evaluate the patchField
      virtual void evaluate(const Pstream::commsTypes);
    // I-O
      //- Write
      virtual void write(Ostream&) const;
};

}  // namespace mousse

#endif

