// mousse: CFD toolbox
// Copyright (C) 2012-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "distributed_delaunay_mesh.hpp"
#include "mesh_search.hpp"
#include "map_distribute.hpp"
#include "zero_gradient_fv_patch_fields.hpp"
#include "point_conversion.hpp"
#include "indexed_vertex_enum.hpp"
#include "iomanip.hpp"


// Static Member Functions
template<class Triangulation>
mousse::autoPtr<mousse::mapDistribute>
mousse::DistributedDelaunayMesh<Triangulation>::buildMap
(
  const List<label>& toProc
)
{
  // Determine send map
  // ~~~~~~~~~~~~~~~~~~
  // 1. Count
  labelList nSend{Pstream::nProcs(), 0};
  FOR_ALL(toProc, i) {
    label procI = toProc[i];
    nSend[procI]++;
  }
  // Send over how many I need to receive
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  labelListList sendSizes{Pstream::nProcs()};
  sendSizes[Pstream::myProcNo()] = nSend;
  combineReduce(sendSizes, UPstream::listEq());
  // 2. Size sendMap
  labelListList sendMap{Pstream::nProcs()};
  FOR_ALL(nSend, procI) {
    sendMap[procI].setSize(nSend[procI]);
    nSend[procI] = 0;
  }
  // 3. Fill sendMap
  FOR_ALL(toProc, i) {
    label procI = toProc[i];
    sendMap[procI][nSend[procI]++] = i;
  }
  // Determine receive map
  // ~~~~~~~~~~~~~~~~~~~~~
  labelListList constructMap{Pstream::nProcs()};
  // Local transfers first
  constructMap[Pstream::myProcNo()] =
    identity
    (
      sendMap[Pstream::myProcNo()].size()
    );
  label constructSize = constructMap[Pstream::myProcNo()].size();
  FOR_ALL(constructMap, procI) {
    if (procI != Pstream::myProcNo()) {
      label nRecv = sendSizes[procI][Pstream::myProcNo()];
      constructMap[procI].setSize(nRecv);
      for (label i = 0; i < nRecv; i++) {
        constructMap[procI][i] = constructSize++;
      }
    }
  }
  return
    autoPtr<mapDistribute>
    {
      new mapDistribute
      {
        constructSize,
        sendMap.xfer(),
        constructMap.xfer()
      }
    };
}


// Constructors 
template<class Triangulation>
mousse::DistributedDelaunayMesh<Triangulation>::DistributedDelaunayMesh
(
  const Time& runTime
)
:
  DelaunayMesh<Triangulation>{runTime},
  allBackgroundMeshBounds_{}
{}


template<class Triangulation>
mousse::DistributedDelaunayMesh<Triangulation>::DistributedDelaunayMesh
(
  const Time& runTime,
  const word& meshName
)
:
  DelaunayMesh<Triangulation>{runTime, meshName},
  allBackgroundMeshBounds_{}
{}


// Destructor 
template<class Triangulation>
mousse::DistributedDelaunayMesh<Triangulation>::~DistributedDelaunayMesh()
{}


// Private Member Functions 
template<class Triangulation>
bool mousse::DistributedDelaunayMesh<Triangulation>::distributeBoundBoxes
(
  const boundBox& bb
)
{
  allBackgroundMeshBounds_.reset(new List<boundBox>{Pstream::nProcs()});
  // Give the bounds of every processor to every other processor
  allBackgroundMeshBounds_()[Pstream::myProcNo()] = bb;
  Pstream::gatherList(allBackgroundMeshBounds_());
  Pstream::scatterList(allBackgroundMeshBounds_());
  return true;
}


template<class Triangulation>
bool mousse::DistributedDelaunayMesh<Triangulation>::isLocal
(
  const Vertex_handle& v
) const
{
  return isLocal(v->procIndex());
}


template<class Triangulation>
bool mousse::DistributedDelaunayMesh<Triangulation>::isLocal
(
  const label localProcIndex
) const
{
  return localProcIndex == Pstream::myProcNo();
}


template<class Triangulation>
mousse::labelList mousse::DistributedDelaunayMesh<Triangulation>::overlapProcessors
(
  const point& centre,
  const scalar radiusSqr
) const
{
  DynamicList<label> toProc{Pstream::nProcs()};
  FOR_ALL(allBackgroundMeshBounds_(), procI) {
    // Test against the bounding box of the processor
    if (!isLocal(procI)
        && allBackgroundMeshBounds_()[procI].overlaps(centre, radiusSqr)) {
      toProc.append(procI);
    }
  }
  return toProc;
}


template<class Triangulation>
bool mousse::DistributedDelaunayMesh<Triangulation>::checkProcBoundaryCell
(
  const Cell_handle& cit,
  Map<labelList>& circumsphereOverlaps
) const
{
  const mousse::point& cc = cit->dual();
  const scalar crSqr = magSqr(cc - topoint(cit->vertex(0)->point()));
  labelList circumsphereOverlap =
    overlapProcessors(cc, sqr(1.01)*crSqr);
  cit->cellIndex() = this->getNewCellIndex();
  if (!circumsphereOverlap.empty()) {
    circumsphereOverlaps.insert(cit->cellIndex(), circumsphereOverlap);
    return true;
  }
  return false;
}


template<class Triangulation>
void mousse::DistributedDelaunayMesh<Triangulation>::findProcessorBoundaryCells
(
  Map<labelList>& circumsphereOverlaps
) const
{
  // Start by assuming that all the cells have no index
  // If they do, they have already been visited so ignore them
  labelHashSet cellToCheck
  {
    static_cast<label>
    (
      Triangulation::number_of_finite_cells()/Pstream::nProcs()
    )
  };
  for (auto cit = Triangulation::all_cells_begin();
       cit != Triangulation::all_cells_end();
       ++cit) {
    if (Triangulation::is_infinite(cit)) {
      // Index of infinite vertex in this cell.
      label i = cit->index(Triangulation::infinite_vertex());
      Cell_handle c = cit->neighbor(i);
      if (c->unassigned()) {
        c->cellIndex() = this->getNewCellIndex();
        if (checkProcBoundaryCell(c, circumsphereOverlaps)) {
          cellToCheck.insert(c->cellIndex());
        }
      }
    } else if (cit->parallelDualVertex()) {
      if (cit->unassigned()) {
        if (checkProcBoundaryCell(cit, circumsphereOverlaps)) {
          cellToCheck.insert(cit->cellIndex());
        }
      }
    }
  }
  for (auto cit = Triangulation::finite_cells_begin();
       cit != Triangulation::finite_cells_end();
       ++cit) {
    if (!cellToCheck.found(cit->cellIndex()))
      continue;
    // Get the neighbours and check them
    for (label adjCellI = 0; adjCellI < 4; ++adjCellI) {
      Cell_handle citNeighbor = cit->neighbor(adjCellI);
      // Ignore if has far point or previously visited
      if (!citNeighbor->unassigned()
          || !citNeighbor->internalOrBoundaryDualVertex()
          || Triangulation::is_infinite(citNeighbor)) {
        continue;
      }
      if (checkProcBoundaryCell(citNeighbor, circumsphereOverlaps)) {
        cellToCheck.insert(citNeighbor->cellIndex());
      }
    }
    cellToCheck.unset(cit->cellIndex());
  }
}


template<class Triangulation>
void mousse::DistributedDelaunayMesh<Triangulation>::markVerticesToRefer
(
  const Map<labelList>& circumsphereOverlaps,
  PtrList<labelPairHashSet>& referralVertices,
  DynamicList<label>& targetProcessor,
  DynamicList<Vb>& parallelInfluenceVertices
)
{
  // Relying on the order of iteration of cells being the same as before
  for (auto cit = Triangulation::finite_cells_begin();
       cit != Triangulation::finite_cells_end();
       ++cit) {
    if (Triangulation::is_infinite(cit)) {
      continue;
    }
    Map<labelList>::const_iterator iter =
      circumsphereOverlaps.find(cit->cellIndex());
    // Pre-tested circumsphere potential influence
    if (iter != circumsphereOverlaps.cend()) {
      const labelList& citOverlaps = iter();
      FOR_ALL(citOverlaps, cOI) {
        label procI = citOverlaps[cOI];
        for (int i = 0; i < 4; i++) {
          Vertex_handle v = cit->vertex(i);
          if (v->farPoint()) {
            continue;
          }
          label vProcIndex = v->procIndex();
          label vIndex = v->index();
          const labelPair procIndexPair{vProcIndex, vIndex};
          // Using the hashSet to ensure that each vertex is only
          // referred once to each processor.
          // Do not refer a vertex to its own processor.
          if (vProcIndex != procI) {
            if (referralVertices[procI].insert(procIndexPair)) {
              targetProcessor.append(procI);
              parallelInfluenceVertices.append
              (
                Vb
                {
                  v->point(),
                  v->index(),
                  v->type(),
                  v->procIndex()
                }
              );
              parallelInfluenceVertices.last().targetCellSize() =
                v->targetCellSize();
              parallelInfluenceVertices.last().alignment() =
                v->alignment();
            }
          }
        }
      }
    }
  }
}


template<class Triangulation>
mousse::label mousse::DistributedDelaunayMesh<Triangulation>::referVertices
(
  const DynamicList<label>& targetProcessor,
  DynamicList<Vb>& parallelVertices,
  PtrList<labelPairHashSet>& referralVertices,
  labelPairHashSet& receivedVertices
)
{
  DynamicList<Vb> referredVertices{targetProcessor.size()};
  const label preDistributionSize = parallelVertices.size();
  mapDistribute pointMap = buildMap(targetProcessor);
  // Make a copy of the original list.
  DynamicList<Vb> originalParallelVertices{parallelVertices};
  pointMap.distribute(parallelVertices);
  for (label procI = 0; procI < Pstream::nProcs(); procI++) {
    const labelList& constructMap = pointMap.constructMap()[procI];
    if (constructMap.size()) {
      FOR_ALL(constructMap, i) {
        const Vb& v = parallelVertices[constructMap[i]];
        if (v.procIndex() != Pstream::myProcNo()
            && !receivedVertices.found(labelPair(v.procIndex(), v.index()))) {
          referredVertices.append(v);
          receivedVertices.insert
          (
            labelPair(v.procIndex(), v.index())
          );
        }
      }
    }
  }
  label preInsertionSize = Triangulation::number_of_vertices();
  labelPairHashSet pointsNotInserted =
    rangeInsertReferredWithInfo
    (
      referredVertices.begin(),
      referredVertices.end(),
      true
    );
  if (!pointsNotInserted.empty()) {
    for (auto iter = pointsNotInserted.begin();
         iter != pointsNotInserted.end();
         ++iter) {
      if (receivedVertices.found(iter.key())) {
        receivedVertices.erase(iter.key());
      }
    }
  }
  boolList pointInserted{parallelVertices.size(), true};
  FOR_ALL(parallelVertices, vI) {
    const labelPair procIndexI
    {
      parallelVertices[vI].procIndex(),
      parallelVertices[vI].index()
    };
    if (pointsNotInserted.found(procIndexI)) {
      pointInserted[vI] = false;
    }
  }
  pointMap.reverseDistribute(preDistributionSize, pointInserted);
  FOR_ALL(originalParallelVertices, vI) {
    const label procIndex = targetProcessor[vI];
    if (!pointInserted[vI]) {
      if (referralVertices[procIndex].size()) {
        if (!referralVertices[procIndex].unset
            (
              labelPair
              {
                originalParallelVertices[vI].procIndex(),
                originalParallelVertices[vI].index()
              }
            )) {
          Pout << "*** not found "
            << originalParallelVertices[vI].procIndex()
            << " " << originalParallelVertices[vI].index() << endl;
        }
      }
    }
  }
  label postInsertionSize = Triangulation::number_of_vertices();
  reduce(preInsertionSize, sumOp<label>());
  reduce(postInsertionSize, sumOp<label>());
  label nTotalToInsert = referredVertices.size();
  reduce(nTotalToInsert, sumOp<label>());
  if (preInsertionSize + nTotalToInsert != postInsertionSize) {
    label nNotInserted =
      returnReduce(pointsNotInserted.size(), sumOp<label>());
    Info << " Inserted = "
      << setw(name(label(Triangulation::number_of_finite_cells())).size())
      << nTotalToInsert - nNotInserted
      << " / " << nTotalToInsert << endl;
    nTotalToInsert -= nNotInserted;
  } else {
    Info << " Inserted = " << nTotalToInsert << endl;
  }
  return nTotalToInsert;
}


template<class Triangulation>
void mousse::DistributedDelaunayMesh<Triangulation>::sync
(
  const boundBox& bb,
  PtrList<labelPairHashSet>& referralVertices,
  labelPairHashSet& receivedVertices,
  bool iterateReferral
)
{
  if (!Pstream::parRun()) {
    return;
  }
  if (allBackgroundMeshBounds_.empty()) {
    distributeBoundBoxes(bb);
  }
  label nVerts = Triangulation::number_of_vertices();
  label nCells = Triangulation::number_of_finite_cells();
  DynamicList<Vb> parallelInfluenceVertices{static_cast<label>(0.1*nVerts)};
  DynamicList<label> targetProcessor{static_cast<label>(0.1*nVerts)};
  // Some of these values will not be used, i.e. for non-real cells
  DynamicList<mousse::point> circumcentre{static_cast<label>(0.1*nVerts)};
  DynamicList<scalar> circumradiusSqr{static_cast<label>(0.1*nVerts)};
  Map<labelList> circumsphereOverlaps{nCells};
  findProcessorBoundaryCells(circumsphereOverlaps);
  Info << "    Influences = "
    << setw(name(nCells).size())
    << returnReduce(circumsphereOverlaps.size(), sumOp<label>()) << " / "
    << returnReduce(nCells, sumOp<label>());
  markVerticesToRefer
  (
    circumsphereOverlaps,
    referralVertices,
    targetProcessor,
    parallelInfluenceVertices
  );
  referVertices
  (
    targetProcessor,
    parallelInfluenceVertices,
    referralVertices,
    receivedVertices
  );
  if (iterateReferral) {
    label oldNReferred = 0;
    label nIterations = 1;
    Info << incrIndent << indent
      << "Iteratively referring referred vertices..."
      << endl;
    do {
      Info << indent << "Iteration " << nIterations++ << ":";
      circumsphereOverlaps.clear();
      targetProcessor.clear();
      parallelInfluenceVertices.clear();
      findProcessorBoundaryCells(circumsphereOverlaps);
      nCells = Triangulation::number_of_finite_cells();
      Info << " Influences = "
        << setw(name(nCells).size())
        << returnReduce(circumsphereOverlaps.size(), sumOp<label>())
        << " / "
        << returnReduce(nCells, sumOp<label>());
      markVerticesToRefer
      (
        circumsphereOverlaps,
        referralVertices,
        targetProcessor,
        parallelInfluenceVertices
      );
      label nReferred =
        referVertices
        (
          targetProcessor,
          parallelInfluenceVertices,
          referralVertices,
          receivedVertices
        );
      if (nReferred == 0 || nReferred == oldNReferred) {
        break;
      }
      oldNReferred = nReferred;
    } while (true);
    Info << decrIndent;
  }
}


// Protected Member Functions 
// Member Functions 
template<class Triangulation>
mousse::scalar
mousse::DistributedDelaunayMesh<Triangulation>::calculateLoadUnbalance() const
{
  label nRealVertices = 0;
  for (auto vit = Triangulation::finite_vertices_begin();
       vit != Triangulation::finite_vertices_end();
       ++vit) {
    // Only store real vertices that are not feature vertices
    if (vit->real() && !vit->featurePoint()) {
      nRealVertices++;
    }
  }
  scalar globalNRealVertices =
    returnReduce
    (
      nRealVertices,
      sumOp<label>()
    );
  scalar unbalance =
    returnReduce
    (
      mag(1.0 - nRealVertices/(globalNRealVertices/Pstream::nProcs())),
      maxOp<scalar>()
    );
  Info << "    Processor unbalance " << unbalance << endl;
  return unbalance;
}


template<class Triangulation>
bool mousse::DistributedDelaunayMesh<Triangulation>::distribute
(
  const boundBox& bb
)
{
  NOT_IMPLEMENTED
  (
    "mousse::DistributedDelaunayMesh<Triangulation>::distribute"
    "("
    "  const boundBox& bb"
    ")"
  );
  if (!Pstream::parRun()) {
    return false;
  }
  distributeBoundBoxes(bb);
  return true;
}


template<class Triangulation>
mousse::autoPtr<mousse::mapDistribute>
mousse::DistributedDelaunayMesh<Triangulation>::distribute
(
  const backgroundMeshDecomposition& decomposition,
  List<mousse::point>& points
)
{
  if (!Pstream::parRun()) {
    return autoPtr<mapDistribute>{};
  }
  distributeBoundBoxes(decomposition.procBounds());
  autoPtr<mapDistribute> mapDist = decomposition.distributePoints(points);
  return mapDist;
}


template<class Triangulation>
void mousse::DistributedDelaunayMesh<Triangulation>::sync(const boundBox& bb)
{
  if (!Pstream::parRun()) {
    return;
  }
  if (allBackgroundMeshBounds_.empty()) {
    distributeBoundBoxes(bb);
  }
  const label nApproxReferred =
    Triangulation::number_of_vertices()/Pstream::nProcs();
  PtrList<labelPairHashSet> referralVertices{Pstream::nProcs()};
  FOR_ALL(referralVertices, procI) {
    if (!isLocal(procI)) {
      referralVertices.set(procI, new labelPairHashSet(nApproxReferred));
    }
  }
  labelPairHashSet receivedVertices{nApproxReferred};
  sync
  (
    bb,
    referralVertices,
    receivedVertices,
    true
  );
}


template<class Triangulation>
template<class PointIterator>
typename mousse::DistributedDelaunayMesh<Triangulation>::labelPairHashSet
mousse::DistributedDelaunayMesh<Triangulation>::rangeInsertReferredWithInfo
(
  PointIterator begin,
  PointIterator end,
  bool printErrors
)
{
  const boundBox& bb = allBackgroundMeshBounds_()[Pstream::myProcNo()];
  typedef DynamicList
  <
    std::pair<scalar, label>
  > vectorPairPointIndex;
  vectorPairPointIndex pointsBbDistSqr;
  label count = 0;
  for (PointIterator it = begin; it != end; ++it) {
    const mousse::point samplePoint{topoint(it->point())};
    scalar distFromBbSqr = 0;
    if (!bb.contains(samplePoint)) {
      const mousse::point nearestPoint = bb.nearest(samplePoint);
      distFromBbSqr = magSqr(nearestPoint - samplePoint);
    }
    pointsBbDistSqr.append
    (
      std::make_pair(distFromBbSqr, count++)
    );
  }
  std::random_shuffle(pointsBbDistSqr.begin(), pointsBbDistSqr.end());
  // Sort in ascending order by the distance of the point from the centre
  // of the processor bounding box
  sort(pointsBbDistSqr.begin(), pointsBbDistSqr.end());
  typename Triangulation::Vertex_handle hint;
  typename Triangulation::Locate_type lt;
  int li, lj;
  label nNotInserted = 0;
  labelPairHashSet uninserted
  {
    static_cast<label>(Triangulation::number_of_vertices()/Pstream::nProcs())
  };
  for (auto p = pointsBbDistSqr.begin();
       p != pointsBbDistSqr.end();
       ++p) {
    const size_t checkInsertion = Triangulation::number_of_vertices();
    const Vb& vert = *(begin + p->second);
    const Point& pointToInsert = vert.point();
    // Locate the point
    Cell_handle c = Triangulation::locate(pointToInsert, lt, li, lj, hint);
    bool inserted = false;
    if (lt == Triangulation::VERTEX) {
      if (printErrors) {
        Vertex_handle nearV =
          Triangulation::nearest_vertex(pointToInsert);
        Pout << "Failed insertion, point already exists" << nl
          << "Failed insertion : " << vert.info()
          << "         nearest : " << nearV->info();
      }
    } else if (lt == Triangulation::OUTSIDE_AFFINE_HULL) {
      WARNING_IN
      (
        "mousse::DistributedDelaunayMesh<Triangulation>"
        "::rangeInsertReferredWithInfo"
      )
      << "Point is outside affine hull! pt = " << pointToInsert
      << endl;
    } else if (lt == Triangulation::OUTSIDE_CONVEX_HULL) {
      // @todo Can this be optimised?
      //
      // Only want to insert if a connection is formed between
      // pointToInsert and an internal or internal boundary point.
      hint = Triangulation::insert(pointToInsert, c);
      inserted = true;
    } else {
      // Get the cells that conflict with p in a vector V,
      // and a facet on the boundary of this hole in f.
      std::vector<Cell_handle> V;
      typename Triangulation::Facet f;
      Triangulation::find_conflicts
      (
        pointToInsert,
        c,
        CGAL::Oneset_iterator<typename Triangulation::Facet>(f),
        std::back_inserter(V)
      );
      for (size_t i = 0; i < V.size(); ++i) {
        Cell_handle conflictingCell = V[i];
        if (Triangulation::dimension() < 3 // 2D triangulation
            ||(!Triangulation::is_infinite(conflictingCell)
               && (conflictingCell->real()
                   || conflictingCell->hasFarPoint()))) {
          hint = Triangulation::insert_in_hole
          (
            pointToInsert,
            V.begin(),
            V.end(),
            f.first,
            f.second
          );
          inserted = true;
          break;
        }
      }
    }
    if (inserted) {
      if (checkInsertion != Triangulation::number_of_vertices() - 1) {
        if (printErrors) {
          Vertex_handle nearV =
            Triangulation::nearest_vertex(pointToInsert);
          Pout << "Failed insertion : " << vert.info()
            << "         nearest : " << nearV->info();
        }
      } else {
        hint->index() = vert.index();
        hint->type() = vert.type();
        hint->procIndex() = vert.procIndex();
        hint->targetCellSize() = vert.targetCellSize();
        hint->alignment() = vert.alignment();
      }
    } else {
      uninserted.insert(labelPair(vert.procIndex(), vert.index()));
      nNotInserted++;
    }
  }
  return uninserted;
}

