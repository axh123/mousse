// mousse: CFD toolbox
// Copyright (C) 2012 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "component_displacement_motion_solver.hpp"
#include "map_poly_mesh.hpp"


// Static Data Members
namespace mousse {

DEFINE_TYPE_NAME_AND_DEBUG(componentDisplacementMotionSolver, 0);

}


// Private Member Functions 
mousse::direction mousse::componentDisplacementMotionSolver::cmpt
(
  const word& cmptName
) const
{
  if (cmptName == "x") {
    return vector::X;
  } else if (cmptName == "y") {
    return vector::Y;
  } else if (cmptName == "z") {
    return vector::Z;
  } else {
    FATAL_ERROR_IN
    (
      "componentDisplacementMotionSolver::"
      "componentDisplacementMotionSolver"
      "(const polyMesh& mesh, const IOdictionary&)"
    )
    << "Given component name " << cmptName << " should be x, y or z"
    << exit(FatalError);
    return 0;
  }
}


// Constructors 
mousse::componentDisplacementMotionSolver::componentDisplacementMotionSolver
(
  const polyMesh& mesh,
  const IOdictionary& dict,
  const word& type
)
:
  motionSolver{mesh, dict, type},
  cmptName_{coeffDict().lookup("component")},
  cmpt_{cmpt(cmptName_)},
  points0_
  {
    pointIOField
    {
      {
        "points",
        time().constant(),
        polyMesh::meshSubDir,
        mesh,
        IOobject::MUST_READ,
        IOobject::NO_WRITE,
        false
      }
    }.component(cmpt_)
  },
  pointDisplacement_
  {
    {
      "pointDisplacement" + cmptName_,
      mesh.time().timeName(),
      mesh,
      IOobject::MUST_READ,
      IOobject::AUTO_WRITE
    },
    pointMesh::New(mesh)
  }
{
  if (points0_.size() != mesh.nPoints()) {
    FATAL_ERROR_IN
    (
      "componentDisplacementMotionSolver::"
      "componentDisplacementMotionSolver\n"
      "(\n"
      "    const polyMesh&,\n"
      "    const IOdictionary&\n"
      ")"
    )
    << "Number of points in mesh " << mesh.nPoints()
    << " differs from number of points " << points0_.size()
    << " read from file "
    << IOobject{"points", mesh.time().constant(), polyMesh::meshSubDir,
                mesh, IOobject::MUST_READ, IOobject::NO_WRITE,
                false}.filePath()
    << exit(FatalError);
  }
}


// Destructor 
mousse::componentDisplacementMotionSolver::~componentDisplacementMotionSolver()
{}


// Member Functions 
void mousse::componentDisplacementMotionSolver::movePoints(const pointField&)
{
  // No local data to update
}


void mousse::componentDisplacementMotionSolver::updateMesh(const mapPolyMesh& mpm)
{
  // pointMesh already updates pointFields.
  motionSolver::updateMesh(mpm);
  // Map points0_. Bit special since we somehow have to come up with
  // a sensible points0 position for introduced points.
  // Find out scaling between points0 and current points
  // Get the new points either from the map or the mesh
  const scalarField points
  {
    mpm.hasMotionPoints()
    ? mpm.preMotionPoints().component(cmpt_)
    : mesh().points().component(cmpt_)
  };
  // Get extents of points0 and points and determine scale
  const scalar scale =
    (gMax(points0_)-gMin(points0_))/(gMax(points)-gMin(points));
  scalarField newPoints0{mpm.pointMap().size()};
  FOR_ALL(newPoints0, pointI) {
    label oldPointI = mpm.pointMap()[pointI];
    if (oldPointI >= 0) {
      label masterPointI = mpm.reversePointMap()[oldPointI];
      if (masterPointI == pointI) {
        newPoints0[pointI] = points0_[oldPointI];
      } else {
        // New point. Assume motion is scaling.
        newPoints0[pointI] =
          points0_[oldPointI] + scale*(points[pointI] - points[masterPointI]);
      }
    } else {
      FATAL_ERROR_IN
      (
        "displacementLaplacianFvMotionSolver::updateMesh"
        "(const mapPolyMesh& mpm)"
      )
      << "Cannot work out coordinates of introduced vertices."
      << " New vertex " << pointI << " at coordinate "
      << points[pointI] << exit(FatalError);
    }
  }
  points0_.transfer(newPoints0);
}

