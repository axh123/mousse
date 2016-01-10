// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::cyclicSlipPointPatch
// Description
//   Cyclic patch with slip constraint
// SourceFiles
//   cyclic_slip_point_patch.cpp
#ifndef cyclic_slip_point_patch_hpp_
#define cyclic_slip_point_patch_hpp_
#include "cyclic_point_patch.hpp"
#include "cyclic_slip_poly_patch.hpp"
namespace mousse
{
class cyclicSlipPointPatch
:
  public cyclicPointPatch
{
public:
  //- Runtime type information
  TYPE_NAME(cyclicSlipPolyPatch::typeName_());
  // Constructors
    //- Construct from components
    cyclicSlipPointPatch
    (
      const polyPatch& patch,
      const pointBoundaryMesh& bm
    )
    :
      cyclicPointPatch(patch, bm)
    {}
  // Destructor
    virtual ~cyclicSlipPointPatch()
    {}
  // Member Functions
    //- Return point unit normals.
    virtual const vectorField& pointNormals() const;
    //- Accumulate the effect of constraint direction of this patch
    virtual void applyConstraint
    (
      const label pointi,
      pointConstraint&
    ) const;
};
}  // namespace mousse
#endif
