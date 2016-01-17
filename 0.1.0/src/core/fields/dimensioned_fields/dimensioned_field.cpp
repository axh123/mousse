// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "dimensioned_field.hpp"
#include "dimensioned_type.hpp"

namespace mousse
{

// check mesh for two fields
#define CHECK_FIELD(df1, df2, op)                                             \
if (&(df1).mesh() != &(df2).mesh())                                           \
{                                                                             \
  FATAL_ERROR_IN("checkField(df1, df2, op)")                                  \
    << "different mesh for fields "                                           \
    << (df1).name() << " and " << (df2).name()                                \
    << " during operatrion " <<  op                                           \
    << abort(FatalError);                                                     \
}


// Constructors
template<class Type, class GeoMesh>
DimensionedField<Type, GeoMesh>::DimensionedField
(
  const IOobject& io,
  const Mesh& mesh,
  const dimensionSet& dims,
  const Field<Type>& field
)
:
  regIOobject{io},
  Field<Type>{field},
  mesh_{mesh},
  dimensions_{dims}
{
  if (field.size() && field.size() != GeoMesh::size(mesh))
  {
    FATAL_ERROR_IN
    (
      "DimensionedField<Type, GeoMesh>::DimensionedField"
      "(const IOobject& io,const Mesh& mesh, "
      "const dimensionSet& dims, const Field<Type>& field)"
    )
    << "size of field = " << field.size()
    << " is not the same as the size of mesh = "
    << GeoMesh::size(mesh)
    << abort(FatalError);
  }
}


template<class Type, class GeoMesh>
DimensionedField<Type, GeoMesh>::DimensionedField
(
  const IOobject& io,
  const Mesh& mesh,
  const dimensionSet& dims,
  const bool checkIOFlags
)
:
  regIOobject{io},
  Field<Type>{GeoMesh::size(mesh)},
  mesh_{mesh},
  dimensions_{dims}
{
  if (checkIOFlags)
  {
    readIfPresent();
  }
}


template<class Type, class GeoMesh>
DimensionedField<Type, GeoMesh>::DimensionedField
(
  const IOobject& io,
  const Mesh& mesh,
  const dimensioned<Type>& dt,
  const bool checkIOFlags
)
:
  regIOobject{io},
  Field<Type>{GeoMesh::size(mesh), dt.value()},
  mesh_{mesh},
  dimensions_{dt.dimensions()}
{
  if (checkIOFlags)
  {
    readIfPresent();
  }
}


template<class Type, class GeoMesh>
DimensionedField<Type, GeoMesh>::DimensionedField
(
  const DimensionedField<Type, GeoMesh>& df
)
:
  regIOobject{df},
  Field<Type>{df},
  mesh_{df.mesh_},
  dimensions_{df.dimensions_}
{}


template<class Type, class GeoMesh>
DimensionedField<Type, GeoMesh>::DimensionedField
(
  DimensionedField<Type, GeoMesh>& df,
  bool reUse
)
:
  regIOobject{df, reUse},
  Field<Type>{df, reUse},
  mesh_{df.mesh_},
  dimensions_{df.dimensions_}
{}


template<class Type, class GeoMesh>
DimensionedField<Type, GeoMesh>::DimensionedField
(
  const Xfer<DimensionedField<Type, GeoMesh>>& df
)
:
  regIOobject{df(), true},
  Field<Type>{df},
  mesh_{df->mesh_},
  dimensions_{df->dimensions_}
{}


#ifndef NoConstructFromTmp
template<class Type, class GeoMesh>
DimensionedField<Type, GeoMesh>::DimensionedField
(
  const tmp<DimensionedField<Type, GeoMesh>>& tdf
)
:
  regIOobject{tdf(), tdf.isTmp()},
  Field<Type>
  {
    const_cast<DimensionedField<Type, GeoMesh>&>(tdf()),
    tdf.isTmp()
  },
  mesh_{tdf().mesh_},
  dimensions_{tdf().dimensions_}
{
  tdf.clear();
}
#endif


template<class Type, class GeoMesh>
DimensionedField<Type, GeoMesh>::DimensionedField
(
  const IOobject& io,
  const DimensionedField<Type, GeoMesh>& df
)
:
  regIOobject{io},
  Field<Type>{df},
  mesh_{df.mesh_},
  dimensions_{df.dimensions_}
{}


template<class Type, class GeoMesh>
DimensionedField<Type, GeoMesh>::DimensionedField
(
  const IOobject& io,
  DimensionedField<Type, GeoMesh>& df,
  bool reUse
)
:
  regIOobject{io, df},
  Field<Type>{df, reUse},
  mesh_{df.mesh_},
  dimensions_{df.dimensions_}
{}


template<class Type, class GeoMesh>
DimensionedField<Type, GeoMesh>::DimensionedField
(
  const word& newName,
  const DimensionedField<Type, GeoMesh>& df
)
:
  regIOobject{newName, df, newName == df.name()},
  Field<Type>{df},
  mesh_{df.mesh_},
  dimensions_{df.dimensions_}
{}


template<class Type, class GeoMesh>
DimensionedField<Type, GeoMesh>::DimensionedField
(
  const word& newName,
  DimensionedField<Type, GeoMesh>& df,
  bool reUse
)
:
  regIOobject{newName, df, true},
  Field<Type>{df, reUse},
  mesh_{df.mesh_},
  dimensions_{df.dimensions_}
{}


template<class Type, class GeoMesh>
DimensionedField<Type, GeoMesh>::DimensionedField
(
  const word& newName,
  const Xfer<DimensionedField<Type, GeoMesh>>& df
)
:
  regIOobject{newName, df, true},
  Field<Type>{df},
  mesh_{df->mesh_},
  dimensions_{df->dimensions_}
{}


#ifndef NoConstructFromTmp
template<class Type, class GeoMesh>
DimensionedField<Type, GeoMesh>::DimensionedField
(
  const word& newName,
  const tmp<DimensionedField<Type, GeoMesh>>& tdf
)
:
  regIOobject{newName, tdf(), true},
  Field<Type>
  {
    const_cast<DimensionedField<Type, GeoMesh>&>(tdf()),
    tdf.isTmp()
  },
  mesh_{tdf().mesh_},
  dimensions_{tdf().dimensions_}
{
  tdf().clear();
}
#endif


template<class Type, class GeoMesh>
tmp<DimensionedField<Type, GeoMesh>>
DimensionedField<Type, GeoMesh>::clone() const
{
  return tmp<DimensionedField<Type, GeoMesh>>
  {
    new DimensionedField<Type, GeoMesh>(*this)
  };
}


// Destructor
template<class Type, class GeoMesh>
DimensionedField<Type, GeoMesh>::~DimensionedField()
{}


// Member Functions
template<class Type, class GeoMesh>
tmp
<
  DimensionedField
    <typename DimensionedField<Type, GeoMesh>::cmptType, GeoMesh>
>
DimensionedField<Type, GeoMesh>::component
(
  const direction d
) const
{
  tmp<DimensionedField<cmptType, GeoMesh>> result
  {
    new DimensionedField<cmptType, GeoMesh>
    {
      IOobject
      {
        name() + ".component(" + ::mousse::name(d) + ')',
        instance(),
        db()
      },
      mesh_,
      dimensions_
    }
  };
  mousse::component(result(), *this, d);
  return result;
}


template<class Type, class GeoMesh>
void DimensionedField<Type, GeoMesh>::replace
(
  const direction d,
  const DimensionedField
    <typename DimensionedField<Type, GeoMesh>::cmptType, GeoMesh>& df
)
{
  Field<Type>::replace(d, df);
}


template<class Type, class GeoMesh>
void DimensionedField<Type, GeoMesh>::replace
(
  const direction d,
  const tmp
  <
    DimensionedField
      <typename DimensionedField<Type, GeoMesh>::cmptType, GeoMesh>
  >& tdf
)
{
  replace(d, tdf());
  tdf.clear();
}


template<class Type, class GeoMesh>
tmp<DimensionedField<Type, GeoMesh>>
DimensionedField<Type, GeoMesh>::T() const
{
  tmp<DimensionedField<Type, GeoMesh>> result
  {
    new DimensionedField<Type, GeoMesh>
    {
      IOobject
      {
        name() + ".T()",
        instance(),
        db()
      },
      mesh_,
      dimensions_
    }
  };
  mousse::T(result(), *this);
  return result;
}


template<class Type, class GeoMesh>
dimensioned<Type> DimensionedField<Type, GeoMesh>::average() const
{
  dimensioned<Type> Average
  {
    this->name() + ".average()",
    this->dimensions(),
    gAverage(field())
  };

  return Average;
}


template<class Type, class GeoMesh>
dimensioned<Type> DimensionedField<Type, GeoMesh>::weightedAverage
(
  const DimensionedField<scalar, GeoMesh>& weightField
) const
{
  return dimensioned<Type>
  {
    this->name() + ".weightedAverage(weights)",
    this->dimensions(),
    gSum(weightField*field())/gSum(weightField)
  };
}


template<class Type, class GeoMesh>
dimensioned<Type> DimensionedField<Type, GeoMesh>::weightedAverage
(
  const tmp<DimensionedField<scalar, GeoMesh>>& tweightField
) const
{
  dimensioned<Type> wa = weightedAverage(tweightField());
  tweightField.clear();
  return wa;
}


// Member Operators
template<class Type, class GeoMesh>
void DimensionedField<Type, GeoMesh>::operator=
(
  const DimensionedField<Type, GeoMesh>& df
)
{
  // Check for assignment to self
  if (this == &df)
  {
    FATAL_ERROR_IN
    (
      "DimensionedField<Type, GeoMesh>::operator="
      "(const DimensionedField<Type, GeoMesh>&)"
    )
    << "attempted assignment to self"
    << abort(FatalError);
  }
  CHECK_FIELD(*this, df, "=");
  dimensions_ = df.dimensions();
  Field<Type>::operator=(df);
}


template<class Type, class GeoMesh>
void DimensionedField<Type, GeoMesh>::operator=
(
  const tmp<DimensionedField<Type, GeoMesh>>& tdf
)
{
  const DimensionedField<Type, GeoMesh>& df = tdf();
  // Check for assignment to self
  if (this == &df)
  {
    FATAL_ERROR_IN
    (
      "DimensionedField<Type, GeoMesh>::operator="
      "(const tmp<DimensionedField<Type, GeoMesh>>&)"
    )
    << "attempted assignment to self"
    << abort(FatalError);
  }
  CHECK_FIELD(*this, df, "=");
  dimensions_ = df.dimensions();
  this->transfer(const_cast<DimensionedField<Type, GeoMesh>&>(df));
  tdf.clear();
}


template<class Type, class GeoMesh>
void DimensionedField<Type, GeoMesh>::operator=
(
  const dimensioned<Type>& dt
)
{
  dimensions_ = dt.dimensions();
  Field<Type>::operator=(dt.value());
}


#define COMPUTED_ASSIGNMENT(TYPE, op)                                         \
                                                                              \
template<class Type, class GeoMesh>                                           \
void DimensionedField<Type, GeoMesh>::operator op                             \
(                                                                             \
  const DimensionedField<TYPE, GeoMesh>& df                                   \
)                                                                             \
{                                                                             \
  CHECK_FIELD(*this, df, #op);                                                \
                                                                              \
  dimensions_ op df.dimensions();                                             \
  Field<Type>::operator op(df);                                               \
}                                                                             \
                                                                              \
template<class Type, class GeoMesh>                                           \
void DimensionedField<Type, GeoMesh>::operator op                             \
(                                                                             \
  const tmp<DimensionedField<TYPE, GeoMesh>>& tdf                             \
)                                                                             \
{                                                                             \
  operator op(tdf());                                                         \
  tdf.clear();                                                                \
}                                                                             \
                                                                              \
template<class Type, class GeoMesh>                                           \
void DimensionedField<Type, GeoMesh>::operator op                             \
(                                                                             \
  const dimensioned<TYPE>& dt                                                 \
)                                                                             \
{                                                                             \
  dimensions_ op dt.dimensions();                                             \
  Field<Type>::operator op(dt.value());                                       \
}

COMPUTED_ASSIGNMENT(Type, +=)
COMPUTED_ASSIGNMENT(Type, -=)
COMPUTED_ASSIGNMENT(scalar, *=)
COMPUTED_ASSIGNMENT(scalar, /=)
#undef COMPUTED_ASSIGNMENT
#undef CHECK_FIELD

}  // namespace mousse

#include "dimensioned_field_io.cpp"
#include "dimensioned_field_functions.cpp"
