#ifndef FINITE_VOLUME_FIELDS_FV_PATCH_FIELDS_DERIVED_FIXED_NORMAL_SLIP_FV_PATCH_FIELD_HPP_
#define FINITE_VOLUME_FIELDS_FV_PATCH_FIELDS_DERIVED_FIXED_NORMAL_SLIP_FV_PATCH_FIELD_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011-2012 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::fixedNormalSlipFvPatchField
// Group
//   grpGenericBoundaryConditions grpWallBoundaryConditions
// Description
//   This boundary condition sets the patch-normal component to a fixed value.
//   \heading Patch usage
//   \table
//     Property     | Description             | Required    | Default value
//     fixedValue   | fixed value             | yes         |
//   \endtable
//   Example of the boundary condition specification:
//   \verbatim
//   myPatch
//   {
//     type            fixedNormalSlip;
//     fixedValue      uniform 0;     // example entry for a scalar field
//   }
//   \endverbatim
// SeeAlso
//   mousse::transformFvPatchField

#include "transform_fv_patch_field.hpp"


namespace mousse {

template<class Type>
class fixedNormalSlipFvPatchField
:
  public transformFvPatchField<Type>
{
  // Private data
    //- Value the normal component of which the boundary is set to
    Field<Type> fixedValue_;
public:
  //- Runtime type information
  TYPE_NAME("fixedNormalSlip");
  // Constructors
    //- Construct from patch and internal field
    fixedNormalSlipFvPatchField
    (
      const fvPatch&,
      const DimensionedField<Type, volMesh>&
    );
    //- Construct from patch, internal field and dictionary
    fixedNormalSlipFvPatchField
    (
      const fvPatch&,
      const DimensionedField<Type, volMesh>&,
      const dictionary&
    );
    //- Construct by mapping given fixedNormalSlipFvPatchField
    //  onto a new patch
    fixedNormalSlipFvPatchField
    (
      const fixedNormalSlipFvPatchField<Type>&,
      const fvPatch&,
      const DimensionedField<Type, volMesh>&,
      const fvPatchFieldMapper&
    );
    //- Construct as copy
    fixedNormalSlipFvPatchField
    (
      const fixedNormalSlipFvPatchField<Type>&
    );
    //- Construct and return a clone
    virtual tmp<fvPatchField<Type>> clone() const
    {
      return tmp<fvPatchField<Type>>
      {
        new fixedNormalSlipFvPatchField<Type>{*this}
      };
    }
    //- Construct as copy setting internal field reference
    fixedNormalSlipFvPatchField
    (
      const fixedNormalSlipFvPatchField<Type>&,
      const DimensionedField<Type, volMesh>&
    );
    //- Construct and return a clone setting internal field reference
    virtual tmp<fvPatchField<Type>> clone
    (
      const DimensionedField<Type, volMesh>& iF
    ) const
    {
      return tmp<fvPatchField<Type>>
      {
        new fixedNormalSlipFvPatchField<Type>{*this, iF}
      };
    }
  // Member functions
    // Mapping functions
      //- Map (and resize as needed) from self given a mapping object
      virtual void autoMap
      (
        const fvPatchFieldMapper&
      );
      //- Reverse map the given fvPatchField onto this fvPatchField
      virtual void rmap
      (
        const fvPatchField<Type>&,
        const labelList&
      );
    // Return defining fields
      virtual Field<Type>& fixedValue()
      {
        return fixedValue_;
      }
      virtual const Field<Type>& fixedValue() const
      {
        return fixedValue_;
      }
    // Evaluation functions
      //- Return gradient at boundary
      virtual tmp<Field<Type>> snGrad() const;
      //- Evaluate the patch field
      virtual void evaluate
      (
        const Pstream::commsTypes commsType=Pstream::blocking
      );
      //- Return face-gradient transform diagonal
      virtual tmp<Field<Type>> snGradTransformDiag() const;
    //- Write
    virtual void write(Ostream&) const;
  // Member operators
    virtual void operator=(const UList<Type>&) {}
    virtual void operator=(const fvPatchField<Type>&) {}
    virtual void operator+=(const fvPatchField<Type>&) {}
    virtual void operator-=(const fvPatchField<Type>&) {}
    virtual void operator*=(const fvPatchField<scalar>&) {}
    virtual void operator/=(const fvPatchField<scalar>&) {}
    virtual void operator+=(const Field<Type>&) {}
    virtual void operator-=(const Field<Type>&) {}
    virtual void operator*=(const Field<scalar>&) {}
    virtual void operator/=(const Field<scalar>&) {}
    virtual void operator=(const Type&) {}
    virtual void operator+=(const Type&) {}
    virtual void operator-=(const Type&) {}
    virtual void operator*=(const scalar) {}
    virtual void operator/=(const scalar) {}
};
}  // namespace mousse

#include "fixed_normal_slip_fv_patch_field.ipp"

#endif
