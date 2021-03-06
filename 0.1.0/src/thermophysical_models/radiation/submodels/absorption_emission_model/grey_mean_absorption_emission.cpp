// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "grey_mean_absorption_emission.hpp"
#include "add_to_run_time_selection_table.hpp"
#include "unit_conversion.hpp"
#include "zero_gradient_fv_patch_fields.hpp"
#include "basic_specie_mixture.hpp"


// Static Data Members
namespace mousse {
namespace radiation {

DEFINE_TYPE_NAME_AND_DEBUG(greyMeanAbsorptionEmission, 0);
ADD_TO_RUN_TIME_SELECTION_TABLE
(
  absorptionEmissionModel,
  greyMeanAbsorptionEmission,
  dictionary
);

}
}


// Constructors 
mousse::radiation::greyMeanAbsorptionEmission::greyMeanAbsorptionEmission
(
  const dictionary& dict,
  const fvMesh& mesh
)
:
  absorptionEmissionModel{dict, mesh},
  coeffsDict_{(dict.subDict(typeName + "Coeffs"))},
  speciesNames_{0},
  specieIndex_{label(0)},
  lookUpTablePtr_(),
  thermo_{mesh.lookupObject<fluidThermo>(basicThermo::dictName)},
  EhrrCoeff_{readScalar(coeffsDict_.lookup("EhrrCoeff"))},
  Yj_{nSpecies_}
{
  if (!isA<basicSpecieMixture>(thermo_)) {
    FATAL_ERROR_IN
    (
      "radiation::greyMeanAbsorptionEmission::greyMeanAbsorptionEmission"
      "("
        "const dictionary&, "
        "const fvMesh&"
      ")"
    )
    << "Model requires a multi-component thermo package"
    << abort(FatalError);
  }
  label nFunc = 0;
  const dictionary& functionDicts = dict.subDict(typeName + "Coeffs");
  FOR_ALL_CONST_ITER(dictionary, functionDicts, iter) {
    // safety:
    if (!iter().isDict()) {
      continue;
    }
    const word& key = iter().keyword();
    speciesNames_.insert(key, nFunc);
    const dictionary& dict = iter().dict();
    coeffs_[nFunc].initialise(dict);
    nFunc++;
  }
  if (coeffsDict_.found("lookUpTableFileName")) {
    const word name = coeffsDict_.lookup("lookUpTableFileName");
    if (name != "none") {
      lookUpTablePtr_.set
      (
        new interpolationLookUpTable<scalar>
        {
          fileName(coeffsDict_.lookup("lookUpTableFileName")),
          mesh.time().constant(),
          mesh
        }
      );
      if (!mesh.foundObject<volScalarField>("ft")) {
        FATAL_ERROR_IN
        (
          "mousse::radiation::greyMeanAbsorptionEmission(const"
          "dictionary& dict, const fvMesh& mesh)"
        )
        << "specie ft is not present to use with "
        << "lookUpTableFileName " << nl
        << exit(FatalError);
      }
    }
  }
  // Check that all the species on the dictionary are present in the
  // look-up table and save the corresponding indices of the look-up table
  label j = 0;
  FOR_ALL_CONST_ITER(HashTable<label>, speciesNames_, iter) {
    if (!lookUpTablePtr_.empty()) {
      if (lookUpTablePtr_().found(iter.key())) {
        label index = lookUpTablePtr_().findFieldIndex(iter.key());
        Info << "specie: " << iter.key() << " found on look-up table "
          << " with index: " << index << endl;
        specieIndex_[iter()] = index;
      } else if (mesh.foundObject<volScalarField>(iter.key())) {
        volScalarField& Y =
          const_cast<volScalarField&>
          (
            mesh.lookupObject<volScalarField>(iter.key())
          );
        Yj_.set(j, &Y);
        specieIndex_[iter()] = 0;
        j++;
        Info << "specie: " << iter.key() << " is being solved" << endl;
      } else {
        FATAL_ERROR_IN
        (
          "mousse::radiation::greyMeanAbsorptionEmission(const"
          "dictionary& dict, const fvMesh& mesh)"
        )
        << "specie: " << iter.key()
        << " is neither in look-up table: "
        << lookUpTablePtr_().tableName()
        << " nor is being solved" << nl
        << exit(FatalError);
      }
    } else if (mesh.foundObject<volScalarField>(iter.key())) {
      volScalarField& Y =
        const_cast<volScalarField&>
        (
          mesh.lookupObject<volScalarField>(iter.key())
        );
      Yj_.set(j, &Y);
      specieIndex_[iter()] = 0;
      j++;
    } else {
      FATAL_ERROR_IN
      (
        "mousse::radiation::greyMeanAbsorptionEmission(const"
        "dictionary& dict, const fvMesh& mesh)"
      )
      << " there is not lookup table and the specie" << nl
      << iter.key() << nl
      << " is not found " << nl
      << exit(FatalError);
    }
  }
}


// Destructor 
mousse::radiation::greyMeanAbsorptionEmission::~greyMeanAbsorptionEmission()
{}


// Member Functions 
mousse::tmp<mousse::volScalarField>
mousse::radiation::greyMeanAbsorptionEmission::aCont(const label bandI) const
{
  const basicSpecieMixture& mixture =
    dynamic_cast<const basicSpecieMixture&>(thermo_);
  const volScalarField& T = thermo_.T();
  const volScalarField& p = thermo_.p();
  tmp<volScalarField> ta
  {
    new volScalarField
    {
      {
        "aCont" + name(bandI),
        mesh().time().timeName(),
        mesh(),
        IOobject::NO_READ,
        IOobject::NO_WRITE
      },
      mesh(),
      {"a", dimless/dimLength, 0.0},
      zeroGradientFvPatchVectorField::typeName
    }
  };
  scalarField& a = ta().internalField();
  FOR_ALL(a, cellI) {
    FOR_ALL_CONST_ITER(HashTable<label>, speciesNames_, iter) {
      label n = iter();
      scalar Xipi = 0.0;
      if (specieIndex_[n] != 0) {
        //Specie found in the lookUpTable.
        const volScalarField& ft =
          mesh_.lookupObject<volScalarField>("ft");
        const List<scalar>& Ynft = lookUpTablePtr_().lookUp(ft[cellI]);
        //moles x pressure [atm]
        Xipi = Ynft[specieIndex_[n]]*paToAtm(p[cellI]);
      } else {
        scalar invWt = 0.0;
        FOR_ALL(mixture.Y(), s) {
          invWt += mixture.Y(s)[cellI]/mixture.W(s);
        }
        label index = mixture.species()[iter.key()];
        scalar Xk = mixture.Y(index)[cellI]/(mixture.W(index)*invWt);
        Xipi = Xk*paToAtm(p[cellI]);
      }
      const absorptionCoeffs::coeffArray& b = coeffs_[n].coeffs(T[cellI]);
      scalar Ti = T[cellI];
      // negative temperature exponents
      if (coeffs_[n].invTemp()) {
        Ti = 1.0/T[cellI];
      }
      a[cellI] +=
        Xipi*(((((b[5]*Ti + b[4])*Ti + b[3])*Ti + b[2])*Ti + b[1])*Ti + b[0]);
    }
  }
  ta().correctBoundaryConditions();
  return ta;
}


mousse::tmp<mousse::volScalarField>
mousse::radiation::greyMeanAbsorptionEmission::eCont(const label bandI) const
{
 return aCont(bandI);
}
mousse::tmp<mousse::volScalarField>
mousse::radiation::greyMeanAbsorptionEmission::ECont(const label bandI) const
{
  tmp<volScalarField> E
  {
    new volScalarField
    {
      {
        "ECont" + name(bandI),
        mesh_.time().timeName(),
        mesh_,
        IOobject::NO_READ,
        IOobject::NO_WRITE
      },
      mesh_,
      {"E", dimMass/dimLength/pow3(dimTime), 0.0}
    }
  };
  if (mesh_.foundObject<volScalarField>("dQ")) {
    const volScalarField& dQ = mesh_.lookupObject<volScalarField>("dQ");
    if (dQ.dimensions() == dimEnergy/dimTime) {
      E().internalField() = EhrrCoeff_*dQ/mesh_.V();
    } else if (dQ.dimensions() == dimEnergy/dimTime/dimVolume) {
      E().internalField() = EhrrCoeff_*dQ;
    } else {
      if (debug) {
        WARNING_IN
        (
          "tmp<volScalarField>"
          "radiation::greyMeanAbsorptionEmission::ECont"
          "("
            "const label"
          ") const"
        )
        << "Incompatible dimensions for dQ field" << endl;
      }
    }
  } else {
    WARNING_IN
    (
      "tmp<volScalarField>"
      "radiation::greyMeanAbsorptionEmission::ECont"
      "("
        "const label"
      ") const"
    )
    << "dQ field not found in mesh" << endl;
  }
  return E;
}

