Info << "Reading thermophysical properties\n" << endl;
autoPtr<psiThermo> pThermo{psiThermo::New(mesh)};
psiThermo& thermo = pThermo();
thermo.validate(args.executable(), "h", "e");
volScalarField rho
{
  {
    "rho",
    runTime.timeName(),
    mesh,
    IOobject::READ_IF_PRESENT,
    IOobject::AUTO_WRITE
  },
  thermo.rho()
};
volScalarField& p = thermo.p();
const volScalarField& psi = thermo.psi();
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
#include "compressible_create_phi.inc"
label pRefCell = 0;
scalar pRefValue = 0.0;
setRefCell(p, simple.dict(), pRefCell, pRefValue);
mesh.setFluxRequired(p.name());
dimensionedScalar rhoMax
{
  dimensionedScalar::lookupOrDefault("rhoMax", simple.dict(), dimDensity, GREAT)
};
dimensionedScalar rhoMin
{
  dimensionedScalar::lookupOrDefault("rhoMin", simple.dict(), dimDensity, 0)
};
Info << "Creating turbulence model\n" << endl;
autoPtr<compressible::turbulenceModel> turbulence
{
  compressible::turbulenceModel::New(rho, U, phi, thermo)
};
dimensionedScalar initialMass = fvc::domainIntegrate(rho);
