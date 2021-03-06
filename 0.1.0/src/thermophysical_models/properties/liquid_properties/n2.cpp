// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "n2.hpp"
#include "add_to_run_time_selection_table.hpp"


// Static Data Members
namespace mousse {

DEFINE_TYPE_NAME_AND_DEBUG(N2, 0);
ADD_TO_RUN_TIME_SELECTION_TABLE(liquidProperties, N2,);
ADD_TO_RUN_TIME_SELECTION_TABLE(liquidProperties, N2, Istream);
ADD_TO_RUN_TIME_SELECTION_TABLE(liquidProperties, N2, dictionary);

}


// Constructors 
mousse::N2::N2()
:
  liquidProperties
  {
    28.014,
    126.10,
    3.3944e+6,
    0.0901,
    0.292,
    63.15,
    1.2517e+4,
    77.35,
    0.0,
    0.0403,
    9.0819e+3
  },
  rho_{88.8716136, 0.28479, 126.1, 0.2925},
  pv_{59.826, -1097.6, -8.6689, 0.046346, 1.0},
  hl_{126.10, 336617.405582923, 1.201, -1.4811, 0.7085, 0.0},
  Cp_
  {
   -1192.26101235097,
    125.187406296852,
   -1.66702363104162,
    0.00759263225530092,
    0.0,
    0.0
  },
  h_
  {
   -5480656.55276541,
   -1192.26101235097,
    62.5937031484258,
   -0.555674543680541,
    0.00189815806382523,
    0.0
  },
  Cpg_{1038.94481330763, 307.52123938031, 1701.6, 3.69351038766331, 909.79},
  B_
  {
    0.00166702363104162,
   -0.533661740558292,
   -2182.12322410223,
    2873563218390.8,
   -165274505604341.0
  },
  mu_{32.165, 496.9, 3.9069, -1.08e-21, 10.0},
  mug_{7.632e-07, 0.58823, 67.75, 0.0},
  K_{0.7259, -0.016728, 0.00016215, -5.7605e-07, 0.0, 0.0},
  Kg_{0.000351, 0.7652, 25.767, 0.0},
  sigma_{126.10, 0.02898, 1.2457, 0.0, 0.0, 0.0},
  D_{147.18, 20.1, 28.014, 28.0} // note: Same as nHeptane
{}


mousse::N2::N2
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
  liquidProperties{l},
  rho_{density},
  pv_{vapourPressure},
  hl_{heatOfVapourisation},
  Cp_{heatCapacity},
  h_{enthalpy},
  Cpg_{idealGasHeatCapacity},
  B_{secondVirialCoeff},
  mu_{dynamicViscosity},
  mug_{vapourDynamicViscosity},
  K_{thermalConductivity},
  Kg_{vapourThermalConductivity},
  sigma_{surfaceTension},
  D_{vapourDiffussivity}
{}


mousse::N2::N2(Istream& is)
:
  liquidProperties{is},
  rho_{is},
  pv_{is},
  hl_{is},
  Cp_{is},
  h_{is},
  Cpg_{is},
  B_{is},
  mu_{is},
  mug_{is},
  K_{is},
  Kg_{is},
  sigma_{is},
  D_{is}
{}


mousse::N2::N2(const dictionary& dict)
:
  liquidProperties{dict},
  rho_{dict.subDict("rho")},
  pv_{dict.subDict("pv")},
  hl_{dict.subDict("hl")},
  Cp_{dict.subDict("Cp")},
  h_{dict.subDict("h")},
  Cpg_{dict.subDict("Cpg")},
  B_{dict.subDict("B")},
  mu_{dict.subDict("mu")},
  mug_{dict.subDict("mug")},
  K_{dict.subDict("K")},
  Kg_{dict.subDict("Kg")},
  sigma_{dict.subDict("sigma")},
  D_{dict.subDict("D")}
{}


mousse::N2::N2(const N2& liq)
:
  liquidProperties{liq},
  rho_{liq.rho_},
  pv_{liq.pv_},
  hl_{liq.hl_},
  Cp_{liq.Cp_},
  h_{liq.h_},
  Cpg_{liq.Cpg_},
  B_{liq.B_},
  mu_{liq.mu_},
  mug_{liq.mug_},
  K_{liq.K_},
  Kg_{liq.Kg_},
  sigma_{liq.sigma_},
  D_{liq.D_}
{}

