// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "nsrds_func7.hpp"
#include "add_to_run_time_selection_table.hpp"


namespace mousse {

DEFINE_TYPE_NAME_AND_DEBUG(NSRDSfunc7, 0);
ADD_TO_RUN_TIME_SELECTION_TABLE(thermophysicalFunction, NSRDSfunc7, Istream);
ADD_TO_RUN_TIME_SELECTION_TABLE(thermophysicalFunction, NSRDSfunc7, dictionary);

}


// Constructors 
mousse::NSRDSfunc7::NSRDSfunc7
(
  const scalar a,
  const scalar b,
  const scalar c,
  const scalar d,
  const scalar e
)
:
  a_{a},
  b_{b},
  c_{c},
  d_{d},
  e_{e}
{}


mousse::NSRDSfunc7::NSRDSfunc7(Istream& is)
:
  a_{readScalar(is)},
  b_{readScalar(is)},
  c_{readScalar(is)},
  d_{readScalar(is)},
  e_{readScalar(is)}
{}


mousse::NSRDSfunc7::NSRDSfunc7(const dictionary& dict)
:
  a_{readScalar(dict.lookup("a"))},
  b_{readScalar(dict.lookup("b"))},
  c_{readScalar(dict.lookup("c"))},
  d_{readScalar(dict.lookup("d"))},
  e_{readScalar(dict.lookup("e"))}
{}

