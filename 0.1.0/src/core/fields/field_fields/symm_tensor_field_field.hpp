// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
//   symm_tensor_field_field.cpp
#ifndef symm_tensor_field_field_hpp_
#define symm_tensor_field_field_hpp_
#include "field_field.hpp"
#include "symm_tensor.hpp"
#define TEMPLATE template<template<class> class Field>
#include "field_field_functions_m.hpp"
namespace mousse
{
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
#include "undef_field_functions_m.hpp"
#ifdef NoRepository
#   include "symm_tensor_field_field.cpp"
#endif
#endif