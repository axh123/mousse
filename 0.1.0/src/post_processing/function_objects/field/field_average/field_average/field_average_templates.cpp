// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "field_average_item.hpp"
#include "vol_fields.hpp"
#include "surface_fields.hpp"
#include "ofstream.hpp"
// Private Member Functions 
template<class Type>
void mousse::fieldAverage::addMeanFieldType(const label fieldI)
{
  faItems_[fieldI].active() = true;
  const word& fieldName = faItems_[fieldI].fieldName();
  const word& meanFieldName = faItems_[fieldI].meanFieldName();
  Info<< "    Reading/initialising field " << meanFieldName << endl;
  if (obr_.foundObject<Type>(meanFieldName))
  {
   // do nothing
  }
  else if (obr_.found(meanFieldName))
  {
    Info<< "    Cannot allocate average field " << meanFieldName
      << " since an object with that name already exists."
      << " Disabling averaging for field." << endl;
    faItems_[fieldI].mean() = false;
  }
  else
  {
    const Type& baseField = obr_.lookupObject<Type>(fieldName);
    // Store on registry
    obr_.store
    (
      new Type
      (
        IOobject
        (
          meanFieldName,
          obr_.time().timeName(obr_.time().startTime().value()),
          obr_,
          resetOnOutput_
         ? IOobject::NO_READ
         : IOobject::READ_IF_PRESENT,
          IOobject::NO_WRITE
        ),
        1*baseField
      )
    );
  }
}
template<class Type>
void mousse::fieldAverage::addMeanField(const label fieldI)
{
  if (faItems_[fieldI].mean())
  {
    typedef GeometricField<Type, fvPatchField, volMesh> volFieldType;
    typedef GeometricField<Type, fvsPatchField, surfaceMesh> surfFieldType;
    const word& fieldName = faItems_[fieldI].fieldName();
    if (obr_.foundObject<volFieldType>(fieldName))
    {
      addMeanFieldType<volFieldType>(fieldI);
    }
    else if (obr_.foundObject<surfFieldType>(fieldName))
    {
      addMeanFieldType<surfFieldType>(fieldI);
    }
  }
}
template<class Type1, class Type2>
void mousse::fieldAverage::addPrime2MeanFieldType(const label fieldI)
{
  const word& fieldName = faItems_[fieldI].fieldName();
  const word& meanFieldName = faItems_[fieldI].meanFieldName();
  const word& prime2MeanFieldName = faItems_[fieldI].prime2MeanFieldName();
  Info<< "    Reading/initialising field " << prime2MeanFieldName << nl;
  if (obr_.foundObject<Type2>(prime2MeanFieldName))
  {
    // do nothing
  }
  else if (obr_.found(prime2MeanFieldName))
  {
    Info<< "    Cannot allocate average field " << prime2MeanFieldName
      << " since an object with that name already exists."
      << " Disabling averaging for field." << nl;
    faItems_[fieldI].prime2Mean() = false;
  }
  else
  {
    const Type1& baseField = obr_.lookupObject<Type1>(fieldName);
    const Type1& meanField = obr_.lookupObject<Type1>(meanFieldName);
    // Store on registry
    obr_.store
    (
      new Type2
      (
        IOobject
        (
          prime2MeanFieldName,
          obr_.time().timeName(obr_.time().startTime().value()),
          obr_,
          resetOnOutput_
         ? IOobject::NO_READ
         : IOobject::READ_IF_PRESENT,
          IOobject::NO_WRITE
        ),
        sqr(baseField) - sqr(meanField)
      )
    );
  }
}
template<class Type1, class Type2>
void mousse::fieldAverage::addPrime2MeanField(const label fieldI)
{
  typedef GeometricField<Type1, fvPatchField, volMesh> volFieldType1;
  typedef GeometricField<Type1, fvsPatchField, surfaceMesh> surfFieldType1;
  typedef GeometricField<Type2, fvPatchField, volMesh> volFieldType2;
  typedef GeometricField<Type2, fvsPatchField, surfaceMesh> surfFieldType2;
  if (faItems_[fieldI].prime2Mean())
  {
    const word& fieldName = faItems_[fieldI].fieldName();
    if (!faItems_[fieldI].mean())
    {
      FATAL_ERROR_IN
      (
        "void mousse::fieldAverage::addPrime2MeanField(const label) const"
      )
        << "To calculate the prime-squared average, the "
        << "mean average must also be selected for field "
        << fieldName << nl << exit(FatalError);
    }
    if (obr_.foundObject<volFieldType1>(fieldName))
    {
      addPrime2MeanFieldType<volFieldType1, volFieldType2>(fieldI);
    }
    else if (obr_.foundObject<surfFieldType1>(fieldName))
    {
      addPrime2MeanFieldType<surfFieldType1, surfFieldType2>(fieldI);
    }
  }
}
template<class Type>
void mousse::fieldAverage::calculateMeanFieldType(const label fieldI) const
{
  const word& fieldName = faItems_[fieldI].fieldName();
  if (obr_.foundObject<Type>(fieldName))
  {
    const Type& baseField = obr_.lookupObject<Type>(fieldName);
    Type& meanField = const_cast<Type&>
    (
      obr_.lookupObject<Type>(faItems_[fieldI].meanFieldName())
    );
    scalar dt = obr_.time().deltaTValue();
    scalar Dt = totalTime_[fieldI];
    if (faItems_[fieldI].iterBase())
    {
      dt = 1.0;
      Dt = scalar(totalIter_[fieldI]);
    }
    scalar alpha = (Dt - dt)/Dt;
    scalar beta = dt/Dt;
    if (faItems_[fieldI].window() > 0)
    {
      const scalar w = faItems_[fieldI].window();
      if (Dt - dt >= w)
      {
        alpha = (w - dt)/w;
        beta = dt/w;
      }
    }
    meanField = alpha*meanField + beta*baseField;
  }
}
template<class Type>
void mousse::fieldAverage::calculateMeanFields() const
{
  typedef GeometricField<Type, fvPatchField, volMesh> volFieldType;
  typedef GeometricField<Type, fvsPatchField, surfaceMesh> surfFieldType;
  FOR_ALL(faItems_, i)
  {
    if (faItems_[i].mean())
    {
      calculateMeanFieldType<volFieldType>(i);
      calculateMeanFieldType<surfFieldType>(i);
    }
  }
}
template<class Type1, class Type2>
void mousse::fieldAverage::calculatePrime2MeanFieldType(const label fieldI) const
{
  const word& fieldName = faItems_[fieldI].fieldName();
  if (obr_.foundObject<Type1>(fieldName))
  {
    const Type1& baseField = obr_.lookupObject<Type1>(fieldName);
    const Type1& meanField =
      obr_.lookupObject<Type1>(faItems_[fieldI].meanFieldName());
    Type2& prime2MeanField = const_cast<Type2&>
    (
      obr_.lookupObject<Type2>(faItems_[fieldI].prime2MeanFieldName())
    );
    scalar dt = obr_.time().deltaTValue();
    scalar Dt = totalTime_[fieldI];
    if (faItems_[fieldI].iterBase())
    {
      dt = 1.0;
      Dt = scalar(totalIter_[fieldI]);
    }
    scalar alpha = (Dt - dt)/Dt;
    scalar beta = dt/Dt;
    if (faItems_[fieldI].window() > 0)
    {
      const scalar w = faItems_[fieldI].window();
      if (Dt - dt >= w)
      {
        alpha = (w - dt)/w;
        beta = dt/w;
      }
    }
    prime2MeanField =
      alpha*prime2MeanField
     + beta*sqr(baseField)
     - sqr(meanField);
  }
}
template<class Type1, class Type2>
void mousse::fieldAverage::calculatePrime2MeanFields() const
{
  typedef GeometricField<Type1, fvPatchField, volMesh> volFieldType1;
  typedef GeometricField<Type1, fvsPatchField, surfaceMesh> surfFieldType1;
  typedef GeometricField<Type2, fvPatchField, volMesh> volFieldType2;
  typedef GeometricField<Type2, fvsPatchField, surfaceMesh> surfFieldType2;
  FOR_ALL(faItems_, i)
  {
    if (faItems_[i].prime2Mean())
    {
      calculatePrime2MeanFieldType<volFieldType1, volFieldType2>(i);
      calculatePrime2MeanFieldType<surfFieldType1, surfFieldType2>(i);
    }
  }
}
template<class Type1, class Type2>
void mousse::fieldAverage::addMeanSqrToPrime2MeanType(const label fieldI) const
{
  const word& fieldName = faItems_[fieldI].fieldName();
  if (obr_.foundObject<Type1>(fieldName))
  {
    const Type1& meanField =
      obr_.lookupObject<Type1>(faItems_[fieldI].meanFieldName());
    Type2& prime2MeanField = const_cast<Type2&>
    (
      obr_.lookupObject<Type2>(faItems_[fieldI].prime2MeanFieldName())
    );
    prime2MeanField += sqr(meanField);
  }
}
template<class Type1, class Type2>
void mousse::fieldAverage::addMeanSqrToPrime2Mean() const
{
  typedef GeometricField<Type1, fvPatchField, volMesh> volFieldType1;
  typedef GeometricField<Type1, fvsPatchField, surfaceMesh> surfFieldType1;
  typedef GeometricField<Type2, fvPatchField, volMesh> volFieldType2;
  typedef GeometricField<Type2, fvsPatchField, surfaceMesh> surfFieldType2;
  FOR_ALL(faItems_, i)
  {
    if (faItems_[i].prime2Mean())
    {
      addMeanSqrToPrime2MeanType<volFieldType1, volFieldType2>(i);
      addMeanSqrToPrime2MeanType<surfFieldType1, surfFieldType2>(i);
    }
  }
}
template<class Type>
void mousse::fieldAverage::writeFieldType(const word& fieldName) const
{
  if (obr_.foundObject<Type>(fieldName))
  {
    const Type& f = obr_.lookupObject<Type>(fieldName);
    f.write();
  }
}
template<class Type>
void mousse::fieldAverage::writeFields() const
{
  typedef GeometricField<Type, fvPatchField, volMesh> volFieldType;
  typedef GeometricField<Type, fvsPatchField, surfaceMesh> surfFieldType;
  FOR_ALL(faItems_, i)
  {
    if (faItems_[i].mean())
    {
      const word& fieldName = faItems_[i].meanFieldName();
      writeFieldType<volFieldType>(fieldName);
      writeFieldType<surfFieldType>(fieldName);
    }
    if (faItems_[i].prime2Mean())
    {
      const word& fieldName = faItems_[i].prime2MeanFieldName();
      writeFieldType<volFieldType>(fieldName);
      writeFieldType<surfFieldType>(fieldName);
    }
  }
}
