// mousse: CFD toolbox
// Copyright (C) 2011-2013 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "point_edge_wave.hpp"
#include "poly_mesh.hpp"
#include "processor_poly_patch.hpp"
#include "cyclic_poly_patch.hpp"
#include "opstream.hpp"
#include "ipstream.hpp"
#include "pstream_combine_reduce_ops.hpp"
#include "debug.hpp"
#include "type_info.hpp"
#include "global_mesh_data.hpp"
#include "point_fields.hpp"


// Static Data Members
template<class Type, class TrackingData>
mousse::scalar mousse::PointEdgeWave<Type, TrackingData>::propagationTol_ = 0.01;

template<class Type, class TrackingData>
int mousse::PointEdgeWave<Type, TrackingData>::dummyTrackData_ = 12345;


namespace mousse {

//- Reduction class. If x and y are not equal assign value.
template<class Type, class TrackingData>
class combineEqOp
{
  TrackingData& td_;
  public:
    combineEqOp(TrackingData& td)
    :
      td_{td}
    {}
  void operator()(Type& x, const Type& y) const
  {
    if (!x.valid(td_) && y.valid(td_)) {
      x = y;
    }
  }
};

}


// Private Member Functions 
// Handle leaving domain. Implementation referred to Type
template<class Type, class TrackingData>
void mousse::PointEdgeWave<Type, TrackingData>::leaveDomain
(
  const polyPatch& patch,
  const labelList& patchPointLabels,
  List<Type>& pointInfo
) const
{
  const labelList& meshPoints = patch.meshPoints();
  FOR_ALL(patchPointLabels, i) {
    label patchPointI = patchPointLabels[i];
    const point& pt = patch.points()[meshPoints[patchPointI]];
    pointInfo[i].leaveDomain(patch, patchPointI, pt, td_);
  }
}


// Handle entering domain. Implementation referred to Type
template<class Type, class TrackingData>
void mousse::PointEdgeWave<Type, TrackingData>::enterDomain
(
  const polyPatch& patch,
  const labelList& patchPointLabels,
  List<Type>& pointInfo
) const
{
  const labelList& meshPoints = patch.meshPoints();
  FOR_ALL(patchPointLabels, i) {
    label patchPointI = patchPointLabels[i];
    const point& pt = patch.points()[meshPoints[patchPointI]];
    pointInfo[i].enterDomain(patch, patchPointI, pt, td_);
  }
}


// Transform. Implementation referred to Type
template<class Type, class TrackingData>
void mousse::PointEdgeWave<Type, TrackingData>::transform
(
  const polyPatch& patch,
  const tensorField& rotTensor,
  List<Type>& pointInfo
) const
{
  if (rotTensor.size() == 1) {
    const tensor& T = rotTensor[0];
    FOR_ALL(pointInfo, i) {
      pointInfo[i].transform(T, td_);
    }
  } else {
    FATAL_ERROR_IN
    (
      "PointEdgeWave<Type, TrackingData>::transform"
      "(const tensorField&, List<Type>&)"
    )
    << "Non-uniform transformation on patch " << patch.name()
    << " of type " << patch.type()
    << " not supported for point fields"
    << abort(FatalError);
    FOR_ALL(pointInfo, i) {
      pointInfo[i].transform(rotTensor[i], td_);
    }
  }
}


// Update info for pointI, at position pt, with information from
// neighbouring edge.
// Updates:
//      - changedPoint_, changedPoints_, nChangedPoints_,
//      - statistics: nEvals_, nUnvisitedPoints_
template<class Type, class TrackingData>
bool mousse::PointEdgeWave<Type, TrackingData>::updatePoint
(
  const label pointI,
  const label neighbourEdgeI,
  const Type& neighbourInfo,
  Type& pointInfo
)
{
  nEvals_++;
  bool wasValid = pointInfo.valid(td_);
  bool propagate =
    pointInfo.updatePoint
    (
      mesh_,
      pointI,
      neighbourEdgeI,
      neighbourInfo,
      propagationTol_,
      td_
    );
  if (propagate) {
    if (!changedPoint_[pointI]) {
      changedPoint_[pointI] = true;
      changedPoints_[nChangedPoints_++] = pointI;
    }
  }
  if (!wasValid && pointInfo.valid(td_)) {
    --nUnvisitedPoints_;
  }
  return propagate;
}


// Update info for pointI, at position pt, with information from
// same point.
// Updates:
//      - changedPoint_, changedPoints_, nChangedPoints_,
//      - statistics: nEvals_, nUnvisitedPoints_
template<class Type, class TrackingData>
bool mousse::PointEdgeWave<Type, TrackingData>::updatePoint
(
  const label pointI,
  const Type& neighbourInfo,
  Type& pointInfo
)
{
  nEvals_++;
  bool wasValid = pointInfo.valid(td_);
  bool propagate =
    pointInfo.updatePoint
    (
      mesh_,
      pointI,
      neighbourInfo,
      propagationTol_,
      td_
    );
  if (propagate) {
    if (!changedPoint_[pointI]) {
      changedPoint_[pointI] = true;
      changedPoints_[nChangedPoints_++] = pointI;
    }
  }
  if (!wasValid && pointInfo.valid(td_)) {
    --nUnvisitedPoints_;
  }
  return propagate;
}


// Update info for edgeI, at position pt, with information from
// neighbouring point.
// Updates:
//      - changedEdge_, changedEdges_, nChangedEdges_,
//      - statistics: nEvals_, nUnvisitedEdge_
template<class Type, class TrackingData>
bool mousse::PointEdgeWave<Type, TrackingData>::updateEdge
(
  const label edgeI,
  const label neighbourPointI,
  const Type& neighbourInfo,
  Type& edgeInfo
)
{
  nEvals_++;
  bool wasValid = edgeInfo.valid(td_);
  bool propagate =
    edgeInfo.updateEdge
    (
      mesh_,
      edgeI,
      neighbourPointI,
      neighbourInfo,
      propagationTol_,
      td_
    );
  if (propagate) {
    if (!changedEdge_[edgeI]) {
      changedEdge_[edgeI] = true;
      changedEdges_[nChangedEdges_++] = edgeI;
    }
  }
  if (!wasValid && edgeInfo.valid(td_)) {
    --nUnvisitedEdges_;
  }
  return propagate;
}


// Check if patches of given type name are present
template<class Type, class TrackingData>
template<class PatchType>
mousse::label mousse::PointEdgeWave<Type, TrackingData>::countPatchType() const
{
  label nPatches = 0;
  FOR_ALL(mesh_.boundaryMesh(), patchI) {
    if (isA<PatchType>(mesh_.boundaryMesh()[patchI])) {
      nPatches++;
    }
  }
  return nPatches;
}


// Transfer all the information to/from neighbouring processors
template<class Type, class TrackingData>
void mousse::PointEdgeWave<Type, TrackingData>::handleProcPatches()
{
  // 1. Send all point info on processor patches.
  PstreamBuffers pBufs{Pstream::nonBlocking};
  DynamicList<Type> patchInfo;
  DynamicList<label> thisPoints;
  DynamicList<label> nbrPoints;
  FOR_ALL(mesh_.globalData().processorPatches(), i) {
    label patchI = mesh_.globalData().processorPatches()[i];
    const processorPolyPatch& procPatch =
      refCast<const processorPolyPatch>(mesh_.boundaryMesh()[patchI]);
    patchInfo.clear();
    patchInfo.reserve(procPatch.nPoints());
    thisPoints.clear();
    thisPoints.reserve(procPatch.nPoints());
    nbrPoints.clear();
    nbrPoints.reserve(procPatch.nPoints());
    // Get all changed points in reverse order
    const labelList& neighbPoints = procPatch.neighbPoints();
    FOR_ALL(neighbPoints, thisPointI) {
      label meshPointI = procPatch.meshPoints()[thisPointI];
      if (changedPoint_[meshPointI]) {
        patchInfo.append(allPointInfo_[meshPointI]);
        thisPoints.append(thisPointI);
        nbrPoints.append(neighbPoints[thisPointI]);
      }
    }
    // Adapt for leaving domain
    leaveDomain(procPatch, thisPoints, patchInfo);
    UOPstream toNeighbour{procPatch.neighbProcNo(), pBufs};
    toNeighbour << nbrPoints << patchInfo;
  }
  pBufs.finishedSends();
  //
  // 2. Receive all point info on processor patches.
  //
  FOR_ALL(mesh_.globalData().processorPatches(), i) {
    label patchI = mesh_.globalData().processorPatches()[i];
    const processorPolyPatch& procPatch =
      refCast<const processorPolyPatch>(mesh_.boundaryMesh()[patchI]);
    List<Type> patchInfo;
    labelList patchPoints;

    {
      UIPstream fromNeighbour{procPatch.neighbProcNo(), pBufs};
      fromNeighbour >> patchPoints >> patchInfo;
    }

    // Apply transform to received data for non-parallel planes
    if (!procPatch.parallel()) {
      transform(procPatch, procPatch.forwardT(), patchInfo);
    }
    // Adapt for entering domain
    enterDomain(procPatch, patchPoints, patchInfo);
    // Merge received info
    const labelList& meshPoints = procPatch.meshPoints();
    FOR_ALL(patchInfo, i) {
      label meshPointI = meshPoints[patchPoints[i]];
      if (!allPointInfo_[meshPointI].equal(patchInfo[i], td_)) {
        updatePoint
        (
          meshPointI,
          patchInfo[i],
          allPointInfo_[meshPointI]
        );
      }
    }
  }
  // Collocated points should be handled by face based transfer
  // (since that is how connectivity is worked out)
  // They are also explicitly equalised in handleCollocatedPoints to
  // guarantee identical values.
}


template<class Type, class TrackingData>
void mousse::PointEdgeWave<Type, TrackingData>::handleCyclicPatches()
{
  // 1. Send all point info on cyclic patches.
  DynamicList<Type> nbrInfo;
  DynamicList<label> nbrPoints;
  DynamicList<label> thisPoints;
  FOR_ALL(mesh_.boundaryMesh(), patchI) {
    const polyPatch& patch = mesh_.boundaryMesh()[patchI];
    if (isA<cyclicPolyPatch>(patch)) {
      const cyclicPolyPatch& cycPatch =
        refCast<const cyclicPolyPatch>(patch);
      nbrInfo.clear();
      nbrInfo.reserve(cycPatch.nPoints());
      nbrPoints.clear();
      nbrPoints.reserve(cycPatch.nPoints());
      thisPoints.clear();
      thisPoints.reserve(cycPatch.nPoints());
      // Collect nbrPatch points that have changed
      {
        const cyclicPolyPatch& nbrPatch = cycPatch.neighbPatch();
        const edgeList& pairs = cycPatch.coupledPoints();
        const labelList& meshPoints = nbrPatch.meshPoints();
        FOR_ALL(pairs, pairI) {
          label thisPointI = pairs[pairI][0];
          label nbrPointI = pairs[pairI][1];
          label meshPointI = meshPoints[nbrPointI];
          if (changedPoint_[meshPointI]) {
            nbrInfo.append(allPointInfo_[meshPointI]);
            nbrPoints.append(nbrPointI);
            thisPoints.append(thisPointI);
          }
        }
        // nbr : adapt for leaving domain
        leaveDomain(nbrPatch, nbrPoints, nbrInfo);
      }
      // Apply rotation for non-parallel planes
      if (!cycPatch.parallel()) {
        // received data from half1
        transform(cycPatch, cycPatch.forwardT(), nbrInfo);
      }
      // Adapt for entering domain
      enterDomain(cycPatch, thisPoints, nbrInfo);
      // Merge received info
      const labelList& meshPoints = cycPatch.meshPoints();
      FOR_ALL(nbrInfo, i) {
        label meshPointI = meshPoints[thisPoints[i]];
        if (!allPointInfo_[meshPointI].equal(nbrInfo[i], td_)) {
          updatePoint
          (
            meshPointI,
            nbrInfo[i],
            allPointInfo_[meshPointI]
          );
        }
      }
    }
  }
}


// Guarantee collocated points have same information.
// Return number of points changed.
template<class Type, class TrackingData>
mousse::label mousse::PointEdgeWave<Type, TrackingData>::handleCollocatedPoints()
{
  // Transfer onto coupled patch
  const globalMeshData& gmd = mesh_.globalData();
  const indirectPrimitivePatch& cpp = gmd.coupledPatch();
  const labelList& meshPoints = cpp.meshPoints();
  const mapDistribute& slavesMap = gmd.globalPointSlavesMap();
  const labelListList& slaves = gmd.globalPointSlaves();
  List<Type> elems{slavesMap.constructSize()};
  FOR_ALL(meshPoints, pointI) {
    elems[pointI] = allPointInfo_[meshPoints[pointI]];
  }
  // Pull slave data onto master (which might or might not have any
  // initialised points). No need to update transformed slots.
  slavesMap.distribute(elems, false);
  // Combine master data with slave data
  combineEqOp<Type, TrackingData> cop{td_};
  FOR_ALL(slaves, pointI) {
    Type& elem = elems[pointI];
    const labelList& slavePoints = slaves[pointI];
    // Combine master with untransformed slave data
    FOR_ALL(slavePoints, j) {
      cop(elem, elems[slavePoints[j]]);
    }
    // Copy result back to slave slots
    FOR_ALL(slavePoints, j) {
      elems[slavePoints[j]] = elem;
    }
  }
  // Push slave-slot data back to slaves
  slavesMap.reverseDistribute(elems.size(), elems, false);
  // Extract back onto mesh
  FOR_ALL(meshPoints, pointI) {
    if (elems[pointI].valid(td_)) {
      label meshPointI = meshPoints[pointI];
      Type& elem = allPointInfo_[meshPointI];
      bool wasValid = elem.valid(td_);
      // Like updatePoint but bypass Type::updatePoint with its tolerance
      // checking
      //if (!elem.valid(td_) || !elem.equal(elems[pointI], td_))
      if (!elem.equal(elems[pointI], td_)) {
        nEvals_++;
        elem = elems[pointI];
        // See if element now valid
        if (!wasValid && elem.valid(td_)) {
          --nUnvisitedPoints_;
        }
        // Update database of changed points
        if (!changedPoint_[meshPointI]) {
          changedPoint_[meshPointI] = true;
          changedPoints_[nChangedPoints_++] = meshPointI;
        }
      }
    }
  }
  // Sum nChangedPoints over all procs
  label totNChanged = nChangedPoints_;
  reduce(totNChanged, sumOp<label>());
  return totNChanged;
}


// Constructors 
// Iterate, propagating changedPointsInfo across mesh, until no change (or
// maxIter reached). Initial point values specified.
template<class Type, class TrackingData>
mousse::PointEdgeWave<Type, TrackingData>::PointEdgeWave
(
  const polyMesh& mesh,
  const labelList& changedPoints,
  const List<Type>& changedPointsInfo,
  UList<Type>& allPointInfo,
  UList<Type>& allEdgeInfo,
  const label maxIter,
  TrackingData& td
)
:
  mesh_{mesh},
  allPointInfo_{allPointInfo},
  allEdgeInfo_{allEdgeInfo},
  td_{td},
  changedPoint_{mesh_.nPoints(), false},
  changedPoints_{mesh_.nPoints()},
  nChangedPoints_{0},
  changedEdge_{mesh_.nEdges(), false},
  changedEdges_{mesh_.nEdges()},
  nChangedEdges_{0},
  nCyclicPatches_{countPatchType<cyclicPolyPatch>()},
  nEvals_{0},
  nUnvisitedPoints_{mesh_.nPoints()},
  nUnvisitedEdges_{mesh_.nEdges()}
{
  if (allPointInfo_.size() != mesh_.nPoints()) {
    FATAL_ERROR_IN
    (
      "PointEdgeWave<Type, TrackingData>::PointEdgeWave"
      "(const polyMesh&, const labelList&, const List<Type>,"
      " List<Type>&, List<Type>&, const label maxIter)"
    )
    << "size of pointInfo work array is not equal to the number"
    << " of points in the mesh" << endl
    << "    pointInfo   :" << allPointInfo_.size() << endl
    << "    mesh.nPoints:" << mesh_.nPoints()
    << exit(FatalError);
  }
  if (allEdgeInfo_.size() != mesh_.nEdges()) {
    FATAL_ERROR_IN
    (
      "PointEdgeWave<Type, TrackingData>::PointEdgeWave"
      "(const polyMesh&, const labelList&, const List<Type>,"
      " List<Type>&, List<Type>&, const label maxIter)"
    )
    << "size of edgeInfo work array is not equal to the number"
    << " of edges in the mesh" << endl
    << "    edgeInfo   :" << allEdgeInfo_.size() << endl
    << "    mesh.nEdges:" << mesh_.nEdges()
    << exit(FatalError);
  }
  // Set from initial changed points data
  setPointInfo(changedPoints, changedPointsInfo);
  if (debug) {
    Info << typeName << ": Seed points               : "
      << returnReduce(nChangedPoints_, sumOp<label>()) << endl;
  }
  // Iterate until nothing changes
  label iter = iterate(maxIter);
  if ((maxIter > 0) && (iter >= maxIter)) {
    FATAL_ERROR_IN
    (
      "PointEdgeWave<Type, TrackingData>::PointEdgeWave"
      "(const polyMesh&, const labelList&, const List<Type>,"
      " List<Type>&, List<Type>&, const label maxIter)"
    )
    << "Maximum number of iterations reached. Increase maxIter." << endl
    << "    maxIter:" << maxIter << endl
    << "    nChangedPoints:" << nChangedPoints_ << endl
    << "    nChangedEdges:" << nChangedEdges_ << endl
    << exit(FatalError);
  }
}


template<class Type, class TrackingData>
mousse::PointEdgeWave<Type, TrackingData>::PointEdgeWave
(
  const polyMesh& mesh,
  UList<Type>& allPointInfo,
  UList<Type>& allEdgeInfo,
  TrackingData& td
)
:
  mesh_{mesh},
  allPointInfo_{allPointInfo},
  allEdgeInfo_{allEdgeInfo},
  td_{td},
  changedPoint_{mesh_.nPoints(), false},
  changedPoints_{mesh_.nPoints()},
  nChangedPoints_{0},
  changedEdge_{mesh_.nEdges(), false},
  changedEdges_{mesh_.nEdges()},
  nChangedEdges_{0},
  nCyclicPatches_{countPatchType<cyclicPolyPatch>()},
  nEvals_{0},
  nUnvisitedPoints_{mesh_.nPoints()},
  nUnvisitedEdges_{mesh_.nEdges()}
{}


// Destructor 
template<class Type, class TrackingData>
mousse::PointEdgeWave<Type, TrackingData>::~PointEdgeWave()
{}


// Member Functions 
template<class Type, class TrackingData>
mousse::label mousse::PointEdgeWave<Type, TrackingData>::getUnsetPoints() const
{
  return nUnvisitedPoints_;
}


template<class Type, class TrackingData>
mousse::label mousse::PointEdgeWave<Type, TrackingData>::getUnsetEdges() const
{
  return nUnvisitedEdges_;
}


// Copy point information into member data
template<class Type, class TrackingData>
void mousse::PointEdgeWave<Type, TrackingData>::setPointInfo
(
  const labelList& changedPoints,
  const List<Type>& changedPointsInfo
)
{
  FOR_ALL(changedPoints, changedPointI) {
    label pointI = changedPoints[changedPointI];
    bool wasValid = allPointInfo_[pointI].valid(td_);
    // Copy info for pointI
    allPointInfo_[pointI] = changedPointsInfo[changedPointI];
    // Maintain count of unset points
    if (!wasValid && allPointInfo_[pointI].valid(td_)) {
      --nUnvisitedPoints_;
    }
    // Mark pointI as changed, both on list and on point itself.
    if (!changedPoint_[pointI]) {
      changedPoint_[pointI] = true;
      changedPoints_[nChangedPoints_++] = pointI;
    }
  }
  // Sync
  handleCollocatedPoints();
}


// Propagate information from edge to point. Return number of points changed.
template<class Type, class TrackingData>
mousse::label mousse::PointEdgeWave<Type, TrackingData>::edgeToPoint()
{
  for
  (
    label changedEdgeI = 0;
    changedEdgeI < nChangedEdges_;
    changedEdgeI++
  ) {
    label edgeI = changedEdges_[changedEdgeI];
    if (!changedEdge_[edgeI]) {
      FATAL_ERROR_IN("PointEdgeWave<Type, TrackingData>::edgeToPoint()")
        << "edge " << edgeI
        << " not marked as having been changed" << nl
        << "This might be caused by multiple occurences of the same"
        << " seed point." << abort(FatalError);
    }
    const Type& neighbourWallInfo = allEdgeInfo_[edgeI];
    // Evaluate all connected points (= edge endpoints)
    const edge& e = mesh_.edges()[edgeI];
    FOR_ALL(e, eI) {
      Type& currentWallInfo = allPointInfo_[e[eI]];
      if (!currentWallInfo.equal(neighbourWallInfo, td_)) {
        updatePoint
        (
          e[eI],
          edgeI,
          neighbourWallInfo,
          currentWallInfo
        );
      }
    }
    // Reset status of edge
    changedEdge_[edgeI] = false;
  }
  // Handled all changed edges by now
  nChangedEdges_ = 0;
  if (nCyclicPatches_ > 0) {
    // Transfer changed points across cyclic halves
    handleCyclicPatches();
  }
  if (Pstream::parRun()) {
    // Transfer changed points from neighbouring processors.
    handleProcPatches();
  }
  // Sum nChangedPoints over all procs
  label totNChanged = nChangedPoints_;
  reduce(totNChanged, sumOp<label>());
  return totNChanged;
}


// Propagate information from point to edge. Return number of edges changed.
template<class Type, class TrackingData>
mousse::label mousse::PointEdgeWave<Type, TrackingData>::pointToEdge()
{
  const labelListList& pointEdges = mesh_.pointEdges();
  for
  (
    label changedPointI = 0;
    changedPointI < nChangedPoints_;
    changedPointI++
  ) {
    label pointI = changedPoints_[changedPointI];
    if (!changedPoint_[pointI]) {
      FATAL_ERROR_IN("PointEdgeWave<Type, TrackingData>::pointToEdge()")
        << "Point " << pointI
        << " not marked as having been changed" << nl
        << "This might be caused by multiple occurences of the same"
        << " seed point." << abort(FatalError);
    }
    const Type& neighbourWallInfo = allPointInfo_[pointI];
    // Evaluate all connected edges
    const labelList& edgeLabels = pointEdges[pointI];
    FOR_ALL(edgeLabels, edgeLabelI) {
      label edgeI = edgeLabels[edgeLabelI];
      Type& currentWallInfo = allEdgeInfo_[edgeI];
      if (!currentWallInfo.equal(neighbourWallInfo, td_)) {
        updateEdge
        (
          edgeI,
          pointI,
          neighbourWallInfo,
          currentWallInfo
        );
      }
    }
    // Reset status of point
    changedPoint_[pointI] = false;
  }
  // Handled all changed points by now
  nChangedPoints_ = 0;
  // Sum nChangedPoints over all procs
  label totNChanged = nChangedEdges_;
  reduce(totNChanged, sumOp<label>());
  return totNChanged;
}


// Iterate
template<class Type, class TrackingData>
mousse::label mousse::PointEdgeWave<Type, TrackingData>::iterate
(
  const label maxIter
)
{
  if (nCyclicPatches_ > 0) {
    // Transfer changed points across cyclic halves
    handleCyclicPatches();
  }
  if (Pstream::parRun()) {
    // Transfer changed points from neighbouring processors.
    handleProcPatches();
  }
  nEvals_ = 0;
  label iter = 0;
  while (iter < maxIter) {
    while (iter < maxIter) {
      if (debug) {
        Info << typeName << ": Iteration " << iter << endl;
      }
      label nEdges = pointToEdge();
      if (debug) {
        Info << typeName << ": Total changed edges       : "
          << nEdges << endl;
      }
      if (nEdges == 0) {
        break;
      }
      label nPoints = edgeToPoint();
      if (debug) {
        Info
          << typeName << ": Total changed points      : "
          << nPoints << nl
          << typeName << ": Total evaluations         : "
          << returnReduce(nEvals_, sumOp<label>()) << nl
          << typeName << ": Remaining unvisited points: "
          << returnReduce(nUnvisitedPoints_, sumOp<label>()) << nl
          << typeName << ": Remaining unvisited edges : "
          << returnReduce(nUnvisitedEdges_, sumOp<label>()) << nl
          << endl;
      }
      if (nPoints == 0) {
        break;
      }
      iter++;
    }
    // Enforce collocated points are exactly equal. This might still mean
    // non-collocated points are not equal though. WIP.
    label nPoints = handleCollocatedPoints();
    if (debug) {
      Info << typeName << ": Collocated point sync     : "
        << nPoints << nl << endl;
    }
    if (nPoints == 0) {
      break;
    }
  }
  return iter;
}

