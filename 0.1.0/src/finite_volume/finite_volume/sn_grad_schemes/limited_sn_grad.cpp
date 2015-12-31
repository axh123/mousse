// mousse: CFD toolbox
// Copyright (C) 2011-2012 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "fv.hpp"
#include "limited_sn_grad.hpp"
#include "vol_fields.hpp"
#include "surface_fields.hpp"
#include "local_max.hpp"
namespace mousse
{
namespace fv
{
// Destructor 
template<class Type>
limitedSnGrad<Type>::~limitedSnGrad()
{}
// Member Functions 
template<class Type>
tmp<GeometricField<Type, fvsPatchField, surfaceMesh> >
limitedSnGrad<Type>::correction
(
  const GeometricField<Type, fvPatchField, volMesh>& vf
) const
{
  const GeometricField<Type, fvsPatchField, surfaceMesh> corr
  (
    correctedScheme_().correction(vf)
  );
  const surfaceScalarField limiter
  (
    min
    (
      limitCoeff_
     *mag(snGradScheme<Type>::snGrad(vf, deltaCoeffs(vf), "SndGrad"))
     /(
        (1 - limitCoeff_)*mag(corr)
       + dimensionedScalar("small", corr.dimensions(), SMALL)
      ),
      dimensionedScalar("one", dimless, 1.0)
    )
  );
  if (fv::debug)
  {
    Info<< "limitedSnGrad :: limiter min: " << min(limiter.internalField())
      << " max: "<< max(limiter.internalField())
      << " avg: " << average(limiter.internalField()) << endl;
  }
  return limiter*corr;
}
}  // namespace fv
}  // namespace mousse
