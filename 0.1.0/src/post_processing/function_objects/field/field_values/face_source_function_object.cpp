// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "face_source_function_object.hpp"


// Static Data Members
namespace mousse {

DEFINE_NAMED_TEMPLATE_TYPE_NAME_AND_DEBUG
(
  faceSourceFunctionObject,
  0
);
ADD_TO_RUN_TIME_SELECTION_TABLE
(
  functionObject,
  faceSourceFunctionObject,
  dictionary
);

}

