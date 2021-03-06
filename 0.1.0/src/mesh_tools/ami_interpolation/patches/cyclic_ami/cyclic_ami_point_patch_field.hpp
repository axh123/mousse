#ifndef MESH_TOOLS_AMI_INTERPOLATION_PATCHES_CYCLIC_AMI_CYCLIC_AMI_POINT_PATCH_FIELD_CYCLIC_AMI_POINT_PATCH_FIELD_HPP_
#define MESH_TOOLS_AMI_INTERPOLATION_PATCHES_CYCLIC_AMI_CYCLIC_AMI_POINT_PATCH_FIELD_CYCLIC_AMI_POINT_PATCH_FIELD_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011-2013 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::cyclicAMIPointPatchField
// Description
//   Cyclic AMI front and back plane patch field

#include "coupled_point_patch_field.hpp"
#include "cyclic_ami_point_patch.hpp"
#include "_primitive_patch_interpolation.hpp"


namespace mousse {

template<class Type>
class cyclicAMIPointPatchField
:
  public coupledPointPatchField<Type>
{
  // Private data

    //- Local reference cast into the cyclicAMI patch
    const cyclicAMIPointPatch& cyclicAMIPatch_;

    //- Owner side patch interpolation pointer
    mutable autoPtr<PrimitivePatchInterpolation<primitivePatch>> ppiPtr_;

    //- Neighbour side patch interpolation pointer
    mutable autoPtr<PrimitivePatchInterpolation<primitivePatch>>
      nbrPpiPtr_;

  // Private Member Functions

    //- Owner side patch interpolation
    const PrimitivePatchInterpolation<primitivePatch>& ppi() const
    {
      if (!ppiPtr_.valid()) {
        ppiPtr_.reset
        (
          new PrimitivePatchInterpolation<primitivePatch>
          {
            cyclicAMIPatch_.cyclicAMIPatch()
          }
        );
      }
      return ppiPtr_();
    }

    //- Neighbour side patch interpolation
    const PrimitivePatchInterpolation<primitivePatch>& nbrPpi() const
    {
      if (!nbrPpiPtr_.valid()) {
        nbrPpiPtr_.reset
        (
          new PrimitivePatchInterpolation<primitivePatch>
          {
            cyclicAMIPatch_.cyclicAMIPatch().neighbPatch()
          }
        );
      }
      return nbrPpiPtr_();
    }

public:

  //- Runtime type information
  TYPE_NAME(cyclicAMIPointPatch::typeName_());

  // Constructors

    //- Construct from patch and internal field
    cyclicAMIPointPatchField
    (
      const pointPatch&,
      const DimensionedField<Type, pointMesh>&
    );

    //- Construct from patch, internal field and dictionary
    cyclicAMIPointPatchField
    (
      const pointPatch&,
      const DimensionedField<Type, pointMesh>&,
      const dictionary&
    );

    //- Construct by mapping given patchField<Type> onto a new patch
    cyclicAMIPointPatchField
    (
      const cyclicAMIPointPatchField<Type>&,
      const pointPatch&,
      const DimensionedField<Type, pointMesh>&,
      const pointPatchFieldMapper&
    );

    //- Construct and return a clone
    virtual autoPtr<pointPatchField<Type>> clone() const
    {
      return autoPtr<pointPatchField<Type>>
      {
        new cyclicAMIPointPatchField<Type>{*this}
      };
    }

    //- Construct as copy setting internal field reference
    cyclicAMIPointPatchField
    (
      const cyclicAMIPointPatchField<Type>&,
      const DimensionedField<Type, pointMesh>&
    );

    //- Construct and return a clone setting internal field reference
    virtual autoPtr<pointPatchField<Type>> clone
    (
      const DimensionedField<Type, pointMesh>& iF
    ) const
    {
      return autoPtr<pointPatchField<Type>>
      {
        new cyclicAMIPointPatchField<Type>{*this, iF}
      };
    }

  // Member functions

    // Constraint handling

      //- Return the constraint type this pointPatchField implements
      virtual const word& constraintType() const
      {
        return cyclicAMIPointPatch::typeName;
      }

    // Cyclic AMI coupled interface functions

      //- Does the patch field perform the transfromation
      virtual bool doTransform() const
      {
        return !(cyclicAMIPatch_.parallel() || pTraits<Type>::rank == 0);
      }

      //- Return face transformation tensor
      virtual const tensorField& forwardT() const
      {
        return cyclicAMIPatch_.forwardT();
      }

      //- Return neighbour-cell transformation tensor
      virtual const tensorField& reverseT() const
      {
        return cyclicAMIPatch_.reverseT();
      }

    // Evaluation functions

      //- Evaluate the patch field
      virtual void evaluate
      (
        const Pstream::commsTypes=Pstream::blocking
      )
      {}

      //- Complete swap of patch point values and add to local values
      virtual void swapAddSeparated
      (
        const Pstream::commsTypes commsType,
        Field<Type>&
      ) const;
};

}  // namespace mousse

#include "cyclic_ami_point_patch_field.ipp"

#endif
