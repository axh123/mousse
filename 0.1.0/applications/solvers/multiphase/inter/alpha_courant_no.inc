// mousse: CFD toolbox
// Copyright (C) 2011-2014 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

scalar maxAlphaCo
{
  readScalar(runTime.controlDict().lookup("maxAlphaCo"))
};
scalar alphaCoNum = 0.0;
scalar meanAlphaCoNum = 0.0;
if (mesh.nInternalFaces()) {
  const auto& V = mesh.V().field();
  const auto dt = runTime.deltaTValue();
  scalarField sumPhi
  {
    mixture.nearInterface()().internalField()
    *fvc::surfaceSum(mag(phi))().internalField()
  };
  alphaCoNum = 0.5*gMax(sumPhi/V)*dt;
  meanAlphaCoNum = 0.5*(gSum(sumPhi)/gSum(V))*dt;
}
Info << "Interface Courant Number mean: " << meanAlphaCoNum
  << " max: " << alphaCoNum << endl;
