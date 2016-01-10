// mousse: CFD toolbox
// Copyright (C) 2012 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

// Member Functions 
template<class GeoField>
mousse::tmp<GeoField> mousse::uniformInterpolate
(
  const HashPtrTable<GeoField, label, Hash<label> >& fields,
  const labelList& indices,
  const scalarField& weights
)
{
  const GeoField& field0 = *(*fields.begin());
  // Interpolate
  tmp<GeoField> tfld
  (
    new GeoField
    (
      IOobject
      (
        "uniformInterpolate(" + field0.name() + ')',
        field0.time().timeName(),
        field0.db(),
        IOobject::NO_READ,
        IOobject::AUTO_WRITE
      ),
      weights[0]*(*fields[indices[0]])
    )
  );
  GeoField& fld = tfld();
  for (label i = 1; i < indices.size(); ++i)
  {
    fld += weights[i]*(*fields[indices[i]]);
  }
  return tfld;
}
template<class GeoField>
mousse::tmp<GeoField> mousse::uniformInterpolate
(
  const IOobject& fieldIO,
  const word& fieldName,
  const wordList& times,
  const scalarField& weights,
  const objectRegistry& fieldsCache
)
{
  // Look up the first field
  const objectRegistry& time0Fields = fieldsCache.lookupObject
  <
    const objectRegistry
  >
  (
    times[0]
  );
  const GeoField& field0 = time0Fields.lookupObject
  <
    const GeoField
  >
  (
    fieldName
  );
  // Interpolate
  tmp<GeoField> tfld(new GeoField(fieldIO, weights[0]*field0));
  GeoField& fld = tfld();
  for (label i = 1; i < times.size(); ++i)
  {
    const objectRegistry& timeIFields = fieldsCache.lookupObject
    <
      const objectRegistry
    >
    (
      times[i]
    );
    const GeoField& fieldI = timeIFields.lookupObject
    <
      const GeoField
    >
    (
      fieldName
    );
    fld += weights[i]*fieldI;
  }
  return tfld;
}
template<class GeoField>
mousse::tmp<GeoField> mousse::uniformInterpolate
(
  const IOobject& fieldIO,
  const word& fieldName,
  const wordList& times,
  const scalarField& weights,
  const word& registryName
)
{
  return uniformInterpolate<GeoField>
  (
    fieldIO,
    fieldName,
    times,
    weights,
    fieldIO.db().subRegistry(registryName, true)
  );
}