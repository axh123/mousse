// mousse: CFD toolbox
// Copyright (C) 2013-2014 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "eager_gamg_proc_agglomeration.hpp"
#include "add_to_run_time_selection_table.hpp"
#include "gamg_agglomeration.hpp"


// Static Data Members
namespace mousse {

DEFINE_TYPE_NAME_AND_DEBUG(eagerGAMGProcAgglomeration, 0);

ADD_TO_RUN_TIME_SELECTION_TABLE
(
  GAMGProcAgglomeration,
  eagerGAMGProcAgglomeration,
  GAMGAgglomeration
);

}

// Private Member Functions
// Constructors
mousse::eagerGAMGProcAgglomeration::eagerGAMGProcAgglomeration
(
  GAMGAgglomeration& agglom,
  const dictionary& controlDict
)
:
  GAMGProcAgglomeration{agglom, controlDict},
  mergeLevels_{controlDict.lookupOrDefault<label>("mergeLevels", 1)}
{}


// Destructor
mousse::eagerGAMGProcAgglomeration::~eagerGAMGProcAgglomeration()
{
  FOR_ALL_REVERSE(comms_, i) {
    if (comms_[i] != -1) {
      UPstream::freeCommunicator(comms_[i]);
    }
  }
}


// Member Functions
bool mousse::eagerGAMGProcAgglomeration::agglomerate()
{
  if (debug) {
    Pout << nl << "Starting mesh overview" << endl;
    printStats(Pout, agglom_);
  }
  if (agglom_.size() >= 1) {
    // Agglomerate one but last level (since also agglomerating
    // restrictAddressing)
    for
    (
      label fineLevelIndex = 2;
      fineLevelIndex < agglom_.size();
      fineLevelIndex++
    ) {
      if (agglom_.hasMeshLevel(fineLevelIndex)) {
        // Get the fine mesh
        const lduMesh& levelMesh = agglom_.meshLevel(fineLevelIndex);
        label levelComm = levelMesh.comm();
        label nProcs = UPstream::nProcs(levelComm);
        if (nProcs > 1) {
          // Processor restriction map: per processor the coarse
          // processor
          labelList procAgglomMap{nProcs};
          FOR_ALL(procAgglomMap, procI) {
            procAgglomMap[procI] = procI/(1<<mergeLevels_);
          }
          // Master processor
          labelList masterProcs;
          // Local processors that agglomerate. agglomProcIDs[0]
          // is in masterProc.
          List<label> agglomProcIDs;
          GAMGAgglomeration::calculateRegionMaster
          (
            levelComm,
            procAgglomMap,
            masterProcs,
            agglomProcIDs
          );
          // Allocate a communicator for the processor-agglomerated
          // matrix
          comms_.append
          (
            UPstream::allocateCommunicator
            (
              levelComm,
              masterProcs
            )
          );
          // Use procesor agglomeration maps to do the actual
          // collecting.
          if (Pstream::myProcNo(levelComm) != -1) {
            GAMGProcAgglomeration::agglomerate
            (
              fineLevelIndex,
              procAgglomMap,
              masterProcs,
              agglomProcIDs,
              comms_.last()
            );
          }
        }
      }
    }
  }
  // Print a bit
  if (debug) {
    Pout << nl << "Agglomerated mesh overview" << endl;
    printStats(Pout, agglom_);
  }
  return true;
}
