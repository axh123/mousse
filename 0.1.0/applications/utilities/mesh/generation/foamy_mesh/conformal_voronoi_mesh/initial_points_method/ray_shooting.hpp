#ifndef UTILITIES_MESH_GENERATION_FOAMY_MESH_CONFORMAL_VORONOI_MESH_INITIAL_POINTS_METHOD_RAY_SHOOTING_HPP_
#define UTILITIES_MESH_GENERATION_FOAMY_MESH_CONFORMAL_VORONOI_MESH_INITIAL_POINTS_METHOD_RAY_SHOOTING_HPP_

// mousse: CFD toolbox
// Copyright (C) 2013-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::rayShooting

#include "initial_points_method.hpp"


namespace mousse {

class rayShooting
:
  public initialPointsMethod
{
private:
  // Private data
    //- Should the initial positions be randomised
    Switch randomiseInitialGrid_;
    //- Randomise the initial positions by fraction of the initialCellSize_
    scalar randomPerturbationCoeff_;
  // Private Member Functions
    void splitLine
    (
      const line<point, point>& l,
      const scalar& pert,
      DynamicList<Vb::Point>& initialPoints
    ) const;
public:
  //- Runtime type information
  TYPE_NAME("rayShooting");
  // Constructors
    //- Construct from components
    rayShooting
    (
      const dictionary& initialPointsDict,
      const Time& runTime,
      Random& rndGen,
      const conformationSurfaces& geometryToConformTo,
      const cellShapeControl& cellShapeControls,
      const autoPtr<backgroundMeshDecomposition>& decomposition
    );
  //- Destructor
  virtual ~rayShooting()
  {}
  // Member Functions
    //- Return the initial points for the conformalVoronoiMesh
    virtual List<Vb::Point> initialPoints() const;
};

}  // namespace mousse

#endif

