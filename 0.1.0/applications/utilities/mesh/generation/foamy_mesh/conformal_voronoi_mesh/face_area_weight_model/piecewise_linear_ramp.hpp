#ifndef UTILITIES_MESH_GENERATION_FOAMY_MESH_CONFORMAL_VORONOI_MESH_FACE_AREA_WEIGHT_MODEL_PIECEWISE_LINEAR_RAMP_HPP_
#define UTILITIES_MESH_GENERATION_FOAMY_MESH_CONFORMAL_VORONOI_MESH_FACE_AREA_WEIGHT_MODEL_PIECEWISE_LINEAR_RAMP_HPP_

// mousse: CFD toolbox
// Copyright (C) 2012-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::piecewiseLinearRamp
// Description
//   A linear ramp between 0 and 1 with definable start and end points.

#include "face_area_weight_model.hpp"


namespace mousse {

class piecewiseLinearRamp
:
  public faceAreaWeightModel
{
private:
  // Private data
    //- Face area fraction below which a weight of 0 is returned
    scalar lAF_;
    //- Face area fraction above which a which of 1 is returned
    scalar uAF_;
public:
  //- Runtime type information
  TYPE_NAME("piecewiseLinearRamp");
  // Constructors
    //- Construct from components
    piecewiseLinearRamp
    (
      const dictionary& faceAreaWeightDict
    );
  //- Destructor
  virtual ~piecewiseLinearRamp()
  {}
  // Member Functions
    //- Return the faceAreaWeight
    virtual scalar faceAreaWeight(scalar faceAreaFraction) const;
};

}  // namespace mousse

#endif

