// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "c4_h10_o.hpp"
#include "add_to_run_time_selection_table.hpp"
// Static Data Members
namespace mousse
{
  defineTypeNameAndDebug(C4H10O, 0);
  addToRunTimeSelectionTable(liquidProperties, C4H10O,);
  addToRunTimeSelectionTable(liquidProperties, C4H10O, Istream);
  addToRunTimeSelectionTable(liquidProperties, C4H10O, dictionary);
}
// Constructors 
mousse::C4H10O::C4H10O()
:
  liquidProperties
  (
    74.123,
    466.70,
    3.6376e+6,
    0.28,
    0.262,
    156.85,
    4.0709e-1,
    307.58,
    3.836e-30,
    0.2846,
    1.5532e+4
  ),
  rho_(75.2793188, 0.27608, 466.7, 0.29358),
  pv_(101.03, -6311.5, -12.27, 1.377e-05, 2),
  hl_(466.70, 566355.921913576, 0.40717, 0, 0, 0),
  Cp_
  (
    599.004357621791,
    17.5519069654493,
   -0.0742009902459426,
    0.00011822241409549,
    0.0,
    0.0
  ),
  h_
  (
   -4312350.92187216,
    599.004357621791,
    8.77595348272466,
   -0.0247336634153142,
    2.95556035238725e-05,
    0.0
  ),
  Cpg_(1163.06679438231, 3441.57683849817, 1541.3, 1938.66950878944, -688.9),
  B_
  (
    0.00215992337061371,
   -1.810504162001,
   -276972.0599544,
   -2.12349742994752e+17,
    3.1016013922804e+19
  ),
  mu_(10.197, -63.8, -3.226, 0.0, 0.0),
  mug_(1.948e-06, 0.41, 495.8, 0.0),
  K_(0.249, -0.0004005, 0.0, 0.0, 0.0, 0.0),
  Kg_(-0.0044894, 0.6155, -3266.3, 0.0),
  sigma_(466.70, 0.057356, 1.288, 0.0, 0.0, 0.0),
  D_(147.18, 20.1, 74.123, 28) // note: Same as nHeptane
{}
mousse::C4H10O::C4H10O
(
  const liquidProperties& l,
  const NSRDSfunc5& density,
  const NSRDSfunc1& vapourPressure,
  const NSRDSfunc6& heatOfVapourisation,
  const NSRDSfunc0& heatCapacity,
  const NSRDSfunc0& enthalpy,
  const NSRDSfunc7& idealGasHeatCapacity,
  const NSRDSfunc4& secondVirialCoeff,
  const NSRDSfunc1& dynamicViscosity,
  const NSRDSfunc2& vapourDynamicViscosity,
  const NSRDSfunc0& thermalConductivity,
  const NSRDSfunc2& vapourThermalConductivity,
  const NSRDSfunc6& surfaceTension,
  const APIdiffCoefFunc& vapourDiffussivity
)
:
  liquidProperties(l),
  rho_(density),
  pv_(vapourPressure),
  hl_(heatOfVapourisation),
  Cp_(heatCapacity),
  h_(enthalpy),
  Cpg_(idealGasHeatCapacity),
  B_(secondVirialCoeff),
  mu_(dynamicViscosity),
  mug_(vapourDynamicViscosity),
  K_(thermalConductivity),
  Kg_(vapourThermalConductivity),
  sigma_(surfaceTension),
  D_(vapourDiffussivity)
{}
mousse::C4H10O::C4H10O(Istream& is)
:
  liquidProperties(is),
  rho_(is),
  pv_(is),
  hl_(is),
  Cp_(is),
  h_(is),
  Cpg_(is),
  B_(is),
  mu_(is),
  mug_(is),
  K_(is),
  Kg_(is),
  sigma_(is),
  D_(is)
{}
mousse::C4H10O::C4H10O(const dictionary& dict)
:
  liquidProperties(dict),
  rho_(dict.subDict("rho")),
  pv_(dict.subDict("pv")),
  hl_(dict.subDict("hl")),
  Cp_(dict.subDict("Cp")),
  h_(dict.subDict("h")),
  Cpg_(dict.subDict("Cpg")),
  B_(dict.subDict("B")),
  mu_(dict.subDict("mu")),
  mug_(dict.subDict("mug")),
  K_(dict.subDict("K")),
  Kg_(dict.subDict("Kg")),
  sigma_(dict.subDict("sigma")),
  D_(dict.subDict("D"))
{}
mousse::C4H10O::C4H10O(const C4H10O& liq)
:
  liquidProperties(liq),
  rho_(liq.rho_),
  pv_(liq.pv_),
  hl_(liq.hl_),
  Cp_(liq.Cp_),
  h_(liq.h_),
  Cpg_(liq.Cpg_),
  B_(liq.B_),
  mu_(liq.mu_),
  mug_(liq.mug_),
  K_(liq.K_),
  Kg_(liq.Kg_),
  sigma_(liq.sigma_),
  D_(liq.D_)
{}
