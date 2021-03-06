// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "les_delta.hpp"
#include "calculated_fv_patch_fields.hpp"


// Static Data Members
namespace mousse {

DEFINE_TYPE_NAME_AND_DEBUG(LESdelta, 0);
DEFINE_RUN_TIME_SELECTION_TABLE(LESdelta, dictionary);

}


// Constructors 
mousse::LESdelta::LESdelta
(
  const word& name,
  const turbulenceModel& turbulence
)
:
  turbulenceModel_{turbulence},
  delta_
  {
    {
      name,
      turbulence.mesh().time().timeName(),
      turbulence.mesh(),
      IOobject::NO_READ,
      IOobject::NO_WRITE
    },
    turbulence.mesh(),
    {name, dimLength, SMALL},
    calculatedFvPatchScalarField::typeName
  }
{}


// Selectors
mousse::autoPtr<mousse::LESdelta> mousse::LESdelta::New
(
  const word& name,
  const turbulenceModel& turbulence,
  const dictionary& dict
)
{
  const word deltaType{dict.lookup("delta")};
  Info << "Selecting LES delta type " << deltaType << endl;
  dictionaryConstructorTable::iterator cstrIter =
    dictionaryConstructorTablePtr_->find(deltaType);
  if (cstrIter == dictionaryConstructorTablePtr_->end()) {
    FATAL_ERROR_IN
    (
      "LESdelta::New(const word& name, const turbulenceModel& turbulence)"
    )
    << "Unknown LESdelta type "
    << deltaType << nl << nl
    << "Valid LESdelta types are :" << endl
    << dictionaryConstructorTablePtr_->sortedToc()
    << exit(FatalError);
  }
  return autoPtr<LESdelta>{cstrIter()(name, turbulence, dict)};
}


mousse::autoPtr<mousse::LESdelta> mousse::LESdelta::New
(
  const word& name,
  const turbulenceModel& turbulence,
  const dictionary& dict,
  const dictionaryConstructorTable& additionalConstructors
)
{
  const word deltaType{dict.lookup("delta")};
  Info << "Selecting LES delta type " << deltaType << endl;
  // First on additional ones
  dictionaryConstructorTable::const_iterator cstrIter =
    additionalConstructors.find(deltaType);
  if (cstrIter != additionalConstructors.end()) {
    return autoPtr<LESdelta>(cstrIter()(name, turbulence, dict));
  } else {
    dictionaryConstructorTable::const_iterator cstrIter =
      dictionaryConstructorTablePtr_->find(deltaType);
    if (cstrIter == dictionaryConstructorTablePtr_->end()) {
      FATAL_ERROR_IN
      (
        "LESdelta::New(const word& name, "
        "const turbulenceModel& turbulence, "
        "const dictionaryConstructorTable&)"
      )
      << "Unknown LESdelta type "
      << deltaType << nl << nl
      << "Valid LESdelta types are :" << endl
      << additionalConstructors.sortedToc()
      << " and "
      << dictionaryConstructorTablePtr_->sortedToc()
      << exit(FatalError);
      return autoPtr<LESdelta>{NULL};
    }
    else
    {
      return autoPtr<LESdelta>{cstrIter()(name, turbulence, dict)};
    }
  }
}
