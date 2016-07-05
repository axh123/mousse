#ifndef DYNAMIC_MESH_MESH_CUT_MESH_MODIFIERS_MESH_CUTTER_MESH_CUTTER_HPP_
#define DYNAMIC_MESH_MESH_CUT_MESH_MODIFIERS_MESH_CUTTER_MESH_CUTTER_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::meshCutter
// Description
//   Cuts (splits) cells.
//   Description of cut is given as a loop of 'cuts' per cell (see cellCuts).
//   setRefinement() takes this cut description and inserts the necessary
//   topoActions (add points/faces/cells) into the polyTopoChange.
//   Stores added cells/faces/points.
//   Cut description gives orientation to cut by calculating 'anchorPoints'.
//   The side of the cell that contains the anchorPoints is the master cell.
//   Likewise the cells' edges will have the split added as a duplicate of the
//   master (anchor) point.
//   Think of it as the cell with the anchor points at the bottom. Add a face
//   at the bottom to split the cell and then sweep this face up to be through
//   the middle of the cell (inflation).
//   -# Start:
//    cell with anchor points at bottom
//   \verbatim
//   +-------+
//   |       +
//   |       +
//   |       +
//   |       +
//   |       +
//   |       +
//   |       +
//   +-------+
//   anchor  anchor
//   \endverbatim
//   -# Topo change:
//    splitface introduced at bottom of cell, introducing a new
//    cell and splitting the side faces into two.
//   \verbatim
//   +-------+
//   |       +
//   |       +
//   |       + <- addedCell
//   |       +
//   |       +
//   |       +
//   +-------+ <- splitFace
//   +-------+ <- original cell
//   anchor  anchor
//   \endverbatim
//   -# Inflation:
//    splitface shifted up to middle of cell (or wherever cut was)
//   \verbatim
//   +-------+
//   |       +
//   |       + <- addedCell
//   |       +
//   +-------+ <- splitFace
//   |       +
//   |       + <- original cell
//   |       +
//   +-------+
//   anchor  anchor
//   \endverbatim
//   Anyway this was the original idea. Inflation was meant to handle
//   conservative properties distribution without interpolation.
//   (just face sweeping through space). But problem was that
//   only if the introduced splitface was exactly the same shape as bottom face
//   (so same 2D topo or perfectly flat) the volume between them was 0.
//   This meshCutting still uses anchorPoints though:
//   - the master cell is the one without the anchor points. The added cell
//    (on top of the splitFace) is the with.
//   - the splitFace is owned by the master cell (since it has the lower number)
//   - the side faces get split and get either the original cell as neighbour
//    or the added cell (if the faces contain the cell anchor points)

#include "edge_vertex.hpp"
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


class meshCutter
:
  public edgeVertex
{
  // Private data
    //- Cells added in last setRefinement. Per splitcell label of added
    //  cell
    Map<label> addedCells_;
    //- Faces added in last setRefinement. Per split cell label of added
    //  face
    Map<label> addedFaces_;
    //- Points added in last setRefinement. Per split edge label of added
    //  point
    HashTable<label, edge, Hash<edge> > addedPoints_;
  // Private Static Functions
    //- Do list 1 and 2 share elements?
    static bool uses(const labelList& elems1, const labelList& elems2);
    //- Do the elements of edge appear in consecutive order in the list
    static bool isIn(const edge&, const labelList&);
  // Private Member Functions
    //- Returns -1 or the cell in cellLabels that is cut.
    label findCutCell(const cellCuts&, const labelList&) const;
    //- Returns first pointI in pointLabels that uses an internal
    //  face. Used to find point to inflate cell/face from (has to be
    //  connected to internal face)
    label findInternalFacePoint(const labelList& pointLabels) const;
    //- Get new owner and neighbour of face. Checks anchor points to see if
    // need to get original or added cell.
    void faceCells
    (
      const cellCuts& cuts,
      const label faceI,
      label& own,
      label& nei
    ) const;
    //- Get patch information for face.
    void getFaceInfo
    (
      const label faceI,
      label& patchID,
      label& zoneID,
      bool& zoneFlip
    ) const;
    //- Adds a face on top of existing faceI. Flips face
    //  if owner>neighbour
    void addFace
    (
      polyTopoChange& meshMod,
      const label faceI,
      const face& newFace,
      const label owner,
      const label neighbour
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
      const label neighbour
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
    //- Get elements of cell.
    void getFacesEdgesPoints
    (
      const label cellI,
      labelHashSet& faces,
      labelHashSet& edges,
      labelHashSet& points
    ) const;
public:
  //- Runtime type information
  CLASS_NAME("meshCutter");
  // Constructors
    //- Construct from mesh
    meshCutter(const polyMesh& mesh);
    //- Disallow default bitwise copy construct
    meshCutter(const meshCutter&) = delete;
    //- Disallow default bitwise assignment
    meshCutter& operator=(const meshCutter&) = delete;
  //- Destructor
  ~meshCutter();
  // Member Functions
    // Edit
      //- Do actual cutting with cut description. Inserts mesh changes
      //  into meshMod.
      void setRefinement(const cellCuts& cuts, polyTopoChange& meshMod);
      //- Force recalculation of locally stored data on topological change
      void updateMesh(const mapPolyMesh&);
    // Access
      //- Cells added. Per split cell label of added cell
      const Map<label>& addedCells() const
      {
        return addedCells_;
      }
      //- Faces added. Per split cell label of added face
      const Map<label>& addedFaces() const
      {
        return addedFaces_;
      }
      //- Points added. Per split edge label of added point
      const HashTable<label, edge, Hash<edge> >& addedPoints() const
      {
        return addedPoints_;
      }
};

}  // namespace mousse

#endif
