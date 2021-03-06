// mousse: CFD toolbox
// Copyright (C) 2011-2012 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "he_rho_thermo.hpp"


// Private Member Functions 
template<class BasicPsiThermo, class MixtureType>
void mousse::heRhoThermo<BasicPsiThermo, MixtureType>::calculate()
{
  const scalarField& hCells = this->he().internalField();
  const scalarField& pCells = this->p_.internalField();
  scalarField& TCells = this->T_.internalField();
  scalarField& psiCells = this->psi_.internalField();
  scalarField& rhoCells = this->rho_.internalField();
  scalarField& muCells = this->mu_.internalField();
  scalarField& alphaCells = this->alpha_.internalField();
  FOR_ALL(TCells, celli) {
    const typename MixtureType::thermoType& mixture_ =
      this->cellMixture(celli);
    TCells[celli] =
      mixture_.THE
      (
        hCells[celli],
        pCells[celli],
        TCells[celli]
      );
    psiCells[celli] = mixture_.psi(pCells[celli], TCells[celli]);
    rhoCells[celli] = mixture_.rho(pCells[celli], TCells[celli]);
    muCells[celli] = mixture_.mu(pCells[celli], TCells[celli]);
    alphaCells[celli] = mixture_.alphah(pCells[celli], TCells[celli]);
  }
  FOR_ALL(this->T_.boundaryField(), patchi) {
    fvPatchScalarField& pp = this->p_.boundaryField()[patchi];
    fvPatchScalarField& pT = this->T_.boundaryField()[patchi];
    fvPatchScalarField& ppsi = this->psi_.boundaryField()[patchi];
    fvPatchScalarField& prho = this->rho_.boundaryField()[patchi];
    fvPatchScalarField& ph = this->he().boundaryField()[patchi];
    fvPatchScalarField& pmu = this->mu_.boundaryField()[patchi];
    fvPatchScalarField& palpha = this->alpha_.boundaryField()[patchi];
    if (pT.fixesValue()) {
      FOR_ALL(pT, facei) {
        const typename MixtureType::thermoType& mixture_ =
          this->patchFaceMixture(patchi, facei);
        ph[facei] = mixture_.HE(pp[facei], pT[facei]);
        ppsi[facei] = mixture_.psi(pp[facei], pT[facei]);
        prho[facei] = mixture_.rho(pp[facei], pT[facei]);
        pmu[facei] = mixture_.mu(pp[facei], pT[facei]);
        palpha[facei] = mixture_.alphah(pp[facei], pT[facei]);
      }
    } else {
      FOR_ALL(pT, facei) {
        const typename MixtureType::thermoType& mixture_ =
          this->patchFaceMixture(patchi, facei);
        pT[facei] = mixture_.THE(ph[facei], pp[facei], pT[facei]);
        ppsi[facei] = mixture_.psi(pp[facei], pT[facei]);
        prho[facei] = mixture_.rho(pp[facei], pT[facei]);
        pmu[facei] = mixture_.mu(pp[facei], pT[facei]);
        palpha[facei] = mixture_.alphah(pp[facei], pT[facei]);
      }
    }
  }
}


// Constructors 
template<class BasicPsiThermo, class MixtureType>
mousse::heRhoThermo<BasicPsiThermo, MixtureType>::heRhoThermo
(
  const fvMesh& mesh,
  const word& phaseName
)
:
  heThermo<BasicPsiThermo, MixtureType>{mesh, phaseName}
{
  calculate();
}


// Destructor 
template<class BasicPsiThermo, class MixtureType>
mousse::heRhoThermo<BasicPsiThermo, MixtureType>::~heRhoThermo()
{}


// Member Functions 
template<class BasicPsiThermo, class MixtureType>
void mousse::heRhoThermo<BasicPsiThermo, MixtureType>::correct()
{
  if (debug) {
    Info << "entering heRhoThermo<MixtureType>::correct()" << endl;
  }
  calculate();
  if (debug) {
    Info << "exiting heRhoThermo<MixtureType>::correct()" << endl;
  }
}

