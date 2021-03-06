#ifndef UTILITIES_MESH_GENERATION_FOAMY_MESH_CONFORMAL_VORONOI_MESH_CELL_SIZE_CONTROL_SURFACES_CELL_SIZE_FUNCTION_SURFACE_OFFSET_LINEAR_DISTANCE_HPP_
#define UTILITIES_MESH_GENERATION_FOAMY_MESH_CONFORMAL_VORONOI_MESH_CELL_SIZE_CONTROL_SURFACES_CELL_SIZE_FUNCTION_SURFACE_OFFSET_LINEAR_DISTANCE_HPP_

// mousse: CFD toolbox
// Copyright (C) 2012-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::surfaceOffsetLinearDistance

#include "cell_size_function.hpp"


namespace mousse {

class surfaceOffsetLinearDistance
:
  public cellSizeFunction
{
private:
  // Private data
    //- Cell size at distance_ from the surface
    scalar distanceCellSize_;
    //- Offset distance from surface for constant size portion
    scalar surfaceOffset_;
    //- Total distance from the surface to control over (distance +
    //  surfaceOffset)
    scalar totalDistance_;
    //- totalDistance squared
    scalar totalDistanceSqr_;
  // Private Member Functions
    //- Calculate the cell size as a function of the given distance
    scalar sizeFunction(const point& pt, scalar d, label index) const;
public:
  //- Runtime type information
  TYPE_NAME("surfaceOffsetLinearDistance");
  // Constructors
    //- Construct from components
    surfaceOffsetLinearDistance
    (
      const dictionary& initialPointsDict,
      const searchableSurface& surface,
      const scalar& defaultCellSize,
      const labelList regionIndices
    );
  //- Destructor
  virtual ~surfaceOffsetLinearDistance()
  {}
  // Member Functions
    virtual bool sizeLocations
    (
      const pointIndexHit& hitPt,
      const vector& n,
      pointField& shapePts,
      scalarField& shapeSizes
    ) const;
    //- Modify scalar argument to the cell size specified by function.
    //  Return a boolean specifying if the function was used, i.e. false if
    //  the point was not in range of the surface for a spatially varying
    //  size.
    virtual bool cellSize
    (
      const point& pt,
      scalar& size
    ) const;
};

}  // namespace mousse

#endif

