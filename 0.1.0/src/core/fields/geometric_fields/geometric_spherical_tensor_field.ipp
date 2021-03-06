// mousse: CFD toolbox
// Copyright (C) 2011-2013 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "geometric_spherical_tensor_field.hpp"
#define TEMPLATE template<template<class> class PatchField, class GeoMesh>
#include "geometric_field_functions_m.ipp"


namespace mousse {

// global functions 
UNARY_FUNCTION(scalar, sphericalTensor, tr, transform)
UNARY_FUNCTION(sphericalTensor, sphericalTensor, sph, transform)
UNARY_FUNCTION(scalar, sphericalTensor, det, pow3)
UNARY_FUNCTION(sphericalTensor, sphericalTensor, inv, inv)
BINARY_OPERATOR(sphericalTensor, scalar, sphericalTensor, /, '|', divide)
BINARY_TYPE_OPERATOR(sphericalTensor, scalar, sphericalTensor, /, '|', divide)

}  // namespace mousse

#include "undef_field_functions_m.inc"
