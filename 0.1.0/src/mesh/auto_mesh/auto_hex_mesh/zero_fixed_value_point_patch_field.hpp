#ifndef MESH_AUTO_MESH_AUTO_HEX_MESH_EXTERNAL_DISPLACEMENT_MESH_MOVER_ZERO_FIXED_VALUE_ZERO_FIXED_VALUE_POINT_PATCH_FIELD_HPP_
#define MESH_AUTO_MESH_AUTO_HEX_MESH_EXTERNAL_DISPLACEMENT_MESH_MOVER_ZERO_FIXED_VALUE_ZERO_FIXED_VALUE_POINT_PATCH_FIELD_HPP_

// mousse: CFD toolbox
// Copyright (C) 2014 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::zeroFixedValuePointPatchField
// Description
//   Enables the specification of a zero fixed value boundary condition.
//   Example of the boundary condition specification:
//   \verbatim
//   inlet
//   {
//     type            zeroFixedValue;
//   }
//   \endverbatim

#include "fixed_value_point_patch_field.hpp"


namespace mousse {

template<class Type>
class zeroFixedValuePointPatchField
:
  public fixedValuePointPatchField<Type>
{
public:
  //- Runtime type information
  TYPE_NAME("zeroFixedValue");
  // Constructors
    //- Construct from patch and internal field
    zeroFixedValuePointPatchField
    (
      const pointPatch&,
      const DimensionedField<Type, pointMesh>&
    );
    //- Construct from patch, internal field and dictionary
    zeroFixedValuePointPatchField
    (
      const pointPatch&,
      const DimensionedField<Type, pointMesh>&,
      const dictionary&
    );
    //- Construct by mapping given patchField<Type> onto a new patch
    zeroFixedValuePointPatchField
    (
      const zeroFixedValuePointPatchField<Type>&,
      const pointPatch&,
      const DimensionedField<Type, pointMesh>&,
      const pointPatchFieldMapper&
    );
    //- Construct as copy
    zeroFixedValuePointPatchField
    (
      const zeroFixedValuePointPatchField<Type>&
    );
    //- Construct and return a clone
    virtual autoPtr<pointPatchField<Type>> clone() const
    {
      return
        autoPtr<pointPatchField<Type>>
        {
          new zeroFixedValuePointPatchField<Type>{*this}
        };
    }
    //- Construct as copy setting internal field reference
    zeroFixedValuePointPatchField
    (
      const zeroFixedValuePointPatchField<Type>&,
      const DimensionedField<Type, pointMesh>&
    );
    //- Construct and return a clone setting internal field reference
    virtual autoPtr<pointPatchField<Type>> clone
    (
      const DimensionedField<Type, pointMesh>& iF
    ) const
    {
      return
        autoPtr<pointPatchField<Type>>
        {
          new zeroFixedValuePointPatchField<Type>{*this, iF}
        };
    }
};

}  // namespace mousse

#include "zero_fixed_value_point_patch_field.ipp"

#endif
