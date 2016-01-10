// mousse: CFD toolbox
// Copyright (C) 2011-2013 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "fv_mesh.hpp"
#include "vol_fields.hpp"
#include "surface_fields.hpp"
#include "demand_driven_data.hpp"
#include "coupled_fv_patch.hpp"
// Static Data Members
namespace mousse
{
DEFINE_TYPE_NAME_AND_DEBUG(surfaceInterpolation, 0);
}
// Protected Member Functions 
void mousse::surfaceInterpolation::clearOut()
{
  deleteDemandDrivenData(weights_);
  deleteDemandDrivenData(deltaCoeffs_);
  deleteDemandDrivenData(nonOrthDeltaCoeffs_);
  deleteDemandDrivenData(nonOrthCorrectionVectors_);
}
// Constructors
mousse::surfaceInterpolation::surfaceInterpolation(const fvMesh& fvm)
:
  mesh_(fvm),
  weights_(NULL),
  deltaCoeffs_(NULL),
  nonOrthDeltaCoeffs_(NULL),
  nonOrthCorrectionVectors_(NULL)
{}
// Destructor
mousse::surfaceInterpolation::~surfaceInterpolation()
{
  clearOut();
}
// Member Functions 
const mousse::surfaceScalarField&
mousse::surfaceInterpolation::weights() const
{
  if (!weights_)
  {
    makeWeights();
  }
  return (*weights_);
}
const mousse::surfaceScalarField&
mousse::surfaceInterpolation::deltaCoeffs() const
{
  if (!deltaCoeffs_)
  {
    makeDeltaCoeffs();
  }
  return (*deltaCoeffs_);
}
const mousse::surfaceScalarField&
mousse::surfaceInterpolation::nonOrthDeltaCoeffs() const
{
  if (!nonOrthDeltaCoeffs_)
  {
    makeNonOrthDeltaCoeffs();
  }
  return (*nonOrthDeltaCoeffs_);
}
const mousse::surfaceVectorField&
mousse::surfaceInterpolation::nonOrthCorrectionVectors() const
{
  if (!nonOrthCorrectionVectors_)
  {
    makeNonOrthCorrectionVectors();
  }
  return (*nonOrthCorrectionVectors_);
}
// Do what is neccessary if the mesh has moved
bool mousse::surfaceInterpolation::movePoints()
{
  deleteDemandDrivenData(weights_);
  deleteDemandDrivenData(deltaCoeffs_);
  deleteDemandDrivenData(nonOrthDeltaCoeffs_);
  deleteDemandDrivenData(nonOrthCorrectionVectors_);
  return true;
}
void mousse::surfaceInterpolation::makeWeights() const
{
  if (debug)
  {
    Pout<< "surfaceInterpolation::makeWeights() : "
      << "Constructing weighting factors for face interpolation"
      << endl;
  }
  weights_ = new surfaceScalarField
  (
    IOobject
    (
      "weights",
      mesh_.pointsInstance(),
      mesh_,
      IOobject::NO_READ,
      IOobject::NO_WRITE,
      false // Do not register
    ),
    mesh_,
    dimless
  );
  surfaceScalarField& weights = *weights_;
  // Set local references to mesh data
  // (note that we should not use fvMesh sliced fields at this point yet
  //  since this causes a loop when generating weighting factors in
  //  coupledFvPatchField evaluation phase)
  const labelUList& owner = mesh_.owner();
  const labelUList& neighbour = mesh_.neighbour();
  const vectorField& Cf = mesh_.faceCentres();
  const vectorField& C = mesh_.cellCentres();
  const vectorField& Sf = mesh_.faceAreas();
  // ... and reference to the internal field of the weighting factors
  scalarField& w = weights.internalField();
  FOR_ALL(owner, facei)
  {
    // Note: mag in the dot-product.
    // For all valid meshes, the non-orthogonality will be less that
    // 90 deg and the dot-product will be positive.  For invalid
    // meshes (d & s <= 0), this will stabilise the calculation
    // but the result will be poor.
    scalar SfdOwn = mag(Sf[facei] & (Cf[facei] - C[owner[facei]]));
    scalar SfdNei = mag(Sf[facei] & (C[neighbour[facei]] - Cf[facei]));
    w[facei] = SfdNei/(SfdOwn + SfdNei);
  }
  FOR_ALL(mesh_.boundary(), patchi)
  {
    mesh_.boundary()[patchi].makeWeights
    (
      weights.boundaryField()[patchi]
    );
  }
  if (debug)
  {
    Pout<< "surfaceInterpolation::makeWeights() : "
      << "Finished constructing weighting factors for face interpolation"
      << endl;
  }
}
void mousse::surfaceInterpolation::makeDeltaCoeffs() const
{
  if (debug)
  {
    Pout<< "surfaceInterpolation::makeDeltaCoeffs() : "
      << "Constructing differencing factors array for face gradient"
      << endl;
  }
  // Force the construction of the weighting factors
  // needed to make sure deltaCoeffs are calculated for parallel runs.
  weights();
  deltaCoeffs_ = new surfaceScalarField
  (
    IOobject
    (
      "deltaCoeffs",
      mesh_.pointsInstance(),
      mesh_,
      IOobject::NO_READ,
      IOobject::NO_WRITE,
      false // Do not register
    ),
    mesh_,
    dimless/dimLength
  );
  surfaceScalarField& DeltaCoeffs = *deltaCoeffs_;
  // Set local references to mesh data
  const volVectorField& C = mesh_.C();
  const labelUList& owner = mesh_.owner();
  const labelUList& neighbour = mesh_.neighbour();
  FOR_ALL(owner, facei)
  {
    DeltaCoeffs[facei] = 1.0/mag(C[neighbour[facei]] - C[owner[facei]]);
  }
  FOR_ALL(DeltaCoeffs.boundaryField(), patchi)
  {
    DeltaCoeffs.boundaryField()[patchi] =
      1.0/mag(mesh_.boundary()[patchi].delta());
  }
}
void mousse::surfaceInterpolation::makeNonOrthDeltaCoeffs() const
{
  if (debug)
  {
    Pout<< "surfaceInterpolation::makeNonOrthDeltaCoeffs() : "
      << "Constructing differencing factors array for face gradient"
      << endl;
  }
  // Force the construction of the weighting factors
  // needed to make sure deltaCoeffs are calculated for parallel runs.
  weights();
  nonOrthDeltaCoeffs_ = new surfaceScalarField
  (
    IOobject
    (
      "nonOrthDeltaCoeffs",
      mesh_.pointsInstance(),
      mesh_,
      IOobject::NO_READ,
      IOobject::NO_WRITE,
      false // Do not register
    ),
    mesh_,
    dimless/dimLength
  );
  surfaceScalarField& nonOrthDeltaCoeffs = *nonOrthDeltaCoeffs_;
  // Set local references to mesh data
  const volVectorField& C = mesh_.C();
  const labelUList& owner = mesh_.owner();
  const labelUList& neighbour = mesh_.neighbour();
  const surfaceVectorField& Sf = mesh_.Sf();
  const surfaceScalarField& magSf = mesh_.magSf();
  FOR_ALL(owner, facei)
  {
    vector delta = C[neighbour[facei]] - C[owner[facei]];
    vector unitArea = Sf[facei]/magSf[facei];
    // Standard cell-centre distance form
    //NonOrthDeltaCoeffs[facei] = (unitArea & delta)/magSqr(delta);
    // Slightly under-relaxed form
    //NonOrthDeltaCoeffs[facei] = 1.0/mag(delta);
    // More under-relaxed form
    //NonOrthDeltaCoeffs[facei] = 1.0/(mag(unitArea & delta) + VSMALL);
    // Stabilised form for bad meshes
    nonOrthDeltaCoeffs[facei] = 1.0/max(unitArea & delta, 0.05*mag(delta));
  }
  FOR_ALL(nonOrthDeltaCoeffs.boundaryField(), patchi)
  {
    vectorField delta(mesh_.boundary()[patchi].delta());
    nonOrthDeltaCoeffs.boundaryField()[patchi] =
      1.0/max(mesh_.boundary()[patchi].nf() & delta, 0.05*mag(delta));
  }
}
void mousse::surfaceInterpolation::makeNonOrthCorrectionVectors() const
{
  if (debug)
  {
    Pout<< "surfaceInterpolation::makeNonOrthCorrectionVectors() : "
      << "Constructing non-orthogonal correction vectors"
      << endl;
  }
  nonOrthCorrectionVectors_ = new surfaceVectorField
  (
    IOobject
    (
      "nonOrthCorrectionVectors",
      mesh_.pointsInstance(),
      mesh_,
      IOobject::NO_READ,
      IOobject::NO_WRITE,
      false // Do not register
    ),
    mesh_,
    dimless
  );
  surfaceVectorField& corrVecs = *nonOrthCorrectionVectors_;
  // Set local references to mesh data
  const volVectorField& C = mesh_.C();
  const labelUList& owner = mesh_.owner();
  const labelUList& neighbour = mesh_.neighbour();
  const surfaceVectorField& Sf = mesh_.Sf();
  const surfaceScalarField& magSf = mesh_.magSf();
  const surfaceScalarField& NonOrthDeltaCoeffs = nonOrthDeltaCoeffs();
  FOR_ALL(owner, facei)
  {
    vector unitArea = Sf[facei]/magSf[facei];
    vector delta = C[neighbour[facei]] - C[owner[facei]];
    corrVecs[facei] = unitArea - delta*NonOrthDeltaCoeffs[facei];
  }
  // Boundary correction vectors set to zero for boundary patches
  // and calculated consistently with internal corrections for
  // coupled patches
  FOR_ALL(corrVecs.boundaryField(), patchi)
  {
    fvsPatchVectorField& patchCorrVecs = corrVecs.boundaryField()[patchi];
    if (!patchCorrVecs.coupled())
    {
      patchCorrVecs = vector::zero;
    }
    else
    {
      const fvsPatchScalarField& patchNonOrthDeltaCoeffs
        = NonOrthDeltaCoeffs.boundaryField()[patchi];
      const fvPatch& p = patchCorrVecs.patch();
      const vectorField patchDeltas(mesh_.boundary()[patchi].delta());
      FOR_ALL(p, patchFacei)
      {
        vector unitArea =
          Sf.boundaryField()[patchi][patchFacei]
         /magSf.boundaryField()[patchi][patchFacei];
        const vector& delta = patchDeltas[patchFacei];
        patchCorrVecs[patchFacei] =
          unitArea - delta*patchNonOrthDeltaCoeffs[patchFacei];
      }
    }
  }
  if (debug)
  {
    Pout<< "surfaceInterpolation::makeNonOrthCorrectionVectors() : "
      << "Finished constructing non-orthogonal correction vectors"
      << endl;
  }
}
