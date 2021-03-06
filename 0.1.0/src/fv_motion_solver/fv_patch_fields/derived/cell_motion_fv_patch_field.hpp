#ifndef FV_MOTION_SOLVER_FV_PATCH_FIELDS_DERIVED_CELL_MOTION_CELL_MOTION_FV_PATCH_FIELD_HPP_
#define FV_MOTION_SOLVER_FV_PATCH_FIELDS_DERIVED_CELL_MOTION_CELL_MOTION_FV_PATCH_FIELD_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::cellMotionFvPatchField
// Description
//   mousse::cellMotionFvPatchField

#include "random.hpp"
#include "fixed_value_fv_patch_fields.hpp"


namespace mousse {

template<class Type>
class cellMotionFvPatchField
:
  public fixedValueFvPatchField<Type>
{
public:
  //- Runtime type information
  TYPE_NAME("cellMotion");
  // Constructors
    //- Construct from patch and internal field
    cellMotionFvPatchField
    (
      const fvPatch&,
      const DimensionedField<Type, volMesh>&
    );
    //- Construct from patch, internal field and dictionary
    cellMotionFvPatchField
    (
      const fvPatch&,
      const DimensionedField<Type, volMesh>&,
      const dictionary&
    );
    //- Construct by mapping given cellMotionFvPatchField
    //  onto a new patch
    cellMotionFvPatchField
    (
      const cellMotionFvPatchField<Type>&,
      const fvPatch&,
      const DimensionedField<Type, volMesh>&,
      const fvPatchFieldMapper&
    );
    //- Construct as copy
    cellMotionFvPatchField
    (
      const cellMotionFvPatchField<Type>&
    );
    //- Construct and return a clone
    virtual tmp<fvPatchField<Type>> clone() const
    {
      return
        tmp<fvPatchField<Type>>
        {
          new cellMotionFvPatchField<Type>{*this}
        };
    }
    //- Construct as copy setting internal field reference
    cellMotionFvPatchField
    (
      const cellMotionFvPatchField<Type>&,
      const DimensionedField<Type, volMesh>&
    );
    //- Construct and return a clone setting internal field reference
    virtual tmp<fvPatchField<Type>> clone
    (
      const DimensionedField<Type, volMesh>& iF
    ) const
    {
      return
        tmp<fvPatchField<Type>>
        {
          new cellMotionFvPatchField<Type>{*this, iF}
        };
    }
  // Member functions
    //- Update the coefficients associated with the patch field
    virtual void updateCoeffs();
    //- Write
    virtual void write(Ostream&) const;
};

}  // namespace mousse

#include "cell_motion_fv_patch_field.ipp"

#endif
