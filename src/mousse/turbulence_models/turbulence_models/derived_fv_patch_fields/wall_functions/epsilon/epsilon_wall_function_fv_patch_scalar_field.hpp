#ifndef TURBULENCE_MODELS_TURBULENCE_MODELS_DERIVED_FV_PATCH_FIELDS_WALL_FUNCTIONS_EPSILON_WALL_FUNCTIONS_EPSILON_WALL_FUNCTION_FV_PATCH_SCALAR_FIELD_HPP_
#define TURBULENCE_MODELS_TURBULENCE_MODELS_DERIVED_FV_PATCH_FIELDS_WALL_FUNCTIONS_EPSILON_WALL_FUNCTIONS_EPSILON_WALL_FUNCTION_FV_PATCH_SCALAR_FIELD_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::epsilonWallFunctionFvPatchScalarField
// Group
//   grpWallFunctions
// Description
//   This boundary condition provides a turbulence dissipation wall function
//   condition for high Reynolds number, turbulent flow cases.
//   The condition can be applied to wall boundaries, whereby it
//   - calculates \c epsilon and \c G
//   - inserts near wall epsilon values directly into the epsilon equation
//     to act as a constraint
//   where
//   \vartable
//     epsilon | turblence dissipation field
//     G       | turblence generation field
//   \endvartable
//   \heading Patch usage
//   \table
//     Property     | Description             | Required    | Default value
//     Cmu          | model coefficient       | no          | 0.09
//     kappa        | Von Karman constant     | no          | 0.41
//     E            | model coefficient       | no          | 9.8
//   \endtable
//   Example of the boundary condition specification:
//   \verbatim
//   myPatch
//   {
//     type            epsilonWallFunction;
//   }
//   \endverbatim
// SeeAlso
//   mousse::fixedInternalValueFvPatchField

#include "fixed_value_fv_patch_field.hpp"


namespace mousse {

class turbulenceModel;


class epsilonWallFunctionFvPatchScalarField
:
  public fixedValueFvPatchField<scalar>
{
protected:
  // Protected data
    //- Tolerance used in weighted calculations
    static scalar tolerance_;
    //- Cmu coefficient
    scalar Cmu_;
    //- Von Karman constant
    scalar kappa_;
    //- E coefficient
    scalar E_;
    //- Local copy of turbulence G field
    scalarField G_;
    //- Local copy of turbulence epsilon field
    scalarField epsilon_;
    //- Initialised flag
    bool initialised_;
    //- Master patch ID
    label master_;
    //- List of averaging corner weights
    List<List<scalar>> cornerWeights_;
  // Protected Member Functions
    //- Check the type of the patch
    virtual void checkType();
    //- Write local wall function variables
    virtual void writeLocalEntries(Ostream&) const;
    //- Set the master patch - master is responsible for updating all
    //  wall function patches
    virtual void setMaster();
    //- Create the averaging weights for cells which are bounded by
    //  multiple wall function faces
    virtual void createAveragingWeights();
    //- Helper function to return non-const access to an epsilon patch
    virtual epsilonWallFunctionFvPatchScalarField& epsilonPatch
    (
      const label patchi
    );
    //- Main driver to calculate the turbulence fields
    virtual void calculateTurbulenceFields
    (
      const turbulenceModel& turbulence,
      scalarField& G0,
      scalarField& epsilon0
    );
    //- Calculate the epsilon and G
    virtual void calculate
    (
      const turbulenceModel& turbulence,
      const List<scalar>& cornerWeights,
      const fvPatch& patch,
      scalarField& G,
      scalarField& epsilon
    );
    //- Return non-const access to the master patch ID
    virtual label& master()
    {
      return master_;
    }
public:
  //- Runtime type information
  TYPE_NAME("epsilonWallFunction");
  // Constructors
    //- Construct from patch and internal field
    epsilonWallFunctionFvPatchScalarField
    (
      const fvPatch&,
      const DimensionedField<scalar, volMesh>&
    );
    //- Construct from patch, internal field and dictionary
    epsilonWallFunctionFvPatchScalarField
    (
      const fvPatch&,
      const DimensionedField<scalar, volMesh>&,
      const dictionary&
    );
    //- Construct by mapping given
    //  epsilonWallFunctionFvPatchScalarField
    //  onto a new patch
    epsilonWallFunctionFvPatchScalarField
    (
      const epsilonWallFunctionFvPatchScalarField&,
      const fvPatch&,
      const DimensionedField<scalar, volMesh>&,
      const fvPatchFieldMapper&
    );
    //- Construct as copy
    epsilonWallFunctionFvPatchScalarField
    (
      const epsilonWallFunctionFvPatchScalarField&
    );
    //- Construct and return a clone
    virtual tmp<fvPatchScalarField> clone() const
    {
      return
        tmp<fvPatchScalarField>
        {
          new epsilonWallFunctionFvPatchScalarField{*this}
        };
    }
    //- Construct as copy setting internal field reference
    epsilonWallFunctionFvPatchScalarField
    (
      const epsilonWallFunctionFvPatchScalarField&,
      const DimensionedField<scalar, volMesh>&
    );
    //- Construct and return a clone setting internal field reference
    virtual tmp<fvPatchScalarField> clone
    (
      const DimensionedField<scalar, volMesh>& iF
    ) const
    {
      return
        tmp<fvPatchScalarField>
        {
          new epsilonWallFunctionFvPatchScalarField{*this, iF}
        };
    }
  //- Destructor
  virtual ~epsilonWallFunctionFvPatchScalarField()
  {}
  // Member functions
    // Access
      //- Return non-const access to the master's G field
      scalarField& G(bool init = false);
      //- Return non-const access to the master's epsilon field
      scalarField& epsilon(bool init = false);
    // Evaluation functions
      //- Update the coefficients associated with the patch field
      virtual void updateCoeffs();
      //- Update the coefficients associated with the patch field
      virtual void updateCoeffs(const scalarField& weights);
      //- Manipulate matrix
      virtual void manipulateMatrix(fvMatrix<scalar>& matrix);
      //- Manipulate matrix with given weights
      virtual void manipulateMatrix
      (
        fvMatrix<scalar>& matrix,
        const scalarField& weights
      );
    // I-O
      //- Write
      virtual void write(Ostream&) const;
};

}  // namespace mousse

#endif
