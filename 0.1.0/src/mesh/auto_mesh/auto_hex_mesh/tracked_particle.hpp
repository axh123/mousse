#ifndef MESH_AUTO_MESH_AUTO_HEX_MESH_TRACKED_PARTICLE_TRACKED_PARTICLE_HPP_
#define MESH_AUTO_MESH_AUTO_HEX_MESH_TRACKED_PARTICLE_TRACKED_PARTICLE_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::trackedParticle
// Description
//   Particle class that marks cells it passes through. Used to mark cells
//   visited by feature edges.

#include "particle.hpp"
#include "auto_ptr.hpp"


namespace mousse {

class trackedParticleCloud;


class trackedParticle
:
  public particle
{
  // Private data
    //- End point to track to
    point end_;
    //- Level of this particle
    label level_;
    //- Passive label (used to store feature edge mesh)
    label i_;
    //- Passive label (used to store feature edge point)
    label j_;
    //- Passive label (used to store feature edge label)
    label k_;
public:
  friend class Cloud<trackedParticle>;
  //- Class used to pass tracking data to the trackToFace function
  class trackingData
  :
    public particle::TrackingData<Cloud<trackedParticle>>
  {
  public:
    labelList& maxLevel_;
    List<PackedBoolList>& featureEdgeVisited_;
    // Constructors
      trackingData
      (
        Cloud<trackedParticle>& cloud,
        labelList& maxLevel,
        List<PackedBoolList>& featureEdgeVisited
      )
      :
        particle::TrackingData<Cloud<trackedParticle>>{cloud},
        maxLevel_{maxLevel},
        featureEdgeVisited_{featureEdgeVisited}
      {}
  };
  // Constructors
    //- Construct from components
    trackedParticle
    (
      const polyMesh& mesh,
      const vector& position,
      const label cellI,
      const label tetFaceI,
      const label tetPtI,
      const point& end,
      const label level,
      const label i,
      const label j,
      const label k
    );
    //- Construct from Istream
    trackedParticle
    (
      const polyMesh& mesh,
      Istream& is,
      bool readFields = true
    );
    //- Construct and return a clone
    autoPtr<particle> clone() const
    {
      return autoPtr<particle>(new trackedParticle(*this));
    }
    //- Factory class to read-construct particles used for
    //  parallel transfer
    class iNew
    {
      const polyMesh& mesh_;
    public:
      iNew(const polyMesh& mesh)
      :
        mesh_{mesh}
      {}
      autoPtr<trackedParticle> operator()(Istream& is) const
      {
        return
          autoPtr<trackedParticle>
          {
            new trackedParticle{mesh_, is, true}
          };
      }
    };
  // Member Functions
    //- Point to track to
    point& end() { return end_; }
    //- Transported label
    label i() const { return i_; }
    //- Transported label
    label& i() { return i_; }
    //- Transported label
    label j() const { return j_; }
    //- Transported label
    label& j() { return j_; }
    //- Transported label
    label k() const { return k_; }
    //- Transported label
    label& k() { return k_; }
    // Tracking
      //- Track all particles to their end point
      bool move(trackingData&, const scalar);
      //- Overridable function to handle the particle hitting a patch
      //  Executed before other patch-hitting functions
      bool hitPatch
      (
        const polyPatch&,
        trackingData& td,
        const label patchI,
        const scalar trackFraction,
        const tetIndices& tetIs
      );
      //- Overridable function to handle the particle hitting a wedge
      void hitWedgePatch
      (
        const wedgePolyPatch&,
        trackingData& td
      );
      //- Overridable function to handle the particle hitting a
      //  symmetry plane
      void hitSymmetryPlanePatch
      (
        const symmetryPlanePolyPatch&,
        trackingData& td
      );
      //- Overridable function to handle the particle hitting a
      //  symmetry patch
      void hitSymmetryPatch
      (
        const symmetryPolyPatch&,
        trackingData& td
      );
      //- Overridable function to handle the particle hitting a cyclic
      void hitCyclicPatch
      (
        const cyclicPolyPatch&,
        trackingData& td
      );
      //- Overridable function to handle the particle hitting a
      //- processorPatch
      void hitProcessorPatch
      (
        const processorPolyPatch&,
        trackingData& td
      );
      //- Overridable function to handle the particle hitting a wallPatch
      void hitWallPatch
      (
        const wallPolyPatch&,
        trackingData& td,
        const tetIndices&
      );
      //- Overridable function to handle the particle hitting a polyPatch
      void hitPatch
      (
        const polyPatch&,
        trackingData& td
      );
      //- Convert processor patch addressing to the global equivalents
      //  and set the cellI to the face-neighbour
      void correctAfterParallelTransfer(const label, trackingData&);
  // Ostream Operator
    friend Ostream& operator<<(Ostream&, const trackedParticle&);
};

template<> inline bool contiguous<trackedParticle>() { return true; }

}  // namespace mousse

#endif

