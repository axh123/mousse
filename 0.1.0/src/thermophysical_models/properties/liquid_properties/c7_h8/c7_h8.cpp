// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "c7_h8.hpp"
#include "add_to_run_time_selection_table.hpp"
// Static Data Members
namespace mousse
{
  DEFINE_TYPE_NAME_AND_DEBUG(C7H8, 0);
  ADD_TO_RUN_TIME_SELECTION_TABLE(liquidProperties, C7H8,);
  ADD_TO_RUN_TIME_SELECTION_TABLE(liquidProperties, C7H8, Istream);
  ADD_TO_RUN_TIME_SELECTION_TABLE(liquidProperties, C7H8, dictionary);
}
// Constructors 
mousse::C7H8::C7H8()
:
  liquidProperties
  (
    92.141,
    591.79,
    4.1086e+6,
    0.31579,
    0.264,
    178.18,
    4.1009e-2,
    383.78,
    1.2008e-30,
    0.2641,
    1.8346e+4
  ),
  rho_(81.32088237, 0.27108, 591.79, 0.29889),
  pv_(83.359, -6995, -9.1635, 6.225e-06, 2.0),
  hl_(591.79, 544383.065085033, 0.3834, 0.0, 0.0, 0.0),
  Cp_
  (
    2066.83235476064,
   -8.14664481609707,
    0.0322581695445024,
   -3.01223125427334e-05,
    0.0,
    0.0
  ),
  h_
  (
   -353094.830249075,
    2066.83235476064,
   -4.07332240804853,
    0.0107527231815008,
   -7.53057813568336e-06,
    0.0
  ),
  Cpg_(630.989461803106, 3107.19440856947, 1440.6, 2059.88647833212, -650.43),
  B_
  (
    0.00191120131103418,
   -2.24970425760519,
   -482293.441573241,
   -7.62309938029759e+17,
    1.00986531511488e+20
  ),
  mu_(-13.362, 1183, 0.333, 0.0, 0.0),
  mug_(2.919e-08, 0.9648, 0.0, 0.0),
  K_(0.2043, -0.000239, 0.0, 0.0, 0.0, 0.0),
  Kg_(2.392e-05, 1.2694, 537, 0.0),
  sigma_(591.79, 0.06685, 1.2456, 0.0, 0.0, 0.0),
  D_(147.18, 20.1, 92.141, 28) // note: Same as nHeptane
{}
mousse::C7H8::C7H8
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
mousse::C7H8::C7H8(Istream& is)
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
mousse::C7H8::C7H8(const dictionary& dict)
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
mousse::C7H8::C7H8(const C7H8& liq)
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
