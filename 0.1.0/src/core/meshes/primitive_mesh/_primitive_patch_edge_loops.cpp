// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "_primitive_patch.hpp"

// Member Functions 
template
<
  class Face,
  template<class> class FaceList,
  class PointField,
  class PointType
>
void
mousse::PrimitivePatch<Face, FaceList, PointField, PointType>::
calcEdgeLoops() const
{
  if (debug)
  {
    Info<< "PrimitivePatch<Face, FaceList, PointField, PointType>::"
      << "calcEdgeLoops() : "
      << "calculating boundary edge loops"
      << endl;
  }
  if (edgeLoopsPtr_)
  {
    // it is considered an error to attempt to recalculate
    // if already allocated
    FATAL_ERROR_IN
    (
      "PrimitivePatch<Face, FaceList, PointField, PointType>::"
      "calcIntBdryEdges()"
    )
    << "edge loops already calculated"
    << abort(FatalError);
  }
  const edgeList& patchEdges = edges();
  label nIntEdges = nInternalEdges();
  label nBdryEdges = patchEdges.size() - nIntEdges;
  if (nBdryEdges == 0)
  {
    edgeLoopsPtr_ = new labelListList(0);
    return;
  }
  const labelListList& patchPointEdges = pointEdges();
  //
  // Walk point-edge-point and assign loop number
  //
  // Loop per (boundary) edge.
  labelList loopNumber{nBdryEdges, -1};
  // Size return list plenty big
  edgeLoopsPtr_ = new labelListList{nBdryEdges};
  labelListList& edgeLoops = *edgeLoopsPtr_;
  // Current loop number.
  label loopI = 0;
  while (true)
  {
    // Find edge not yet given a loop number.
    label currentEdgeI = -1;
    for (label edgeI = nIntEdges; edgeI < patchEdges.size(); edgeI++)
    {
      if (loopNumber[edgeI-nIntEdges] == -1)
      {
        currentEdgeI = edgeI;
        break;
      }
    }
    if (currentEdgeI == -1)
    {
      // Did not find edge not yet assigned a loop number so done all.
      break;
    }
    // Temporary storage for vertices of current loop
    DynamicList<label> loop{nBdryEdges};
    // Walk from first all the way round, assigning loops
    label currentVertI = patchEdges[currentEdgeI].start();
    do
    {
      loop.append(currentVertI);
      loopNumber[currentEdgeI - nIntEdges] = loopI;
      // Step to next vertex
      currentVertI = patchEdges[currentEdgeI].otherVertex(currentVertI);
      // Step to next (unmarked, boundary) edge.
      const labelList& curEdges = patchPointEdges[currentVertI];
      currentEdgeI = -1;
      FOR_ALL(curEdges, pI)
      {
        label edgeI = curEdges[pI];
        if (edgeI >= nIntEdges && (loopNumber[edgeI - nIntEdges] == -1))
        {
          // Unassigned boundary edge.
          currentEdgeI = edgeI;
          break;
        }
      }
    }
    while (currentEdgeI != -1);
    // Done all for current loop. Transfer to edgeLoops.
    edgeLoops[loopI].transfer(loop);
    loopI++;
  }
  edgeLoops.setSize(loopI);
  if (debug)
  {
    Info<< "PrimitivePatch<Face, FaceList, PointField, PointType>::"
      << "calcEdgeLoops() : "
      << "finished calculating boundary edge loops"
      << endl;
  }
}


template
<
  class Face,
  template<class> class FaceList,
  class PointField,
  class PointType
>
const mousse::labelListList&
mousse::PrimitivePatch<Face, FaceList, PointField, PointType>::
edgeLoops() const
{
  if (!edgeLoopsPtr_)
  {
    calcEdgeLoops();
  }
  return *edgeLoopsPtr_;
}
