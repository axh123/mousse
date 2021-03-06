// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "phase_properties.hpp"


// Static Data Members
namespace mousse {

template<>
const char* mousse::NamedEnum
<
  mousse::phaseProperties::phaseType,
  4
>::names[] =
{
  "gas",
  "liquid",
  "solid",
  "unknown"
};

}

const mousse::NamedEnum<mousse::phaseProperties::phaseType, 4>
  mousse::phaseProperties::phaseTypeNames;


// Private Member Functions 
void mousse::phaseProperties::reorder(const wordList& specieNames)
{
  // ***HGW Unfortunately in the current implementation it is assumed that
  // if no species are specified the phase is not present and this MUST
  // be checked at the point of use.  This needs a rewrite.
  if (!names_.size()) {
    return;
  }
  // Store the current sames and mass-fractions
  List<word> names0{names_};
  scalarField Y0{Y_};
  // Update the specie names to those given
  names_ = specieNames;
  // Re-size mass-fractions if necessary, initialize to 0
  if (names_.size() != names0.size()) {
    Y_.setSize(names_.size());
    Y_ = 0;
  }
  // Set the mass-fraction for each specie in the list to the corresponding
  // value in the original list
  FOR_ALL(names0, i) {
    bool found = false;
    FOR_ALL(names_, j) {
      if (names_[j] == names0[i]) {
        Y_[j] = Y0[i];
        found = true;
        break;
      }
    }
    if (!found) {
      FATAL_ERROR_IN
      (
        "void phaseProperties::reorder(const wordList&)"
      )
      << "Could not find specie " << names0[i]
      << " in list " <<  names_
      << " for phase " << phaseTypeNames[phase_]
      << exit(FatalError);
    }
  }
}


void mousse::phaseProperties::setCarrierIds
(
  const wordList& carrierNames
)
{
  carrierIds_ = -1;
  FOR_ALL(names_, i) {
    FOR_ALL(carrierNames, j) {
      if (carrierNames[j] == names_[i]) {
        carrierIds_[i] = j;
        break;
      }
    }
    if (carrierIds_[i] == -1) {
      FATAL_ERROR_IN
      (
        "void phaseProperties::setCarrierIds"
        "(const wordList& carrierNames)"
      )
      << "Could not find carrier specie " << names_[i]
      << " in species list" <<  nl
      << "Available species are: " << nl << carrierNames << nl
      << exit(FatalError);
    }
  }
}


void mousse::phaseProperties::checkTotalMassFraction() const
{
  scalar total = 0.0;
  FOR_ALL(Y_, speciei) {
    total += Y_[speciei];
  }
  if (Y_.size() != 0 && mag(total - 1.0) > SMALL) {
    FATAL_ERROR_IN
    (
      "void phaseProperties::checkTotalMassFraction() const"
    )
    << "Specie fractions must total to unity for phase "
    << phaseTypeNames[phase_] << nl
    << "Species: " << nl << names_ << nl
    << exit(FatalError);
  }
}


mousse::word mousse::phaseProperties::phaseToStateLabel(const phaseType pt) const
{
  word state = "(unknown)";
  switch (pt) {
    case GAS:
      state = "(g)";
      break;
    case LIQUID:
      state = "(l)";
      break;
    case SOLID:
      state = "(s)";
      break;
    default:
      FATAL_ERROR_IN
      (
        "phaseProperties::phaseToStateLabel(phaseType pt)"
      )
      << "Invalid phase: " << phaseTypeNames[pt] << nl
      << "    phase must be gas, liquid or solid" << nl
      << exit(FatalError);
  }
  return state;
}


// Constructors 
mousse::phaseProperties::phaseProperties()
:
  phase_{UNKNOWN},
  stateLabel_{"(unknown)"},
  names_{0},
  Y_{0},
  carrierIds_{0}
{}


mousse::phaseProperties::phaseProperties(const phaseProperties& pp)
:
  phase_{pp.phase_},
  stateLabel_{pp.stateLabel_},
  names_{pp.names_},
  Y_{pp.Y_},
  carrierIds_{pp.carrierIds_}
{}


// Destructor 
mousse::phaseProperties::~phaseProperties()
{}


// Member Functions 
void mousse::phaseProperties::reorder
(
  const wordList& gasNames,
  const wordList& liquidNames,
  const wordList& solidNames
)
{
  // Determine the addressing to map between species listed in the phase
  // with those given in the (main) thermo properties
  switch (phase_) {
    case GAS:
    {
      // The list of gaseous species in the mixture may be a sub-set of
      // the gaseous species in the carrier phase
      setCarrierIds(gasNames);
      break;
    }
    case LIQUID:
    {
      // Set the list of liquid species to correspond to the complete list
      // defined in the thermodynamics package.
      reorder(liquidNames);
      // Set the ids of the corresponding species in the carrier phase
      setCarrierIds(gasNames);
      break;
    }
    case SOLID:
    {
      // Set the list of solid species to correspond to the complete list
      // defined in the thermodynamics package.
      reorder(solidNames);
      // Assume there is no correspondence between the solid species and
      // the species in the carrier phase (no sublimation).
      break;
    }
    default:
    {
      FATAL_ERROR_IN
      (
        "phaseProperties::reorder"
        "("
        "  const wordList& gasNames, "
        "  const wordList& liquidNames, "
        "  const wordList& solidNames"
        ")"
      )
      << "Invalid phase: " << phaseTypeNames[phase_] << nl
      << "    phase must be gas, liquid or solid" << nl
      << exit(FatalError);
    }
  }
}


mousse::phaseProperties::phaseType mousse::phaseProperties::phase() const
{
  return phase_;
}


const mousse::word& mousse::phaseProperties::stateLabel() const
{
  return stateLabel_;
}


mousse::word mousse::phaseProperties::phaseTypeName() const
{
  return phaseTypeNames[phase_];
}


const mousse::List<mousse::word>& mousse::phaseProperties::names() const
{
  return names_;
}


const mousse::word& mousse::phaseProperties::name(const label speciei) const
{
  if (speciei >= names_.size()) {
    FATAL_ERROR_IN
    (
      "const word& phaseProperties::name(const label) const"
    )
    << "Requested specie " << speciei << "out of range" << nl
    << "Available phase species:" << nl << names_ << nl
    << exit(FatalError);
  }
  return names_[speciei];
}


const mousse::scalarField& mousse::phaseProperties::Y() const
{
  return Y_;
}


mousse::scalar& mousse::phaseProperties::Y(const label speciei)
{
  if (speciei >= Y_.size()) {
    FATAL_ERROR_IN
    (
      "const scalar& phaseProperties::Y(const label) const"
    )
    << "Requested specie " << speciei << "out of range" << nl
    << "Available phase species:" << nl << names_ << nl
    << exit(FatalError);
  }
  return Y_[speciei];
}


const mousse::labelList& mousse::phaseProperties::carrierIds() const
{
  return carrierIds_;
}


mousse::label mousse::phaseProperties::id(const word& specieName) const
{
  FOR_ALL(names_, speciei) {
    if (names_[speciei] == specieName) {
      return speciei;
    }
  }
  return -1;
}

