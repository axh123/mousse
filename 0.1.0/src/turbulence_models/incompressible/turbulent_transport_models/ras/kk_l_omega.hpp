#ifndef TURBULENCE_MODELS_INCOMPRESSIBLE_TURBULENT_TRANSPORT_MODELS_RAS_KK_L_OMEGA_HPP_
#define TURBULENCE_MODELS_INCOMPRESSIBLE_TURBULENT_TRANSPORT_MODELS_RAS_KK_L_OMEGA_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::incompressible::RASModels::kkLOmega
// Group
//   grpIcoRASTurbulence
// Description
//   Low Reynolds-number k-kl-omega turbulence model for
//   incompressible flows.
//   This turbulence model is described in:
//   \verbatim
//     Walters, D. K., & Cokljat, D. (2008).
//     A three-equation eddy-viscosity model for Reynolds-averaged
//     Navier–Stokes simulations of transitional flow.
//     Journal of Fluids Engineering, 130(12), 121401.
//   \endverbatim
//   however the paper contains several errors which must be corrected for the
//   model to operation correctly as explained in
//   \verbatim
//     Furst, J. (2013).
//     Numerical simulation of transitional flows with laminar kinetic energy.
//     Engineering MECHANICS, 20(5), 379-388.
//   \endverbatim
//   All these corrections and updates are included in this implementation.
//   The default model coefficients are
//   \verbatim
//     kkLOmegaCoeffs
//     {
//       A0             4.04
//       As             2.12
//       Av             6.75
//       Abp            0.6
//       Anat           200
//       Ats            200
//       CbpCrit        1.2
//       Cnc            0.1
//       CnatCrit       1250
//       Cint           0.75
//       CtsCrit        1000
//       CrNat          0.02
//       C11            3.4e-6
//       C12            1.0e-10
//       CR             0.12
//       CalphaTheta    0.035
//       Css            1.5
//       CtauL          4360
//       Cw1            0.44
//       Cw2            0.92
//       Cw3            0.3
//       CwR            1.5
//       Clambda        2.495
//       CmuStd         0.09
//       Prtheta        0.85
//       Sigmak         1
//       Sigmaw         1.17
//     }
//   \endverbatim

#include "turbulent_transport_model.hpp"
#include "eddy_viscosity.hpp"


namespace mousse {
namespace incompressible {
namespace RASModels {

class kkLOmega
:
  public eddyViscosity<incompressible::RASModel>
{
  // Private memmber functions
    tmp<volScalarField> fv(const volScalarField& Ret) const;
    tmp<volScalarField> fINT() const;
    tmp<volScalarField> fSS(const volScalarField& omega) const;
    tmp<volScalarField> Cmu(const volScalarField& S) const;
    tmp<volScalarField> BetaTS(const volScalarField& Rew) const;
    tmp<volScalarField> fTaul
    (
      const volScalarField& lambdaEff,
      const volScalarField& ktL,
      const volScalarField& omega
    ) const;
    tmp<volScalarField> alphaT
    (
      const volScalarField& lambdaEff,
      const volScalarField& fv,
      const volScalarField& ktS
    ) const;
    tmp<volScalarField> fOmega
    (
      const volScalarField& lambdaEff,
      const volScalarField& lambdaT
    ) const;
    tmp<volScalarField> phiBP(const volScalarField& omega) const;
    tmp<volScalarField> phiNAT
    (
      const volScalarField& ReOmega,
      const volScalarField& fNatCrit
    ) const;
    tmp<volScalarField> D(const volScalarField& k) const;
protected:
  // Protected data
    // Model coefficients
      dimensionedScalar A0_;
      dimensionedScalar As_;
      dimensionedScalar Av_;
      dimensionedScalar Abp_;
      dimensionedScalar Anat_;
      dimensionedScalar Ats_;
      dimensionedScalar CbpCrit_;
      dimensionedScalar Cnc_;
      dimensionedScalar CnatCrit_;
      dimensionedScalar Cint_;
      dimensionedScalar CtsCrit_;
      dimensionedScalar CrNat_;
      dimensionedScalar C11_;
      dimensionedScalar C12_;
      dimensionedScalar CR_;
      dimensionedScalar CalphaTheta_;
      dimensionedScalar Css_;
      dimensionedScalar CtauL_;
      dimensionedScalar Cw1_;
      dimensionedScalar Cw2_;
      dimensionedScalar Cw3_;
      dimensionedScalar CwR_;
      dimensionedScalar Clambda_;
      dimensionedScalar CmuStd_;
      dimensionedScalar Prtheta_;
      dimensionedScalar Sigmak_;
      dimensionedScalar Sigmaw_;
    // Fields
      volScalarField kt_;
      volScalarField kl_;
      volScalarField omega_;
      volScalarField epsilon_;
      //- Wall distance
      //  Note: different to wall distance in parent RASModel
      //  which is for near-wall cells only
      const volScalarField& y_;
  // Protected Member Functions
    virtual void correctNut();
public:
  //- Runtime type information
  TYPE_NAME("kkLOmega");
  // Constructors
    //- Construct from components
    kkLOmega
    (
      const geometricOneField& alpha,
      const geometricOneField& rho,
      const volVectorField& U,
      const surfaceScalarField& alphaRhoPhi,
      const surfaceScalarField& phi,
      const transportModel& transport,
      const word& propertiesName = turbulenceModel::propertiesName,
      const word& type = typeName
    );
  //- Destructor
  virtual ~kkLOmega()
  {}
  // Member Functions
    //- Read RASProperties dictionary
    virtual bool read();
    //- Return the effective diffusivity for k
    tmp<volScalarField> DkEff(const volScalarField& alphaT) const
    {
      return tmp<volScalarField>
      {
        new volScalarField{"DkEff", alphaT/Sigmak_ + nu()}
      };
    }
    //- Return the effective diffusivity for omega
    tmp<volScalarField> DomegaEff(const volScalarField& alphaT) const
    {
      return tmp<volScalarField>
      {
        new volScalarField{"DomegaEff", alphaT/Sigmaw_ + nu()}
      };
    }
    //- Return the laminar kinetic energy
    virtual tmp<volScalarField> kl() const
    {
      return kl_;
    }
    //- Return the turbulence kinetic energy
    virtual tmp<volScalarField> kt() const
    {
      return kt_;
    }
    //- Return the turbulence specific dissipation rate
    virtual tmp<volScalarField> omega() const
    {
      return omega_;
    }
    //- Return the total fluctuation kinetic energy
    virtual tmp<volScalarField> k() const
    {
      return tmp<volScalarField>
      {
        new volScalarField
        {
          {
            "k",
            mesh_.time().timeName(),
            mesh_
          },
          kt_ + kl_,
          omega_.boundaryField().types()
        }
      };
    }
    //- Return the total fluctuation kinetic energy dissipation rate
    virtual tmp<volScalarField> epsilon() const
    {
      return epsilon_;
    }
    //- Solve the turbulence equations and correct the turbulence viscosity
    virtual void correct();
};

}  // namespace RASModels
}  // namespace incompressible
}  // namespace mousse

#endif

