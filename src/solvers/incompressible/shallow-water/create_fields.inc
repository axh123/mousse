Info << "Reading field h\n" << endl;
volScalarField h
{
  {
    "h",
    runTime.timeName(),
    mesh,
    IOobject::MUST_READ,
    IOobject::AUTO_WRITE
  },
  mesh
};
Info << "Reading field h0 if present\n" << endl;
volScalarField h0
{
  {
    "h0",
    runTime.findInstance("polyMesh", "points"),
    mesh,
    IOobject::READ_IF_PRESENT
  },
  mesh,
  {"h0", dimLength, 0.0}
};
Info << "Creating field hU\n" << endl;
volVectorField hU
{
  {
    "hU",
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
    IOobject::NO_READ,
    IOobject::AUTO_WRITE
  },
  hU/h
};
Info << "Creating field hTotal for post processing\n" << endl;
volScalarField hTotal
{
  {
    "hTotal",
    runTime.timeName(),
    mesh,
    IOobject::READ_IF_PRESENT,
    IOobject::AUTO_WRITE
  },
  h+h0
};
hTotal.write();
#include "create_phi.inc"
Info << "Creating Coriolis Force" << endl;
const dimensionedVector F{"F", ((2.0*Omega) & gHat)*gHat};
mesh.setFluxRequired(h.name());