{
  // Foam version 2.1 changes the addressing of faces in faceProcAddressing
  // The following code checks and modifies the addressing for cases where
  // the decomposition has been done with the foam2.0 and earlier tools, but
  // the reconstruction is attempted with version 2.1 or later
  label minFaceIndex = labelMax;
  PtrList<labelIOList>& faceProcAddressing = procMeshes.faceProcAddressing();
  /*
  FOR_ALL(faceProcAddressing, procI)
  {
    const labelList& curFaceAddr = faceProcAddressing[procI];
    FOR_ALL(curFaceAddr, faceI)
    {
      if (mag(curFaceAddr[faceI]) < minFaceIndex)
      {
        minFaceIndex = mag(curFaceAddr[faceI]);
      }
    }
  }
  */
  for (const auto& curFaceAddr : faceProcAddressing)
  {
    for (const auto& face : curFaceAddr)
    {
      if (mag(face) < minFaceIndex)
      {
        minFaceIndex = mag(face);
      }
    }
  }
  if (minFaceIndex < 1)
  {
    WARNING_IN(args.executable())
      << "parallel decomposition addressing." << endl
      << "It looks like you are trying to reconstruct the case "
      << "decomposed with an earlier version of FOAM, which could\n"
      << "potentially cause compatibility problems.  The code will "
      << "attempt to update the addressing automatically; in case of\n"
      << "failure, please repeat the decomposition of the case using "
      << "the current version fo decomposePar"
      << endl;
    FOR_ALL(faceProcAddressing, procI)
    {
      labelList& curFaceAddr = faceProcAddressing[procI];
      FOR_ALL(curFaceAddr, faceI)
      {
        curFaceAddr[faceI] += sign(curFaceAddr[faceI]);
      }
      faceProcAddressing[procI].write();
    }
  }
}