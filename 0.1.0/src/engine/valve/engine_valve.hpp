#ifndef ENGINE_ENGINE_VALVE_ENGINE_VALVE_HPP_
#define ENGINE_ENGINE_VALVE_ENGINE_VALVE_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::engineValve
// Description
//   mousse::engineValve

#include "word.hpp"
#include "coordinate_system.hpp"
#include "poly_patch_id.hpp"
#include "graph.hpp"


namespace mousse {

// Forward declaration of classes
class polyMesh;
class engineTime;


class engineValve
{
  // Private data
    //- Name of valve
    word name_;
    //- Reference to engine mesh
    const polyMesh& mesh_;
    //- Reference to engine database
    const engineTime& engineDB_;
    //- Coordinate system
    autoPtr<coordinateSystem> csPtr_;
    // Patch and zone names
      //- Valve bottom patch
      polyPatchID bottomPatch_;
      //- Valve poppet patch
      polyPatchID poppetPatch_;
      //- Valve stem patch
      polyPatchID stemPatch_;
      //- Valve curtain manifold patch
      polyPatchID curtainInPortPatch_;
      //- Valve curtain cylinder patch
      polyPatchID curtainInCylinderPatch_;
      //- Valve detach in cylinder patch
      polyPatchID detachInCylinderPatch_;
      //- Valve detach in port patch
      polyPatchID detachInPortPatch_;
      //- Faces to detach
      labelList detachFaces_;
    // Valve lift data
      //- Valve lift profile
      graph liftProfile_;
      //- Lift curve start angle
      scalar liftProfileStart_;
      //- Lift curve end angle
      scalar liftProfileEnd_;
      //- Minimum valve lift.  On this lift the valve is considered closed
      const scalar minLift_;
      // Valve layering data
        //- Min top layer thickness
        const scalar minTopLayer_;
        //- Max top layer thickness
        const scalar maxTopLayer_;
        //- Min bottom layer thickness
        const scalar minBottomLayer_;
        //- Max bottom layer thickness
        const scalar maxBottomLayer_;
      //- Valve diameter
      const scalar diameter_;
  // Private Member Functions
    //- Adjust crank angle to drop within the limits of the lift profile
    scalar adjustCrankAngle(const scalar theta) const;
public:
  // Static data members
  // Constructors
    //- Construct from components
    engineValve
    (
      const word& name,
      const polyMesh& mesh,
      const autoPtr<coordinateSystem>& valveCS,
      const word& bottomPatchName,
      const word& poppetPatchName,
      const word& stemPatchName,
      const word& curtainInPortPatchName,
      const word& curtainInCylinderPatchName,
      const word& detachInCylinderPatchName,
      const word& detachInPortPatchName,
      const labelList& detachFaces,
      const graph& liftProfile,
      const scalar minLift,
      const scalar minTopLayer,
      const scalar maxTopLayer,
      const scalar minBottomLayer,
      const scalar maxBottomLayer,
      const scalar diameter
    );
    //- Construct from dictionary
    engineValve
    (
      const word& name,
      const polyMesh& mesh,
      const dictionary& dict
    );
    //- Disallow default bitwise copy construct
    engineValve(const engineValve&) = delete;
    //- Disallow default bitwise assignment
    engineValve& operator=(const engineValve&) = delete;
  // Destructor - default
  // Member Functions
    //- Return name
    const word& name() const { return name_; }
    //- Return coordinate system
    const coordinateSystem& cs() const { return csPtr_(); }
    //- Return lift profile
    const graph& liftProfile() const { return liftProfile_; }
    //- Return valve diameter
    scalar diameter() const { return diameter_; }
    // Valve patches
      //- Return ID of bottom patch
      const polyPatchID& bottomPatchID() const { return bottomPatch_; }
      //- Return ID of poppet patch
      const polyPatchID& poppetPatchID() const { return poppetPatch_; }
      //- Return ID of stem patch
      const polyPatchID& stemPatchID() const { return stemPatch_; }
      //- Return ID of curtain in cylinder patch
      const polyPatchID& curtainInCylinderPatchID() const
      {
        return curtainInCylinderPatch_;
      }
      //- Return ID of curtain in port patch
      const polyPatchID& curtainInPortPatchID() const
      {
        return curtainInPortPatch_;
      }
      //- Return ID of detach in cylinder patch
      const polyPatchID& detachInCylinderPatchID() const
      {
        return detachInCylinderPatch_;
      }
      //- Return ID of detach in port patch
      const polyPatchID& detachInPortPatchID() const
      {
        return detachInPortPatch_;
      }
      //- Return face labels of detach curtain
      const labelList& detachFaces() const { return detachFaces_; }
    // Valve layering thickness
      scalar minTopLayer() const { return minTopLayer_; }
      scalar maxTopLayer() const { return maxTopLayer_; }
      scalar minBottomLayer() const { return minBottomLayer_; }
      scalar maxBottomLayer() const { return maxBottomLayer_; }
    // Valve position and velocity
      //- Return valve lift given crank angle in degrees
      scalar lift(const scalar theta) const;
      //- Is the valve open?
      bool isOpen() const;
      //- Return current lift
      scalar curLift() const;
      //- Return valve velocity for current time-step
      scalar curVelocity() const;
      //- Return list of active patch labels for the valve head
      //  (stem is excluded)
      labelList movingPatchIDs() const;
    //- Write dictionary
    void writeDict(Ostream&) const;
};

}  // namespace mousse

#endif

