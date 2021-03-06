  const wordList solidsNames{rp["solid"]};
  PtrList<fvMesh> solidRegions{solidsNames.size()};
  FOR_ALL(solidsNames, i) {
    Info << "Create solid mesh for region " << solidsNames[i]
      << " for time = " << runTime.timeName() << nl << endl;
    solidRegions.set
    (
      i,
      new fvMesh
      {
        {
          solidsNames[i],
          runTime.timeName(),
          runTime,
          IOobject::MUST_READ
        }
      }
    );
  }
