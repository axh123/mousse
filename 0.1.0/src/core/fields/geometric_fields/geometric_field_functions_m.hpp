// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#define UNARY_FUNCTION(ReturnType, Type1, Func, Dfunc)                        \
                                                                              \
TEMPLATE                                                                      \
tmp<GeometricField<ReturnType, PatchField, GeoMesh> > Func                    \
(                                                                             \
  const GeometricField<Type1, PatchField, GeoMesh>& df                        \
);                                                                            \
                                                                              \
TEMPLATE                                                                      \
tmp<GeometricField<ReturnType, PatchField, GeoMesh> > Func                    \
(                                                                             \
  const tmp<GeometricField<Type1, PatchField, GeoMesh> >& tdf1                \
);


#define UNARY_OPERATOR(ReturnType, Type1, Op, opFunc, Dfunc)                  \
                                                                              \
TEMPLATE                                                                      \
tmp<GeometricField<ReturnType, PatchField, GeoMesh> > operator Op             \
(                                                                             \
  const GeometricField<Type1, PatchField, GeoMesh>& df1                       \
);                                                                            \
                                                                              \
TEMPLATE                                                                      \
tmp<GeometricField<ReturnType, PatchField, GeoMesh> > operator Op             \
(                                                                             \
  const tmp<GeometricField<Type1, PatchField, GeoMesh> >& tdf1                \
);


#define BINARY_FUNCTION(ReturnType, Type1, Type2, Func)                       \
                                                                              \
TEMPLATE                                                                      \
tmp<GeometricField<ReturnType, PatchField, GeoMesh> > Func                    \
(                                                                             \
  const GeometricField<Type1, PatchField, GeoMesh>& df1,                      \
  const GeometricField<Type2, PatchField, GeoMesh>& df2                       \
);                                                                            \
                                                                              \
TEMPLATE                                                                      \
tmp<GeometricField<ReturnType, PatchField, GeoMesh> > Func                    \
(                                                                             \
  const GeometricField<Type1, PatchField, GeoMesh>& df1,                      \
  const tmp<GeometricField<Type2, PatchField, GeoMesh> >& tdf2                \
);                                                                            \
                                                                              \
TEMPLATE                                                                      \
tmp<GeometricField<ReturnType, PatchField, GeoMesh> > Func                    \
(                                                                             \
  const tmp<GeometricField<Type1, PatchField, GeoMesh> >& tdf1,               \
  const GeometricField<Type2, PatchField, GeoMesh>& df2                       \
);                                                                            \
                                                                              \
TEMPLATE                                                                      \
tmp<GeometricField<ReturnType, PatchField, GeoMesh> > Func                    \
(                                                                             \
  const tmp<GeometricField<Type1, PatchField, GeoMesh> >& tdf1,               \
  const tmp<GeometricField<Type2, PatchField, GeoMesh> >& tdf2                \
);


#define BINARY_TYPE_FUNCTION_SF(ReturnType, Type1, Type2, Func)               \
                                                                              \
TEMPLATE                                                                      \
tmp<GeometricField<ReturnType, PatchField, GeoMesh> > Func                    \
(                                                                             \
  const dimensioned<Type1>& dt1,                                              \
  const GeometricField<Type2, PatchField, GeoMesh>& df2                       \
);                                                                            \
                                                                              \
TEMPLATE                                                                      \
tmp<GeometricField<ReturnType, PatchField, GeoMesh> > Func                    \
(                                                                             \
  const Type1& t1,                                                            \
  const GeometricField<Type2, PatchField, GeoMesh>& df2                       \
);                                                                            \
                                                                              \
TEMPLATE                                                                      \
tmp<GeometricField<ReturnType, PatchField, GeoMesh> > Func                    \
(                                                                             \
  const dimensioned<Type1>& dt1,                                              \
  const tmp<GeometricField<Type2, PatchField, GeoMesh> >& tdf2                \
);                                                                            \
                                                                              \
TEMPLATE                                                                      \
tmp<GeometricField<ReturnType, PatchField, GeoMesh> > Func                    \
(                                                                             \
  const Type1& t1,                                                            \
  const tmp<GeometricField<Type2, PatchField, GeoMesh> >& tdf2                \
);


#define BINARY_TYPE_FUNCTION_FS(ReturnType, Type1, Type2, Func)               \
                                                                              \
TEMPLATE                                                                      \
tmp<GeometricField<ReturnType, PatchField, GeoMesh> > Func                    \
(                                                                             \
  const GeometricField<Type1, PatchField, GeoMesh>& df1,                      \
  const dimensioned<Type2>& dt2                                               \
);                                                                            \
                                                                              \
TEMPLATE                                                                      \
tmp<GeometricField<ReturnType, PatchField, GeoMesh> > Func                    \
(                                                                             \
  const GeometricField<Type1, PatchField, GeoMesh>& df1,                      \
  const Type2& t2                                                             \
);                                                                            \
                                                                              \
TEMPLATE                                                                      \
tmp<GeometricField<ReturnType, PatchField, GeoMesh> > Func                    \
(                                                                             \
  const tmp<GeometricField<Type1, PatchField, GeoMesh> >& tdf1,               \
  const dimensioned<Type2>& dt2                                               \
);                                                                            \
                                                                              \
TEMPLATE                                                                      \
tmp<GeometricField<ReturnType, PatchField, GeoMesh> > Func                    \
(                                                                             \
  const tmp<GeometricField<Type1, PatchField, GeoMesh> >& tdf2,               \
  const Type2& t2                                                             \
);


#define BINARY_TYPE_FUNCTION(ReturnType, Type1, Type2, Func)                  \
  BINARY_TYPE_FUNCTION_SF(ReturnType, Type1, Type2, Func)                     \
  BINARY_TYPE_FUNCTION_FS(ReturnType, Type1, Type2, Func)


#define BINARY_OPERATOR(ReturnType, Type1, Type2, Op, OpName, OpFunc)         \
                                                                              \
TEMPLATE                                                                      \
tmp<GeometricField<ReturnType, PatchField, GeoMesh> > operator Op             \
(                                                                             \
  const GeometricField<Type1, PatchField, GeoMesh>& df1,                      \
  const GeometricField<Type2, PatchField, GeoMesh>& df2                       \
);                                                                            \
                                                                              \
TEMPLATE                                                                      \
tmp<GeometricField<ReturnType, PatchField, GeoMesh> > operator Op             \
(                                                                             \
  const GeometricField<Type1, PatchField, GeoMesh>& df1,                      \
  const tmp<GeometricField<Type2, PatchField, GeoMesh> >& tdf2                \
);                                                                            \
                                                                              \
TEMPLATE                                                                      \
tmp<GeometricField<ReturnType, PatchField, GeoMesh> > operator Op             \
(                                                                             \
  const tmp<GeometricField<Type1, PatchField, GeoMesh> >& tdf1,               \
  const GeometricField<Type2, PatchField, GeoMesh>& df2                       \
);                                                                            \
                                                                              \
TEMPLATE                                                                      \
tmp<GeometricField<ReturnType, PatchField, GeoMesh> > operator Op             \
(                                                                             \
  const tmp<GeometricField<Type1, PatchField, GeoMesh> >& tdf1,               \
  const tmp<GeometricField<Type2, PatchField, GeoMesh> >& tdf2                \
);


#define BINARY_TYPE_OPERATOR_SF(ReturnType, Type1, Type2, Op, OpName, OpFunc) \
                                                                              \
TEMPLATE                                                                      \
tmp<GeometricField<ReturnType, PatchField, GeoMesh> > operator Op             \
(                                                                             \
  const dimensioned<Type1>& dt1,                                              \
  const GeometricField<Type2, PatchField, GeoMesh>& df2                       \
);                                                                            \
                                                                              \
TEMPLATE                                                                      \
tmp<GeometricField<ReturnType, PatchField, GeoMesh> > operator Op             \
(                                                                             \
  const Type1& t1,                                                            \
  const GeometricField<Type2, PatchField, GeoMesh>& df2                       \
);                                                                            \
                                                                              \
TEMPLATE                                                                      \
tmp<GeometricField<ReturnType, PatchField, GeoMesh> > operator Op             \
(                                                                             \
  const dimensioned<Type1>& dt1,                                              \
  const tmp<GeometricField<Type2, PatchField, GeoMesh> >& tdf2                \
);                                                                            \
                                                                              \
TEMPLATE                                                                      \
tmp<GeometricField<ReturnType, PatchField, GeoMesh> > operator Op             \
(                                                                             \
  const Type1& t1,                                                            \
  const tmp<GeometricField<Type2, PatchField, GeoMesh> >& tdf2                \
);


#define BINARY_TYPE_OPERATOR_FS(ReturnType, Type1, Type2, Op, OpName, OpFunc) \
                                                                              \
TEMPLATE                                                                      \
tmp<GeometricField<ReturnType, PatchField, GeoMesh> > operator Op             \
(                                                                             \
  const GeometricField<Type1, PatchField, GeoMesh>& df1,                      \
  const dimensioned<Type2>& dt2                                               \
);                                                                            \
                                                                              \
TEMPLATE                                                                      \
tmp<GeometricField<ReturnType, PatchField, GeoMesh> > operator Op             \
(                                                                             \
  const GeometricField<Type1, PatchField, GeoMesh>& df1,                      \
  const Type2& t2                                                             \
);                                                                            \
                                                                              \
TEMPLATE                                                                      \
tmp<GeometricField<ReturnType, PatchField, GeoMesh> > operator Op             \
(                                                                             \
  const tmp<GeometricField<Type1, PatchField, GeoMesh> >& tdf1,               \
  const dimensioned<Type2>& dt2                                               \
);                                                                            \
                                                                              \
TEMPLATE                                                                      \
tmp<GeometricField<ReturnType, PatchField, GeoMesh> > operator Op             \
(                                                                             \
  const tmp<GeometricField<Type1, PatchField, GeoMesh> >& tdf1,               \
  const Type2& t2                                                             \
);


#define BINARY_TYPE_OPERATOR(ReturnType, Type1, Type2, Op, OpName, OpFunc)    \
  BINARY_TYPE_OPERATOR_SF(ReturnType, Type1, Type2, Op, OpName, OpFunc)       \
  BINARY_TYPE_OPERATOR_FS(ReturnType, Type1, Type2, Op, OpName, OpFunc)