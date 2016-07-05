Info << "Reading thermophysical properties\n" << endl;
autoPtr<psiThermo> pThermo{psiThermo::New(mesh)};
psiThermo& thermo = pThermo();
thermo.validate(args.executable(), "e");
volScalarField& p = thermo.p();
volScalarField& e = thermo.he();
const volScalarField& psi = thermo.psi();
volScalarField rho
{
  {
    "rho",
    runTime.timeName(),
    mesh
  },
  thermo.rho()
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
#include "compressible_create_phi.inc"
mesh.setFluxRequired(p.name());
Info << "Creating turbulence model\n" << endl;
autoPtr<compressible::turbulenceModel> turbulence
{
  compressible::turbulenceModel::New(rho, U, phi, thermo)
};
Info << "Creating field kinetic energy K\n" << endl;
volScalarField K{"K", 0.5*magSqr(U)};