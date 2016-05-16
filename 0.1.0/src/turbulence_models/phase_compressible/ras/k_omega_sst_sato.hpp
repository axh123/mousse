#ifndef TURBULENCE_MODELS_PHASE_COMPRESSIBLE_RAS_K_OMEGA_SST_SATO_HPP_
#define TURBULENCE_MODELS_PHASE_COMPRESSIBLE_RAS_K_OMEGA_SST_SATO_HPP_

// mousse: CFD toolbox
// Copyright (C) 2014-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::RASModels::kOmegaSSTSato
// Group
//   grpRASTurbulence
// Description
//   Implementation of the k-omega-SST turbulence model for dispersed bubbly
//   flows with Sato (1981) bubble induced turbulent viscosity model.
//   Bubble induced turbulent viscosity model described in:
//   \verbatim
//     Sato, Y., Sadatomi, M.
//     "Momentum and heat transfer in two-phase bubble flow - I, Theory"
//     International Journal of Multiphase FLow 7, pp. 167-177, 1981.
//   \endverbatim
//   Turbulence model described in:
//   \verbatim
//     Menter, F., Esch, T.
//     "Elements of Industrial Heat Transfer Prediction"
//     16th Brazilian Congress of Mechanical Engineering (COBEM),
//     Nov. 2001
//   \endverbatim
//   with the addition of the optional F3 term for rough walls from
//   \verbatim
//     Hellsten, A.
//     "Some Improvements in Menter’s k-omega-SST turbulence model"
//     29th AIAA Fluid Dynamics Conference,
//     AIAA-98-2554,
//     June 1998.
//   \endverbatim
//   Note that this implementation is written in terms of alpha diffusion
//   coefficients rather than the more traditional sigma (alpha = 1/sigma) so
//   that the blending can be applied to all coefficuients in a consistent
//   manner.  The paper suggests that sigma is blended but this would not be
//   consistent with the blending of the k-epsilon and k-omega models.
//   Also note that the error in the last term of equation (2) relating to
//   sigma has been corrected.
//   Wall-functions are applied in this implementation by using equations (14)
//   to specify the near-wall omega as appropriate.
//   The blending functions (15) and (16) are not currently used because of the
//   uncertainty in their origin, range of applicability and that is y+ becomes
//   sufficiently small blending u_tau in this manner clearly becomes nonsense.
//   The default model coefficients correspond to the following:
//   \verbatim
//     kOmegaSSTCoeffs
//     {
//       alphaK1     0.85034;
//       alphaK2     1.0;
//       alphaOmega1 0.5;
//       alphaOmega2 0.85616;
//       Prt         1.0;    // only for compressible
//       beta1       0.075;
//       beta2       0.0828;
//       betaStar    0.09;
//       gamma1      0.5532;
//       gamma2      0.4403;
//       a1          0.31;
//       b1          1.0;
//       c1          10.0;
//       F3          no;
//       Cmub        0.6;
//     }
//   \endverbatim

#include "k_omega_sst.hpp"


namespace mousse {
namespace RASModels {

template<class BasicTurbulenceModel>
class kOmegaSSTSato
:
  public kOmegaSST<BasicTurbulenceModel>
{
  // Private data
    mutable const PhaseCompressibleTurbulenceModel
    <
      typename BasicTurbulenceModel::transportModel
    > *gasTurbulencePtr_;
  // Private Member Functions
    //- Return the turbulence model for the gas phase
    const PhaseCompressibleTurbulenceModel
    <
      typename BasicTurbulenceModel::transportModel
    >&
    gasTurbulence() const;
protected:
  // Protected data
    // Model coefficients
      dimensionedScalar Cmub_;
  // Protected Member Functions
    virtual void correctNut();
public:
  typedef typename BasicTurbulenceModel::alphaField alphaField;
  typedef typename BasicTurbulenceModel::rhoField rhoField;
  typedef typename BasicTurbulenceModel::transportModel transportModel;
  //- Runtime type information
  TYPE_NAME("kOmegaSSTSato");
  // Constructors
    //- Construct from components
    kOmegaSSTSato
    (
      const alphaField& alpha,
      const rhoField& rho,
      const volVectorField& U,
      const surfaceScalarField& alphaRhoPhi,
      const surfaceScalarField& phi,
      const transportModel& transport,
      const word& propertiesName = turbulenceModel::propertiesName,
      const word& type = typeName
    );
    // Disallow default bitwise copy construct and assignment
    kOmegaSSTSato(const kOmegaSSTSato&) = delete;
    kOmegaSSTSato& operator=(const kOmegaSSTSato&) = delete;
  //- Destructor
  virtual ~kOmegaSSTSato()
  {}
  // Member Functions
    //- Read model coefficients if they have changed
    virtual bool read();
    //- Solve the turbulence equations and correct the turbulence viscosity
    virtual void correct();
};

}  // namespace RASModels
}  // namespace mousse

#include "k_omega_sst_sato.ipp"

#endif
