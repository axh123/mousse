#ifndef COMBUSTION_MODELS_FSD_HPP_
#define COMBUSTION_MODELS_FSD_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::combustionModels::FSD
// Description
//   Flame Surface Dennsity (FDS) combustion model.
//   The fuel source term is given by mgft*pc*omegaFuelBar.
//   where:
//      mgft: filtered flame area.
//      pc:   probability of the combustion progress.
//      omegaFuelBar: filtered consumption speed per unit of flame area.
//   pc is considered from the IFC solution.
//   omegaFuelBar is calculated solving a relaxation equation which tends to
//   omegaEq. This omegaEq is obtained from the flamelet solution for
//   different strain rates and fit using a expential distribution.
//   The spacial distribution of the consumption speed (omega) is obtained also
//   from a strained flamelet solution and it is assumed to have a guassian
//   distribution.
//   If the grid resolution is not enough to resolve the flame, the consumption
//   speed distribution is linearly thickened conserving the overall heat
//   release.
//   If the turbulent fluctuation of the mixture fraction at the sub-grid level
//   is large (>1e-04) then a beta pdf is used for filtering.
//   At the moment the flame area combustion model is only fit to work in a LES
//   frame work. In RAS the subgrid fluctiuation has to be solved by an extra
//   transport equation.

#include "single_step_combustion.hpp"
#include "reaction_rate_flame_area.hpp"


namespace mousse {
namespace combustionModels {

template<class CombThermoType, class ThermoType>
class FSD
:
  public singleStepCombustion <CombThermoType, ThermoType>
{
  // Private data
    //- Auto pointer to consumption speed per unit of flame area model
    autoPtr<reactionRateFlameArea> reactionRateFlameArea_;
    //- Mixture fraction
    volScalarField ft_;
    //- Fuel mass concentration on the fuel stream
    dimensionedScalar YFuelFuelStream_;
    //- Oxygen mass concentration on the oxydizer stream
    dimensionedScalar YO2OxiStream_;
    //- Similarity constant for the sub-grid ft fluctuations
    scalar Cv_;
    //- Model constant
    scalar C_;
    //- Lower flammability limit
    scalar ftMin_;
    //- Upper flammability limit
    scalar ftMax_;
    //- Dimension of the ft space. Used to integrate the beta-pdf
    scalar ftDim_;
    //- Minimum mixture freaction variance to calculate pdf
    scalar ftVarMin_;
  // Private Member Functions
    //- Calculate the normalised fuel source term
    void calculateSourceNorm();
public:
  //- Runtime type information
  TYPE_NAME("FSD");
  // Constructors
    //- Construct from components
    FSD(const word& modelType, const fvMesh& mesh, const word& phaseName);
    //- Disallow copy construct
    FSD(const FSD&) = delete;
    //- Disallow default bitwise assignment
    FSD& operator=(const FSD&) = delete;
  // Destructor
    virtual ~FSD();
  // Evolution
    //- Correct combustion rate
    virtual void correct();
  // IO
    //- Update properties
    virtual bool read();
};

}  // namespace combustionModels
}  // namespace mousse

#include "fsd.ipp"

#endif
