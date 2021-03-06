// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "_variable_hard_sphere.hpp"
#include "constants.hpp"


using namespace mousse::constant::mathematical;


// Constructors 
template<class CloudType>
mousse::VariableHardSphere<CloudType>::VariableHardSphere
(
  const dictionary& dict,
  CloudType& cloud
)
:
  BinaryCollisionModel<CloudType>{dict, cloud, typeName},
  Tref_{readScalar(this->coeffDict().lookup("Tref"))}
{}


// Destructor 
template<class CloudType>
mousse::VariableHardSphere<CloudType>::~VariableHardSphere()
{}


// Member Functions 
template<class CloudType>
bool mousse::VariableHardSphere<CloudType>::active() const
{
  return true;
}


template<class CloudType>
mousse::scalar mousse::VariableHardSphere<CloudType>::sigmaTcR
(
  const typename CloudType::parcelType& pP,
  const typename CloudType::parcelType& pQ
) const
{
  const CloudType& cloud(this->owner());
  label typeIdP = pP.typeId();
  label typeIdQ = pQ.typeId();
  scalar dPQ =
    0.5*(cloud.constProps(typeIdP).d() + cloud.constProps(typeIdQ).d());
  scalar omegaPQ =
    0.5*(cloud.constProps(typeIdP).omega() + cloud.constProps(typeIdQ).omega());
  scalar cR = mag(pP.U() - pQ.U());
  if (cR < VSMALL) {
    return 0;
  }
  scalar mP = cloud.constProps(typeIdP).mass();
  scalar mQ = cloud.constProps(typeIdQ).mass();
  scalar mR = mP*mQ/(mP + mQ);
  // calculating cross section = pi*dPQ^2, where dPQ is from Bird, eq. 4.79
  scalar sigmaTPQ =
    pi*dPQ*dPQ
    *pow(2.0*physicoChemical::k.value()*Tref_/(mR*cR*cR), omegaPQ - 0.5)
    /exp(mousse::lgamma(2.5 - omegaPQ));
  return sigmaTPQ*cR;
}


template<class CloudType>
void mousse::VariableHardSphere<CloudType>::collide
(
  typename CloudType::parcelType& pP,
  typename CloudType::parcelType& pQ
)
{
  CloudType& cloud = this->owner();
  label typeIdP = pP.typeId();
  label typeIdQ = pQ.typeId();
  vector& UP = pP.U();
  vector& UQ = pQ.U();
  Random& rndGen = cloud.rndGen();
  scalar mP = cloud.constProps(typeIdP).mass();
  scalar mQ = cloud.constProps(typeIdQ).mass();
  vector Ucm = (mP*UP + mQ*UQ)/(mP + mQ);
  scalar cR = mag(UP - UQ);
  scalar cosTheta = 2.0*rndGen.scalar01() - 1.0;
  scalar sinTheta = sqrt(1.0 - cosTheta*cosTheta);
  scalar phi = twoPi*rndGen.scalar01();
  vector postCollisionRelU =
    cR*vector{cosTheta, sinTheta*cos(phi), sinTheta*sin(phi)};
  UP = Ucm + postCollisionRelU*mQ/(mP + mQ);
  UQ = Ucm - postCollisionRelU*mP/(mP + mQ);
}

