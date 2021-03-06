#ifndef LAGRANGIAN_INTERMEDIATE_SUBMODELS_TREACTING_MULTIPHASE_TSTOCHASTIC_COLLISION_TSUPPRESSION_COLLISION_HPP_
#define LAGRANGIAN_INTERMEDIATE_SUBMODELS_TREACTING_MULTIPHASE_TSTOCHASTIC_COLLISION_TSUPPRESSION_COLLISION_HPP_

// mousse: CFD toolbox
// Copyright (C) 2013 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::SuppressionCollision
// Description
//   Inter-cloud collision model, whereby the \c canReact flag can be used
//   to inhibit devolatilisation and surface reactions

#include "_stochastic_collision_model.hpp"


namespace mousse {

template<class CloudType>
class SuppressionCollision
:
  public StochasticCollisionModel<CloudType>
{
protected:
  // Protected data
    //- Name of cloud used for suppression
    const word suppressionCloud_;
    //- Suppressed parcel type - optional
    const label suppressedParcelType_;
 // Protected Member Functions
    //- Update the model
    virtual void collide(const scalar dt);
public:
  //- Runtime type information
  TYPE_NAME("suppressionCollision");
  // Constructors
    //- Construct from dictionary
    SuppressionCollision(const dictionary& dict, CloudType& owner);
    //- Construct copy
    SuppressionCollision(const SuppressionCollision<CloudType>& cm);
    //- Construct and return a clone
    virtual autoPtr<StochasticCollisionModel<CloudType>> clone() const
    {
      return
        autoPtr<StochasticCollisionModel<CloudType>>
        {
          new SuppressionCollision<CloudType>{*this}
        };
    }
  //- Destructor
  virtual ~SuppressionCollision();
};

}  // namespace mousse

#include "_suppression_collision.ipp"

#endif
