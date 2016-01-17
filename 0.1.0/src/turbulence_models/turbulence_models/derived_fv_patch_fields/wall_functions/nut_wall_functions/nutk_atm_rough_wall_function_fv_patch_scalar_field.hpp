// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::nutkAtmRoughWallFunctionFvPatchScalarField
// Group
//   grpWallFunctions
// Description
//   This boundary condition provides a turbulent kinematic viscosity for
//   atmospheric velocity profiles.  It is desinged to be used in conjunction
//   with the atmBoundaryLayerInletVelocity boundary condition.  The values
//   are calculated using:
//     \f[
//       U = frac{U_f}{K} ln(\frac{z + z_0}{z_0})
//     \f]
//   where
//   \vartable
//     U_f | frictional velocity
//     K   | Von Karman's constant
//     z_0 | surface roughness length
//     z   | vertical co-ordinate
//   \endvartable
//   \heading Patch usage
//   \table
//     Property     | Description             | Required    | Default value
//     z0           | surface roughness length| yes         |
//   \endtable
//   Example of the boundary condition specification:
//   \verbatim
//   myPatch
//   {
//     type            nutkAtmRoughWallFunction;
//     z0              uniform 0;
//   }
//   \endverbatim
// SeeAlso
//   mousse::nutkWallFunctionFvPatchField
// SourceFiles
//   nutk_atm_rough_wall_function_fv_patch_scalar_field.cpp
#ifndef nutk_atm_rough_wall_function_fv_patch_scalar_field_hpp_
#define nutk_atm_rough_wall_function_fv_patch_scalar_field_hpp_
#include "nutk_wall_function_fv_patch_scalar_field.hpp"
namespace mousse
{
class nutkAtmRoughWallFunctionFvPatchScalarField
:
  public nutkWallFunctionFvPatchScalarField
{
protected:
  // Protected data
    //- Surface roughness length
    scalarField z0_;
  // Protected Member Functions
    //- Calculate the turbulence viscosity
    virtual tmp<scalarField> calcNut() const;
public:
  //- Runtime type information
  TYPE_NAME("nutkAtmRoughWallFunction");
  // Constructors
    //- Construct from patch and internal field
    nutkAtmRoughWallFunctionFvPatchScalarField
    (
      const fvPatch&,
      const DimensionedField<scalar, volMesh>&
    );
    //- Construct from patch, internal field and dictionary
    nutkAtmRoughWallFunctionFvPatchScalarField
    (
      const fvPatch&,
      const DimensionedField<scalar, volMesh>&,
      const dictionary&
    );
    //- Construct by mapping given
    //  nutkAtmRoughWallFunctionFvPatchScalarField
    //  onto a new patch
    nutkAtmRoughWallFunctionFvPatchScalarField
    (
      const nutkAtmRoughWallFunctionFvPatchScalarField&,
      const fvPatch&,
      const DimensionedField<scalar, volMesh>&,
      const fvPatchFieldMapper&
    );
    //- Construct as copy
    nutkAtmRoughWallFunctionFvPatchScalarField
    (
      const nutkAtmRoughWallFunctionFvPatchScalarField&
    );
    //- Construct and return a clone
    virtual tmp<fvPatchScalarField> clone() const
    {
      return tmp<fvPatchScalarField>
      (
        new nutkAtmRoughWallFunctionFvPatchScalarField(*this)
      );
    }
    //- Construct as copy setting internal field reference
    nutkAtmRoughWallFunctionFvPatchScalarField
    (
      const nutkAtmRoughWallFunctionFvPatchScalarField&,
      const DimensionedField<scalar, volMesh>&
    );
    //- Construct and return a clone setting internal field reference
    virtual tmp<fvPatchScalarField> clone
    (
      const DimensionedField<scalar, volMesh>& iF
    ) const
    {
      return tmp<fvPatchScalarField>
      (
        new nutkAtmRoughWallFunctionFvPatchScalarField(*this, iF)
      );
    }
  // Member functions
    // Acces functions
      // Return z0
      scalarField& z0()
      {
        return z0_;
      }
    // Mapping functions
      //- Map (and resize as needed) from self given a mapping object
      virtual void autoMap(const fvPatchFieldMapper&);
      //- Reverse map the given fvPatchField onto this fvPatchField
      virtual void rmap
      (
        const fvPatchScalarField&,
        const labelList&
      );
    // I-O
      //- Write
      virtual void write(Ostream&) const;
};
}  // namespace mousse
#endif
