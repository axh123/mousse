#ifndef CORE_FIELDS_FIELD_FIELDS_SYMM_TENSOR_FIELD_FIELD_HPP_
#define CORE_FIELDS_FIELD_FIELDS_SYMM_TENSOR_FIELD_FIELD_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "field_field.hpp"
#include "symm_tensor.hpp"
#define TEMPLATE template<template<class> class Field>
#include "field_field_functions_m.inc"


namespace mousse {

UNARY_FUNCTION(symmTensor, vector, sqr)
UNARY_FUNCTION(symmTensor, symmTensor, innerSqr)
UNARY_FUNCTION(scalar, symmTensor, tr)
UNARY_FUNCTION(sphericalTensor, symmTensor, sph)
UNARY_FUNCTION(symmTensor, symmTensor, symm)
UNARY_FUNCTION(symmTensor, symmTensor, twoSymm)
UNARY_FUNCTION(symmTensor, symmTensor, dev)
UNARY_FUNCTION(symmTensor, symmTensor, dev2)
UNARY_FUNCTION(scalar, symmTensor, det)
UNARY_FUNCTION(symmTensor, symmTensor, cof)
UNARY_FUNCTION(symmTensor, symmTensor, inv)
// global operators 
UNARY_OPERATOR(vector, symmTensor, *, hdual)
BINARY_OPERATOR(tensor, symmTensor, symmTensor, &, dot)
BINARY_TYPE_OPERATOR(tensor, symmTensor, symmTensor, &, dot)

}  // namespace mousse

#include "undef_field_functions_m.inc"
#include "symm_tensor_field_field.ipp"

#endif
