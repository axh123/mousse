#ifndef THERMOPHYSICAL_MODELS_RADIATION_DERIVED_FV_PATCH_FIELDS_MARSHAK_RADIATION_FV_PATCH_FIELD_HPP_
#define THERMOPHYSICAL_MODELS_RADIATION_DERIVED_FV_PATCH_FIELDS_MARSHAK_RADIATION_FV_PATCH_FIELD_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::MarshakRadiationFvPatchScalarField
// Group
//   grpThermoBoundaryConditions
// Description
//   A 'mixed' boundary condition that implements a Marshak condition for the
//   incident radiation field (usually written as G)
//   The radiation temperature is retrieved from the mesh database, using a
//   user specified temperature field name.
//   \heading Patch usage
//   \table
//     Property     | Description             | Required    | Default value
//     T            | temperature field name  | no          | T
//   \endtable
//   Example of the boundary condition specification:
//   \verbatim
//   myPatch
//   {
//     type            MarshakRadiation;
//     T               T;
//     value           uniform 0;
//   }
//   \endverbatim
// SeeAlso
//   mousse::radiationCoupledBase
//   mousse::mixedFvPatchField

#include "mixed_fv_patch_fields.hpp"
#include "radiation_coupled_base.hpp"


namespace mousse {

class MarshakRadiationFvPatchScalarField
:
  public mixedFvPatchScalarField,
  public radiationCoupledBase
{
  // Private data
    //- Name of temperature field
    word TName_;
public:
  //- Runtime type information
  TYPE_NAME("MarshakRadiation");
  // Constructors
    //- Construct from patch and internal field
    MarshakRadiationFvPatchScalarField
    (
      const fvPatch&,
      const DimensionedField<scalar, volMesh>&
    );
    //- Construct from patch, internal field and dictionary
    MarshakRadiationFvPatchScalarField
    (
      const fvPatch&,
      const DimensionedField<scalar, volMesh>&,
      const dictionary&
    );
    //- Construct by mapping given MarshakRadiationFvPatchField onto a new
    //  patch
    MarshakRadiationFvPatchScalarField
    (
      const MarshakRadiationFvPatchScalarField&,
      const fvPatch&,
      const DimensionedField<scalar, volMesh>&,
      const fvPatchFieldMapper&
    );
    //- Construct as copy
    MarshakRadiationFvPatchScalarField
    (
      const MarshakRadiationFvPatchScalarField&
    );
    //- Construct and return a clone
    virtual tmp<fvPatchScalarField> clone() const
    {
      return tmp<fvPatchScalarField>
      {
        new MarshakRadiationFvPatchScalarField{*this}
      };
    }
    //- Construct as copy setting internal field reference
    MarshakRadiationFvPatchScalarField
    (
      const MarshakRadiationFvPatchScalarField&,
      const DimensionedField<scalar, volMesh>&
    );
    //- Construct and return a clone setting internal field reference
    virtual tmp<fvPatchScalarField> clone
    (
      const DimensionedField<scalar, volMesh>& iF
    ) const
    {
      return tmp<fvPatchScalarField>
      {
        new MarshakRadiationFvPatchScalarField{*this, iF}
      };
    }
  // Member functions
    // Access
      //- Return the temperature field name
      const word& TName() const { return TName_; }
      //- Return reference to the temperature field name to allow adjustment
      word& TName() { return TName_; }
    // Mapping functions
      //- Map (and resize as needed) from self given a mapping object
      virtual void autoMap
      (
        const fvPatchFieldMapper&
      );
      //- Reverse map the given fvPatchField onto this fvPatchField
      virtual void rmap
      (
        const fvPatchScalarField&,
        const labelList&
      );
    // Evaluation functions
      //- Update the coefficients associated with the patch field
      virtual void updateCoeffs();
    // I-O
      //- Write
      virtual void write(Ostream&) const;
};

}  // namespace mousse

#endif

