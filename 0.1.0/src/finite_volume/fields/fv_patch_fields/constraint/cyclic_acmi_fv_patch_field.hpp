#ifndef FINITE_VOLUME_FIELDS_FV_PATCH_FIELDS_CONSTRAINT_CYCLIC_ACMI_FV_PATCH_FIELD_HPP_
#define FINITE_VOLUME_FIELDS_FV_PATCH_FIELDS_CONSTRAINT_CYCLIC_ACMI_FV_PATCH_FIELD_HPP_

// mousse: CFD toolbox
// Copyright (C) 2013-2014 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::cyclicACMIFvPatchField
// Group
//   grpCoupledBoundaryConditions
// Description
//   This boundary condition enforces a cyclic condition between a pair of
//   boundaries, whereby communication between the patches is performed using
//   an arbitrarily coupled mesh interface (ACMI) interpolation.
//   \heading Patch usage
//   Example of the boundary condition specification:
//   \verbatim
//   myPatch
//   {
//     type            cyclicACMI;
//   }
//   \endverbatim
// SeeAlso
//   mousse::AMIInterpolation

#include "coupled_fv_patch_field.hpp"
#include "cyclic_acmi_ldu_interface_field.hpp"
#include "cyclic_acmi_fv_patch.hpp"


namespace mousse {

template<class Type>
class cyclicACMIFvPatchField
:
  virtual public cyclicACMILduInterfaceField,
  public coupledFvPatchField<Type>
{
  // Private data
    //- Local reference cast into the cyclic patch
    const cyclicACMIFvPatch& cyclicACMIPatch_;
  // Private Member Functions
    //- Return neighbour side field given internal fields
    template<class Type2>
    tmp<Field<Type2>> neighbourSideField
    (
      const Field<Type2>&
    ) const;
public:
  //- Runtime type information
  TYPE_NAME(cyclicACMIFvPatch::typeName_());
  // Constructors
    //- Construct from patch and internal field
    cyclicACMIFvPatchField
    (
      const fvPatch&,
      const DimensionedField<Type, volMesh>&
    );
    //- Construct from patch, internal field and dictionary
    cyclicACMIFvPatchField
    (
      const fvPatch&,
      const DimensionedField<Type, volMesh>&,
      const dictionary&
    );
    //- Construct by mapping given cyclicACMIFvPatchField onto a new patch
    cyclicACMIFvPatchField
    (
      const cyclicACMIFvPatchField<Type>&,
      const fvPatch&,
      const DimensionedField<Type, volMesh>&,
      const fvPatchFieldMapper&
    );
    //- Construct as copy
    cyclicACMIFvPatchField(const cyclicACMIFvPatchField<Type>&);
    //- Construct and return a clone
    virtual tmp<fvPatchField<Type>> clone() const
    {
      return tmp<fvPatchField<Type>>
      {
        new cyclicACMIFvPatchField<Type>{*this}
      };
    }
    //- Construct as copy setting internal field reference
    cyclicACMIFvPatchField
    (
      const cyclicACMIFvPatchField<Type>&,
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
        new cyclicACMIFvPatchField<Type>{*this, iF}
      };
    }
  // Member functions
    // Access
      //- Return local reference cast into the cyclic AMI patch
      const cyclicACMIFvPatch& cyclicACMIPatch() const
      {
        return cyclicACMIPatch_;
      }
    // Evaluation functions
      //- Return true if coupled. Note that the underlying patch
      //  is not coupled() - the points don't align
      virtual bool coupled() const;
      //- Return true if this patch field fixes a value
      //  Needed to check if a level has to be specified while solving
      //  Poissons equations
      virtual bool fixesValue() const
      {
        const scalarField& mask =
          cyclicACMIPatch_.cyclicACMIPatch().mask();
        if (gMax(mask) > 1e-5) {
          // regions connected
          return false;
        } else {
          // fully separated
          return nonOverlapPatchField().fixesValue();
        }
      }
      //- Return neighbour coupled internal cell data
      virtual tmp<Field<Type>> patchNeighbourField() const;
      //- Return reference to neighbour patchField
      const cyclicACMIFvPatchField<Type>& neighbourPatchField() const;
      //- Return reference to non-overlapping patchField
      const fvPatchField<Type>& nonOverlapPatchField() const;
      //- Update result field based on interface functionality
      virtual void updateInterfaceMatrix
      (
        scalarField& result,
        const scalarField& psiInternal,
        const scalarField& coeffs,
        const direction cmpt,
        const Pstream::commsTypes commsType
      ) const;
      //- Update result field based on interface functionality
      virtual void updateInterfaceMatrix
      (
        Field<Type>&,
        const Field<Type>&,
        const scalarField&,
        const Pstream::commsTypes commsType
      ) const;
      //- Return patch-normal gradient
      virtual tmp<Field<Type>> snGrad
      (
        const scalarField& deltaCoeffs
      ) const;
      //- Update the coefficients associated with the patch field
      void updateCoeffs();
      //- Initialise the evaluation of the patch field
      virtual void initEvaluate
      (
        const Pstream::commsTypes commsType
      );
      //- Evaluate the patch field
      virtual void evaluate
      (
        const Pstream::commsTypes commsType
      );
      //- Return the matrix diagonal coefficients corresponding to the
      //  evaluation of the value of this patchField with given weights
      virtual tmp<Field<Type>> valueInternalCoeffs
      (
        const tmp<scalarField>&
      ) const;
      //- Return the matrix source coefficients corresponding to the
      //  evaluation of the value of this patchField with given weights
      virtual tmp<Field<Type>> valueBoundaryCoeffs
      (
        const tmp<scalarField>&
      ) const;
      //- Return the matrix diagonal coefficients corresponding to the
      //  evaluation of the gradient of this patchField
      virtual tmp<Field<Type>> gradientInternalCoeffs
      (
        const scalarField& deltaCoeffs
      ) const;
      //- Return the matrix diagonal coefficients corresponding to the
      //  evaluation of the gradient of this patchField
      virtual tmp<Field<Type>> gradientInternalCoeffs() const;
      //- Return the matrix source coefficients corresponding to the
      //  evaluation of the gradient of this patchField
      virtual tmp<Field<Type>> gradientBoundaryCoeffs
      (
        const scalarField& deltaCoeffs
      ) const;
      //- Return the matrix source coefficients corresponding to the
      //  evaluation of the gradient of this patchField
      virtual tmp<Field<Type>> gradientBoundaryCoeffs() const;
      //- Manipulate matrix
      virtual void manipulateMatrix(fvMatrix<Type>& matrix);
    // Cyclic AMI coupled interface functions
      //- Does the patch field perform the transformation
      virtual bool doTransform() const
      {
        return
          !(cyclicACMIPatch_.parallel() || pTraits<Type>::rank == 0);
      }
      //- Return face transformation tensor
      virtual const tensorField& forwardT() const
      {
        return cyclicACMIPatch_.forwardT();
      }
      //- Return neighbour-cell transformation tensor
      virtual const tensorField& reverseT() const
      {
        return cyclicACMIPatch_.reverseT();
      }
      //- Return rank of component for transform
      virtual int rank() const
      {
        return pTraits<Type>::rank;
      }
    // I-O
      //- Write
      virtual void write(Ostream& os) const;
};
}  // namespace mousse

#include "cyclic_acmi_fv_patch_field.ipp"

#endif
