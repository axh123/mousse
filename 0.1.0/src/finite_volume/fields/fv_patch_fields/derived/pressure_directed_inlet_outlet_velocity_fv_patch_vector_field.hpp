// mousse: CFD toolbox
// Copyright (C) 2011-2012 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::pressureDirectedInletOutletVelocityFvPatchVectorField
// Group
//   grpInletBoundaryConditions grpOutletBoundaryConditions
// Description
//   This velocity inlet/outlet boundary condition is applied to pressure
//   boundaries where the pressure is specified.  A zero-gradient condtion is
//   applied for outflow (as defined by the flux); for inflow, the velocity
//   is obtained from the flux with the specified inlet direction.
//   \heading Patch usage
//   \table
//     Property     | Description             | Required    | Default value
//     phi          | flux field name         | no          | phi
//     rho          | density field name      | no          | rho
//     inletDirection | inlet direction per patch face | yes |
//   \endtable
//   Example of the boundary condition specification:
//   \verbatim
//   myPatch
//   {
//     type            pressureDirectedInletOutletVelocity;
//     phi             phi;
//     rho             rho;
//     inletDirection  uniform (1 0 0);
//     value           uniform 0;
//   }
//   \endverbatim
// Note
//   Sign conventions:
//   - positive flux (out of domain): apply zero-gradient condition
//   - negative flux (into of domain): derive from the flux with specified
//    direction
// SeeAlso
//   mousse::mixedFvPatchVectorField
// SourceFiles
//   pressure_directed_inlet_outlet_velocity_fv_patch_vector_field.cpp
#ifndef pressure_directed_inlet_outlet_velocity_fv_patch_vector_field_hpp_
#define pressure_directed_inlet_outlet_velocity_fv_patch_vector_field_hpp_
#include "fv_patch_fields.hpp"
#include "mixed_fv_patch_fields.hpp"
namespace mousse
{
class pressureDirectedInletOutletVelocityFvPatchVectorField
:
  public mixedFvPatchVectorField
{
  // Private data
    //- Flux field name
    word phiName_;
    //- Density field name
    word rhoName_;
    //- Inlet direction
    vectorField inletDir_;
public:
  //- Runtime type information
  TYPE_NAME("pressureDirectedInletOutletVelocity");
  // Constructors
    //- Construct from patch and internal field
    pressureDirectedInletOutletVelocityFvPatchVectorField
    (
      const fvPatch&,
      const DimensionedField<vector, volMesh>&
    );
    //- Construct from patch, internal field and dictionary
    pressureDirectedInletOutletVelocityFvPatchVectorField
    (
      const fvPatch&,
      const DimensionedField<vector, volMesh>&,
      const dictionary&
    );
    //- Construct by mapping given
    //  pressureDirectedInletOutletVelocityFvPatchVectorField
    //  onto a new patch
    pressureDirectedInletOutletVelocityFvPatchVectorField
    (
      const pressureDirectedInletOutletVelocityFvPatchVectorField&,
      const fvPatch&,
      const DimensionedField<vector, volMesh>&,
      const fvPatchFieldMapper&
    );
    //- Construct as copy
    pressureDirectedInletOutletVelocityFvPatchVectorField
    (
      const pressureDirectedInletOutletVelocityFvPatchVectorField&
    );
    //- Construct and return a clone
    virtual tmp<fvPatchVectorField> clone() const
    {
      return tmp<fvPatchVectorField>
      (
        new pressureDirectedInletOutletVelocityFvPatchVectorField
        (
          *this
        )
      );
    }
    //- Construct as copy setting internal field reference
    pressureDirectedInletOutletVelocityFvPatchVectorField
    (
      const pressureDirectedInletOutletVelocityFvPatchVectorField&,
      const DimensionedField<vector, volMesh>&
    );
    //- Construct and return a clone setting internal field reference
    virtual tmp<fvPatchVectorField> clone
    (
      const DimensionedField<vector, volMesh>& iF
    ) const
    {
      return tmp<fvPatchVectorField>
      (
        new pressureDirectedInletOutletVelocityFvPatchVectorField
        (
          *this,
          iF
        )
      );
    }
  // Member functions
    // Access
      //- Return the name of rho
      const word& rhoName() const
      {
        return rhoName_;
      }
      //- Return reference to the name of rho to allow adjustment
      word& rhoName()
      {
        return rhoName_;
      }
      //- Return the name of phi
      const word& phiName() const
      {
        return phiName_;
      }
      //- Return reference to the name of phi to allow adjustment
      word& phiName()
      {
        return phiName_;
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
        const fvPatchVectorField&,
        const labelList&
      );
    //- Update the coefficients associated with the patch field
    virtual void updateCoeffs();
    //- Write
    virtual void write(Ostream&) const;
  // Member operators
    virtual void operator=(const fvPatchField<vector>& pvf);
};
}  // namespace mousse
#endif
