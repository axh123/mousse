#ifndef UTILITIES_MESH_GENERATION_FOAMY_MESH_CONFORMAL_VORONOI_MESH_CELL_SHAPE_CONTROL_SMOOTH_ALIGNMENT_SOLVER_HPP_
#define UTILITIES_MESH_GENERATION_FOAMY_MESH_CONFORMAL_VORONOI_MESH_CELL_SHAPE_CONTROL_SMOOTH_ALIGNMENT_SOLVER_HPP_

// mousse: CFD toolbox
// Copyright (C) 2013-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::smoothAlignmentSolver

#include "cell_shape_control_mesh.hpp"
#include "triad_field.hpp"


namespace mousse {

class smoothAlignmentSolver
{
  // Private data
    cellShapeControlMesh& mesh_;
  // Private Member Functions
    template<class Triangulation, class Type>
    tmp<Field<Type>> filterFarPoints
    (
      const Triangulation& mesh,
      const Field<Type>& field
    );
    template<class Triangulation>
    autoPtr<mapDistribute> buildMap
    (
      const Triangulation& mesh,
      labelListList& pointPoints
    );
    template<class Triangulation>
    autoPtr<mapDistribute> buildReferredMap
    (
      const Triangulation& mesh,
      labelList& indices
    );
    template<class Triangulation>
    tmp<triadField> buildAlignmentField(const Triangulation& mesh);
    template<class Triangulation>
    tmp<pointField> buildPointField(const Triangulation& mesh);
    //- Apply the fixed alignments to the triad
    void applyBoundaryConditions
    (
      const triad& fixedAlignment,
      triad& t
    ) const;
public:
  // Constructors
    //- Construct null
    smoothAlignmentSolver(cellShapeControlMesh& mesh);
    //- Disallow default bitwise copy construct
    smoothAlignmentSolver(const smoothAlignmentSolver&) = delete;
    //- Disallow default bitwise assignment
    void operator=(const smoothAlignmentSolver&) = delete;
  //- Destructor
  ~smoothAlignmentSolver();
  // Member Functions
    // Edit
      //- Smooth the alignments on the mesh
      void smoothAlignments(const label maxSmoothingIterations);
};

}  // namespace mousse

#endif

