#ifndef LAGRANGIAN_INTERMEDIATE_SUBMODELS_TREACTING_TCOMPOSITION_MODEL_TSINGLE_MIXTURE_FRACTION_HPP_
#define LAGRANGIAN_INTERMEDIATE_SUBMODELS_TREACTING_TCOMPOSITION_MODEL_TSINGLE_MIXTURE_FRACTION_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::SingleMixtureFraction
// Description
//   Templated parcel multi-phase, multi-component class

#include "_composition_model.hpp"


namespace mousse {

template<class CloudType>
class SingleMixtureFraction
:
  public CompositionModel<CloudType>
{
  // Private data
    // Indices of the phases
      //- Gas
      label idGas_;
      //- Liquid
      label idLiquid_;
      //- Solid
      label idSolid_;
   // Mixture properties
      //- Phase component total fractions
      scalarField YMixture0_;
  // Private Member Functions
    //- Construct the indices and check correct specification of
    //  1 gas, 1 liquid and 1 solid
    void constructIds();
public:
  //- Runtime type information
  TYPE_NAME("singleMixtureFraction");
  // Constructors
    //- Construct from dictionary
    SingleMixtureFraction(const dictionary& dict, CloudType& owner);
    //- Construct copy
    SingleMixtureFraction(const SingleMixtureFraction<CloudType>& cm);
    //- Construct and return a clone
    virtual autoPtr<CompositionModel<CloudType>> clone() const
    {
      return
        autoPtr<CompositionModel<CloudType>>
        {
          new SingleMixtureFraction<CloudType>{*this}
        };
    }
  //- Destructor
  virtual ~SingleMixtureFraction();
  // Member Functions
    // Access
      // Mixture properties
        //- Return the list of mixture mass fractions
        virtual const scalarField& YMixture0() const;
        // Indices of gas, liquid and solid phases in phase properties
        // list
          //- Gas id
          virtual label idGas() const;
          //- Liquid id
          virtual label idLiquid() const;
          //- Solid id
          virtual label idSolid() const;
};

#include "_single_mixture_fraction.ipp"

}  // namespace mousse
#endif
