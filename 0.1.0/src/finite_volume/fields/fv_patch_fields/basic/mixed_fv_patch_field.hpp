// mousse: CFD toolbox
// Copyright (C) 2011-2012 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::mixedFvPatchField
// Group
//   grpGenericBoundaryConditions
// Description
//   This boundary condition provides a base class for 'mixed' type boundary
//   conditions, i.e. conditions that mix fixed value and patch-normal gradient
//   conditions.
//   The respective contributions from each is determined by a weight field:
//     \f[
//       x_p = w x_p + (1-w) \left(x_c + \frac{\nabla_\perp x}{\Delta}\right)
//     \f]
//   where
//   \vartable
//     x_p   | patch values
//     x_c   | patch internal cell values
//     w     | weight field
//     \Delta| inverse distance from face centre to internal cell centre
//     w     | weighting (0-1)
//   \endvartable
//   \heading Patch usage
//   \table
//     Property     | Description             | Required    | Default value
//     valueFraction | weight field           | yes         |
//     refValue     | fixed value             | yes         |
//     refGrad      | patch normal gradient   | yes         |
//   \endtable
// Note
//   This condition is not usually applied directly; instead, use a derived
//   mixed condition such as \c inletOutlet
// SeeAlso
//   mousse::inletOutletFvPatchField
// SourceFiles
//   mixed_fv_patch_field.cpp

#ifndef mixed_fv_patch_field_hpp_
#define mixed_fv_patch_field_hpp_

#include "fv_patch_field.hpp"

namespace mousse
{

template<class Type>
class mixedFvPatchField
:
  public fvPatchField<Type>
{
  // Private data

    //- Value field
    Field<Type> refValue_;

    //- Normal gradient field
    Field<Type> refGrad_;

    //- Fraction (0-1) of value used for boundary condition
    scalarField valueFraction_;

public:

  //- Runtime type information
  TYPE_NAME("mixed");

  // Constructors

    //- Construct from patch and internal field
    mixedFvPatchField
    (
      const fvPatch&,
      const DimensionedField<Type, volMesh>&
    );

    //- Construct from patch, internal field and dictionary
    mixedFvPatchField
    (
      const fvPatch&,
      const DimensionedField<Type, volMesh>&,
      const dictionary&
    );

    //- Construct by mapping the given mixedFvPatchField onto a new patch
    mixedFvPatchField
    (
      const mixedFvPatchField<Type>&,
      const fvPatch&,
      const DimensionedField<Type, volMesh>&,
      const fvPatchFieldMapper&
    );

    //- Construct as copy
    mixedFvPatchField
    (
      const mixedFvPatchField<Type>&
    );

    //- Construct and return a clone
    virtual tmp<fvPatchField<Type>> clone() const
    {
      return tmp<fvPatchField<Type>>
      {
        new mixedFvPatchField<Type>{*this}
      };
    }

    //- Construct as copy setting internal field reference
    mixedFvPatchField
    (
      const mixedFvPatchField<Type>&,
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
        new mixedFvPatchField<Type>{*this, iF}
      };
    }

  // Member functions

    // Access

      //- Return true if this patch field fixes a value.
      //  Needed to check if a level has to be specified while solving
      //  Poissons equations.
      virtual bool fixesValue() const
      {
        return true;
      }

    // Return defining fields

      virtual Field<Type>& refValue()
      {
        return refValue_;
      }

      virtual const Field<Type>& refValue() const
      {
        return refValue_;
      }

      virtual Field<Type>& refGrad()
      {
        return refGrad_;
      }

      virtual const Field<Type>& refGrad() const
      {
        return refGrad_;
      }

      virtual scalarField& valueFraction()
      {
        return valueFraction_;
      }

      virtual const scalarField& valueFraction() const
      {
        return valueFraction_;
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
        const fvPatchField<Type>&,
        const labelList&
      );

    // Evaluation functions

      //- Return gradient at boundary
      virtual tmp<Field<Type> > snGrad() const;

      //- Evaluate the patch field
      virtual void evaluate
      (
        const Pstream::commsTypes commsType=Pstream::blocking
      );

      //- Return the matrix diagonal coefficients corresponding to the
      //  evaluation of the value of this patchField with given weights
      virtual tmp<Field<Type> > valueInternalCoeffs
      (
        const tmp<scalarField>&
      ) const;

      //- Return the matrix source coefficients corresponding to the
      //  evaluation of the value of this patchField with given weights
      virtual tmp<Field<Type> > valueBoundaryCoeffs
      (
        const tmp<scalarField>&
      ) const;

      //- Return the matrix diagonal coefficients corresponding to the
      //  evaluation of the gradient of this patchField
      virtual tmp<Field<Type> > gradientInternalCoeffs() const;

      //- Return the matrix source coefficients corresponding to the
      //  evaluation of the gradient of this patchField
      virtual tmp<Field<Type> > gradientBoundaryCoeffs() const;

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

#ifdef NoRepository
#   include "mixed_fv_patch_field.cpp"
#endif
#endif
