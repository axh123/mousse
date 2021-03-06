#ifndef LAGRANGIAN_INTERMEDIATE_SUBMODELS_TKINEMATIC_TPARTICLE_FORCES_TDRAG_TSPHERE_DRAG_FORCE_HPP_
#define LAGRANGIAN_INTERMEDIATE_SUBMODELS_TKINEMATIC_TPARTICLE_FORCES_TDRAG_TSPHERE_DRAG_FORCE_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::SphereDragForce
// Description
//   Drag model based on assumption of solid spheres

#include "_particle_force.hpp"


namespace mousse {

template<class CloudType>
class SphereDragForce
:
  public ParticleForce<CloudType>
{
  // Private Member Functions
    //- Drag coefficient multiplied by Reynolds number
    scalar CdRe(const scalar Re) const;
public:
  //- Runtime type information
  TYPE_NAME("sphereDrag");
  // Constructors
    //- Construct from mesh
    SphereDragForce
    (
      CloudType& owner,
      const fvMesh& mesh,
      const dictionary& dict
    );
    //- Construct copy
    SphereDragForce(const SphereDragForce<CloudType>& df);
    //- Construct and return a clone
    virtual autoPtr<ParticleForce<CloudType>> clone() const
    {
      return
        autoPtr<ParticleForce<CloudType>>
        {
          new SphereDragForce<CloudType>{*this}
        };
    }
  //- Destructor
  virtual ~SphereDragForce();
  // Member Functions
    // Evaluation
      //- Calculate the coupled force
      virtual forceSuSp calcCoupled
      (
        const typename CloudType::parcelType& p,
        const scalar dt,
        const scalar mass,
        const scalar Re,
        const scalar muc
      ) const;
};

}  // namespace mousse

#include "_sphere_drag_force.ipp"

#endif
