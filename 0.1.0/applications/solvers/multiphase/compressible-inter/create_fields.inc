Info << "Reading field p_rgh\n" << endl;
volScalarField p_rgh
{
  {
    "p_rgh",
    runTime.timeName(),
    mesh,
    IOobject::MUST_READ,
    IOobject::AUTO_WRITE
  },
  mesh
};
Info << "Reading field U\n" << endl;
volVectorField U
{
  {
    "U",
    runTime.timeName(),
    mesh,
    IOobject::MUST_READ,
    IOobject::AUTO_WRITE
  },
  mesh
};
#include "create_phi.inc"
Info << "Constructing twoPhaseMixtureThermo\n" << endl;
twoPhaseMixtureThermo mixture{mesh};
volScalarField& alpha1 = mixture.alpha1();
volScalarField& alpha2 = mixture.alpha2();
Info << "Reading thermophysical properties\n" << endl;
volScalarField& p = mixture.p();
volScalarField& T = mixture.T();
volScalarField& rho1 = mixture.thermo1().rho();
const volScalarField& psi1 = mixture.thermo1().psi();
volScalarField& rho2 = mixture.thermo2().rho();
const volScalarField& psi2 = mixture.thermo2().psi();
volScalarField rho
{
  {
    "rho",
    runTime.timeName(),
    mesh,
    IOobject::READ_IF_PRESENT,
    IOobject::AUTO_WRITE
  },
  alpha1*rho1 + alpha2*rho2
};
dimensionedScalar pMin{"pMin", dimPressure, mixture};
mesh.setFluxRequired(p_rgh.name());
#include "read_gravitational_acceleration.inc"
#include "read_href.inc"
#include "gh.inc"
// Mass flux
// Initialisation does not matter because rhoPhi is reset after the
// alpha1 solution before it is used in the U equation.
surfaceScalarField rhoPhi
{
  {
    "rhoPhi",
    runTime.timeName(),
    mesh
  },
  fvc::interpolate(rho)*phi
};
volScalarField dgdt{pos(alpha2)*fvc::div(phi)/max(alpha2, scalar(0.0001))};
// Construct interface from alpha1 distribution
interfaceProperties interface{alpha1, U, mixture};
// Construct compressible turbulence model
autoPtr<compressible::turbulenceModel> turbulence
{
  compressible::turbulenceModel::New(rho, U, rhoPhi, mixture)
};
Info << "Creating field kinetic energy K\n" << endl;
volScalarField K{"K", 0.5*magSqr(U)};
