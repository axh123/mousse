// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "c12_h26.hpp"
#include "add_to_run_time_selection_table.hpp"
// Static Data Members
namespace mousse
{
  DEFINE_TYPE_NAME_AND_DEBUG(C12H26, 0);
  ADD_TO_RUN_TIME_SELECTION_TABLE(liquidProperties, C12H26,);
  ADD_TO_RUN_TIME_SELECTION_TABLE(liquidProperties, C12H26, Istream);
  ADD_TO_RUN_TIME_SELECTION_TABLE(liquidProperties, C12H26, dictionary);
}
// Constructors 
mousse::C12H26::C12H26()
:
  liquidProperties
  (
    170.338,
    658.0,
    1.82e+6,
    0.716,
    0.238,
    263.57,
    6.152e-1,
    489.47,
    0.0,
    0.5764,
    1.59e+4
  ),
  rho_(60.53982858, 0.25511, 658.0, 0.29368),
  pv_(137.47, -11976.0, -16.698, 8.0906e-06, 2.0),
  hl_(658.0, 454020.829174935, 0.40681, 0.0, 0.0, 0.0),
  Cp_(2983.53861146661, -8.0352006011577, 0.018207916025784, 0.0, 0.0, 0.0),
  h_
  (
   -2755166.83820769,
    2983.53861146661,
   -4.01760030057885,
    0.00606930534192801,
    0.0,
    0.0
  ),
  Cpg_(1250.16144371778, 3894.02247296552, 1715.5, 2650.67101879792, 777.5),
  B_
  (
    0.00516619896910848,
   -6.40491258556517,
   -295295.236529723,
   -3.22147729807794e+19,
    8.78195117941974e+21
  ),
  mu_(-20.607, 1943, 1.3205, 0.0, 0.0),
  mug_(6.344e-08, 0.8287, 219.5, 0.0),
  K_(0.2047, -0.0002326, 0.0, 0.0, 0.0, 0.0),
  Kg_(5.719e-06, 1.4699, 579.4, 0.0),
  sigma_(658.0, 0.055493, 1.3262, 0.0, 0.0, 0.0),
  D_(147.18, 20.1, 170.338, 28.0) // note: Same as nHeptane
{}
mousse::C12H26::C12H26
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
mousse::C12H26::C12H26(Istream& is)
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
mousse::C12H26::C12H26(const dictionary& dict)
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
mousse::C12H26::C12H26(const C12H26& liq)
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
