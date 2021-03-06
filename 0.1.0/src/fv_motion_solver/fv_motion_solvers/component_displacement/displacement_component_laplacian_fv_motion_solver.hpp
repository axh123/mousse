#ifndef FV_MOTION_SOLVER_FV_MOTION_SOLVERS_COMPONENT_DISPLACEMENT_COMPONENT_LAPLACIAN_DISPLACEMENT_COMPONENT_LAPLACIAN_FV_MOTION_SOLVER_HPP_
#define FV_MOTION_SOLVER_FV_MOTION_SOLVERS_COMPONENT_DISPLACEMENT_COMPONENT_LAPLACIAN_DISPLACEMENT_COMPONENT_LAPLACIAN_FV_MOTION_SOLVER_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011-2012 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::displacementComponentLaplacianFvMotionSolver
// Description
//   Mesh motion solver for an fvMesh.  Based on solving the cell-centre
//   Laplacian for the given component of the motion displacement.

#include "component_displacement_motion_solver.hpp"
#include "fv_motion_solver_core.hpp"


namespace mousse {

// Forward class declarations
class motionDiffusivity;


class displacementComponentLaplacianFvMotionSolver
:
  public componentDisplacementMotionSolver,
  public fvMotionSolverCore
{
  // Private data
    //- Cell-centre motion field
    mutable volScalarField cellDisplacement_;
    //- Optionally read point-position field. Used only for position
    //  boundary conditions.
    mutable autoPtr<pointVectorField> pointLocation_;
    //- Diffusivity used to control the motion
    autoPtr<motionDiffusivity> diffusivityPtr_;
    //- Frozen points (that are not on patches). -1 or points that are
    //  fixed to be at points0_ location
    label frozenPointsZone_;
  // Private Member Functions
    //- Return the component corresponding to the given component name
    direction cmpt(const word& cmptName) const;
public:
  //- Runtime type information
  TYPE_NAME("displacementComponentLaplacian");
  // Constructors
    //- Construct from polyMesh and IOdictionary
    displacementComponentLaplacianFvMotionSolver
    (
      const polyMesh&,
      const IOdictionary&
    );
    //- Disallow default bitwise copy construct
    displacementComponentLaplacianFvMotionSolver
    (
      const displacementComponentLaplacianFvMotionSolver&
    ) = delete;
    //- Disallow default bitwise assignment
    displacementComponentLaplacianFvMotionSolver& operator=
    (
      const displacementComponentLaplacianFvMotionSolver&
    ) = delete;
  //- Destructor
  ~displacementComponentLaplacianFvMotionSolver();
  // Member Functions
    //- Non-const access to the cellDisplacement in order to allow
    //  changes to the boundary motion
    volScalarField& cellDisplacement()
    {
      return cellDisplacement_;
    }
    //- Return point location obtained from the current motion field
    virtual tmp<pointField> curPoints() const;
    //- Solve for motion
    virtual void solve();
    //- Update topology
    virtual void updateMesh(const mapPolyMesh&);
};

}  // namespace mousse

#endif

