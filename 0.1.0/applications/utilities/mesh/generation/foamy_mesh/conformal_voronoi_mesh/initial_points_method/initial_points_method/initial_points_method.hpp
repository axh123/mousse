// mousse: CFD toolbox
// Copyright (C) 2012-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::initialPointsMethod
// Description
//   Abstract base class for generating initial points for a conformalVoronoiMesh
// SourceFiles
//   initial_points_method.cpp
#ifndef initial_points_method_hpp_
#define initial_points_method_hpp_
#include "point.hpp"
#include "conformal_voronoi_mesh.hpp"
#include "background_mesh_decomposition.hpp"
#include "dictionary.hpp"
#include "random.hpp"
#include "switch.hpp"
#include "auto_ptr.hpp"
#include "run_time_selection_tables.hpp"
namespace mousse
{
class initialPointsMethod
:
  public dictionary
{
protected:
  // Protected data
    const Time& runTime_;
    Random& rndGen_;
    const conformationSurfaces& geometryToConformTo_;
    const cellShapeControl& cellShapeControls_;
    const autoPtr<backgroundMeshDecomposition>& decomposition_;
    //- Method details dictionary
    dictionary detailsDict_;
    //- Only allow the placement of initial points that are within the
    //  surfaces to be meshed by minimumSurfaceDistanceCoeff multiplied by
    //  the local target cell size.  Store square of value.
    scalar minimumSurfaceDistanceCoeffSqr_;
    Switch fixInitialPoints_;
private:
  // Private Member Functions
    //- Disallow default bitwise copy construct
    initialPointsMethod(const initialPointsMethod&);
    //- Disallow default bitwise assignment
    void operator=(const initialPointsMethod&);
public:
  //- Runtime type information
  TypeName("initialPointsMethod");
  // Declare run-time constructor selection table
    declareRunTimeSelectionTable
    (
      autoPtr,
      initialPointsMethod,
      dictionary,
      (
        const dictionary& initialPointsDict,
        const Time& runTime,
        Random& rndGen,
        const conformationSurfaces& geometryToConformTo,
        const cellShapeControl& cellShapeControls,
        const autoPtr<backgroundMeshDecomposition>& decomposition
      ),
      (
        initialPointsDict,
        runTime,
        rndGen,
        geometryToConformTo,
        cellShapeControls,
        decomposition
      )
    );
  // Constructors
    //- Construct from components
    initialPointsMethod
    (
      const word& type,
      const dictionary& initialPointsDict,
      const Time& runTime,
      Random& rndGen,
      const conformationSurfaces& geometryToConformTo,
      const cellShapeControl& cellShapeControls,
      const autoPtr<backgroundMeshDecomposition>& decomposition
    );
  // Selectors
    //- Return a reference to the selected initialPointsMethod
    static autoPtr<initialPointsMethod> New
    (
      const dictionary& initialPointsDict,
      const Time& runTime,
      Random& rndGen,
      const conformationSurfaces& geometryToConformTo,
      const cellShapeControl& cellShapeControls,
      const autoPtr<backgroundMeshDecomposition>& decomposition
    );
  //- Destructor
  virtual ~initialPointsMethod();
  // Member Functions
    // Access
      const Time& time() const
      {
        return runTime_;
      }
      Random& rndGen() const
      {
        return rndGen_;
      }
      const conformationSurfaces& geometryToConformTo() const
      {
        return geometryToConformTo_;
      }
      const cellShapeControl& cellShapeControls() const
      {
        return cellShapeControls_;
      }
      const backgroundMeshDecomposition& decomposition() const
      {
        return decomposition_;
      }
      //- Const access to the details dictionary
      const dictionary& detailsDict() const
      {
        return detailsDict_;
      }
      Switch fixInitialPoints() const
      {
        return fixInitialPoints_;
      }
    // Queries
      //- Return the initial points for the conformalVoronoiMesh
      virtual List<Vb::Point> initialPoints() const = 0;
};
}  // namespace mousse
#endif
