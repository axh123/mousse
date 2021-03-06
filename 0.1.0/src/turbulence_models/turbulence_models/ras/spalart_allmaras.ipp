// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "spalart_allmaras.hpp"
#include "bound.hpp"
#include "wall_dist.hpp"


namespace mousse {
namespace RASModels {

// Protected Member Functions 
template<class BasicTurbulenceModel>
tmp<volScalarField> SpalartAllmaras<BasicTurbulenceModel>::chi() const
{
  return nuTilda_/this->nu();
}


template<class BasicTurbulenceModel>
tmp<volScalarField> SpalartAllmaras<BasicTurbulenceModel>::fv1
(
  const volScalarField& chi
) const
{
  const volScalarField chi3(pow3(chi));
  return chi3/(chi3 + pow3(Cv1_));
}


template<class BasicTurbulenceModel>
tmp<volScalarField> SpalartAllmaras<BasicTurbulenceModel>::fv2
(
  const volScalarField& chi,
  const volScalarField& fv1
) const
{
  return 1.0 - chi/(1.0 + chi*fv1);
}


template<class BasicTurbulenceModel>
tmp<volScalarField> SpalartAllmaras<BasicTurbulenceModel>::Stilda
(
  const volScalarField& chi,
  const volScalarField& fv1
) const
{
  volScalarField Omega{::sqrt(2.0)*mag(skew(fvc::grad(this->U_)))};
  return
    max
    (
      Omega + fv2(chi, fv1)*nuTilda_/sqr(kappa_*y_),
      Cs_*Omega
    );
}


template<class BasicTurbulenceModel>
tmp<volScalarField> SpalartAllmaras<BasicTurbulenceModel>::fw
(
  const volScalarField& Stilda
) const
{

  const dimensionedScalar Tsmall{"SMALL", Stilda.dimensions(), SMALL};
  volScalarField r
  {
    min(nuTilda_/max(Stilda, Tsmall)*sqr(kappa_*y_), scalar(10.0))
  };
  r.boundaryField() == 0.0;
  const volScalarField g{r + Cw2_*(pow6(r) - r)};
  return g*pow((1.0 + pow6(Cw3_))/(pow6(g) + pow6(Cw3_)), 1.0/6.0);
}


template<class BasicTurbulenceModel>
void SpalartAllmaras<BasicTurbulenceModel>::correctNut
(
  const volScalarField& fv1
)
{
  this->nut_ = nuTilda_*fv1;
  this->nut_.correctBoundaryConditions();
  BasicTurbulenceModel::correctNut();
}


template<class BasicTurbulenceModel>
void SpalartAllmaras<BasicTurbulenceModel>::correctNut()
{
  correctNut(fv1(this->chi()));
}


// Constructors 
template<class BasicTurbulenceModel>
SpalartAllmaras<BasicTurbulenceModel>::SpalartAllmaras
(
  const alphaField& alpha,
  const rhoField& rho,
  const volVectorField& U,
  const surfaceScalarField& alphaRhoPhi,
  const surfaceScalarField& phi,
  const transportModel& transport,
  const word& propertiesName,
  const word& type
)
:
  eddyViscosity<RASModel<BasicTurbulenceModel>>
  {
    type,
    alpha,
    rho,
    U,
    alphaRhoPhi,
    phi,
    transport,
    propertiesName
  },
  sigmaNut_
  {
    dimensioned<scalar>::lookupOrAddToDict
    (
      "sigmaNut",
      this->coeffDict_,
      0.66666
    )
  },
  kappa_
  {
    dimensioned<scalar>::lookupOrAddToDict
    (
      "kappa",
      this->coeffDict_,
      0.41
    )
  },
  Cb1_
  {
    dimensioned<scalar>::lookupOrAddToDict
    (
      "Cb1",
      this->coeffDict_,
      0.1355
    )
  },
  Cb2_
  {
    dimensioned<scalar>::lookupOrAddToDict
    (
      "Cb2",
      this->coeffDict_,
      0.622
    )
  },
  Cw1_{Cb1_/sqr(kappa_) + (1.0 + Cb2_)/sigmaNut_},
  Cw2_
  {
    dimensioned<scalar>::lookupOrAddToDict
    (
      "Cw2",
      this->coeffDict_,
      0.3
    )
  },
  Cw3_
  {
    dimensioned<scalar>::lookupOrAddToDict
    (
      "Cw3",
      this->coeffDict_,
      2.0
    )
  },
  Cv1_
  {
    dimensioned<scalar>::lookupOrAddToDict
    (
      "Cv1",
      this->coeffDict_,
      7.1
    )
  },
  Cs_
  {
    dimensioned<scalar>::lookupOrAddToDict
    (
      "Cs",
      this->coeffDict_,
      0.3
    )
  },
  nuTilda_
  {
    IOobject
    {
      "nuTilda",
      this->runTime_.timeName(),
      this->mesh_,
      IOobject::MUST_READ,
      IOobject::AUTO_WRITE
    },
    this->mesh_
  },
  y_{wallDist::New(this->mesh_).y()}
{
  if (type == typeName) {
    correctNut();
    this->printCoeffs(type);
  }
}


// Member Functions 
template<class BasicTurbulenceModel>
bool SpalartAllmaras<BasicTurbulenceModel>::read()
{
  if (eddyViscosity<RASModel<BasicTurbulenceModel>>::read()) {
    sigmaNut_.readIfPresent(this->coeffDict());
    kappa_.readIfPresent(this->coeffDict());
    Cb1_.readIfPresent(this->coeffDict());
    Cb2_.readIfPresent(this->coeffDict());
    Cw1_ = Cb1_/sqr(kappa_) + (1.0 + Cb2_)/sigmaNut_;
    Cw2_.readIfPresent(this->coeffDict());
    Cw3_.readIfPresent(this->coeffDict());
    Cv1_.readIfPresent(this->coeffDict());
    Cs_.readIfPresent(this->coeffDict());
    return true;
  }
  return false;
}


template<class BasicTurbulenceModel>
tmp<volScalarField> SpalartAllmaras<BasicTurbulenceModel>::DnuTildaEff() const
{
  return
    tmp<volScalarField>
    {
      new volScalarField{"DnuTildaEff", (nuTilda_ + this->nu())/sigmaNut_}
    };
}


template<class BasicTurbulenceModel>
tmp<volScalarField> SpalartAllmaras<BasicTurbulenceModel>::k() const
{
  return
    tmp<volScalarField>
    {
      new volScalarField
      {
        {
          "k",
          this->runTime_.timeName(),
          this->mesh_
        },
        this->mesh_,
        {"0", dimensionSet(0, 2, -2, 0, 0), 0}
      }
    };
}


template<class BasicTurbulenceModel>
tmp<volScalarField> SpalartAllmaras<BasicTurbulenceModel>::epsilon() const
{
  WARNING_IN("tmp<volScalarField> SpalartAllmaras::epsilon() const")
    << "Turbulence kinetic energy dissipation rate not defined for "
    << "Spalart-Allmaras model. Returning zero field"
    << endl;
  return
    tmp<volScalarField>
    {
      new volScalarField
      {
        {
          "epsilon",
          this->runTime_.timeName(),
          this->mesh_
        },
        this->mesh_,
        {"0", {0, 2, -3, 0, 0}, 0}
      }
    };
}


template<class BasicTurbulenceModel>
void SpalartAllmaras<BasicTurbulenceModel>::correct()
{
  if (!this->turbulence_) {
    return;
  }
  // Local references
  const alphaField& alpha = this->alpha_;
  const rhoField& rho = this->rho_;
  const surfaceScalarField& alphaRhoPhi = this->alphaRhoPhi_;
  eddyViscosity<RASModel<BasicTurbulenceModel>>::correct();
  const volScalarField chi{this->chi()};
  const volScalarField fv1{this->fv1(chi)};
  const volScalarField Stilda{this->Stilda(chi, fv1)};
  tmp<fvScalarMatrix> nuTildaEqn {
    fvm::ddt(alpha, rho, nuTilda_)
  + fvm::div(alphaRhoPhi, nuTilda_)
  - fvm::laplacian(alpha*rho*DnuTildaEff(), nuTilda_)
  - Cb2_/sigmaNut_*alpha*rho*magSqr(fvc::grad(nuTilda_))
  ==
    Cb1_*alpha*rho*Stilda*nuTilda_
  - fvm::Sp(Cw1_*alpha*rho*fw(Stilda)*nuTilda_/sqr(y_), nuTilda_)
  };
  nuTildaEqn().relax();
  solve(nuTildaEqn);
  bound(nuTilda_, dimensionedScalar("0", nuTilda_.dimensions(), 0.0));
  nuTilda_.correctBoundaryConditions();
  correctNut(fv1);
}

}  // namespace RASModels
}  // namespace mousse

