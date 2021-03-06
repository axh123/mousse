#ifndef LAGRANGIAN_INTERMEDIATE_SUBMODELS_TKINEMATIC_TPARTICLE_FORCES_TVIRTUAL_MASS_FORCE_HPP_
#define LAGRANGIAN_INTERMEDIATE_SUBMODELS_TKINEMATIC_TPARTICLE_FORCES_TVIRTUAL_MASS_FORCE_HPP_

// mousse: CFD toolbox
// Copyright (C) 2012 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::VirtualMassForce
// Description
//   Calculates particle virtual mass force

#include "_pressure_gradient_force.hpp"


namespace mousse {

template<class CloudType>
class VirtualMassForce
:
  public PressureGradientForce<CloudType>
{
  // Private data
    //- Virtual mass coefficient - typically 0.5
    scalar Cvm_;
public:
  //- Runtime type information
  TYPE_NAME("virtualMass");
  // Constructors
    //- Construct from mesh
    VirtualMassForce
    (
      CloudType& owner,
      const fvMesh& mesh,
      const dictionary& dict,
      const word& forceType = typeName
    );
    //- Construct copy
    VirtualMassForce(const VirtualMassForce& pgf);
    //- Construct and return a clone
    virtual autoPtr<ParticleForce<CloudType>> clone() const
    {
      return
        autoPtr<ParticleForce<CloudType>>
        {
          new VirtualMassForce<CloudType>{*this}
        };
    }
  //- Destructor
  virtual ~VirtualMassForce();
  // Member Functions
    // Evaluation
      //- Cache fields
      virtual void cacheFields(const bool store);
      //- Calculate the non-coupled force
      virtual forceSuSp calcCoupled
      (
        const typename CloudType::parcelType& p,
        const scalar dt,
        const scalar mass,
        const scalar Re,
        const scalar muc
      ) const;
      //- Return the added mass
      virtual scalar massAdd
      (
        const typename CloudType::parcelType& p,
        const scalar mass
      ) const;
};

}  // namespace mousse

#include "_virtual_mass_force.ipp"

#endif
