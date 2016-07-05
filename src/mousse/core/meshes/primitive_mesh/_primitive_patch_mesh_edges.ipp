// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "_primitive_patch.hpp"


// Private Member Functions 
template
<
  class Face,
  template<class> class FaceList,
  class PointField,
  class PointType
>
mousse::labelList
mousse::PrimitivePatch<Face, FaceList, PointField, PointType>::
meshEdges
(
  const edgeList& allEdges,
  const labelListList& cellEdges,
  const labelList& faceCells
) const
{
  if (debug) {
    Info << "labelList PrimitivePatch<Face, FaceList, PointField, PointType>"
      << "::meshEdges() : "
      << "calculating labels of patch edges in mesh edge list"
      << endl;
  }
  // get reference to the list of edges on the patch
  const edgeList& PatchEdges = edges();
  const labelListList& EdgeFaces = edgeFaces();
  // create the storage
  labelList meshEdges{PatchEdges.size()};
  bool found = false;
  // get reference to the points on the patch
  const labelList& pp = meshPoints();
  // WARNING: Remember that local edges address into local point list;
  // local-to-global point label translation is necessary
  FOR_ALL(PatchEdges, edgeI) {
    const edge curEdge
    {
      pp[PatchEdges[edgeI].start()],
      pp[PatchEdges[edgeI].end()]
    };
    found = false;
    // get the patch faces sharing the edge
    const labelList& curFaces = EdgeFaces[edgeI];
    FOR_ALL(curFaces, faceI) {
      // get the cell next to the face
      label curCell = faceCells[curFaces[faceI]];
      // get reference to edges on the cell
      const labelList& ce = cellEdges[curCell];
      FOR_ALL(ce, cellEdgeI) {
        if (allEdges[ce[cellEdgeI]] == curEdge) {
          found = true;
          meshEdges[edgeI] = ce[cellEdgeI];
          break;
        }
      }
      if (found)
        break;
    }
  }

  return meshEdges;
}


template
<
  class Face,
  template<class> class FaceList,
  class PointField,
  class PointType
>
mousse::labelList
mousse::PrimitivePatch<Face, FaceList, PointField, PointType>::
meshEdges
(
  const edgeList& allEdges,
  const labelListList& pointEdges
) const
{
  if (debug) {
    Info << "labelList PrimitivePatch<Face, FaceList, PointField, PointType>"
      << "::meshEdges() : "
      << "calculating labels of patch edges in mesh edge list"
      << endl;
  }
  // get reference to the list of edges on the patch
  const edgeList& PatchEdges = edges();
  // create the storage
  labelList meshEdges{PatchEdges.size()};
  // get reference to the points on the patch
  const labelList& pp = meshPoints();
  // WARNING: Remember that local edges address into local point list;
  // local-to-global point label translation is necessary
  FOR_ALL(PatchEdges, edgeI) {
    const label globalPointI = pp[PatchEdges[edgeI].start()];
    const edge curEdge(globalPointI, pp[PatchEdges[edgeI].end()]);
    const labelList& pe = pointEdges[globalPointI];
    FOR_ALL(pe, i) {
      if (allEdges[pe[i]] == curEdge) {
        meshEdges[edgeI] = pe[i];
        break;
      }
    }
  }

  return meshEdges;
}


// Member Functions 
template
<
  class Face,
  template<class> class FaceList,
  class PointField,
  class PointType
>
mousse::label
mousse::PrimitivePatch<Face, FaceList, PointField, PointType>::
whichEdge
(
  const edge& e
) const
{
  // Get pointEdges from the starting point and search all the candidates
  const edgeList& Edges = edges();
  if (e.start() > -1 && e.start() < nPoints()) {
    const labelList& pe = pointEdges()[e.start()];
    FOR_ALL(pe, peI) {
      if (e == Edges[pe[peI]]) {
        return pe[peI];
      }
    }
  }

  // Edge not found.  Return -1
  return -1;
}

