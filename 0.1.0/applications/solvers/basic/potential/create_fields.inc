Info << "Reading velocity field U\n" << endl;
volVectorField U
{
  // IOobject
  {
    "U",
    runTime.timeName(),
    mesh,
    IOobject::MUST_READ,
    IOobject::AUTO_WRITE
  },
  mesh
};
U = dimensionedVector{"0", U.dimensions(), vector::zero};
surfaceScalarField phi
{
  IOobject
  {
    "phi",
    runTime.timeName(),
    mesh,
    IOobject::NO_READ,
    IOobject::AUTO_WRITE
  },
  fvc::interpolate(U) & mesh.Sf()
};
if (args.optionFound("initialiseUBCs")) {
  U.correctBoundaryConditions();
  phi = fvc::interpolate(U) & mesh.Sf();
}
// Default name for the pressure field
word pName{"p"};
// Update name of the pressure field from the command-line option
args.optionReadIfPresent("pName", pName);
// Infer the pressure BCs from the velocity BCs
wordList pBCTypes
{
  U.boundaryField().size(),
  fixedValueFvPatchScalarField::typeName
};
FOR_ALL(U.boundaryField(), patchi) {
  if (U.boundaryField()[patchi].fixesValue()) {
    pBCTypes[patchi] = zeroGradientFvPatchScalarField::typeName;
  }
}
Info << "Constructing pressure field " << pName << nl << endl;
volScalarField p
{
  IOobject
  {
    pName,
    runTime.timeName(),
    mesh,
    IOobject::READ_IF_PRESENT,
    IOobject::NO_WRITE
  },
  mesh,
  dimensionedScalar(pName, sqr(dimVelocity), 0),
  pBCTypes
};
Info << "Constructing velocity potential field phiU\n" << endl;
volScalarField phiU
{
  IOobject
  {
    "phiU",
    runTime.timeName(),
    mesh,
    IOobject::READ_IF_PRESENT,
    IOobject::NO_WRITE
  },
  mesh,
  dimensionedScalar("phiU", dimLength*dimVelocity, 0),
  p.boundaryField().types()
};
label phiURefCell = 0;
scalar phiURefValue = 0;
setRefCell
(
  phiU,
  potentialFlow.dict(),
  phiURefCell,
  phiURefValue
);
mesh.setFluxRequired(phiU.name());
