// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
//   tensor_field_field.cpp
#ifndef tensor_field_field_hpp_
#define tensor_field_field_hpp_
#include "spherical_tensor_field_field.hpp"
#include "symm_tensor_field_field.hpp"
#include "tensor.hpp"
#define TEMPLATE template<template<class> class Field>
#include "field_field_functions_m.hpp"
namespace mousse
{
UNARY_FUNCTION(scalar, tensor, tr)
UNARY_FUNCTION(sphericalTensor, tensor, sph)
UNARY_FUNCTION(symmTensor, tensor, symm)
UNARY_FUNCTION(symmTensor, tensor, twoSymm)
UNARY_FUNCTION(tensor, tensor, skew)
UNARY_FUNCTION(tensor, tensor, dev)
UNARY_FUNCTION(tensor, tensor, dev2)
UNARY_FUNCTION(scalar, tensor, det)
UNARY_FUNCTION(tensor, tensor, cof)
UNARY_FUNCTION(tensor, tensor, inv)
UNARY_FUNCTION(vector, tensor, eigenValues)
UNARY_FUNCTION(tensor, tensor, eigenVectors)
UNARY_FUNCTION(vector, symmTensor, eigenValues)
UNARY_FUNCTION(symmTensor, symmTensor, eigenVectors)
// global operators 
UNARY_OPERATOR(vector, tensor, *, hdual)
UNARY_OPERATOR(tensor, vector, *, hdual)
BINARY_OPERATOR(vector, vector, tensor, /, divide)
BINARY_TYPE_OPERATOR(vector, vector, tensor, /, divide)
}  // namespace mousse
#include "undef_field_functions_m.hpp"
#ifdef NoRepository
#   include "tensor_field_field.cpp"
#endif
#endif
