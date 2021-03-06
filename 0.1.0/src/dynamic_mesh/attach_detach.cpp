// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "attach_detach.hpp"
#include "poly_topo_changer.hpp"
#include "poly_mesh.hpp"
#include "time.hpp"
#include "primitive_mesh.hpp"
#include "poly_topo_change.hpp"
#include "add_to_run_time_selection_table.hpp"


// Static Data Members
namespace mousse {

DEFINE_TYPE_NAME_AND_DEBUG(attachDetach, 0);
ADD_TO_RUN_TIME_SELECTION_TABLE
(
  polyMeshModifier,
  attachDetach,
  dictionary
);

}


// Private Member Functions 
void mousse::attachDetach::checkDefinition()
{
  if (!faceZoneID_.active() || !masterPatchID_.active()
      || !slavePatchID_.active()) {
    FATAL_ERROR_IN
    (
      "void mousse::attachDetach::checkDefinition()"
    )
    << "Not all zones and patches needed in the definition "
    << "have been found.  Please check your mesh definition."
    << abort(FatalError);
  }
  const polyMesh& mesh = topoChanger().mesh();
  if (debug) {
    Pout << "Attach/detach object " << name() << " :" << nl
      << "    faceZoneID:   " << faceZoneID_ << nl
      << "    masterPatchID: " << masterPatchID_ << nl
      << "    slavePatchID: " << slavePatchID_ << endl;
  }
  // Check the sizes and set up state
  if (mesh.boundaryMesh()[masterPatchID_.index()].empty()
      && mesh.boundaryMesh()[slavePatchID_.index()].empty()) {
    // Boundary is attached
    if (debug) {
      Pout << "    Attached on construction" << endl;
    }
    state_ = ATTACHED;
    // Check if there are faces in the master zone
    if (mesh.faceZones()[faceZoneID_.index()].empty()) {
      FATAL_ERROR_IN
      (
        "void mousse::attachDetach::checkDefinition()"
      )
      << "Attach/detach zone contains no faces.  Please check your "
      << "mesh definition."
      << abort(FatalError);
    }
    // Check that all the faces in the face zone are internal
    if (debug) {
      const labelList& addr = mesh.faceZones()[faceZoneID_.index()];
      DynamicList<label> bouFacesInZone{addr.size()};
      FOR_ALL(addr, faceI) {
        if (!mesh.isInternalFace(addr[faceI])) {
          bouFacesInZone.append(addr[faceI]);
        }
      }
      if (bouFacesInZone.size()) {
        FATAL_ERROR_IN
        (
          "void mousse::attachDetach::checkDefinition()"
        )
        << "Found boundary faces in the zone defining "
        << "attach/detach boundary "
        << " for object " << name()
        << " : .  This is not allowed." << nl
        << "Boundary faces: " << bouFacesInZone
        << abort(FatalError);
      }
    }
  } else {
    // Boundary is detached
    if (debug) {
      Pout << "    Detached on construction" << endl;
    }
    state_ = DETACHED;
    // Check that the sizes of master and slave patch are identical
    // and identical to the size of the face zone
    if((mesh.boundaryMesh()[masterPatchID_.index()].size()
        != mesh.boundaryMesh()[slavePatchID_.index()].size())
       || (mesh.boundaryMesh()[masterPatchID_.index()].size()
           != mesh.faceZones()[faceZoneID_.index()].size())) {
      FATAL_ERROR_IN
      (
        "void mousse::attachDetach::checkDefinition()"
      )
      << "Problem with sizes in mesh modifier. The face zone,"
      << " master and slave patch should have the same size"
      << " for object " << name() << ". " << nl
      << "Zone size: "
      << mesh.faceZones()[faceZoneID_.index()].size()
      << " Master patch size: "
      << mesh.boundaryMesh()[masterPatchID_.index()].size()
      << " Slave patch size: "
      << mesh.boundaryMesh()[slavePatchID_.index()].size()
      << abort(FatalError);
    }
    // Check that all the faces belong to either master or slave patch
    if (debug) {
      const labelList& addr = mesh.faceZones()[faceZoneID_.index()];
      DynamicList<label> zoneProblemFaces{addr.size()};
      FOR_ALL(addr, faceI) {
        label facePatch =
          mesh.boundaryMesh().whichPatch(addr[faceI]);
        if (facePatch != masterPatchID_.index()
            && facePatch != slavePatchID_.index()) {
          zoneProblemFaces.append(addr[faceI]);
        }
      }
      if (zoneProblemFaces.size()) {
        FATAL_ERROR_IN
        (
          "void mousse::attachDetach::checkDefinition()"
        )
        << "Found faces in the zone defining "
        << "attach/detach boundary which do not belong to "
        << "either master or slave patch.  "
        << "This is not allowed." << nl
        << "Problem faces: " << zoneProblemFaces
        << abort(FatalError);
      }
    }
  }
  // Check that trigger times are in ascending order
  bool triggersOK = true;
  for (label i = 0; i < triggerTimes_.size() - 1; i++) {
    triggersOK = triggersOK && (triggerTimes_[i] < triggerTimes_[i + 1]);
  }
  if (!triggersOK || (triggerTimes_.empty() && !manualTrigger())) {
    FATAL_ERROR_IN
    (
      "void mousse::attachDetach::checkDefinition()"
    )
    << "Problem with definition of trigger times: "
    << triggerTimes_
    << abort(FatalError);
  }
}


void mousse::attachDetach::clearAddressing() const
{
  deleteDemandDrivenData(pointMatchMapPtr_);
}


// Constructors 

// Construct from components
mousse::attachDetach::attachDetach
(
  const word& name,
  const label index,
  const polyTopoChanger& mme,
  const word& faceZoneName,
  const word& masterPatchName,
  const word& slavePatchName,
  const scalarField& triggerTimes,
  const bool manualTrigger
)
:
  polyMeshModifier{name, index, mme, true},
  faceZoneID_{faceZoneName, mme.mesh().faceZones()},
  masterPatchID_{masterPatchName, mme.mesh().boundaryMesh()},
  slavePatchID_{slavePatchName, mme.mesh().boundaryMesh()},
  triggerTimes_{triggerTimes},
  manualTrigger_{manualTrigger},
  triggerIndex_{0},
  state_{UNKNOWN},
  trigger_{false},
  pointMatchMapPtr_{NULL}
{
  checkDefinition();
}


// Construct from components
mousse::attachDetach::attachDetach
(
  const word& name,
  const dictionary& dict,
  const label index,
  const polyTopoChanger& mme
)
:
  polyMeshModifier{name, index, mme, Switch(dict.lookup("active"))},
  faceZoneID_{dict.lookup("faceZoneName"), mme.mesh().faceZones()},
  masterPatchID_{dict.lookup("masterPatchName"), mme.mesh().boundaryMesh()},
  slavePatchID_{dict.lookup("slavePatchName"), mme.mesh().boundaryMesh()},
  triggerTimes_{dict.lookup("triggerTimes")},
  manualTrigger_{dict.lookup("manualTrigger")},
  triggerIndex_{0},
  state_{UNKNOWN},
  trigger_{false},
  pointMatchMapPtr_{NULL}
{
  checkDefinition();
}


// Destructor 
mousse::attachDetach::~attachDetach()
{
  clearAddressing();
}


// Member Functions 
bool mousse::attachDetach::setAttach() const
{
  if (!attached()) {
    trigger_ = true;
  } else {
    trigger_ = false;
  }
  return trigger_;
}


bool mousse::attachDetach::setDetach() const
{
  if (attached()) {
    trigger_ = true;
  } else {
    trigger_ = false;
  }
  return trigger_;
}


bool mousse::attachDetach::changeTopology() const
{
  if (manualTrigger()) {
    if (debug) {
      Pout << "bool attachDetach::changeTopology() const "
        << " for object " << name() << " : "
        << "Manual trigger" << endl;
    }
    return trigger_;
  }
  // To deal with multiple calls within the same time step, return true
  // if trigger is already set
  if (trigger_) {
    if (debug) {
      Pout << "bool attachDetach::changeTopology() const "
        << " for object " << name() << " : "
        << "Already triggered for current time step" << endl;
    }
    return true;
  }
  // If the end of the list of trigger times has been reached, no
  // new topological changes will happen
  if (triggerIndex_ >= triggerTimes_.size()) {
    if (debug) {
      Pout
        << "bool attachDetach::changeTopology() const "
        << " for object " << name() << " : "
        << "Reached end of trigger list" << endl;
    }
    return false;
  }
  if (debug) {
    Pout << "bool attachDetach::changeTopology() const "
      << " for object " << name() << " : "
      << "Triggering attach/detach topology change." << nl
      << "Current time: " << topoChanger().mesh().time().value()
      << " current trigger time: " << triggerTimes_[triggerIndex_]
      << " trigger index: " << triggerIndex_ << endl;
  }
  // Check if the time is greater than the currentTime.  If so, increment
  // the current lookup and request topology change
  if (topoChanger().mesh().time().value() >= triggerTimes_[triggerIndex_]) {
    trigger_ = true;
    // Increment the trigger index
    triggerIndex_++;
    return true;
  }
  // No topological change
  return false;
}


void mousse::attachDetach::setRefinement(polyTopoChange& ref) const
{
  // Insert the attach/detach instructions into the topological change
  if (trigger_) {
    // Clear point addressing from previous attach/detach event
    clearAddressing();
    if (state_ == ATTACHED) {
      detachInterface(ref);
      // Set the state to detached
      state_ = DETACHED;
    } else if (state_ == DETACHED) {
      attachInterface(ref);
      // Set the state to attached
      state_ = ATTACHED;
    } else {
      FATAL_ERROR_IN
      (
        "void attachDetach::setRefinement(polyTopoChange&) const"
      )
      << "Requested attach/detach event and currect state "
      << "is not known."
      << abort(FatalError);
    }
    trigger_ = false;
  }
}


void mousse::attachDetach::updateMesh(const mapPolyMesh&)
{
  // Mesh has changed topologically.  Update local topological data
  const polyMesh& mesh = topoChanger().mesh();
  faceZoneID_.update(mesh.faceZones());
  masterPatchID_.update(mesh.boundaryMesh());
  slavePatchID_.update(mesh.boundaryMesh());
  clearAddressing();
}


void mousse::attachDetach::write(Ostream& os) const
{
  os << nl << type() << nl
    << name()<< nl
    << faceZoneID_.name() << nl
    << masterPatchID_.name() << nl
    << slavePatchID_.name() << nl
    << triggerTimes_ << endl;
}


void mousse::attachDetach::writeDict(Ostream& os) const
{
  os << nl << name() << nl << token::BEGIN_BLOCK << nl
    << "    type " << type()
    << token::END_STATEMENT << nl
    << "    faceZoneName " << faceZoneID_.name()
    << token::END_STATEMENT << nl
    << "    masterPatchName " << masterPatchID_.name()
    << token::END_STATEMENT << nl
    << "    slavePatchName " << slavePatchID_.name()
    << token::END_STATEMENT << nl
    << "    triggerTimes " << triggerTimes_
    << token::END_STATEMENT << nl
    << "    manualTrigger " << manualTrigger()
    << token::END_STATEMENT << nl
    << "    active " << active()
    << token::END_STATEMENT << nl
    << token::END_BLOCK << endl;
}

