  IOMRFZoneList MRF{mesh};
  FOR_ALL_ITER(PtrDictionary<phaseModel>, fluid.phases(), iter)
  {
    MRF.correctBoundaryVelocity(iter().U());
  }
  MRF.correctBoundaryVelocity(U);
