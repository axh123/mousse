// mousse: CFD toolbox
// Copyright (C) 2011-2012 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "multi_dir_refinement.hpp"
#include "poly_mesh.hpp"
#include "poly_topo_changer.hpp"
#include "time.hpp"
#include "undoable_mesh_cutter.hpp"
#include "hex_cell_looper.hpp"
#include "geom_cell_looper.hpp"
#include "topo_set.hpp"
#include "directions.hpp"
#include "hex_ref8.hpp"
#include "map_poly_mesh.hpp"
#include "poly_topo_change.hpp"
#include "list_ops.hpp"
#include "cell_modeller.hpp"


// Static Data Members
namespace mousse {

DEFINE_TYPE_NAME_AND_DEBUG(multiDirRefinement, 0);

}


// Private Statc Functions
// Update refCells pattern for split cells. Note refCells is current
// list of cells to refine (these should all have been refined)
void mousse::multiDirRefinement::addCells
(
  const Map<label>& splitMap,
  List<refineCell>& refCells
)
{
  label newRefI = refCells.size();
  label oldSize = refCells.size();
  refCells.setSize(newRefI + splitMap.size());
  for (label refI = 0; refI < oldSize; refI++) {
    const refineCell& refCell = refCells[refI];
    Map<label>::const_iterator iter = splitMap.find(refCell.cellNo());
    if (iter == splitMap.end()) {
      FATAL_ERROR_IN
      (
        "multiDirRefinement::addCells(const Map<label>&"
        ", List<refineCell>&)"
      )
      << "Problem : cannot find added cell for cell "
      << refCell.cellNo() << abort(FatalError);
    }
    refCells[newRefI++] = refineCell(iter(), refCell.direction());
  }
}


// Update vectorField for all the new cells. Takes over value of
// original cell.
void mousse::multiDirRefinement::update
(
  const Map<label>& splitMap,
  vectorField& field
)
{
  field.setSize(field.size() + splitMap.size());
  FOR_ALL_CONST_ITER(Map<label>, splitMap, iter) {
    field[iter()] = field[iter.key()];
  }
}


// Add added cells to labelList
void mousse::multiDirRefinement::addCells
(
  const Map<label>& splitMap,
  labelList& labels
)
{
  label newCellI = labels.size();
  labels.setSize(labels.size() + splitMap.size());
  FOR_ALL_CONST_ITER(Map<label>, splitMap, iter) {
    labels[newCellI++] = iter();
  }
}


// Private Member Functions 
void mousse::multiDirRefinement::addCells
(
  const primitiveMesh& mesh,
  const Map<label>& splitMap
)
{
  // Construct inverse addressing: from new to original cell.
  labelList origCell{mesh.nCells(), -1};
  FOR_ALL(addedCells_, cellI) {
    const labelList& added = addedCells_[cellI];
    FOR_ALL(added, i) {
      label slave = added[i];
      if (origCell[slave] == -1) {
        origCell[slave] = cellI;
      } else if (origCell[slave] != cellI) {
        FATAL_ERROR_IN
        (
          "multiDirRefinement::addCells(const primitiveMesh&"
          ", const Map<label>&"
        )
        << "Added cell " << slave << " has two different masters:"
        << origCell[slave] << " , " << cellI
        << abort(FatalError);
      }
    }
  }
  FOR_ALL_CONST_ITER(Map<label>, splitMap, iter) {
    label masterI = iter.key();
    label newCellI = iter();
    while (origCell[masterI] != -1 && origCell[masterI] != masterI) {
      masterI = origCell[masterI];
    }
    if (masterI >= addedCells_.size()) {
      FATAL_ERROR_IN
      (
        "multiDirRefinement::addCells(const primitiveMesh&"
        ", const Map<label>&"
      )
      << "Map of added cells contains master cell " << masterI
      << " which is not a valid cell number" << endl
      << "This means that the mesh is not consistent with the"
      << " done refinement" << endl
      << "newCell:" << newCellI << abort(FatalError);
    }
    labelList& added = addedCells_[masterI];
    if (added.empty()) {
      added.setSize(2);
      added[0] = masterI;
      added[1] = newCellI;
    } else if (findIndex(added, newCellI) == -1) {
      label sz = added.size();
      added.setSize(sz + 1);
      added[sz] = newCellI;
    }
  }
}


mousse::labelList mousse::multiDirRefinement::splitOffHex(const primitiveMesh& mesh)
{
  const cellModel& hex = *(cellModeller::lookup("hex"));
  const cellShapeList& cellShapes = mesh.cellShapes();
  // Split cellLabels_ into two lists.
  labelList nonHexLabels{cellLabels_.size()};
  label nonHexI = 0;
  labelList hexLabels{cellLabels_.size()};
  label hexI = 0;
  FOR_ALL(cellLabels_, i) {
    label cellI = cellLabels_[i];
    if (cellShapes[cellI].model() == hex) {
      hexLabels[hexI++] = cellI;
    } else {
      nonHexLabels[nonHexI++] = cellI;
    }
  }
  nonHexLabels.setSize(nonHexI);
  cellLabels_.transfer(nonHexLabels);
  hexLabels.setSize(hexI);
  return hexLabels;
}


void mousse::multiDirRefinement::refineHex8
(
  polyMesh& mesh,
  const labelList& hexCells,
  const bool writeMesh
)
{
  if (debug) {
    Pout << "multiDirRefinement : Refining hexes " << hexCells.size() << endl;
  }
  hexRef8 hexRefiner
  {
    mesh,
    labelList{mesh.nCells(), 0},    // cellLevel
    labelList{mesh.nPoints(), 0},   // pointLevel
    refinementHistory
    {
      {
        "refinementHistory",
        mesh.facesInstance(),
        polyMesh::meshSubDir,
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE,
        false
      },
      List<refinementHistory::splitCell8>{0},
      labelList{0}
    }                                   // refinement history
  };
  polyTopoChange meshMod{mesh};
  labelList consistentCells
  {
    hexRefiner.consistentRefinement(hexCells, true /* buffer layer */)
  };
  // Check that consistentRefinement hasn't added cells
  {
    // Create count 1 for original cells
    Map<label> hexCellSet{2*hexCells.size()};
    FOR_ALL(hexCells, i) {
      hexCellSet.insert(hexCells[i], 1);
    }
    // Increment count
    FOR_ALL(consistentCells, i) {
      const label cellI = consistentCells[i];
      Map<label>::iterator iter = hexCellSet.find(cellI);
      if (iter == hexCellSet.end()) {
        FATAL_ERROR_IN
        (
          "multiDirRefinement::refineHex8"
          "(polyMesh&, const labelList&, const bool)"
        )
        << "Resulting mesh would not satisfy 2:1 ratio"
        << " when refining cell " << cellI << abort(FatalError);
      } else {
        iter() = 2;
      }
    }
    // Check if all been visited (should always be since
    // consistentRefinement set up to extend set.
    FOR_ALL_CONST_ITER(Map<label>, hexCellSet, iter) {
      if (iter() != 2) {
        FATAL_ERROR_IN
        (
          "multiDirRefinement::refineHex8"
          "(polyMesh&, const labelList&, const bool)"
        )
        << "Resulting mesh would not satisfy 2:1 ratio"
        << " when refining cell " << iter.key()
        << abort(FatalError);
      }
    }
  }
  hexRefiner.setRefinement(consistentCells, meshMod);
  // Change mesh, no inflation
  autoPtr<mapPolyMesh> morphMapPtr = meshMod.changeMesh(mesh, false, true);
  const mapPolyMesh& morphMap = morphMapPtr();
  if (morphMap.hasMotionPoints()) {
    mesh.movePoints(morphMap.preMotionPoints());
  }
  if (writeMesh) {
    mesh.write();
  }
  if (debug) {
    Pout << "multiDirRefinement : updated mesh at time "
      << mesh.time().timeName() << endl;
  }
  hexRefiner.updateMesh(morphMap);
  // Collect all cells originating from same old cell (original + 7 extra)
  FOR_ALL(consistentCells, i) {
    addedCells_[consistentCells[i]].setSize(8);
  }
  labelList nAddedCells{addedCells_.size(), 0};
  const labelList& cellMap = morphMap.cellMap();
  FOR_ALL(cellMap, cellI) {
    const label oldCellI = cellMap[cellI];
    if (addedCells_[oldCellI].size()) {
      addedCells_[oldCellI][nAddedCells[oldCellI]++] = cellI;
    }
  }
}


void mousse::multiDirRefinement::refineAllDirs
(
  polyMesh& mesh,
  List<vectorField>& cellDirections,
  const cellLooper& cellWalker,
  undoableMeshCutter& cutter,
  const bool writeMesh
)
{
  // Iterator
  refinementIterator refiner{mesh, cutter, cellWalker, writeMesh};
  FOR_ALL(cellDirections, dirI) {
    if (debug) {
      Pout << "multiDirRefinement : Refining " << cellLabels_.size()
        << " cells in direction " << dirI << endl
        << endl;
    }
    const vectorField& dirField = cellDirections[dirI];
    // Combine cell to be cut with direction to cut in.
    // If dirField is only one element use this for all cells.
    List<refineCell> refCells{cellLabels_.size()};
    if (dirField.size() == 1) {
      // Uniform directions.
      if (debug) {
        Pout << "multiDirRefinement : Uniform refinement:"
          << dirField[0] << endl;
      }
      FOR_ALL(refCells, refI) {
        label cellI = cellLabels_[refI];
        refCells[refI] = refineCell(cellI, dirField[0]);
      }
    } else {
      // Non uniform directions.
      FOR_ALL(refCells, refI) {
        const label cellI = cellLabels_[refI];
        refCells[refI] = refineCell(cellI, dirField[cellI]);
      }
    }
    // Do refine mesh (multiple iterations). Remember added cells.
    Map<label> splitMap = refiner.setRefinement(refCells);
    // Update overall map of added cells
    addCells(mesh, splitMap);
    // Add added cells to list of cells to refine in next iteration
    addCells(splitMap, cellLabels_);
    // Update refinement direction for added cells.
    if (dirField.size() != 1) {
      FOR_ALL(cellDirections, i) {
        update(splitMap, cellDirections[i]);
      }
    }
    if (debug) {
      Pout << "multiDirRefinement : Done refining direction " << dirI
        << " resulting in " << cellLabels_.size() << " cells" << nl
        << endl;
    }
  }
}


void mousse::multiDirRefinement::refineFromDict
(
  polyMesh& mesh,
  List<vectorField>& cellDirections,
  const dictionary& dict,
  const bool writeMesh
)
{
  // How to walk cell circumference.
  Switch pureGeomCut{dict.lookup("geometricCut")};
  autoPtr<cellLooper> cellWalker{NULL};
  if (pureGeomCut) {
    cellWalker.reset(new geomCellLooper(mesh));
  } else {
    cellWalker.reset(new hexCellLooper(mesh));
  }
  // Construct undoable refinement topology modifier.
  //Note: undoability switched off.
  // Might want to reconsider if needs to be possible. But then can always
  // use other constructor.
  undoableMeshCutter cutter{mesh, false};
  refineAllDirs(mesh, cellDirections, cellWalker(), cutter, writeMesh);
}


// Constructors 

// Construct from dictionary
mousse::multiDirRefinement::multiDirRefinement
(
  polyMesh& mesh,
  const labelList& cellLabels,        // list of cells to refine
  const dictionary& dict
)
:
  cellLabels_{cellLabels},
  addedCells_{mesh.nCells()}
{
  Switch useHex{dict.lookup("useHexTopology")};
  Switch writeMesh{dict.lookup("writeMesh")};
  wordList dirNames{dict.lookup("directions")};
  if (useHex && dirNames.size() == 3) {
    // Filter out hexes from cellLabels_
    labelList hexCells{splitOffHex(mesh)};
    refineHex8(mesh, hexCells, writeMesh);
  }
  label nRemainingCells = cellLabels_.size();
  reduce(nRemainingCells, sumOp<label>());
  if (nRemainingCells > 0) {
    // Any cells to refine using meshCutter
    // Determine directions for every cell. Can either be uniform
    // (size = 1) or non-uniform (one vector per cell)
    directions cellDirections{mesh, dict};
    refineFromDict(mesh, cellDirections, dict, writeMesh);
  }
}


// Construct from directionary and directions to refine.
mousse::multiDirRefinement::multiDirRefinement
(
  polyMesh& mesh,
  const labelList& cellLabels,        // list of cells to refine
  const List<vectorField>& cellDirs,  // Explicitly provided directions
  const dictionary& dict
)
:
  cellLabels_{cellLabels},
  addedCells_{mesh.nCells()}
{
  Switch useHex{dict.lookup("useHexTopology")};
  Switch writeMesh{dict.lookup("writeMesh")};
  wordList dirNames{dict.lookup("directions")};
  if (useHex && dirNames.size() == 3) {
    // Filter out hexes from cellLabels_
    labelList hexCells{splitOffHex(mesh)};
    refineHex8(mesh, hexCells, writeMesh);
  }
  label nRemainingCells = cellLabels_.size();
  reduce(nRemainingCells, sumOp<label>());
  if (nRemainingCells > 0) {
    // Any cells to refine using meshCutter
    // Working copy of cells to refine
    List<vectorField> cellDirections{cellDirs};
    refineFromDict(mesh, cellDirections, dict, writeMesh);
  }
}


// Construct from components. Implies meshCutter since directions provided.
mousse::multiDirRefinement::multiDirRefinement
(
  polyMesh& mesh,
  undoableMeshCutter& cutter,     // actual mesh modifier
  const cellLooper& cellWalker,   // how to cut a single cell with
                  // a plane
  const labelList& cellLabels,    // list of cells to refine
  const List<vectorField>& cellDirs,
  const bool writeMesh            // write intermediate meshes
)
:
  cellLabels_{cellLabels},
  addedCells_{mesh.nCells()}
{
  // Working copy of cells to refine
  List<vectorField> cellDirections{cellDirs};
  refineAllDirs(mesh, cellDirections, cellWalker, cutter, writeMesh);
}

