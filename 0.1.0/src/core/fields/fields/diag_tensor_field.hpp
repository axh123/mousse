#ifndef CORE_FIELDS_FIELDS_DIAG_TENSOR_FIELD_HPP_
#define CORE_FIELDS_FIELDS_DIAG_TENSOR_FIELD_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "tensor_field.hpp"
#include "diag_tensor.hpp"
#define TEMPLATE
#include "field_functions_m.inc"


namespace mousse {

typedef Field<diagTensor> diagTensorField;
UNARY_FUNCTION(diagTensor, tensor, diag)
UNARY_FUNCTION(scalar, diagTensor, tr)
UNARY_FUNCTION(sphericalTensor, diagTensor, sph)
UNARY_FUNCTION(scalar, diagTensor, det)
UNARY_FUNCTION(diagTensor, diagTensor, inv)
BINARY_OPERATOR(tensor, diagTensor, tensor, +, add)
BINARY_OPERATOR(tensor, diagTensor, tensor, -, subtract)
BINARY_TYPE_OPERATOR(tensor, diagTensor, tensor, +, add)
BINARY_TYPE_OPERATOR(tensor, diagTensor, tensor, -, subtract)
BINARY_OPERATOR(vector, vector, diagTensor, /, divide)
BINARY_TYPE_OPERATOR(vector, vector, diagTensor, /, divide)

}  // namespace mousse

#include "undef_field_functions_m.inc"

#endif
