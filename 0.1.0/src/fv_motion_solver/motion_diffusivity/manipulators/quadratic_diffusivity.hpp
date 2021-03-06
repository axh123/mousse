#ifndef FV_MOTION_SOLVER_MOTION_DIFFUSIVITY_MANIPULATORS_QUADRATIC_QUADRATIC_DIFFUSIVITY_HPP_
#define FV_MOTION_SOLVER_MOTION_DIFFUSIVITY_MANIPULATORS_QUADRATIC_QUADRATIC_DIFFUSIVITY_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011-2012 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::quadraticDiffusivity
// Description
//   Mesh motion diffusivity maipulator which returns the sqr of the given
//   diffusivity.

#include "motion_diffusivity.hpp"


namespace mousse {

class quadraticDiffusivity
:
  public motionDiffusivity
{
  // Private data
    autoPtr<motionDiffusivity> basicDiffusivityPtr_;
public:
  //- Runtime type information
  TYPE_NAME("quadratic");
  // Constructors
    //- Construct for the given fvMesh and data Istream
    quadraticDiffusivity(const fvMesh& mesh, Istream& mdData);
    //- Disallow default bitwise copy construct
    quadraticDiffusivity(const quadraticDiffusivity&) = delete;
    //- Disallow default bitwise assignment
    quadraticDiffusivity& operator=(const quadraticDiffusivity&) = delete;
  //- Destructor
  virtual ~quadraticDiffusivity();
  // Member Functions
    //- Return diffusivity field
    virtual tmp<surfaceScalarField> operator()() const;
    //- Correct the motion diffusivity
    virtual void correct();
};

}  // namespace mousse

#endif

