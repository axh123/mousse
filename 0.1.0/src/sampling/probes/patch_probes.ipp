// mousse: CFD toolbox
// Copyright (C) 2011-2013 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "patch_probes.hpp"
#include "vol_fields.hpp"
#include "iomanip.hpp"


// Private Member Functions 
template<class Type>
void mousse::patchProbes::sampleAndWrite
(
  const GeometricField<Type, fvPatchField, volMesh>& vField
)
{
  Field<Type> values{sample(vField)};
  if (Pstream::master()) {
    unsigned int w = IOstream::defaultPrecision() + 7;
    OFstream& probeStream = *probeFilePtrs_[vField.name()];
    probeStream
      << setw(w)
      << vField.time().timeToUserTime(vField.time().value());
    FOR_ALL(values, probeI) {
      probeStream << ' ' << setw(w) << values[probeI];
    }
    probeStream << endl;
  }
}


template<class Type>
void mousse::patchProbes::sampleAndWrite
(
  const GeometricField<Type, fvsPatchField, surfaceMesh>& sField
)
{
  Field<Type> values{sample(sField)};
  if (Pstream::master()) {
    unsigned int w = IOstream::defaultPrecision() + 7;
    OFstream& probeStream = *probeFilePtrs_[sField.name()];
    probeStream
      << setw(w)
      << sField.time().timeToUserTime(sField.time().value());
    FOR_ALL(values, probeI) {
      probeStream << ' ' << setw(w) << values[probeI];
    }
    probeStream << endl;
  }
}


template<class Type>
void mousse::patchProbes::sampleAndWrite
(
  const fieldGroup<Type>& fields
)
{
  FOR_ALL(fields, fieldI) {
    if (loadFromFiles_) {
      sampleAndWrite
      (
        GeometricField<Type, fvPatchField, volMesh>
        {
          {
            fields[fieldI],
            mesh_.time().timeName(),
            mesh_,
            IOobject::MUST_READ,
            IOobject::NO_WRITE,
            false
          },
          mesh_
        }
      );
    } else {
      objectRegistry::const_iterator iter = mesh_.find(fields[fieldI]);
      if (iter != objectRegistry::end()
          && iter()->type()
              == GeometricField<Type, fvPatchField, volMesh>::typeName) {
        sampleAndWrite
        (
          mesh_.lookupObject<GeometricField<Type, fvPatchField, volMesh>>
          (
            fields[fieldI]
          )
        );
      }
    }
  }
}


template<class Type>
void mousse::patchProbes::sampleAndWriteSurfaceFields
(
  const fieldGroup<Type>& fields
)
{
  FOR_ALL(fields, fieldI) {
    if (loadFromFiles_) {
      sampleAndWrite
      (
        GeometricField<Type, fvsPatchField, surfaceMesh>
        {
          {
            fields[fieldI],
            mesh_.time().timeName(),
            mesh_,
            IOobject::MUST_READ,
            IOobject::NO_WRITE,
            false
          },
          mesh_
        }
      );
    } else {
      objectRegistry::const_iterator iter = mesh_.find(fields[fieldI]);
      if (iter != objectRegistry::end()
          && iter()->type()
                == GeometricField<Type, fvsPatchField, surfaceMesh>::typeName) {
        sampleAndWrite
        (
          mesh_.lookupObject<GeometricField<Type, fvsPatchField, surfaceMesh>>
          (
            fields[fieldI]
          )
        );
      }
    }
  }
}


// Member Functions 
template<class Type>
mousse::tmp<mousse::Field<Type>>
mousse::patchProbes::sample
(
  const GeometricField<Type, fvPatchField, volMesh>& vField
) const
{
  const Type unsetVal{-VGREAT*pTraits<Type>::one};
  tmp<Field<Type>> tValues{new Field<Type>{this->size(), unsetVal}};
  Field<Type>& values = tValues();
  const polyBoundaryMesh& patches = mesh_.boundaryMesh();
  FOR_ALL(*this, probeI) {
    label faceI = elementList_[probeI];
    if (faceI >= 0) {
      label patchI = patches.whichPatch(faceI);
      label localFaceI = patches[patchI].whichFace(faceI);
      values[probeI] = vField.boundaryField()[patchI][localFaceI];
    }
  }
  Pstream::listCombineGather(values, isNotEqOp<Type>());
  Pstream::listCombineScatter(values);
  return tValues;
}


template<class Type>
mousse::tmp<mousse::Field<Type>>
mousse::patchProbes::sample(const word& fieldName) const
{
  return sample
  (
    mesh_.lookupObject<GeometricField<Type, fvPatchField, volMesh>>
    (
      fieldName
    )
  );
}


template<class Type>
mousse::tmp<mousse::Field<Type>>
mousse::patchProbes::sample
(
  const GeometricField<Type, fvsPatchField, surfaceMesh>& sField
) const
{
  const Type unsetVal{-VGREAT*pTraits<Type>::one};
  tmp<Field<Type>> tValues{new Field<Type>{this->size(), unsetVal}};
  Field<Type>& values = tValues();
  const polyBoundaryMesh& patches = mesh_.boundaryMesh();
  FOR_ALL(*this, probeI) {
    label faceI = elementList_[probeI];
    if (faceI >= 0) {
      label patchI = patches.whichPatch(faceI);
      label localFaceI = patches[patchI].whichFace(faceI);
      values[probeI] = sField.boundaryField()[patchI][localFaceI];
    }
  }
  Pstream::listCombineGather(values, isNotEqOp<Type>());
  Pstream::listCombineScatter(values);
  return tValues;
}

