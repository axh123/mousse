Info << "Reading field p\n" << endl;
volScalarField p
{
  {
    "p",
    runTime.timeName(),
    mesh,
    IOobject::MUST_READ,
    IOobject::AUTO_WRITE
  },
  mesh
};
volScalarField rho
{
  {
    "rho",
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
mesh.setFluxRequired(p.name());
// Mass flux (corrected by rhoEqn.H)
surfaceScalarField rhoPhi
{
  {
    "rhoPhi",
    runTime.timeName(),
    mesh
  },
  fvc::interpolate(rho)*phi
};
Info << "Reading transportProperties\n" << endl;
incompressibleTwoPhaseMixture mixture{U, phi};
volScalarField& alphav{mixture.alpha1()};
alphav.oldTime();
volScalarField& alphal{mixture.alpha2()};
Info << "Creating compressibilityModel\n" << endl;
autoPtr<barotropicCompressibilityModel> psiModel
{
  barotropicCompressibilityModel::New(thermodynamicProperties, alphav)
};
const volScalarField& psi = psiModel->psi();
rho ==
  max(psi*p + alphal*rhol0 + ((alphav*psiv + alphal*psil) - psi)*pSat, rhoMin);
mesh.setFluxRequired(p.name());
mesh.setFluxRequired(rho.name());
// Create incompressible turbulence model
autoPtr<incompressible::turbulenceModel> turbulence
{
  incompressible::turbulenceModel::New(U, phi, mixture)
};
