// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "geometric_scalar_field.hpp"

#define TEMPLATE                                                              \
  template<class Type, template<class> class PatchField, class GeoMesh>
#include "geometric_field_functions_m.inc"

namespace mousse {

// Global functions
template<class Type, template<class> class PatchField, class GeoMesh>
void component
(
  GeometricField
  <
    typename GeometricField<Type, PatchField, GeoMesh>::cmptType,
    PatchField,
    GeoMesh
  >& gcf,
  const GeometricField<Type, PatchField, GeoMesh>& gf,
  const direction d
);

template<class Type, template<class> class PatchField, class GeoMesh>
void T
(
  GeometricField<Type, PatchField, GeoMesh>& gf,
  const GeometricField<Type, PatchField, GeoMesh>& gf1
);

template<class Type, template<class> class PatchField, class GeoMesh, int r>
void pow
(
  GeometricField<typename powProduct<Type, r>::type, PatchField, GeoMesh>& gf,
  const GeometricField<Type, PatchField, GeoMesh>& gf1
);

template<class Type, template<class> class PatchField, class GeoMesh, int r>
tmp
<
  GeometricField
  <typename powProduct<Type, r>::type, PatchField, GeoMesh>
>
pow
(
  const GeometricField<Type, PatchField, GeoMesh>& gf,
  typename powProduct<Type, r>::type
);

template<class Type, template<class> class PatchField, class GeoMesh, int r>
tmp
<
  GeometricField
  <typename powProduct<Type, r>::type, PatchField, GeoMesh>
>
pow
(
  const tmp<GeometricField<Type, PatchField, GeoMesh> >& tgf,
  typename powProduct<Type, r>::type
);

template<class Type, template<class> class PatchField, class GeoMesh>
void sqr
(
  GeometricField
  <typename outerProduct<Type, Type>::type, PatchField, GeoMesh>& gf,
  const GeometricField<Type, PatchField, GeoMesh>& gf1
);

template<class Type, template<class> class PatchField, class GeoMesh>
tmp
<
  GeometricField
  <
    typename outerProduct<Type, Type>::type,
    PatchField,
    GeoMesh
  >
>
sqr(const GeometricField<Type, PatchField, GeoMesh>& gf);

template<class Type, template<class> class PatchField, class GeoMesh>
tmp
<
  GeometricField
  <
    typename outerProduct<Type, Type>::type,
    PatchField,
    GeoMesh
  >
>
sqr(const tmp<GeometricField<Type, PatchField, GeoMesh> >& tgf);

template<class Type, template<class> class PatchField, class GeoMesh>
void magSqr
(
  GeometricField<scalar, PatchField, GeoMesh>& gsf,
  const GeometricField<Type, PatchField, GeoMesh>& gf
);

template<class Type, template<class> class PatchField, class GeoMesh>
tmp<GeometricField<scalar, PatchField, GeoMesh> > magSqr
(
  const GeometricField<Type, PatchField, GeoMesh>& gf
);

template<class Type, template<class> class PatchField, class GeoMesh>
tmp<GeometricField<scalar, PatchField, GeoMesh> > magSqr
(
  const tmp<GeometricField<Type, PatchField, GeoMesh> >& tgf
);

template<class Type, template<class> class PatchField, class GeoMesh>
void mag
(
  GeometricField<scalar, PatchField, GeoMesh>& gsf,
  const GeometricField<Type, PatchField, GeoMesh>& gf
);

template<class Type, template<class> class PatchField, class GeoMesh>
tmp<GeometricField<scalar, PatchField, GeoMesh> > mag
(
  const GeometricField<Type, PatchField, GeoMesh>& gf
);

template<class Type, template<class> class PatchField, class GeoMesh>
tmp<GeometricField<scalar, PatchField, GeoMesh> > mag
(
  const tmp<GeometricField<Type, PatchField, GeoMesh> >& tgf
);

template<class Type, template<class> class PatchField, class GeoMesh>
void cmptAv
(
  GeometricField
  <
    typename GeometricField<Type, PatchField, GeoMesh>::cmptType,
    PatchField,
    GeoMesh
  >& gcf,
  const GeometricField<Type, PatchField, GeoMesh>& gf
);

template<class Type, template<class> class PatchField, class GeoMesh>
tmp
<
  GeometricField
  <
    typename GeometricField<Type, PatchField, GeoMesh>::cmptType,
    PatchField,
    GeoMesh
  >
>
cmptAv(const GeometricField<Type, PatchField, GeoMesh>& gf);

template<class Type, template<class> class PatchField, class GeoMesh>
tmp
<
  GeometricField
  <
    typename GeometricField<Type, PatchField, GeoMesh>::cmptType,
    PatchField,
    GeoMesh
  >
>
cmptAv(const tmp<GeometricField<Type, PatchField, GeoMesh> >& tgf);

#define UNARY_REDUCTION_FUNCTION_WITH_BOUNDARY(returnType, func, gFunc)       \
                                                                              \
template<class Type, template<class> class PatchField, class GeoMesh>         \
dimensioned<returnType> func                                                  \
(                                                                             \
  const GeometricField<Type, PatchField, GeoMesh>& gf                         \
);                                                                            \
                                                                              \
template<class Type, template<class> class PatchField, class GeoMesh>         \
dimensioned<returnType> func                                                  \
(                                                                             \
  const tmp<GeometricField<Type, PatchField, GeoMesh> >& tgf1                 \
);

UNARY_REDUCTION_FUNCTION_WITH_BOUNDARY(Type, max, gMax)
UNARY_REDUCTION_FUNCTION_WITH_BOUNDARY(Type, min, gMin)
#undef UNARY_REDUCTION_FUNCTION_WITH_BOUNDARY

#define UNARY_REDUCTION_FUNCTION(returnType, func, gFunc)                     \
                                                                              \
template<class Type, template<class> class PatchField, class GeoMesh>         \
dimensioned<returnType> func                                                  \
(                                                                             \
  const GeometricField<Type, PatchField, GeoMesh>& gf                         \
);                                                                            \
                                                                              \
template<class Type, template<class> class PatchField, class GeoMesh>         \
dimensioned<returnType> func                                                  \
(                                                                             \
  const tmp<GeometricField<Type, PatchField, GeoMesh> >& tgf1                 \
);

UNARY_REDUCTION_FUNCTION(Type, sum, gSum)
UNARY_REDUCTION_FUNCTION(scalar, sumMag, gSumMag)
UNARY_REDUCTION_FUNCTION(Type, average, gAverage)
#undef UNARY_REDUCTION_FUNCTION

BINARY_FUNCTION(Type, Type, Type, max)
BINARY_FUNCTION(Type, Type, Type, min)
BINARY_FUNCTION(Type, Type, Type, cmptMultiply)
BINARY_FUNCTION(Type, Type, Type, cmptDivide)
BINARY_TYPE_FUNCTION(Type, Type, Type, max)
BINARY_TYPE_FUNCTION(Type, Type, Type, min)
BINARY_TYPE_FUNCTION(Type, Type, Type, cmptMultiply)
BINARY_TYPE_FUNCTION(Type, Type, Type, cmptDivide)

// Global operators
UNARY_OPERATOR(Type, Type, -, negate, transform)
BINARY_OPERATOR(Type, Type, scalar, *, '*', multiply)
BINARY_OPERATOR(Type, scalar, Type, *, '*', multiply)
BINARY_OPERATOR(Type, Type, scalar, /, '|', divide)
BINARY_TYPE_OPERATOR_SF(Type, scalar, Type, *, '*', multiply)
BINARY_TYPE_OPERATOR_FS(Type, Type, scalar, *, '*', multiply)
BINARY_TYPE_OPERATOR_FS(Type, Type, scalar, /, '|', divide)


#define PRODUCT_OPERATOR(product, op, opFunc)                                 \
                                                                              \
template                                                                      \
<class Type1, class Type2, template<class> class PatchField, class GeoMesh>   \
void opFunc                                                                   \
(                                                                             \
  GeometricField                                                              \
  <typename product<Type1, Type2>::type, PatchField, GeoMesh>& gf,            \
  const GeometricField<Type1, PatchField, GeoMesh>& gf1,                      \
  const GeometricField<Type2, PatchField, GeoMesh>& gf2                       \
);                                                                            \
                                                                              \
template                                                                      \
<class Type1, class Type2, template<class> class PatchField, class GeoMesh>   \
tmp                                                                           \
<                                                                             \
  GeometricField                                                              \
    <typename product<Type1, Type2>::type, PatchField, GeoMesh>               \
>                                                                             \
operator op                                                                   \
(                                                                             \
  const GeometricField<Type1, PatchField, GeoMesh>& gf1,                      \
  const GeometricField<Type2, PatchField, GeoMesh>& gf2                       \
);                                                                            \
                                                                              \
template                                                                      \
<class Type1, class Type2, template<class> class PatchField, class GeoMesh>   \
tmp                                                                           \
<                                                                             \
  GeometricField                                                              \
  <typename product<Type1, Type2>::type, PatchField, GeoMesh>                 \
>                                                                             \
operator op                                                                   \
(                                                                             \
  const GeometricField<Type1, PatchField, GeoMesh>& gf1,                      \
  const tmp<GeometricField<Type2, PatchField, GeoMesh> >& tgf2                \
);                                                                            \
                                                                              \
template                                                                      \
<class Type1, class Type2, template<class> class PatchField, class GeoMesh>   \
tmp                                                                           \
<                                                                             \
  GeometricField                                                              \
  <typename product<Type1, Type2>::type, PatchField, GeoMesh>                 \
>                                                                             \
operator op                                                                   \
(                                                                             \
  const tmp<GeometricField<Type1, PatchField, GeoMesh> >& tgf1,               \
  const GeometricField<Type2, PatchField, GeoMesh>& gf2                       \
);                                                                            \
                                                                              \
template                                                                      \
<class Type1, class Type2, template<class> class PatchField, class GeoMesh>   \
tmp                                                                           \
<                                                                             \
  GeometricField                                                              \
  <typename product<Type1, Type2>::type, PatchField, GeoMesh>                 \
>                                                                             \
operator op                                                                   \
(                                                                             \
  const tmp<GeometricField<Type1, PatchField, GeoMesh> >& tgf1,               \
  const tmp<GeometricField<Type2, PatchField, GeoMesh> >& tgf2                \
);                                                                            \
                                                                              \
template                                                                      \
<class Form, class Type, template<class> class PatchField, class GeoMesh>     \
void opFunc                                                                   \
(                                                                             \
  GeometricField                                                              \
  <typename product<Type, Form>::type, PatchField, GeoMesh>& gf,              \
  const GeometricField<Type, PatchField, GeoMesh>& gf1,                       \
  const dimensioned<Form>& dvs                                                \
);                                                                            \
                                                                              \
template                                                                      \
<class Form, class Type, template<class> class PatchField, class GeoMesh>     \
tmp                                                                           \
<                                                                             \
  GeometricField                                                              \
  <typename product<Type, Form>::type, PatchField, GeoMesh>                   \
>                                                                             \
operator op                                                                   \
(                                                                             \
  const GeometricField<Type, PatchField, GeoMesh>& gf1,                       \
  const dimensioned<Form>& dvs                                                \
);                                                                            \
                                                                              \
template                                                                      \
<                                                                             \
  class Form,                                                                 \
  class Cmpt,                                                                 \
  int nCmpt,                                                                  \
  class Type, template<class> class PatchField,                               \
  class GeoMesh                                                               \
>                                                                             \
tmp                                                                           \
<                                                                             \
  GeometricField                                                              \
  <typename product<Form, Type>::type, PatchField, GeoMesh>                   \
>                                                                             \
operator op                                                                   \
(                                                                             \
  const GeometricField<Type, PatchField, GeoMesh>& gf1,                       \
  const VectorSpace<Form,Cmpt,nCmpt>& vs                                      \
);                                                                            \
                                                                              \
template                                                                      \
<class Form, class Type, template<class> class PatchField, class GeoMesh>     \
tmp                                                                           \
<                                                                             \
  GeometricField                                                              \
  <typename product<Type, Form>::type, PatchField, GeoMesh>                   \
>                                                                             \
operator op                                                                   \
(                                                                             \
  const tmp<GeometricField<Type, PatchField, GeoMesh> >& tgf1,                \
  const dimensioned<Form>& dvs                                                \
);                                                                            \
                                                                              \
template                                                                      \
<                                                                             \
  class Form,                                                                 \
  class Cmpt,                                                                 \
  int nCmpt,                                                                  \
  class Type, template<class> class PatchField,                               \
  class GeoMesh                                                               \
>                                                                             \
tmp                                                                           \
<                                                                             \
  GeometricField                                                              \
  <typename product<Form, Type>::type, PatchField, GeoMesh>                   \
>                                                                             \
operator op                                                                   \
(                                                                             \
  const tmp<GeometricField<Type, PatchField, GeoMesh> >& tgf1,                \
  const VectorSpace<Form,Cmpt,nCmpt>& vs                                      \
);                                                                            \
                                                                              \
template                                                                      \
<class Form, class Type, template<class> class PatchField, class GeoMesh>     \
void opFunc                                                                   \
(                                                                             \
  GeometricField                                                              \
  <typename product<Form, Type>::type, PatchField, GeoMesh>& gf,              \
  const dimensioned<Form>& dvs,                                               \
  const GeometricField<Type, PatchField, GeoMesh>& gf1                        \
);                                                                            \
                                                                              \
template                                                                      \
<class Form, class Type, template<class> class PatchField, class GeoMesh>     \
tmp                                                                           \
<                                                                             \
  GeometricField                                                              \
  <typename product<Form, Type>::type, PatchField, GeoMesh>                   \
>                                                                             \
operator op                                                                   \
(                                                                             \
  const dimensioned<Form>& dvs,                                               \
  const GeometricField<Type, PatchField, GeoMesh>& gf1                        \
);                                                                            \
                                                                              \
template                                                                      \
<                                                                             \
  class Form,                                                                 \
  class Cmpt,                                                                 \
  int nCmpt,                                                                  \
  class Type, template<class> class PatchField,                               \
  class GeoMesh                                                               \
>                                                                             \
tmp                                                                           \
<                                                                             \
  GeometricField                                                              \
  <typename product<Form, Type>::type, PatchField, GeoMesh>                   \
>                                                                             \
operator op                                                                   \
(                                                                             \
  const VectorSpace<Form,Cmpt,nCmpt>& vs,                                     \
  const GeometricField<Type, PatchField, GeoMesh>& gf1                        \
);                                                                            \
                                                                              \
template                                                                      \
<class Form, class Type, template<class> class PatchField, class GeoMesh>     \
tmp                                                                           \
<                                                                             \
  GeometricField                                                              \
  <typename product<Form, Type>::type, PatchField, GeoMesh>                   \
>                                                                             \
operator op                                                                   \
(                                                                             \
  const dimensioned<Form>& dvs,                                               \
  const tmp<GeometricField<Type, PatchField, GeoMesh> >& tgf1                 \
);                                                                            \
                                                                              \
template                                                                      \
<                                                                             \
  class Form,                                                                 \
  class Cmpt,                                                                 \
  int nCmpt,                                                                  \
  class Type, template<class> class PatchField,                               \
  class GeoMesh                                                               \
>                                                                             \
tmp                                                                           \
<                                                                             \
  GeometricField                                                              \
  <typename product<Form, Type>::type, PatchField, GeoMesh>                   \
>                                                                             \
operator op                                                                   \
(                                                                             \
  const VectorSpace<Form,Cmpt,nCmpt>& vs,                                     \
  const tmp<GeometricField<Type, PatchField, GeoMesh> >& tgf1                 \
);

PRODUCT_OPERATOR(typeOfSum, +, add)
PRODUCT_OPERATOR(typeOfSum, -, subtract)
PRODUCT_OPERATOR(outerProduct, *, outer)
PRODUCT_OPERATOR(crossProduct, ^, cross)
PRODUCT_OPERATOR(innerProduct, &, dot)
PRODUCT_OPERATOR(scalarProduct, &&, dotdot)
#undef PRODUCT_OPERATOR

}  // namespace mousse
#include "undef_field_functions_m.inc"
