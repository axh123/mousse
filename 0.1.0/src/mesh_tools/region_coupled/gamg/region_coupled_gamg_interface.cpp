// mousse: CFD toolbox
// Copyright (C) 2011-2013 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "region_coupled_gamg_interface.hpp"
#include "add_to_run_time_selection_table.hpp"


// Static Data Members
namespace mousse {

DEFINE_TYPE_NAME_AND_DEBUG(regionCoupledGAMGInterface, 0);
ADD_TO_RUN_TIME_SELECTION_TABLE
(
  GAMGInterface,
  regionCoupledGAMGInterface,
  lduInterface
);

}


// Constructors 
mousse::regionCoupledGAMGInterface::regionCoupledGAMGInterface
(
  const label index,
  const lduInterfacePtrsList& coarseInterfaces,
  const lduInterface& fineInterface,
  const labelField& localRestrictAddressing,
  const labelField& neighbourRestrictAddressing,
  const label fineLevelIndex,
  const label coarseComm
)
:
  regionCoupledBaseGAMGInterface
  {
    index,
    coarseInterfaces,
    fineInterface,
    localRestrictAddressing,
    neighbourRestrictAddressing,
    fineLevelIndex,
    coarseComm
  }
{}


// Desstructor
mousse::regionCoupledGAMGInterface::~regionCoupledGAMGInterface()
{}
