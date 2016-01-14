// mousse: CFD toolbox
// Copyright (C) 2012 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "remove_registered_object_function_object.hpp"
// Static Data Members
namespace mousse
{
  DEFINE_NAMED_TEMPLATE_TYPE_NAME_AND_DEBUG
  (
    removeRegisteredObjectFunctionObject,
    0
  );
  ADD_TO_RUN_TIME_SELECTION_TABLE
  (
    functionObject,
    removeRegisteredObjectFunctionObject,
    dictionary
  );
}
