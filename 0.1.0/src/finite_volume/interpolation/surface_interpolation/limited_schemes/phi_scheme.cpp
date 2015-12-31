// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "vol_fields.hpp"
#include "surface_fields.hpp"
#include "fvc_grad.hpp"
#include "coupled_fv_patch_fields.hpp"
#include "surface_interpolate.hpp"
template<class Type, class PhiLimiter>
mousse::tmp<mousse::surfaceScalarField>
mousse::PhiScheme<Type, PhiLimiter>::limiter
(
  const GeometricField<Type, fvPatchField, volMesh>& phi
) const
{
  const fvMesh& mesh = this->mesh();
  tmp<surfaceScalarField> tLimiter
  (
    new surfaceScalarField
    (
      IOobject
      (
        "PhiLimiter",
        mesh.time().timeName(),
        mesh
      ),
      mesh,
      dimless
    )
  );
  surfaceScalarField& Limiter = tLimiter();
  const surfaceScalarField& CDweights = mesh.surfaceInterpolation::weights();
  const surfaceVectorField& Sf = mesh.Sf();
  const surfaceScalarField& magSf = mesh.magSf();
  const labelUList& owner = mesh.owner();
  const labelUList& neighbour = mesh.neighbour();
  tmp<surfaceScalarField> tUflux = this->faceFlux_;
  if (this->faceFlux_.dimensions() == dimDensity*dimVelocity*dimArea)
  {
    const volScalarField& rho =
      phi.db().objectRegistry::template lookupObject<volScalarField>
      ("rho");
    tUflux = this->faceFlux_/fvc::interpolate(rho);
  }
  else if (this->faceFlux_.dimensions() != dimVelocity*dimArea)
  {
    FatalErrorIn
    (
      "PhiScheme<PhiLimiter>::limiter"
      "(const GeometricField<Type, fvPatchField, volMesh>& phi)"
    )   << "dimensions of faceFlux are not correct"
      << exit(FatalError);
  }
  const surfaceScalarField& Uflux = tUflux();
  scalarField& pLimiter = Limiter.internalField();
  forAll(pLimiter, face)
  {
    pLimiter[face] = PhiLimiter::limiter
    (
      CDweights[face],
      Uflux[face],
      phi[owner[face]],
      phi[neighbour[face]],
      Sf[face],
      magSf[face]
    );
  }
  surfaceScalarField::GeometricBoundaryField& bLimiter =
    Limiter.boundaryField();
  forAll(bLimiter, patchI)
  {
    scalarField& pLimiter = bLimiter[patchI];
    if (bLimiter[patchI].coupled())
    {
      const scalarField& pCDweights = CDweights.boundaryField()[patchI];
      const vectorField& pSf = Sf.boundaryField()[patchI];
      const scalarField& pmagSf = magSf.boundaryField()[patchI];
      const scalarField& pFaceFlux = Uflux.boundaryField()[patchI];
      const Field<Type> pphiP
      (
        phi.boundaryField()[patchI].patchInternalField()
      );
      const Field<Type> pphiN
      (
        phi.boundaryField()[patchI].patchNeighbourField()
      );
      forAll(pLimiter, face)
      {
        pLimiter[face] = PhiLimiter::limiter
        (
          pCDweights[face],
          pFaceFlux[face],
          pphiP[face],
          pphiN[face],
          pSf[face],
          pmagSf[face]
        );
      }
    }
    else
    {
      pLimiter = 1.0;
    }
  }
  return tLimiter;
}
