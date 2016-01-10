// mousse: CFD toolbox
// Copyright (C) 2011-2013 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "sync_tools.hpp"
#include "poly_mesh.hpp"
#include "processor_poly_patch.hpp"
#include "cyclic_poly_patch.hpp"
#include "global_mesh_data.hpp"
#include "contiguous.hpp"
#include "transform.hpp"
#include "transform_list.hpp"
#include "sub_field.hpp"

// Private Member Functions 
// Combine val with existing value at index
template<class T, class CombineOp>
void mousse::syncTools::combine
(
  Map<T>& pointValues,
  const CombineOp& cop,
  const label index,
  const T& val
)
{
  typename Map<T>::iterator iter = pointValues.find(index);
  if (iter != pointValues.end())
  {
    cop(iter(), val);
  }
  else
  {
    pointValues.insert(index, val);
  }
}


// Combine val with existing value at (implicit index) e.
template<class T, class CombineOp>
void mousse::syncTools::combine
(
  EdgeMap<T>& edgeValues,
  const CombineOp& cop,
  const edge& index,
  const T& val
)
{
  typename EdgeMap<T>::iterator iter = edgeValues.find(index);
  if (iter != edgeValues.end())
  {
    cop(iter(), val);
  }
  else
  {
    edgeValues.insert(index, val);
  }
}


template<class T, class CombineOp, class TransformOp>
void mousse::syncTools::syncPointMap
(
  const polyMesh& mesh,
  Map<T>& pointValues,        // from mesh point label to value
  const CombineOp& cop,
  const TransformOp& top
)
{
  const polyBoundaryMesh& patches = mesh.boundaryMesh();
  // Synchronize multiple shared points.
  const globalMeshData& pd = mesh.globalData();
  // Values on shared points. Keyed on global shared index.
  Map<T> sharedPointValues{0};
  if (pd.nGlobalPoints() > 0)
  {
    // meshPoint per local index
    const labelList& sharedPtLabels = pd.sharedPointLabels();
    // global shared index per local index
    const labelList& sharedPtAddr = pd.sharedPointAddr();
    sharedPointValues.resize(sharedPtAddr.size());
    // Fill my entries in the shared points
    FOR_ALL(sharedPtLabels, i)
    {
      label meshPointI = sharedPtLabels[i];
      typename Map<T>::const_iterator fnd =
        pointValues.find(meshPointI);
      if (fnd != pointValues.end())
      {
        combine
        (
          sharedPointValues,
          cop,
          sharedPtAddr[i],    // index
          fnd()               // value
        );
      }
    }
  }
  if (Pstream::parRun())
  {
    PstreamBuffers pBufs(Pstream::nonBlocking);
    // Send
    FOR_ALL(patches, patchI)
    {
      if(isA<processorPolyPatch>(patches[patchI])
         && patches[patchI].nPoints() > 0)
      {
        const processorPolyPatch& procPatch =
          refCast<const processorPolyPatch>(patches[patchI]);
        // Get data per patchPoint in neighbouring point numbers.
        const labelList& meshPts = procPatch.meshPoints();
        const labelList& nbrPts = procPatch.neighbPoints();
        // Extract local values. Create map from nbrPoint to value.
        // Note: how small initial size?
        Map<T> patchInfo{meshPts.size()/20};
        FOR_ALL(meshPts, i)
        {
          typename Map<T>::const_iterator iter =
            pointValues.find(meshPts[i]);
          if (iter != pointValues.end())
          {
            patchInfo.insert(nbrPts[i], iter());
          }
        }
        UOPstream toNeighb{procPatch.neighbProcNo(), pBufs};
        toNeighb << patchInfo;
      }
    }
    pBufs.finishedSends();
    // Receive and combine.
    FOR_ALL(patches, patchI)
    {
      if(isA<processorPolyPatch>(patches[patchI])
         && patches[patchI].nPoints() > 0)
      {
        const processorPolyPatch& procPatch =
          refCast<const processorPolyPatch>(patches[patchI]);
        UIPstream fromNb{procPatch.neighbProcNo(), pBufs};
        Map<T> nbrPatchInfo{fromNb};
        // Transform
        top(procPatch, nbrPatchInfo);
        const labelList& meshPts = procPatch.meshPoints();
        // Only update those values which come from neighbour
        FOR_ALL_CONST_ITER(typename Map<T>, nbrPatchInfo, nbrIter)
        {
          combine
          (
            pointValues,
            cop,
            meshPts[nbrIter.key()],
            nbrIter()
          );
        }
      }
    }
  }
  // Do the cyclics.
  FOR_ALL(patches, patchI)
  {
    if (isA<cyclicPolyPatch>(patches[patchI]))
    {
      const cyclicPolyPatch& cycPatch =
        refCast<const cyclicPolyPatch>(patches[patchI]);
      if (cycPatch.owner())
      {
        // Owner does all.
        const cyclicPolyPatch& nbrPatch = cycPatch.neighbPatch();
        const edgeList& coupledPoints = cycPatch.coupledPoints();
        const labelList& meshPtsA = cycPatch.meshPoints();
        const labelList& meshPtsB = nbrPatch.meshPoints();
        // Extract local values. Create map from coupled-edge to value.
        Map<T> half0Values{meshPtsA.size()/20};
        Map<T> half1Values{half0Values.size()};
        FOR_ALL(coupledPoints, i)
        {
          const edge& e = coupledPoints[i];
          typename Map<T>::const_iterator point0Fnd =
            pointValues.find(meshPtsA[e[0]]);
          if (point0Fnd != pointValues.end())
          {
            half0Values.insert(i, point0Fnd());
          }
          typename Map<T>::const_iterator point1Fnd =
            pointValues.find(meshPtsB[e[1]]);
          if (point1Fnd != pointValues.end())
          {
            half1Values.insert(i, point1Fnd());
          }
        }
        // Transform to receiving side
        top(cycPatch, half1Values);
        top(nbrPatch, half0Values);
        FOR_ALL(coupledPoints, i)
        {
          const edge& e = coupledPoints[i];
          typename Map<T>::const_iterator half0Fnd =
            half0Values.find(i);
          if (half0Fnd != half0Values.end())
          {
            combine
            (
              pointValues,
              cop,
              meshPtsB[e[1]],
              half0Fnd()
            );
          }
          typename Map<T>::const_iterator half1Fnd =
            half1Values.find(i);
          if (half1Fnd != half1Values.end())
          {
            combine
            (
              pointValues,
              cop,
              meshPtsA[e[0]],
              half1Fnd()
            );
          }
        }
      }
    }
  }
  // Synchronize multiple shared points.
  if (pd.nGlobalPoints() > 0)
  {
    // meshPoint per local index
    const labelList& sharedPtLabels = pd.sharedPointLabels();
    // global shared index per local index
    const labelList& sharedPtAddr = pd.sharedPointAddr();
    // Reduce on master.
    if (Pstream::parRun())
    {
      if (Pstream::master())
      {
        // Receive the edges using shared points from the slave.
        for
        (
          int slave=Pstream::firstSlave();
          slave<=Pstream::lastSlave();
          slave++
        )
        {
          IPstream fromSlave{Pstream::scheduled, slave};
          Map<T> nbrValues{fromSlave};
          // Merge neighbouring values with my values
          FOR_ALL_CONST_ITER(typename Map<T>, nbrValues, iter)
          {
            combine
            (
              sharedPointValues,
              cop,
              iter.key(), // edge
              iter()      // value
            );
          }
        }
        // Send back
        for
        (
          int slave=Pstream::firstSlave();
          slave<=Pstream::lastSlave();
          slave++
        )
        {
          OPstream toSlave{Pstream::scheduled, slave};
          toSlave << sharedPointValues;
        }
      }
      else
      {
        // Slave: send to master
        {
          OPstream toMaster{Pstream::scheduled, Pstream::masterNo()};
          toMaster << sharedPointValues;
        }
        // Receive merged values
        {
          IPstream fromMaster{Pstream::scheduled, Pstream::masterNo()};
          fromMaster >> sharedPointValues;
        }
      }
    }
    // Merge sharedPointValues (keyed on sharedPointAddr) into
    // pointValues (keyed on mesh points).
    // Map from global shared index to meshpoint
    Map<label> sharedToMeshPoint{2*sharedPtAddr.size()};
    FOR_ALL(sharedPtAddr, i)
    {
      sharedToMeshPoint.insert(sharedPtAddr[i], sharedPtLabels[i]);
    }
    FOR_ALL_CONST_ITER(Map<label>, sharedToMeshPoint, iter)
    {
      // Do I have a value for my shared point
      typename Map<T>::const_iterator sharedFnd =
        sharedPointValues.find(iter.key());
      if (sharedFnd != sharedPointValues.end())
      {
        pointValues.set(iter(), sharedFnd());
      }
    }
  }
}


template<class T, class CombineOp, class TransformOp>
void mousse::syncTools::syncEdgeMap
(
  const polyMesh& mesh,
  EdgeMap<T>& edgeValues,
  const CombineOp& cop,
  const TransformOp& top
)
{
  const polyBoundaryMesh& patches = mesh.boundaryMesh();
  // Do synchronisation without constructing globalEdge addressing
  // (since this constructs mesh edge addressing)
  // Swap proc patch info
  // ~~~~~~~~~~~~~~~~~~~~
  if (Pstream::parRun())
  {
    PstreamBuffers pBufs{Pstream::nonBlocking};
    // Send
    FOR_ALL(patches, patchI)
    {
      if(isA<processorPolyPatch>(patches[patchI])
         && patches[patchI].nEdges() > 0)
      {
        const processorPolyPatch& procPatch =
          refCast<const processorPolyPatch>(patches[patchI]);
        // Get data per patch edge in neighbouring edge.
        const edgeList& edges = procPatch.edges();
        const labelList& meshPts = procPatch.meshPoints();
        const labelList& nbrPts = procPatch.neighbPoints();
        EdgeMap<T> patchInfo{edges.size()/20};
        FOR_ALL(edges, i)
        {
          const edge& e = edges[i];
          const edge meshEdge{meshPts[e[0]], meshPts[e[1]]};
          typename EdgeMap<T>::const_iterator iter =
            edgeValues.find(meshEdge);
          if (iter != edgeValues.end())
          {
            const edge nbrEdge{nbrPts[e[0]], nbrPts[e[1]]};
            patchInfo.insert(nbrEdge, iter());
          }
        }
        UOPstream toNeighb{procPatch.neighbProcNo(), pBufs};
        toNeighb << patchInfo;
      }
    }
    pBufs.finishedSends();
    // Receive and combine.
    FOR_ALL(patches, patchI)
    {
      if(isA<processorPolyPatch>(patches[patchI])
         && patches[patchI].nEdges() > 0)
      {
        const processorPolyPatch& procPatch =
          refCast<const processorPolyPatch>(patches[patchI]);
        EdgeMap<T> nbrPatchInfo;
        {
          UIPstream fromNbr{procPatch.neighbProcNo(), pBufs};
          fromNbr >> nbrPatchInfo;
        }
        // Apply transform to convert to this side properties.
        top(procPatch, nbrPatchInfo);
        // Only update those values which come from neighbour
        const labelList& meshPts = procPatch.meshPoints();
        FOR_ALL_CONST_ITER(typename EdgeMap<T>, nbrPatchInfo, nbrIter)
        {
          const edge& e = nbrIter.key();
          const edge meshEdge{meshPts[e[0]], meshPts[e[1]]};
          combine
          (
            edgeValues,
            cop,
            meshEdge,   // edge
            nbrIter()   // value
          );
        }
      }
    }
  }
  // Swap cyclic info
  // ~~~~~~~~~~~~~~~~
  FOR_ALL(patches, patchI)
  {
    if (isA<cyclicPolyPatch>(patches[patchI]))
    {
      const cyclicPolyPatch& cycPatch =
        refCast<const cyclicPolyPatch>(patches[patchI]);
      if (cycPatch.owner())
      {
        // Owner does all.
        const edgeList& coupledEdges = cycPatch.coupledEdges();
        const labelList& meshPtsA = cycPatch.meshPoints();
        const edgeList& edgesA = cycPatch.edges();
        const cyclicPolyPatch& nbrPatch = cycPatch.neighbPatch();
        const labelList& meshPtsB = nbrPatch.meshPoints();
        const edgeList& edgesB = nbrPatch.edges();
        // Extract local values. Create map from edge to value.
        Map<T> half0Values{edgesA.size()/20};
        Map<T> half1Values{half0Values.size()};
        FOR_ALL(coupledEdges, i)
        {
          const edge& twoEdges = coupledEdges[i];
          {
            const edge& e0 = edgesA[twoEdges[0]];
            const edge meshEdge0{meshPtsA[e0[0]], meshPtsA[e0[1]]};
            typename EdgeMap<T>::const_iterator iter =
              edgeValues.find(meshEdge0);
            if (iter != edgeValues.end())
            {
              half0Values.insert(i, iter());
            }
          }
          {
            const edge& e1 = edgesB[twoEdges[1]];
            const edge meshEdge1{meshPtsB[e1[0]], meshPtsB[e1[1]]};
            typename EdgeMap<T>::const_iterator iter =
              edgeValues.find(meshEdge1);
            if (iter != edgeValues.end())
            {
              half1Values.insert(i, iter());
            }
          }
        }
        // Transform to this side
        top(cycPatch, half1Values);
        top(nbrPatch, half0Values);
        // Extract and combine information
        FOR_ALL(coupledEdges, i)
        {
          const edge& twoEdges = coupledEdges[i];
          typename Map<T>::const_iterator half1Fnd =
            half1Values.find(i);
          if (half1Fnd != half1Values.end())
          {
            const edge& e0 = edgesA[twoEdges[0]];
            const edge meshEdge0{meshPtsA[e0[0]], meshPtsA[e0[1]]};
            combine
            (
              edgeValues,
              cop,
              meshEdge0,  // edge
              half1Fnd()  // value
            );
          }
          typename Map<T>::const_iterator half0Fnd =
            half0Values.find(i);
          if (half0Fnd != half0Values.end())
          {
            const edge& e1 = edgesB[twoEdges[1]];
            const edge meshEdge1{meshPtsB[e1[0]], meshPtsB[e1[1]]};
            combine
            (
              edgeValues,
              cop,
              meshEdge1,  // edge
              half0Fnd()  // value
            );
          }
        }
      }
    }
  }
  // Synchronize multiple shared points.
  // Problem is that we don't want to construct shared edges so basically
  // we do here like globalMeshData but then using sparse edge representation
  // (EdgeMap instead of mesh.edges())
  const globalMeshData& pd = mesh.globalData();
  const labelList& sharedPtAddr = pd.sharedPointAddr();
  const labelList& sharedPtLabels = pd.sharedPointLabels();
  // 1. Create map from meshPoint to globalShared index.
  Map<label> meshToShared{2*sharedPtLabels.size()};
  FOR_ALL(sharedPtLabels, i)
  {
    meshToShared.insert(sharedPtLabels[i], sharedPtAddr[i]);
  }
  // Values on shared points. From two sharedPtAddr indices to a value.
  EdgeMap<T> sharedEdgeValues{meshToShared.size()};
  // From shared edge to mesh edge. Used for merging later on.
  EdgeMap<edge> potentialSharedEdge{meshToShared.size()};
  // 2. Find any edges using two global shared points. These will always be
  // on the outside of the mesh. (though might not be on coupled patch
  // if is single edge and not on coupled face)
  // Store value (if any) on sharedEdgeValues
  for (label faceI = mesh.nInternalFaces(); faceI < mesh.nFaces(); faceI++)
  {
    const face& f = mesh.faces()[faceI];
    FOR_ALL(f, fp)
    {
      label v0 = f[fp];
      label v1 = f[f.fcIndex(fp)];
      Map<label>::const_iterator v0Fnd = meshToShared.find(v0);
      if (v0Fnd != meshToShared.end())
      {
        Map<label>::const_iterator v1Fnd = meshToShared.find(v1);
        if (v1Fnd != meshToShared.end())
        {
          const edge meshEdge{v0, v1};
          // edge in shared point labels
          const edge sharedEdge{v0Fnd(), v1Fnd()};
          // Store mesh edge as a potential shared edge.
          potentialSharedEdge.insert(sharedEdge, meshEdge);
          typename EdgeMap<T>::const_iterator edgeFnd =
            edgeValues.find(meshEdge);
          if (edgeFnd != edgeValues.end())
          {
            // edge exists in edgeValues. See if already in map
            // (since on same processor, e.g. cyclic)
            combine
            (
              sharedEdgeValues,
              cop,
              sharedEdge, // edge
              edgeFnd()   // value
            );
          }
        }
      }
    }
  }
  // Now sharedEdgeValues will contain per potential sharedEdge the value.
  // (potential since an edge having two shared points is not necessary a
  //  shared edge).
  // Reduce this on the master.
  if (Pstream::parRun())
  {
    if (Pstream::master())
    {
      // Receive the edges using shared points from the slave.
      for
      (
        int slave=Pstream::firstSlave();
        slave<=Pstream::lastSlave();
        slave++
      )
      {
        IPstream fromSlave{Pstream::scheduled, slave};
        EdgeMap<T> nbrValues{fromSlave};
        // Merge neighbouring values with my values
        FOR_ALL_CONST_ITER(typename EdgeMap<T>, nbrValues, iter)
        {
          combine
          (
            sharedEdgeValues,
            cop,
            iter.key(), // edge
            iter()      // value
          );
        }
      }
      // Send back
      for
      (
        int slave=Pstream::firstSlave();
        slave<=Pstream::lastSlave();
        slave++
      )
      {
        OPstream toSlave{Pstream::scheduled, slave};
        toSlave << sharedEdgeValues;
      }
    }
    else
    {
      // Send to master
      {
        OPstream toMaster{Pstream::scheduled, Pstream::masterNo()};
        toMaster << sharedEdgeValues;
      }
      // Receive merged values
      {
        IPstream fromMaster{Pstream::scheduled, Pstream::masterNo()};
        fromMaster >> sharedEdgeValues;
      }
    }
  }
  // Merge sharedEdgeValues (keyed on sharedPointAddr) into edgeValues
  // (keyed on mesh points).
  // Loop over all my shared edges.
  FOR_ALL_CONST_ITER(typename EdgeMap<edge>, potentialSharedEdge, iter)
  {
    const edge& sharedEdge = iter.key();
    const edge& meshEdge = iter();
    // Do I have a value for the shared edge?
    typename EdgeMap<T>::const_iterator sharedFnd =
      sharedEdgeValues.find(sharedEdge);
    if (sharedFnd != sharedEdgeValues.end())
    {
      combine
      (
        edgeValues,
        cop,
        meshEdge,       // edge
        sharedFnd()     // value
      );
    }
  }
}


template<class T, class CombineOp, class TransformOp>
void mousse::syncTools::syncPointList
(
  const polyMesh& mesh,
  List<T>& pointValues,
  const CombineOp& cop,
  const T& /*nullValue*/,
  const TransformOp& top
)
{
  if (pointValues.size() != mesh.nPoints())
  {
    FATAL_ERROR_IN
    (
      "syncTools<class T, class CombineOp>::syncPointList"
      "(const polyMesh&, List<T>&, const CombineOp&, const T&"
      ", const bool)"
    )
    << "Number of values " << pointValues.size()
    << " is not equal to the number of points in the mesh "
    << mesh.nPoints() << abort(FatalError);
  }
  mesh.globalData().syncPointData(pointValues, cop, top);
}


template<class T, class CombineOp, class TransformOp>
void mousse::syncTools::syncPointList
(
  const polyMesh& mesh,
  const labelList& meshPoints,
  List<T>& pointValues,
  const CombineOp& cop,
  const T& nullValue,
  const TransformOp& top
)
{
  if (pointValues.size() != meshPoints.size())
  {
    FATAL_ERROR_IN
    (
      "syncTools<class T, class CombineOp>::syncPointList"
      "(const polyMesh&, List<T>&, const CombineOp&, const T&)"
    )
    << "Number of values " << pointValues.size()
    << " is not equal to the number of meshPoints "
    << meshPoints.size() << abort(FatalError);
  }
  const globalMeshData& gd = mesh.globalData();
  const indirectPrimitivePatch& cpp = gd.coupledPatch();
  const Map<label>& mpm = cpp.meshPointMap();
  List<T> cppFld{cpp.nPoints(), nullValue};
  FOR_ALL(meshPoints, i)
  {
    label pointI = meshPoints[i];
    Map<label>::const_iterator iter = mpm.find(pointI);
    if (iter != mpm.end())
    {
      cppFld[iter()] = pointValues[i];
    }
  }
  globalMeshData::syncData
  (
    cppFld,
    gd.globalPointSlaves(),
    gd.globalPointTransformedSlaves(),
    gd.globalPointSlavesMap(),
    gd.globalTransforms(),
    cop,
    top
  );
  FOR_ALL(meshPoints, i)
  {
    label pointI = meshPoints[i];
    Map<label>::const_iterator iter = mpm.find(pointI);
    if (iter != mpm.end())
    {
      pointValues[i] = cppFld[iter()];
    }
  }
}


template<class T, class CombineOp, class TransformOp>
void mousse::syncTools::syncEdgeList
(
  const polyMesh& mesh,
  List<T>& edgeValues,
  const CombineOp& cop,
  const T& /*nullValue*/,
  const TransformOp& top
)
{
  if (edgeValues.size() != mesh.nEdges())
  {
    FATAL_ERROR_IN
    (
      "syncTools<class T, class CombineOp>::syncEdgeList"
      "(const polyMesh&, List<T>&, const CombineOp&, const T&)"
    )
    << "Number of values " << edgeValues.size()
    << " is not equal to the number of edges in the mesh "
    << mesh.nEdges() << abort(FatalError);
  }
  const globalMeshData& gd = mesh.globalData();
  const labelList& meshEdges = gd.coupledPatchMeshEdges();
  const globalIndexAndTransform& git = gd.globalTransforms();
  const mapDistribute& edgeMap = gd.globalEdgeSlavesMap();
  List<T> cppFld{UIndirectList<T>{edgeValues, meshEdges}};
  globalMeshData::syncData
  (
    cppFld,
    gd.globalEdgeSlaves(),
    gd.globalEdgeTransformedSlaves(),
    edgeMap,
    git,
    cop,
    top
  );
  // Extract back onto mesh
  FOR_ALL(meshEdges, i)
  {
    edgeValues[meshEdges[i]] = cppFld[i];
  }
}


template<class T, class CombineOp, class TransformOp>
void mousse::syncTools::syncEdgeList
(
  const polyMesh& mesh,
  const labelList& meshEdges,
  List<T>& edgeValues,
  const CombineOp& cop,
  const T& nullValue,
  const TransformOp& top
)
{
  if (edgeValues.size() != meshEdges.size())
  {
    FATAL_ERROR_IN
    (
      "syncTools<class T, class CombineOp>::syncEdgeList"
      "(const polyMesh&, List<T>&, const CombineOp&, const T&)"
    )
    << "Number of values " << edgeValues.size()
    << " is not equal to the number of meshEdges "
    << meshEdges.size() << abort(FatalError);
  }
  const globalMeshData& gd = mesh.globalData();
  const indirectPrimitivePatch& cpp = gd.coupledPatch();
  const Map<label>& mpm = gd.coupledPatchMeshEdgeMap();
  List<T> cppFld{cpp.nEdges(), nullValue};
  FOR_ALL(meshEdges, i)
  {
    label edgeI = meshEdges[i];
    Map<label>::const_iterator iter = mpm.find(edgeI);
    if (iter != mpm.end())
    {
      cppFld[iter()] = edgeValues[i];
    }
  }
  globalMeshData::syncData
  (
    cppFld,
    gd.globalEdgeSlaves(),
    gd.globalEdgeTransformedSlaves(),
    gd.globalEdgeSlavesMap(),
    gd.globalTransforms(),
    cop,
    top
  );
  FOR_ALL(meshEdges, i)
  {
    label edgeI = meshEdges[i];
    Map<label>::const_iterator iter = mpm.find(edgeI);
    if (iter != mpm.end())
    {
      edgeValues[i] = cppFld[iter()];
    }
  }
}


template<class T, class CombineOp, class TransformOp>
void mousse::syncTools::syncBoundaryFaceList
(
  const polyMesh& mesh,
  UList<T>& faceValues,
  const CombineOp& cop,
  const TransformOp& top
)
{
  const label nBFaces = mesh.nFaces() - mesh.nInternalFaces();
  if (faceValues.size() != nBFaces)
  {
    FATAL_ERROR_IN
    (
      "syncTools<class T, class CombineOp>::syncBoundaryFaceList"
      "(const polyMesh&, UList<T>&, const CombineOp&"
      ", const bool)"
    )
    << "Number of values " << faceValues.size()
    << " is not equal to the number of boundary faces in the mesh "
    << nBFaces << abort(FatalError);
  }
  const polyBoundaryMesh& patches = mesh.boundaryMesh();
  if (Pstream::parRun())
  {
    PstreamBuffers pBufs{Pstream::nonBlocking};
    // Send
    FOR_ALL(patches, patchI)
    {
      if(isA<processorPolyPatch>(patches[patchI])
         && patches[patchI].size() > 0)
      {
        const processorPolyPatch& procPatch =
          refCast<const processorPolyPatch>(patches[patchI]);
        label patchStart = procPatch.start() - mesh.nInternalFaces();
        UOPstream toNbr{procPatch.neighbProcNo(), pBufs};
        toNbr << SubField<T>{faceValues, procPatch.size(), patchStart};
      }
    }
    pBufs.finishedSends();
    // Receive and combine.
    FOR_ALL(patches, patchI)
    {
      if(isA<processorPolyPatch>(patches[patchI])
         && patches[patchI].size() > 0)
      {
        const processorPolyPatch& procPatch =
          refCast<const processorPolyPatch>(patches[patchI]);
        Field<T> nbrPatchInfo{procPatch.size()};
        UIPstream fromNeighb{procPatch.neighbProcNo(), pBufs};
        fromNeighb >> nbrPatchInfo;
        top(procPatch, nbrPatchInfo);
        label bFaceI = procPatch.start() - mesh.nInternalFaces();
        FOR_ALL(nbrPatchInfo, i)
        {
          cop(faceValues[bFaceI++], nbrPatchInfo[i]);
        }
      }
    }
  }
  // Do the cyclics.
  FOR_ALL(patches, patchI)
  {
    if (isA<cyclicPolyPatch>(patches[patchI]))
    {
      const cyclicPolyPatch& cycPatch =
        refCast<const cyclicPolyPatch>(patches[patchI]);
      if (cycPatch.owner())
      {
        // Owner does all.
        const cyclicPolyPatch& nbrPatch = cycPatch.neighbPatch();
        label ownStart = cycPatch.start() - mesh.nInternalFaces();
        label nbrStart = nbrPatch.start() - mesh.nInternalFaces();
        label sz = cycPatch.size();
        // Transform (copy of) data on both sides
        Field<T> ownVals{SubField<T>{faceValues, sz, ownStart}};
        top(nbrPatch, ownVals);
        Field<T> nbrVals{SubField<T>{faceValues, sz, nbrStart}};
        top(cycPatch, nbrVals);
        label i0 = ownStart;
        FOR_ALL(nbrVals, i)
        {
          cop(faceValues[i0++], nbrVals[i]);
        }
        label i1 = nbrStart;
        FOR_ALL(ownVals, i)
        {
          cop(faceValues[i1++], ownVals[i]);
        }
      }
    }
  }
}


// Member Functions 
template<unsigned nBits, class CombineOp>
void mousse::syncTools::syncFaceList
(
  const polyMesh& mesh,
  PackedList<nBits>& faceValues,
  const CombineOp& cop
)
{
  if (faceValues.size() != mesh.nFaces())
  {
    FATAL_ERROR_IN
    (
      "syncTools<unsigned nBits, class CombineOp>::syncFaceList"
      "(const polyMesh&, PackedList<nBits>&, const CombineOp&)"
    )
    << "Number of values " << faceValues.size()
    << " is not equal to the number of faces in the mesh "
    << mesh.nFaces() << abort(FatalError);
  }
  const polyBoundaryMesh& patches = mesh.boundaryMesh();
  if (Pstream::parRun())
  {
    PstreamBuffers pBufs{Pstream::nonBlocking};
    // Send
    FOR_ALL(patches, patchI)
    {
      if (isA<processorPolyPatch>(patches[patchI])
          && patches[patchI].size() > 0)
      {
        const processorPolyPatch& procPatch =
          refCast<const processorPolyPatch>(patches[patchI]);
        List<unsigned int> patchInfo{procPatch.size()};
        FOR_ALL(procPatch, i)
        {
          patchInfo[i] = faceValues[procPatch.start()+i];
        }
        UOPstream toNbr{procPatch.neighbProcNo(), pBufs};
        toNbr << patchInfo;
      }
    }
    pBufs.finishedSends();
    // Receive and combine.
    FOR_ALL(patches, patchI)
    {
      if (isA<processorPolyPatch>(patches[patchI])
          && patches[patchI].size() > 0)
      {
        const processorPolyPatch& procPatch =
          refCast<const processorPolyPatch>(patches[patchI]);
        List<unsigned int> patchInfo{procPatch.size()};
        {
          UIPstream fromNbr{procPatch.neighbProcNo(), pBufs};
          fromNbr >> patchInfo;
        }
        // Combine (bitwise)
        FOR_ALL(procPatch, i)
        {
          unsigned int patchVal = patchInfo[i];
          label meshFaceI = procPatch.start()+i;
          unsigned int faceVal = faceValues[meshFaceI];
          cop(faceVal, patchVal);
          faceValues[meshFaceI] = faceVal;
        }
      }
    }
  }
  // Do the cyclics.
  FOR_ALL(patches, patchI)
  {
    if (isA<cyclicPolyPatch>(patches[patchI]))
    {
      const cyclicPolyPatch& cycPatch =
        refCast<const cyclicPolyPatch>(patches[patchI]);
      if (cycPatch.owner())
      {
        // Owner does all.
        const cyclicPolyPatch& nbrPatch = cycPatch.neighbPatch();
        for (label i = 0; i < cycPatch.size(); i++)
        {
          label meshFace0 = cycPatch.start()+i;
          unsigned int val0 = faceValues[meshFace0];
          label meshFace1 = nbrPatch.start()+i;
          unsigned int val1 = faceValues[meshFace1];
          unsigned int t = val0;
          cop(t, val1);
          faceValues[meshFace0] = t;
          cop(val1, val0);
          faceValues[meshFace1] = val1;
        }
      }
    }
  }
}


template<class T>
void mousse::syncTools::swapBoundaryCellList
(
  const polyMesh& mesh,
  const UList<T>& cellData,
  List<T>& neighbourCellData
)
{
  if (cellData.size() != mesh.nCells())
  {
    FATAL_ERROR_IN
    (
      "syncTools<class T>::swapBoundaryCellList"
      "(const polyMesh&, const UList<T>&, List<T>&)"
    )
    << "Number of cell values " << cellData.size()
    << " is not equal to the number of cells in the mesh "
    << mesh.nCells() << abort(FatalError);
  }
  const polyBoundaryMesh& patches = mesh.boundaryMesh();
  label nBnd = mesh.nFaces() - mesh.nInternalFaces();
  neighbourCellData.setSize(nBnd);
  FOR_ALL(patches, patchI)
  {
    const polyPatch& pp = patches[patchI];
    const labelUList& faceCells = pp.faceCells();
    FOR_ALL(faceCells, i)
    {
      label bFaceI = pp.start() + i - mesh.nInternalFaces();
      neighbourCellData[bFaceI] = cellData[faceCells[i]];
    }
  }
  syncTools::swapBoundaryFaceList(mesh, neighbourCellData);
}


template<unsigned nBits>
void mousse::syncTools::swapFaceList
(
  const polyMesh& mesh,
  PackedList<nBits>& faceValues
)
{
  syncFaceList(mesh, faceValues, eqOp<unsigned int>());
}


template<unsigned nBits, class CombineOp>
void mousse::syncTools::syncPointList
(
  const polyMesh& mesh,
  PackedList<nBits>& pointValues,
  const CombineOp& cop,
  const unsigned int /*nullValue*/
)
{
  if (pointValues.size() != mesh.nPoints())
  {
    FATAL_ERROR_IN
    (
      "syncTools<unsigned nBits, class CombineOp>::syncPointList"
      "(const polyMesh&, PackedList<nBits>&, const CombineOp&"
      ", const unsigned int)"
    )
    << "Number of values " << pointValues.size()
    << " is not equal to the number of points in the mesh "
    << mesh.nPoints() << abort(FatalError);
  }
  const globalMeshData& gd = mesh.globalData();
  const labelList& meshPoints = gd.coupledPatch().meshPoints();
  List<unsigned int> cppFld{gd.globalPointSlavesMap().constructSize()};
  FOR_ALL(meshPoints, i)
  {
    cppFld[i] = pointValues[meshPoints[i]];
  }
  globalMeshData::syncData
  (
    cppFld,
    gd.globalPointSlaves(),
    gd.globalPointTransformedSlaves(),
    gd.globalPointSlavesMap(),
    cop
  );
  // Extract back to mesh
  FOR_ALL(meshPoints, i)
  {
    pointValues[meshPoints[i]] = cppFld[i];
  }
}


template<unsigned nBits, class CombineOp>
void mousse::syncTools::syncEdgeList
(
  const polyMesh& mesh,
  PackedList<nBits>& edgeValues,
  const CombineOp& cop,
  const unsigned int /*nullValue*/
)
{
  if (edgeValues.size() != mesh.nEdges())
  {
    FATAL_ERROR_IN
    (
      "syncTools<unsigned nBits, class CombineOp>::syncEdgeList"
      "(const polyMesh&, PackedList<nBits>&, const CombineOp&"
      ", const unsigned int)"
    )
    << "Number of values " << edgeValues.size()
    << " is not equal to the number of edges in the mesh "
    << mesh.nEdges() << abort(FatalError);
  }
  const globalMeshData& gd = mesh.globalData();
  const labelList& meshEdges = gd.coupledPatchMeshEdges();
  List<unsigned int> cppFld{gd.globalEdgeSlavesMap().constructSize()};
  FOR_ALL(meshEdges, i)
  {
    cppFld[i] = edgeValues[meshEdges[i]];
  }
  globalMeshData::syncData
  (
    cppFld,
    gd.globalEdgeSlaves(),
    gd.globalEdgeTransformedSlaves(),
    gd.globalEdgeSlavesMap(),
    cop
  );
  // Extract back to mesh
  FOR_ALL(meshEdges, i)
  {
    edgeValues[meshEdges[i]] = cppFld[i];
  }
}
