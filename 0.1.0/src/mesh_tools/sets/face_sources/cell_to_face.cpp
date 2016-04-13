// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "cell_to_face.hpp"
#include "poly_mesh.hpp"
#include "cell_set.hpp"
#include "time.hpp"
#include "sync_tools.hpp"
#include "add_to_run_time_selection_table.hpp"


// Static Data Members
namespace mousse {

DEFINE_TYPE_NAME_AND_DEBUG(cellToFace, 0);
ADD_TO_RUN_TIME_SELECTION_TABLE(topoSetSource, cellToFace, word);
ADD_TO_RUN_TIME_SELECTION_TABLE(topoSetSource, cellToFace, istream);

template<>
const char* mousse::NamedEnum
<
  mousse::cellToFace::cellAction,
  2
>::names[] =
{
  "all",
  "both"
};

}

mousse::topoSetSource::addToUsageTable mousse::cellToFace::usage_
{
  cellToFace::typeName,
  "\n    Usage: cellToFace <cellSet> all|both\n\n"
  "    Select -all : all faces of cells in the cellSet\n"
  "           -both: faces where both neighbours are in the cellSet\n\n"
};

const mousse::NamedEnum<mousse::cellToFace::cellAction, 2>
  mousse::cellToFace::cellActionNames_;


// Private Member Functions 
void mousse::cellToFace::combine(topoSet& set, const bool add) const
{
  // Load the set
  if (!exists(mesh_.time().path()/topoSet::localPath(mesh_, setName_))) {
    SeriousError << "Cannot load set "
      << setName_ << endl;
  }
  cellSet loadedSet{mesh_, setName_};
  if (option_ == ALL) {
    // Add all faces from cell
    FOR_ALL_CONST_ITER(cellSet, loadedSet, iter) {
      const label cellI = iter.key();
      const labelList& cFaces = mesh_.cells()[cellI];
      FOR_ALL(cFaces, cFaceI) {
        addOrDelete(set, cFaces[cFaceI], add);
      }
    }
  } else if (option_ == BOTH) {
    // Add all faces whose both neighbours are in set.
    const label nInt = mesh_.nInternalFaces();
    const labelList& own = mesh_.faceOwner();
    const labelList& nei = mesh_.faceNeighbour();
    const polyBoundaryMesh& patches = mesh_.boundaryMesh();
    // Check all internal faces
    for (label faceI = 0; faceI < nInt; faceI++) {
      if (loadedSet.found(own[faceI]) && loadedSet.found(nei[faceI])) {
        addOrDelete(set, faceI, add);
      }
    }
    // Get coupled cell status
    boolList neiInSet{mesh_.nFaces()-nInt, false};
    FOR_ALL(patches, patchI) {
      const polyPatch& pp = patches[patchI];
      if (pp.coupled()) {
        label faceI = pp.start();
        FOR_ALL(pp, i) {
          neiInSet[faceI-nInt] = loadedSet.found(own[faceI]);
          faceI++;
        }
      }
    }
    syncTools::swapBoundaryFaceList(mesh_, neiInSet);
    // Check all boundary faces
    FOR_ALL(patches, patchI) {
      const polyPatch& pp = patches[patchI];
      if (pp.coupled()) {
        label faceI = pp.start();
        FOR_ALL(pp, i) {
          if (loadedSet.found(own[faceI]) && neiInSet[faceI-nInt]) {
            addOrDelete(set, faceI, add);
          }
          faceI++;
        }
      }
    }
  }
}


// Constructors 
// Construct from componenta
mousse::cellToFace::cellToFace
(
  const polyMesh& mesh,
  const word& setName,
  const cellAction option
)
:
  topoSetSource{mesh},
  setName_{setName},
  option_{option}
{}


// Construct from dictionary
mousse::cellToFace::cellToFace
(
  const polyMesh& mesh,
  const dictionary& dict
)
:
  topoSetSource{mesh},
  setName_{dict.lookup("set")},
  option_{cellActionNames_.read(dict.lookup("option"))}
{}


// Construct from Istream
mousse::cellToFace::cellToFace
(
  const polyMesh& mesh,
  Istream& is
)
:
  topoSetSource{mesh},
  setName_{checkIs(is)},
  option_{cellActionNames_.read(checkIs(is))}
{}


// Destructor 
mousse::cellToFace::~cellToFace()
{}


// Member Functions 
void mousse::cellToFace::applyToSet
(
  const topoSetSource::setAction action,
  topoSet& set
) const
{
  if ((action == topoSetSource::NEW) || (action == topoSetSource::ADD)) {
    Info << "    Adding faces according to cellSet " << setName_
      << " ..." << endl;
    combine(set, true);
  } else if (action == topoSetSource::DELETE) {
    Info << "    Removing faces according to cellSet " << setName_
      << " ..." << endl;
    combine(set, false);
  }
}

