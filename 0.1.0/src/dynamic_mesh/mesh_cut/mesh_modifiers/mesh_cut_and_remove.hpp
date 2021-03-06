#ifndef DYNAMIC_MESH_MESH_CUT_MESH_MODIFIERS_MESH_CUT_AND_REMOVE_MESH_CUT_AND_REMOVE_HPP_
#define DYNAMIC_MESH_MESH_CUT_MESH_MODIFIERS_MESH_CUT_AND_REMOVE_MESH_CUT_AND_REMOVE_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::meshCutAndRemove
// Description
//   like meshCutter but also removes non-anchor side of cell.

#include "edge_vertex.hpp"
#include "bool_list.hpp"
#include "label_list.hpp"
#include "type_info.hpp"
#include "map.hpp"


namespace mousse {

// Forward declaration of classes
class Time;
class polyTopoChange;
class cellCuts;
class polyMesh;
class face;
class mapPolyMesh;


class meshCutAndRemove
:
  public edgeVertex
{
  // Private data
    //- Faces added in last setRefinement. Per split cell label of added
    //  face
    Map<label> addedFaces_;
    //- Points added in last setRefinement. Per split edge label of added
    //  point
    HashTable<label, edge, Hash<edge> > addedPoints_;
  // Private Static Functions
    // Returns -1 or index in elems1 of first shared element.
    static label firstCommon(const labelList& lst1, const labelList& lst2);
    //- Do the elements of edge appear in consecutive order in the list
    static bool isIn(const edge&, const labelList&);
  // Private Member Functions
    //- Returns -1 or the cell in cellLabels that is cut.
    label findCutCell(const cellCuts&, const labelList&) const;
    //- Returns first pointI in pointLabels that uses an internal
    //  face. Used to find point to inflate cell/face from (has to be
    //  connected to internal face)
    label findInternalFacePoint(const labelList& pointLabels) const;
    //- Find point on face that is part of original mesh and that is
    //  point connected to the patch
    label findPatchFacePoint(const face& f, const label patchI) const;
    //- Get new owner and neighbour of face. Checks anchor points to see if
    // need to get original or added cell.
    void faceCells
    (
      const cellCuts& cuts,
      const label exposedPatchI,
      const label faceI,
      label& own,
      label& nei,
      label& patchID
    ) const;
    //- Get zone information for face.
    void getZoneInfo
    (
      const label faceI,
      label& zoneID,
      bool& zoneFlip
    ) const;
    //- Adds a face from point. Flips face if owner>neighbour
    void addFace
    (
      polyTopoChange& meshMod,
      const label faceI,
      const label masterPointI,
      const face& newFace,
      const label owner,
      const label neighbour,
      const label patchID
    );
    //- Modifies existing faceI for either new owner/neighbour or
    //  new face points. Checks if anything changed and flips face
    //  if owner>neighbour
    void modFace
    (
      polyTopoChange& meshMod,
      const label faceI,
      const face& newFace,
      const label owner,
      const label neighbour,
      const label patchID
    );
    // Copies face starting from startFp. Jumps cuts. Marks visited
    // vertices in visited.
    void copyFace
    (
      const face& f,
      const label startFp,
      const label endFp,
      face& newFace
    ) const;
    //- Split face along cut into two faces. Faces are in same point
    //  order as original face (i.e. maintain normal direction)
    void splitFace
    (
      const face& f,
      const label v0,
      const label v1,
      face& f0,
      face& f1
    ) const;
    //- Add cuts of edges to face
    face addEdgeCutsToFace(const label faceI) const;
    //- Convert loop of cuts into face.
    face loopToFace
    (
      const label cellI,
      const labelList& loop
    ) const;
public:
  //- Runtime type information
  CLASS_NAME("meshCutAndRemove");
  // Constructors
    //- Construct from mesh
    meshCutAndRemove(const polyMesh& mesh);
    //- Disallow default bitwise copy construct
    meshCutAndRemove(const meshCutAndRemove&) = delete;
    //- Disallow default bitwise assignment
    meshCutAndRemove& operator=(const meshCutAndRemove&) = delete;
  // Member Functions
    // Edit
      //- Do actual cutting with cut description. Inserts mesh changes
      //  into meshMod.
      //  cuts: all loops and topological information
      //  cutPatch: for every cell that has loop the patch number
      //  exposedPatch: patch for other exposed faces
      void setRefinement
      (
        const label exposedPatchI,
        const cellCuts& cuts,
        const labelList& cutPatch,
        polyTopoChange& meshMod
      );
      //- Force recalculation of locally stored data on topological change
      void updateMesh(const mapPolyMesh&);
    // Access
      //- Faces added. Per split cell label of added face
      const Map<label>& addedFaces() const
      {
        return addedFaces_;
      }
      //- Points added. Per split edge label of added point.
      //  (note: fairly useless across topology changes since one of the
      //  points of the edge will probably disappear)
      const HashTable<label, edge, Hash<edge> >& addedPoints() const
      {
        return addedPoints_;
      }
};

}  // namespace mousse

#endif

