#ifndef DYNAMIC_MESH_MESH_CUT_MESH_MODIFIERS_MULTI_DIR_REFINEMENT_MULTI_DIR_REFINEMENT_HPP_
#define DYNAMIC_MESH_MESH_CUT_MESH_MODIFIERS_MULTI_DIR_REFINEMENT_MULTI_DIR_REFINEMENT_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::multiDirRefinement
// Description
//   Does multiple pass refinement to refine cells in multiple directions.
//   Gets a list of cells to refine and vectorFields for the whole mesh.
//   It then tries to refine in one direction after the other the wanted cells.
//   After construction the mesh will have been refined in multiple directions.
//   Holds the list of cells to refine and the map from original to added for
//   every refinement level.
//   Gets constructed from a dictionary or from components.
//   Uses an undoableMeshCutter which does the actual cutting. Undo facility
//   is switched of unless constructed from external one which allows this.
//   The cut cells get stored in addedCells which is for every vectorField
//   to cut with the map from uncut to added cell (i.e. from master to slave).
//   Note: map is only valid for a given direction.
//   Parallel: should be ok. Uses 'reduce' whenever it needs to make a
//   local decision.

#include "refinement_iterator.hpp"
#include "vector_field.hpp"
#include "map.hpp"
#include "class_name.hpp"


namespace mousse {

// Forward declaration of classes
class undoableMeshCutter;
class cellLooper;
class topoSet;


class multiDirRefinement
{
  // Private data
    //- Current set of cells to refine. Extended with added cells.
    labelList cellLabels_;
    //- From original to added cells.
    //  Gives for every cell in the original mesh an empty list or the
    //  list of cells this one has been split into (note: will include
    //  itself so e.g. for hex will be 8 if 2x2x2 refinement)
    labelListList addedCells_;
  // Private Static Functions
    //- Given map from original to added cell set the refineCell for
    //  the added cells to be equal to the one on the original cells.
    static void addCells(const Map<label>&, List<refineCell>&);
    //- Given map from original to added cell set the vectorField for
    //  the added cells to be equal to the one on the original cells.
    static void update(const Map<label>&, vectorField&);
    //- Given map from original to added cell add the added cell to the
    //  list of labels
    static void addCells(const Map<label>&, labelList& labels);
  // Private Member Functions
    //- Add new cells from map to overall list (addedCells_).
    void addCells(const primitiveMesh&, const Map<label>&);
    //- Remove hexes from cellLabels_ and return these in a list.
    labelList splitOffHex(const primitiveMesh& mesh);
    //- Refine cells (hex only) in all 3 directions.
    void refineHex8
    (
      polyMesh& mesh,
      const labelList& hexCells,
      const bool writeMesh
    );
    //- Refine cells in cellLabels_ in directions mentioned.
    void refineAllDirs
    (
      polyMesh& mesh,
      List<vectorField>& cellDirections,
      const cellLooper& cellWalker,
      undoableMeshCutter& cutter,
      const bool writeMesh
    );
    //- Refine based on dictionary. Calls refineAllDirs.
    void refineFromDict
    (
      polyMesh& mesh,
      List<vectorField>& cellDirections,
      const dictionary& dict,
      const bool writeMesh
    );
public:
  //- Runtime type information
  CLASS_NAME("multiDirRefinement");
  // Constructors
    //- Construct from dictionary. After construction all refinement will
    //  have been done (and runTime will have increased a few time steps if
    //  writeMesh = true)
    multiDirRefinement
    (
      polyMesh& mesh,
      const labelList& cellLabels,    // cells to refine
      const dictionary& dict
    );
    //- Explicitly provided directions to split in.
    multiDirRefinement
    (
      polyMesh& mesh,
      const labelList& cellLabels,    // cells to refine
      const List<vectorField>&,       // Explicitly provided directions
      const dictionary& dict
    );
    //- Construct from components. Only this one would allow undo actions.
    multiDirRefinement
    (
      polyMesh& mesh,
      undoableMeshCutter& cutter,     // actual mesh modifier
      const cellLooper& cellCutter,   // how to cut a single cell with
                      // a plane
      const labelList& cellLabels,    // list of cells to refine
      const List<vectorField>& directions,
      const bool writeMesh = false    // write intermediate meshes
    );
    //- Disallow default bitwise copy construct
    multiDirRefinement(const multiDirRefinement&) = delete;
    //- Disallow default bitwise assignment
    multiDirRefinement& operator=(const multiDirRefinement&) = delete;
  // Member Functions
    //- Access to addedCells (on the original mesh; see above)
    const labelListList& addedCells() const
    {
      return addedCells_;
    }
};

}  // namespace mousse

#endif

