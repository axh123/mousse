// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "rotor_disk_source.hpp"
#include "vol_fields.hpp"
#include "unit_conversion.hpp"


using namespace mousse::constant;


// Protected Member Functions 
template<class RhoFieldType>
void mousse::fv::rotorDiskSource::calculate
(
  const RhoFieldType& rho,
  const vectorField& U,
  const scalarField& thetag,
  vectorField& force,
  const bool divideVolume,
  const bool output
) const
{
  const scalarField& V = mesh_.V();
  // Logging info
  scalar dragEff = 0.0;
  scalar liftEff = 0.0;
  scalar AOAmin = GREAT;
  scalar AOAmax = -GREAT;
  FOR_ALL(cells_, i) {
    if (area_[i] <= ROOTVSMALL)
      continue;
    const label cellI = cells_[i];
    const scalar radius = x_[i].x();
    // Transform velocity into local cylindrical reference frame
    vector Uc = cylindrical_->invTransform(U[cellI], i);
    // Transform velocity into local coning system
    Uc = R_[i] & Uc;
    // Set radial component of velocity to zero
    Uc.x() = 0.0;
    // Set blade normal component of velocity
    Uc.y() = radius*omega_ - Uc.y();
    // Determine blade data for this radius
    // i2 = index of upper radius bound data point in blade list
    scalar twist = 0.0;
    scalar chord = 0.0;
    label i1 = -1;
    label i2 = -1;
    scalar invDr = 0.0;
    blade_.interpolate(radius, twist, chord, i1, i2, invDr);
    // Flip geometric angle if blade is spinning in reverse (clockwise)
    scalar alphaGeom = thetag[i] + twist;
    if (omega_ < 0) {
      alphaGeom = mathematical::pi - alphaGeom;
    }
    // Effective angle of attack
    scalar alphaEff = alphaGeom - atan2(-Uc.z(), Uc.y());
    if (alphaEff > mathematical::pi) {
      alphaEff -= mathematical::twoPi;
    }
    if (alphaEff < -mathematical::pi) {
      alphaEff += mathematical::twoPi;
    }
    AOAmin = min(AOAmin, alphaEff);
    AOAmax = max(AOAmax, alphaEff);
    // Determine profile data for this radius and angle of attack
    const label profile1 = blade_.profileID()[i1];
    const label profile2 = blade_.profileID()[i2];
    scalar Cd1 = 0.0;
    scalar Cl1 = 0.0;
    profiles_[profile1].Cdl(alphaEff, Cd1, Cl1);
    scalar Cd2 = 0.0;
    scalar Cl2 = 0.0;
    profiles_[profile2].Cdl(alphaEff, Cd2, Cl2);
    scalar Cd = invDr*(Cd2 - Cd1) + Cd1;
    scalar Cl = invDr*(Cl2 - Cl1) + Cl1;
    // Apply tip effect for blade lift
    scalar tipFactor = neg(radius/rMax_ - tipEffect_);
    // Calculate forces perpendicular to blade
    scalar pDyn = 0.5*rho[cellI]*magSqr(Uc);
    scalar f = pDyn*chord*nBlades_*area_[i]/radius/mathematical::twoPi;
    vector localForce = vector(0.0, -f*Cd, tipFactor*f*Cl);
    // Accumulate forces
    dragEff += rhoRef_*localForce.y();
    liftEff += rhoRef_*localForce.z();
    // Transform force from local coning system into rotor cylindrical
    localForce = invR_[i] & localForce;
    // Transform force into global Cartesian co-ordinate system
    force[cellI] = cylindrical_->transform(localForce, i);
    if (divideVolume) {
      force[cellI] /= V[cellI];
    }
  }
  if (output) {
    reduce(AOAmin, minOp<scalar>());
    reduce(AOAmax, maxOp<scalar>());
    reduce(dragEff, sumOp<scalar>());
    reduce(liftEff, sumOp<scalar>());
    Info << type() << " output:" << nl
      << "    min/max(AOA)   = " << radToDeg(AOAmin) << ", "
      << radToDeg(AOAmax) << nl
      << "    Effective drag = " << dragEff << nl
      << "    Effective lift = " << liftEff << endl;
  }
}


template<class Type>
void mousse::fv::rotorDiskSource::writeField
(
  const word& name,
  const List<Type>& values,
  const bool writeNow
) const
{
  typedef GeometricField<Type, fvPatchField, volMesh> fieldType;
  if (mesh_.time().outputTime() || writeNow) {
    tmp<fieldType> tfield
    {
      new fieldType
      {
        IOobject
        {
          name,
          mesh_.time().timeName(),
          mesh_,
          IOobject::NO_READ,
          IOobject::NO_WRITE
        },
        mesh_,
        {"zero", dimless, pTraits<Type>::zero}
      }
    };
    Field<Type>& field = tfield().internalField();
    if (cells_.size() != values.size()) {
      FATAL_ERROR_IN("") << "cells_.size() != values_.size()"
        << abort(FatalError);
    }
    FOR_ALL(cells_, i) {
      const label cellI = cells_[i];
      field[cellI] = values[i];
    }
    tfield().write();
  }
}

