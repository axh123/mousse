#ifndef CORE_FIELDS_GEOMETRIC_FIELDS_GEOMETRIC_FIELD_HPP_
#define CORE_FIELDS_GEOMETRIC_FIELDS_GEOMETRIC_FIELD_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::GeometricField
// Description
//   Generic GeometricField class.

#include "reg_ioobject.hpp"
#include "dimensioned_types.hpp"
#include "dimensioned_field.hpp"
#include "field_field.hpp"
#include "ldu_interface_field_ptrs_list.hpp"
#include "_ldu_interface_field_ptrs_list.hpp"


namespace mousse {

class dictionary;

// Forward declaration of friend functions and operators
template<class Type, template<class> class PatchField, class GeoMesh>
class GeometricField;

template<class Type, template<class> class PatchField, class GeoMesh>
Ostream& operator<<
(
  Ostream&,
  const GeometricField<Type, PatchField, GeoMesh>&
);

template<class Type, template<class> class PatchField, class GeoMesh>
Ostream& operator<<
(
  Ostream&,
  const tmp<GeometricField<Type, PatchField, GeoMesh>>&
);


template<class Type, template<class> class PatchField, class GeoMesh>
class GeometricField
:
  public DimensionedField<Type, GeoMesh>
{
  // Private Member Functions
    //- Read from file if it is present
    bool readIfPresent();
    //- Read old time field from file if it is present
    bool readOldTimeIfPresent();
public:
  // Public typedefs
    typedef typename GeoMesh::Mesh Mesh;
    typedef typename GeoMesh::BoundaryMesh BoundaryMesh;
    typedef DimensionedField<Type, GeoMesh> DimensionedInternalField;
    typedef Field<Type> InternalField;
    typedef PatchField<Type> PatchFieldType;
  class GeometricBoundaryField
  :
    public FieldField<PatchField, Type>
  {
    // Private data
      //- Reference to BoundaryMesh for which this field is defined
      const BoundaryMesh& bmesh_;
  public:
    // Constructors
      //- Construct from a BoundaryMesh
      GeometricBoundaryField(const BoundaryMesh&);
      //- Construct from a BoundaryMesh,
      //  reference to the internal field
      //  and a patch type
      GeometricBoundaryField
      (
        const BoundaryMesh&,
        const DimensionedInternalField&,
        const word&
      );
      //- Construct from a BoundaryMesh,
      //  reference to the internal field
      //  and a wordList of patch types and optional the actual patch
      //  types (to override constraint patches)
      GeometricBoundaryField
      (
        const BoundaryMesh&,
        const DimensionedInternalField&,
        const wordList& wantedPatchTypes,
        const wordList& actualPatchTypes = wordList()
      );
      //- Construct from a BoundaryMesh,
      //  reference to the internal field
      //  and a PtrList<PatchField<Type>>
      GeometricBoundaryField
      (
        const BoundaryMesh&,
        const DimensionedInternalField&,
        const PtrList<PatchField<Type>>&
      );
      //- Construct as copy setting the reference to the internal field
      GeometricBoundaryField
      (
        const DimensionedInternalField&,
        const GeometricBoundaryField&
      );
      //- Construct as copy
      //  Dangerous because Field may be set to a field which gets deleted
      //  Need new type of BoundaryField, one which is part of a geometric
      //  field for which snGrad etc. may be called and a free standing
      //  BoundaryField for which such operations are unavailable.
      GeometricBoundaryField
      (
        const GeometricBoundaryField&
      );
      //- Construct from dictionary
      GeometricBoundaryField
      (
        const BoundaryMesh&,
        const DimensionedInternalField&,
        const dictionary&
      );
    // Member functions
      //- Read the boundary field
      void readField
      (
        const DimensionedField<Type, GeoMesh>& field,
        const dictionary& dict
      );
      //- Update the boundary condition coefficients
      void updateCoeffs();
      //- Evaluate boundary conditions
      void evaluate();
      //- Return a list of the patch types
      wordList types() const;
      //- Return BoundaryField of the cell values neighbouring
      //  the boundary
      GeometricBoundaryField boundaryInternalField() const;
      //- Return a list of pointers for each patch field with only those
      //  pointing to interfaces being set
      LduInterfaceFieldPtrsList<Type> interfaces() const;
      //- Return a list of pointers for each patch field with only those
      //  pointing to interfaces being set
      lduInterfaceFieldPtrsList scalarInterfaces() const;
      //- Write boundary field as dictionary entry
      void writeEntry(const word& keyword, Ostream& os) const;
    // Member operators
      //- Assignment to BoundaryField<Type, PatchField, BoundaryMesh>
      void operator=(const GeometricBoundaryField&);
      //- Assignment to FieldField<PatchField, Type>
      void operator=(const FieldField<PatchField, Type>&);
      //- Assignment to Type
      void operator=(const Type&);
      //- Forced assignment to
      //  BoundaryField<Type, PatchField, BoundaryMesh>
      void operator==(const GeometricBoundaryField&);
      //- Forced assignment to FieldField<PatchField, Type>
      void operator==(const FieldField<PatchField, Type>&);
      //- Forced assignment to Type
      void operator==(const Type&);
  };
private:
  // Private data
    //- Current time index.
    //  Used to trigger the storing of the old-time value
    mutable label timeIndex_;
    //- Pointer to old time field
    mutable GeometricField<Type, PatchField, GeoMesh>* field0Ptr_;
    //-  Pointer to previous iteration (used for under-relaxation)
    mutable GeometricField<Type, PatchField, GeoMesh>* fieldPrevIterPtr_;
    //- Boundary Type field containing boundary field values
    GeometricBoundaryField boundaryField_;
  // Private Member Functions
    //- Read the field from the dictionary
    void readFields(const dictionary&);
    //- Read the field - create the field dictionary on-the-fly
    void readFields();
public:
  //- Runtime type information
  TYPE_NAME("GeometricField");
  // Public typedefs
    typedef typename Field<Type>::cmptType cmptType;
  // Static Member Functions
    //- Return a null geometric field
    inline static const GeometricField<Type, PatchField, GeoMesh>& null();
  // Constructors
    //- Constructor given IOobject, mesh, dimensions and patch type.
    //  This allocates storage for the field but not values.
    //  Used only within this class to create TEMPORARY variables
    GeometricField
    (
      const IOobject&,
      const Mesh&,
      const dimensionSet&,
      const word& patchFieldType=PatchField<Type>::calculatedType()
    );
    //- Constructor given IOobject, mesh, dimensions and patch types.
    //  This allocates storage for the field but not values.
    //  Used only within this class to create TEMPORARY variables
    GeometricField
    (
      const IOobject&,
      const Mesh&,
      const dimensionSet&,
      const wordList& wantedPatchTypes,
      const wordList& actualPatchTypes = wordList()
    );
    //- Constructor given IOobject, mesh, dimensioned<Type> and patch type.
    GeometricField
    (
      const IOobject&,
      const Mesh&,
      const dimensioned<Type>&,
      const word& patchFieldType=PatchField<Type>::calculatedType()
    );
    //- Constructor given IOobject, mesh, dimensioned<Type> and patch types.
    GeometricField
    (
      const IOobject&,
      const Mesh&,
      const dimensioned<Type>&,
      const wordList& wantedPatchTypes,
      const wordList& actualPatchTypes = wordList()
    );
    //- Constructor from components
    GeometricField
    (
      const IOobject&,
      const Mesh&,
      const dimensionSet&,
      const Field<Type>&,
      const PtrList<PatchField<Type>>&
    );
    //- Construct and read given IOobject
    GeometricField
    (
      const IOobject&,
      const Mesh&
    );
    //- Construct from dictionary
    GeometricField
    (
      const IOobject&,
      const Mesh&,
      const dictionary&
    );
    //- Construct as copy
    GeometricField
    (
      const GeometricField<Type, PatchField, GeoMesh>&
    );
    //- Construct as copy of tmp<GeometricField> deleting argument
    #ifndef NoConstructFromTmp
    GeometricField
    (
      const tmp<GeometricField<Type, PatchField, GeoMesh>>&
    );
    #endif
    //- Construct as copy resetting IO parameters
    GeometricField
    (
      const IOobject&,
      const GeometricField<Type, PatchField, GeoMesh>&
    );
    //- Construct as copy of tmp<GeometricField> resetting IO parameters
    #ifndef NoConstructFromTmp
    GeometricField
    (
      const IOobject&,
      const tmp<GeometricField<Type, PatchField, GeoMesh>>&
    );
    #endif
    //- Construct as copy resetting name
    GeometricField
    (
      const word& newName,
      const GeometricField<Type, PatchField, GeoMesh>&
    );
    //- Construct as copy resetting name
    #ifndef NoConstructFromTmp
    GeometricField
    (
      const word& newName,
      const tmp<GeometricField<Type, PatchField, GeoMesh>>&
    );
    #endif
    //- Construct as copy resetting IO parameters and patch type
    GeometricField
    (
      const IOobject&,
      const GeometricField<Type, PatchField, GeoMesh>&,
      const word& patchFieldType
    );
    //- Construct as copy resetting IO parameters and boundary types
    GeometricField
    (
      const IOobject&,
      const GeometricField<Type, PatchField, GeoMesh>&,
      const wordList& patchFieldTypes,
      const wordList& actualPatchTypes = wordList()
    );
  //- Destructor
  virtual ~GeometricField();
  // Member Functions
    //- Return dimensioned internal field
    DimensionedInternalField& dimensionedInternalField();
    //- Return dimensioned internal field
    inline const DimensionedInternalField& dimensionedInternalField() const;
    //- Return internal field
    InternalField& internalField();
    //- Return internal field
    inline const InternalField& internalField() const;
    //- Return reference to GeometricBoundaryField
    GeometricBoundaryField& boundaryField();
    //- Return reference to GeometricBoundaryField for const field
    inline const GeometricBoundaryField& boundaryField() const;
    //- Return the time index of the field
    inline label timeIndex() const;
    //- Return the time index of the field
    inline label& timeIndex();
    //- Store the old-time fields
    void storeOldTimes() const;
    //- Store the old-time field
    void storeOldTime() const;
    //- Return the number of old time fields stored
    label nOldTimes() const;
    //- Return old time field
    const GeometricField<Type, PatchField, GeoMesh>& oldTime() const;
    //- Return non-const old time field
    //  (Not a good idea but it is used for sub-cycling)
    GeometricField<Type, PatchField, GeoMesh>& oldTime();
    //- Store the field as the previous iteration value
    void storePrevIter() const;
    //- Return previous iteration field
    const GeometricField<Type, PatchField, GeoMesh>& prevIter() const;
    //- Correct boundary field
    void correctBoundaryConditions();
    //- Does the field need a reference level for solution
    bool needReference() const;
    //- Return a component of the field
    tmp<GeometricField<cmptType, PatchField, GeoMesh>> component
    (
      const direction
    ) const;
    //- WriteData member function required by regIOobject
    bool writeData(Ostream&) const;
    //- Return transpose (only if it is a tensor field)
    tmp<GeometricField<Type, PatchField, GeoMesh>> T() const;
    //- Relax field (for steady-state solution).
    //  alpha = 1 : no relaxation
    //  alpha < 1 : relaxation
    //  alpha = 0 : do nothing
    void relax(const scalar alpha);
    //- Relax field (for steady-state solution).
    //  alpha is read from controlDict
    void relax();
    //- Select the final iteration parameters if `final' is true
    //  by returning the field name + "Final"
    //  otherwise the standard parameters by returning the field name
    word select(bool final) const;
    //- Helper function to write the min and max to an Ostream
    void writeMinMax(Ostream& os) const;
  // Member function *this operators
    void negate();
    void replace
    (
      const direction,
      const GeometricField<cmptType, PatchField, GeoMesh>&
    );
    void replace
    (
      const direction,
      const dimensioned<cmptType>&
    );
    void max(const dimensioned<Type>&);
    void min(const dimensioned<Type>&);
    void max
    (
      const GeometricField<Type, PatchField, GeoMesh>&,
      const dimensioned<Type>&
    );
    void min
    (
      const GeometricField<Type, PatchField, GeoMesh>&,
      const dimensioned<Type>&
    );
    void scale
    (
      const GeometricField<Type, PatchField, GeoMesh>&,
      const GeometricField<Type, PatchField, GeoMesh>&
    );
    void scale
    (
      const GeometricField<Type, PatchField, GeoMesh>&,
      const dimensioned<Type>&
    );
  // Member operators
    void operator=(const GeometricField<Type, PatchField, GeoMesh>&);
    void operator=(const tmp<GeometricField<Type, PatchField, GeoMesh>>&);
    void operator=(const dimensioned<Type>&);
    void operator==(const tmp<GeometricField<Type, PatchField, GeoMesh>>&);
    void operator==(const dimensioned<Type>&);
    void operator+=(const GeometricField<Type, PatchField, GeoMesh>&);
    void operator+=(const tmp<GeometricField<Type, PatchField, GeoMesh>>&);
    void operator-=(const GeometricField<Type, PatchField, GeoMesh>&);
    void operator-=(const tmp<GeometricField<Type, PatchField, GeoMesh>>&);
    void operator*=(const GeometricField<scalar, PatchField, GeoMesh>&);
    void operator*=(const tmp<GeometricField<scalar,PatchField,GeoMesh>>&);
    void operator/=(const GeometricField<scalar, PatchField, GeoMesh>&);
    void operator/=(const tmp<GeometricField<scalar,PatchField,GeoMesh>>&);
    void operator+=(const dimensioned<Type>&);
    void operator-=(const dimensioned<Type>&);
    void operator*=(const dimensioned<scalar>&);
    void operator/=(const dimensioned<scalar>&);
  // Ostream operators
    friend Ostream& operator<< <Type, PatchField, GeoMesh>
    (
      Ostream&,
      const GeometricField<Type, PatchField, GeoMesh>&
    );
    friend Ostream& operator<< <Type, PatchField, GeoMesh>
    (
      Ostream&,
      const tmp<GeometricField<Type, PatchField, GeoMesh>>&
    );
};
template<class Type, template<class> class PatchField, class GeoMesh>
Ostream& operator<<
(
  Ostream&,
  const typename GeometricField<Type, PatchField, GeoMesh>::
  GeometricBoundaryField&
);
}  // namespace mousse

// Member Functions 
template<class Type, template<class> class PatchField, class GeoMesh>
inline const mousse::GeometricField<Type, PatchField, GeoMesh>&
mousse::GeometricField<Type, PatchField, GeoMesh>::null()
{
  return NullObjectRef<GeometricField<Type, PatchField, GeoMesh>>();
}
template<class Type, template<class> class PatchField, class GeoMesh>
inline
const typename
mousse::GeometricField<Type, PatchField, GeoMesh>::DimensionedInternalField&
mousse::GeometricField<Type, PatchField, GeoMesh>::
dimensionedInternalField() const
{
  return *this;
}
template<class Type, template<class> class PatchField, class GeoMesh>
inline
const typename
mousse::GeometricField<Type, PatchField, GeoMesh>::InternalField&
mousse::GeometricField<Type, PatchField, GeoMesh>::internalField() const
{
  return *this;
}
template<class Type, template<class> class PatchField, class GeoMesh>
inline const typename mousse::GeometricField<Type, PatchField, GeoMesh>::
GeometricBoundaryField&
mousse::GeometricField<Type, PatchField, GeoMesh>::boundaryField() const
{
  return boundaryField_;
}
template<class Type, template<class> class PatchField, class GeoMesh>
inline mousse::label
mousse::GeometricField<Type, PatchField, GeoMesh>::timeIndex() const
{
  return timeIndex_;
}
template<class Type, template<class> class PatchField, class GeoMesh>
inline mousse::label&
mousse::GeometricField<Type, PatchField, GeoMesh>::timeIndex()
{
  return timeIndex_;
}

#include "geometric_field.ipp"
#include "geometric_field_functions.hpp"

#endif
