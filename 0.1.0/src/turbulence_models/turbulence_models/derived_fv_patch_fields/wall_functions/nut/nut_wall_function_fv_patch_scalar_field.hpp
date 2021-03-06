#ifndef TURBULENCE_MODELS_TURBULENCE_MODELS_DERIVED_FV_PATCH_FIELDS_WALL_FUNCTIONS_NUT_WALL_FUNCTIONS_NUT_WALL_FUNCTION_FV_PATCH_SCALAR_FIELD_HPP_
#define TURBULENCE_MODELS_TURBULENCE_MODELS_DERIVED_FV_PATCH_FIELDS_WALL_FUNCTIONS_NUT_WALL_FUNCTIONS_NUT_WALL_FUNCTION_FV_PATCH_SCALAR_FIELD_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::nutWallFunctionFvPatchScalarField
// Group
//   grpWallFunctions
// Description
//   This boundary condition provides a turbulent kinematic viscosity condition
//   when using wall functions, based on turbulence kinetic energy.
//   - replicates OpenFOAM v1.5 (and earlier) behaviour
//   \heading Patch usage
//   \table
//     Property  | Description         | Required   | Default value
//     Cmu       | Cmu coefficient     | no         | 0.09
//     kappa     | Von Karman constant | no         | 0.41
//     E         | E coefficient       | no         | 9.8
//   \endtable
//   Examples of the boundary condition specification:
//   \verbatim
//   myPatch
//   {
//     type            nutWallFunction;
//     value           uniform 0.0;
//   }
//   \endverbatim
//   Reference for the default model coefficients:
//   \verbatim
//     H. Versteeg, W. Malalasekera
//     An Introduction to Computational Fluid Dynamics: The Finite Volume
//     Method, subsection "3.5.2 k-epsilon model"
//   \endverbatim
// SeeAlso
//   mousse::fixedValueFvPatchField

#include "fixed_value_fv_patch_fields.hpp"


namespace mousse {

class nutWallFunctionFvPatchScalarField
:
  public fixedValueFvPatchScalarField
{
protected:
  // Protected data
    //- Cmu coefficient
    scalar Cmu_;
    //- Von Karman constant
    scalar kappa_;
    //- E coefficient
    scalar E_;
    //- Y+ at the edge of the laminar sublayer
    scalar yPlusLam_;
  // Protected Member Functions
    //- Check the type of the patch
    virtual void checkType();
    //- Calculate the turbulence viscosity
    virtual tmp<scalarField> calcNut() const = 0;
    //- Write local wall function variables
    virtual void writeLocalEntries(Ostream&) const;
public:
  //- Runtime type information
  TYPE_NAME("nutWallFunction");
  // Constructors
    //- Construct from patch and internal field
    nutWallFunctionFvPatchScalarField
    (
      const fvPatch&,
      const DimensionedField<scalar, volMesh>&
    );
    //- Construct from patch, internal field and dictionary
    nutWallFunctionFvPatchScalarField
    (
      const fvPatch&,
      const DimensionedField<scalar, volMesh>&,
      const dictionary&
    );
    //- Construct by mapping given
    //  nutWallFunctionFvPatchScalarField
    //  onto a new patch
    nutWallFunctionFvPatchScalarField
    (
      const nutWallFunctionFvPatchScalarField&,
      const fvPatch&,
      const DimensionedField<scalar, volMesh>&,
      const fvPatchFieldMapper&
    );
    //- Construct as copy
    nutWallFunctionFvPatchScalarField
    (
      const nutWallFunctionFvPatchScalarField&
    );
    //- Construct as copy setting internal field reference
    nutWallFunctionFvPatchScalarField
    (
      const nutWallFunctionFvPatchScalarField&,
      const DimensionedField<scalar, volMesh>&
    );
  // Member functions
    //- Calculate the Y+ at the edge of the laminar sublayer
    static scalar yPlusLam(const scalar kappa, const scalar E);
    //- Calculate and return the yPlus at the boundary
    virtual tmp<scalarField> yPlus() const = 0;
    // Evaluation functions
      //- Update the coefficients associated with the patch field
      virtual void updateCoeffs();
    // I-O
      //- Write
      virtual void write(Ostream&) const;
};
}  // namespace mousse
#endif
