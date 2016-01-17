// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "background_mesh_decomposition.hpp"
#include "mesh_search.hpp"
#include "conformation_surfaces.hpp"
#include "zero_gradient_fv_patch_fields.hpp"
#include "time.hpp"
#include "random.hpp"
#include "point_conversion.hpp"
// Static Data Members
namespace mousse
{
defineTypeNameAndDebug(backgroundMeshDecomposition, 0);
}
// Static Member Functions
mousse::autoPtr<mousse::mapDistribute> mousse::backgroundMeshDecomposition::buildMap
(
  const List<label>& toProc
)
{
  // Determine send map
  // ~~~~~~~~~~~~~~~~~~
  // 1. Count
  labelList nSend(Pstream::nProcs(), 0);
  forAll(toProc, i)
  {
    label procI = toProc[i];
    nSend[procI]++;
  }
  // Send over how many I need to receive
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  labelListList sendSizes(Pstream::nProcs());
  sendSizes[Pstream::myProcNo()] = nSend;
  combineReduce(sendSizes, UPstream::listEq());
  // 2. Size sendMap
  labelListList sendMap(Pstream::nProcs());
  forAll(nSend, procI)
  {
    sendMap[procI].setSize(nSend[procI]);
    nSend[procI] = 0;
  }
  // 3. Fill sendMap
  forAll(toProc, i)
  {
    label procI = toProc[i];
    sendMap[procI][nSend[procI]++] = i;
  }
  // Determine receive map
  // ~~~~~~~~~~~~~~~~~~~~~
  labelListList constructMap(Pstream::nProcs());
  // Local transfers first
  constructMap[Pstream::myProcNo()] = identity
  (
    sendMap[Pstream::myProcNo()].size()
  );
  label constructSize = constructMap[Pstream::myProcNo()].size();
  forAll(constructMap, procI)
  {
    if (procI != Pstream::myProcNo())
    {
      label nRecv = sendSizes[procI][Pstream::myProcNo()];
      constructMap[procI].setSize(nRecv);
      for (label i = 0; i < nRecv; i++)
      {
        constructMap[procI][i] = constructSize++;
      }
    }
  }
  return autoPtr<mapDistribute>
  (
    new mapDistribute
    (
      constructSize,
      sendMap.xfer(),
      constructMap.xfer()
    )
  );
}
// Private Member Functions 
void mousse::backgroundMeshDecomposition::initialRefinement()
{
  volScalarField cellWeights
  (
    IOobject
    (
      "cellWeights",
      mesh_.time().timeName(),
      mesh_,
      IOobject::NO_READ,
      IOobject::NO_WRITE
    ),
    mesh_,
    dimensionedScalar("one", dimless, 1.0),
    zeroGradientFvPatchScalarField::typeName
  );
  const conformationSurfaces& geometry = geometryToConformTo_;
  decompositionMethod& decomposer = decomposerPtr_();
  volScalarField::InternalField& icellWeights = cellWeights.internalField();
  // For each cell in the mesh has it been determined if it is fully
  // inside, outside, or overlaps the surface
  List<volumeType> volumeStatus
  (
    mesh_.nCells(),
    volumeType::UNKNOWN
  );
  // Surface refinement
  {
    while (true)
    {
      // Determine/update the status of each cell
      forAll(volumeStatus, cellI)
      {
        if (volumeStatus[cellI] == volumeType::UNKNOWN)
        {
          treeBoundBox cellBb
          (
            mesh_.cells()[cellI].points
            (
              mesh_.faces(),
              mesh_.points()
            )
          );
          if (geometry.overlaps(cellBb))
          {
            volumeStatus[cellI] = volumeType::MIXED;
          }
          else if (geometry.inside(cellBb.midpoint()))
          {
            volumeStatus[cellI] = volumeType::INSIDE;
          }
          else
          {
            volumeStatus[cellI] = volumeType::OUTSIDE;
          }
        }
      }
      {
        labelList refCells = selectRefinementCells
        (
          volumeStatus,
          cellWeights
        );
        // Maintain 2:1 ratio
        labelList newCellsToRefine
        (
          meshCutter_.consistentRefinement
          (
            refCells,
            true                  // extend set
          )
        );
        forAll(newCellsToRefine, nCTRI)
        {
          label cellI = newCellsToRefine[nCTRI];
          if (volumeStatus[cellI] == volumeType::MIXED)
          {
            volumeStatus[cellI] = volumeType::UNKNOWN;
          }
          icellWeights[cellI] = max
          (
            1.0,
            icellWeights[cellI]/8.0
          );
        }
        if (returnReduce(newCellsToRefine.size(), sumOp<label>()) == 0)
        {
          break;
        }
        // Mesh changing engine.
        polyTopoChange meshMod(mesh_);
        // Play refinement commands into mesh changer.
        meshCutter_.setRefinement(newCellsToRefine, meshMod);
        // Create mesh, return map from old to new mesh.
        autoPtr<mapPolyMesh> map = meshMod.changeMesh
        (
          mesh_,
          false,  // inflate
          true,   // syncParallel
          true,   // orderCells (to reduce cell transfers)
          false   // orderPoints
        );
        // Update fields
        mesh_.updateMesh(map);
        // Update numbering of cells/vertices.
        meshCutter_.updateMesh(map);
        {
          // Map volumeStatus
          const labelList& cellMap = map().cellMap();
          List<volumeType> newVolumeStatus(cellMap.size());
          forAll(cellMap, newCellI)
          {
            label oldCellI = cellMap[newCellI];
            if (oldCellI == -1)
            {
              newVolumeStatus[newCellI] = volumeType::UNKNOWN;
            }
            else
            {
              newVolumeStatus[newCellI] = volumeStatus[oldCellI];
            }
          }
          volumeStatus.transfer(newVolumeStatus);
        }
        Info<< "    Background mesh refined from "
          << returnReduce(map().nOldCells(), sumOp<label>())
          << " to " << mesh_.globalData().nTotalCells()
          << " cells." << endl;
      }
      // Determine/update the status of each cell
      forAll(volumeStatus, cellI)
      {
        if (volumeStatus[cellI] == volumeType::UNKNOWN)
        {
          treeBoundBox cellBb
          (
            mesh_.cells()[cellI].points
            (
              mesh_.faces(),
              mesh_.points()
            )
          );
          if (geometry.overlaps(cellBb))
          {
            volumeStatus[cellI] = volumeType::MIXED;
          }
          else if (geometry.inside(cellBb.midpoint()))
          {
            volumeStatus[cellI] = volumeType::INSIDE;
          }
          else
          {
            volumeStatus[cellI] = volumeType::OUTSIDE;
          }
        }
      }
      // Hard code switch for this stage for testing
      bool removeOutsideCells = false;
      if (removeOutsideCells)
      {
        DynamicList<label> cellsToRemove;
        forAll(volumeStatus, cellI)
        {
          if (volumeStatus[cellI] == volumeType::OUTSIDE)
          {
            cellsToRemove.append(cellI);
          }
        }
        removeCells cellRemover(mesh_);
        // Mesh changing engine.
        polyTopoChange meshMod(mesh_);
        labelList exposedFaces = cellRemover.getExposedFaces
        (
          cellsToRemove
        );
        // Play refinement commands into mesh changer.
        cellRemover.setRefinement
        (
          cellsToRemove,
          exposedFaces,
          labelList(exposedFaces.size(), 0),  // patchID dummy
          meshMod
        );
        // Create mesh, return map from old to new mesh.
        autoPtr<mapPolyMesh> map = meshMod.changeMesh
        (
          mesh_,
          false,  // inflate
          true,   // syncParallel
          true,   // orderCells (to reduce cell transfers)
          false   // orderPoints
        );
        // Update fields
        mesh_.updateMesh(map);
        // Update numbering of cells/vertices.
        meshCutter_.updateMesh(map);
        cellRemover.updateMesh(map);
        {
          // Map volumeStatus
          const labelList& cellMap = map().cellMap();
          List<volumeType> newVolumeStatus(cellMap.size());
          forAll(cellMap, newCellI)
          {
            label oldCellI = cellMap[newCellI];
            if (oldCellI == -1)
            {
              newVolumeStatus[newCellI] = volumeType::UNKNOWN;
            }
            else
            {
              newVolumeStatus[newCellI] = volumeStatus[oldCellI];
            }
          }
          volumeStatus.transfer(newVolumeStatus);
        }
        Info<< "Removed "
          << returnReduce(map().nOldCells(), sumOp<label>())
          - mesh_.globalData().nTotalCells()
          << " cells." << endl;
      }
      if (debug)
      {
        // const_cast<Time&>(mesh_.time())++;
        // Info<< "Time " << mesh_.time().timeName() << endl;
        meshCutter_.write();
        mesh_.write();
        cellWeights.write();
      }
      labelList newDecomp = decomposer.decompose
      (
        mesh_,
        mesh_.cellCentres(),
        icellWeights
      );
      fvMeshDistribute distributor(mesh_, mergeDist_);
      autoPtr<mapDistributePolyMesh> mapDist = distributor.distribute
      (
        newDecomp
      );
      meshCutter_.distribute(mapDist);
      mapDist().distributeCellData(volumeStatus);
      if (debug)
      {
        printMeshData(mesh_);
        // const_cast<Time&>(mesh_.time())++;
        // Info<< "Time " << mesh_.time().timeName() << endl;
        meshCutter_.write();
        mesh_.write();
        cellWeights.write();
      }
    }
  }
  if (debug)
  {
    // const_cast<Time&>(mesh_.time())++;
    // Info<< "Time " << mesh_.time().timeName() << endl;
    cellWeights.write();
    mesh_.write();
  }
  buildPatchAndTree();
}
void mousse::backgroundMeshDecomposition::printMeshData
(
  const polyMesh& mesh
) const
{
  // Collect all data on master
  globalIndex globalCells(mesh.nCells());
  // labelListList patchNeiProcNo(Pstream::nProcs());
  // labelListList patchSize(Pstream::nProcs());
  // const labelList& pPatches = mesh.globalData().processorPatches();
  // patchNeiProcNo[Pstream::myProcNo()].setSize(pPatches.size());
  // patchSize[Pstream::myProcNo()].setSize(pPatches.size());
  // forAll(pPatches, i)
  // {
  //     const processorPolyPatch& ppp = refCast<const processorPolyPatch>
  //     (
  //         mesh.boundaryMesh()[pPatches[i]]
  //     );
  //     patchNeiProcNo[Pstream::myProcNo()][i] = ppp.neighbProcNo();
  //     patchSize[Pstream::myProcNo()][i] = ppp.size();
  // }
  // Pstream::gatherList(patchNeiProcNo);
  // Pstream::gatherList(patchSize);
  // // Print stats
  // globalIndex globalBoundaryFaces(mesh.nFaces()-mesh.nInternalFaces());
  for (label procI = 0; procI < Pstream::nProcs(); procI++)
  {
    Info<< "Processor " << procI << " "
      << "Number of cells = " << globalCells.localSize(procI)
      << endl;
    // label nProcFaces = 0;
    // const labelList& nei = patchNeiProcNo[procI];
    // forAll(patchNeiProcNo[procI], i)
    // {
    //     Info<< "    Number of faces shared with processor "
    //         << patchNeiProcNo[procI][i] << " = " << patchSize[procI][i]
    //         << endl;
    //     nProcFaces += patchSize[procI][i];
    // }
    // Info<< "    Number of processor patches = " << nei.size() << nl
    //     << "    Number of processor faces = " << nProcFaces << nl
    //     << "    Number of boundary faces = "
    //     << globalBoundaryFaces.localSize(procI) << endl;
  }
}
bool mousse::backgroundMeshDecomposition::refineCell
(
  label cellI,
  volumeType volType,
  scalar& weightEstimate
) const
{
  // Sample the box to find an estimate of the min size, and a volume
  // estimate when overlapping == true.
//    const conformationSurfaces& geometry = geometryToConformTo_;
  treeBoundBox cellBb
  (
    mesh_.cells()[cellI].points
    (
      mesh_.faces(),
      mesh_.points()
    )
  );
  weightEstimate = 1.0;
  if (volType == volumeType::MIXED)
  {
//        // Assess the cell size at the nearest point on the surface for the
//        // MIXED cells, if the cell is large with respect to the cell size,
//        // then refine it.
//
//        pointField samplePoints
//        (
//            volRes_*volRes_*volRes_,
//            vector::zero
//        );
//
//        // scalar sampleVol = cellBb.volume()/samplePoints.size();
//
//        vector delta = cellBb.span()/volRes_;
//
//        label pI = 0;
//
//        for (label i = 0; i < volRes_; i++)
//        {
//            for (label j = 0; j < volRes_; j++)
//            {
//                for (label k = 0; k < volRes_; k++)
//                {
//                    samplePoints[pI++] =
//                        cellBb.min()
//                      + vector
//                        (
//                            delta.x()*(i + 0.5),
//                            delta.y()*(j + 0.5),
//                            delta.z()*(k + 0.5)
//                        );
//                }
//            }
//        }
//
//        List<pointIndexHit> hitInfo;
//        labelList hitSurfaces;
//
//        geometry.findSurfaceNearest
//        (
//            samplePoints,
//            scalarField(samplePoints.size(), sqr(GREAT)),
//            hitInfo,
//            hitSurfaces
//        );
//
//        // weightEstimate = 0.0;
//
//        scalar minCellSize = GREAT;
//
//        forAll(samplePoints, i)
//        {
//            scalar s = cellShapeControl_.cellSize
//            (
//                hitInfo[i].hitPoint()
//            );
//
//            // Info<< "cellBb.midpoint() " << cellBb.midpoint() << nl
//            //     << samplePoints[i] << nl
//            //     << hitInfo[i] << nl
//            //     << "cellBb.span() " << cellBb.span() << nl
//            //     << "cellBb.mag() " << cellBb.mag() << nl
//            //     << s << endl;
//
//            if (s < minCellSize)
//            {
//                minCellSize = max(s, minCellSizeLimit_);
//            }
//
//            // Estimate the number of points in the cell by the surface size,
//            // this is likely to be too small, so reduce.
//            // weightEstimate += sampleVol/pow3(s);
//        }
//
//        if (sqr(spanScale_)*sqr(minCellSize) < magSqr(cellBb.span()))
//        {
//            return true;
//        }
  }
  else if (volType == volumeType::INSIDE)
  {
    // scalar s =
    //    foamyHexMesh_.cellShapeControl_.cellSize(cellBb.midpoint());
    // Estimate the number of points in the cell by the size at the cell
    // midpoint
    // weightEstimate = cellBb.volume()/pow3(s);
    return false;
  }
  // else
  // {
  //     weightEstimate = 1.0;
  //     return false;
  // }
  return false;
}
mousse::labelList mousse::backgroundMeshDecomposition::selectRefinementCells
(
  List<volumeType>& volumeStatus,
  volScalarField& cellWeights
) const
{
  volScalarField::InternalField& icellWeights = cellWeights.internalField();
  labelHashSet cellsToRefine;
  // Determine/update the status of each cell
  forAll(volumeStatus, cellI)
  {
    if (volumeStatus[cellI] == volumeType::MIXED)
    {
      if (meshCutter_.cellLevel()[cellI] < minLevels_)
      {
        cellsToRefine.insert(cellI);
      }
    }
    if (volumeStatus[cellI] != volumeType::OUTSIDE)
    {
      if
      (
        refineCell
        (
          cellI,
          volumeStatus[cellI],
          icellWeights[cellI]
        )
      )
      {
        cellsToRefine.insert(cellI);
      }
    }
  }
  return cellsToRefine.toc();
}
void mousse::backgroundMeshDecomposition::buildPatchAndTree()
{
  primitivePatch tmpBoundaryFaces
  (
    SubList<face>
    (
      mesh_.faces(),
      mesh_.nFaces() - mesh_.nInternalFaces(),
      mesh_.nInternalFaces()
    ),
    mesh_.points()
  );
  boundaryFacesPtr_.reset
  (
    new bPatch
    (
      tmpBoundaryFaces.localFaces(),
      tmpBoundaryFaces.localPoints()
    )
  );
  // Overall bb
  treeBoundBox overallBb(boundaryFacesPtr_().localPoints());
  Random& rnd = rndGen_;
  bFTreePtr_.reset
  (
    new indexedOctree<treeDataBPatch>
    (
      treeDataBPatch
      (
        false,
        boundaryFacesPtr_(),
        indexedOctree<treeDataBPatch>::perturbTol()
      ),
      overallBb.extend(rnd, 1e-4),
      10, // maxLevel
      10, // leafSize
      3.0 // duplicity
    )
  );
  // Give the bounds of every processor to every other processor
  allBackgroundMeshBounds_[Pstream::myProcNo()] = overallBb;
  Pstream::gatherList(allBackgroundMeshBounds_);
  Pstream::scatterList(allBackgroundMeshBounds_);
  point bbMin(GREAT, GREAT, GREAT);
  point bbMax(-GREAT, -GREAT, -GREAT);
  forAll(allBackgroundMeshBounds_, procI)
  {
    bbMin = min(bbMin, allBackgroundMeshBounds_[procI].min());
    bbMax = max(bbMax, allBackgroundMeshBounds_[procI].max());
  }
  globalBackgroundBounds_ = treeBoundBox(bbMin, bbMax);
  if (false)
  {
    OFstream fStr
    (
      mesh_.time().path()
     /"backgroundMeshDecomposition_proc_"
     + name(Pstream::myProcNo())
     + "_boundaryFaces.obj"
    );
    const faceList& faces = boundaryFacesPtr_().localFaces();
    const List<point>& points = boundaryFacesPtr_().localPoints();
    Map<label> foamToObj(points.size());
    label vertI = 0;
    forAll(faces, i)
    {
      const face& f = faces[i];
      forAll(f, fPI)
      {
        if (foamToObj.insert(f[fPI], vertI))
        {
          meshTools::writeOBJ(fStr, points[f[fPI]]);
          vertI++;
        }
      }
      fStr<< 'f';
      forAll(f, fPI)
      {
        fStr<< ' ' << foamToObj[f[fPI]] + 1;
      }
      fStr<< nl;
    }
  }
}
// Constructors 
mousse::backgroundMeshDecomposition::backgroundMeshDecomposition
(
  const Time& runTime,
  Random& rndGen,
  const conformationSurfaces& geometryToConformTo,
  const dictionary& coeffsDict
)
:
  runTime_(runTime),
  geometryToConformTo_(geometryToConformTo),
  rndGen_(rndGen),
  mesh_
  (
    IOobject
    (
      "backgroundMeshDecomposition",
      runTime_.timeName(),
      runTime_,
      IOobject::MUST_READ,
      IOobject::AUTO_WRITE,
      false
    )
  ),
  meshCutter_
  (
    mesh_,
    labelList(mesh_.nCells(), 0),
    labelList(mesh_.nPoints(), 0)
  ),
  boundaryFacesPtr_(),
  bFTreePtr_(),
  allBackgroundMeshBounds_(Pstream::nProcs()),
  globalBackgroundBounds_(),
  decomposeDict_
  (
    IOobject
    (
      "decomposeParDict",
      runTime_.system(),
      runTime_,
      IOobject::MUST_READ_IF_MODIFIED,
      IOobject::NO_WRITE
    )
  ),
  decomposerPtr_(decompositionMethod::New(decomposeDict_)),
  mergeDist_(1e-6*mesh_.bounds().mag()),
  spanScale_(readScalar(coeffsDict.lookup("spanScale"))),
  minCellSizeLimit_
  (
    coeffsDict.lookupOrDefault<scalar>("minCellSizeLimit", 0.0)
  ),
  minLevels_(readLabel(coeffsDict.lookup("minLevels"))),
  volRes_(readLabel(coeffsDict.lookup("sampleResolution"))),
  maxCellWeightCoeff_(readScalar(coeffsDict.lookup("maxCellWeightCoeff")))
{
  if (!Pstream::parRun())
  {
    FatalErrorIn
    (
      "mousse::backgroundMeshDecomposition::backgroundMeshDecomposition"
      "("
        "const dictionary& coeffsDict, "
        "const conformalVoronoiMesh& foamyHexMesh"
      ")"
    )
      << "This cannot be used when not running in parallel."
      << exit(FatalError);
  }
  if (!decomposerPtr_().parallelAware())
  {
    FatalErrorIn
    (
      "void mousse::backgroundMeshDecomposition::initialRefinement() const"
    )
      << "You have selected decomposition method "
      << decomposerPtr_().typeName
      << " which is not parallel aware." << endl
      << exit(FatalError);
  }
  Info<< nl << "Building initial background mesh decomposition" << endl;
  initialRefinement();
}
// Destructor 
mousse::backgroundMeshDecomposition::~backgroundMeshDecomposition()
{}
// Member Functions 
mousse::autoPtr<mousse::mapDistributePolyMesh>
mousse::backgroundMeshDecomposition::distribute
(
  volScalarField& cellWeights
)
{
  if (debug)
  {
    // const_cast<Time&>(mesh_.time())++;
    // Info<< "Time " << mesh_.time().timeName() << endl;
    cellWeights.write();
    mesh_.write();
  }
  volScalarField::InternalField& icellWeights = cellWeights.internalField();
  while (true)
  {
    // Refine large cells if necessary
    label nOccupiedCells = 0;
    forAll(icellWeights, cI)
    {
      if (icellWeights[cI] > 1 - SMALL)
      {
        nOccupiedCells++;
      }
    }
    // Only look at occupied cells, as there is a possibility of runaway
    // refinement if the number of cells grows too fast.  Also, clip the
    // minimum cellWeightLimit at maxCellWeightCoeff_
    scalar cellWeightLimit = max
    (
      maxCellWeightCoeff_
     *sum(cellWeights).value()
     /returnReduce(nOccupiedCells, sumOp<label>()),
      maxCellWeightCoeff_
    );
    if (debug)
    {
      Info<< "    cellWeightLimit " << cellWeightLimit << endl;
      Pout<< "    sum(cellWeights) " << sum(cellWeights.internalField())
        << " max(cellWeights) " << max(cellWeights.internalField())
        << endl;
    }
    labelHashSet cellsToRefine;
    forAll(icellWeights, cWI)
    {
      if (icellWeights[cWI] > cellWeightLimit)
      {
        cellsToRefine.insert(cWI);
      }
    }
    if (returnReduce(cellsToRefine.size(), sumOp<label>()) == 0)
    {
      break;
    }
    // Maintain 2:1 ratio
    labelList newCellsToRefine
    (
      meshCutter_.consistentRefinement
      (
        cellsToRefine.toc(),
        true                  // extend set
      )
    );
    if (debug && !cellsToRefine.empty())
    {
      Pout<< "    cellWeights too large in " << cellsToRefine.size()
        << " cells" << endl;
    }
    forAll(newCellsToRefine, nCTRI)
    {
      label cellI = newCellsToRefine[nCTRI];
      icellWeights[cellI] /= 8.0;
    }
    // Mesh changing engine.
    polyTopoChange meshMod(mesh_);
    // Play refinement commands into mesh changer.
    meshCutter_.setRefinement(newCellsToRefine, meshMod);
    // Create mesh, return map from old to new mesh.
    autoPtr<mapPolyMesh> map = meshMod.changeMesh
    (
      mesh_,
      false,  // inflate
      true,   // syncParallel
      true,   // orderCells (to reduce cell motion)
      false   // orderPoints
    );
    // Update fields
    mesh_.updateMesh(map);
    // Update numbering of cells/vertices.
    meshCutter_.updateMesh(map);
    Info<< "    Background mesh refined from "
      << returnReduce(map().nOldCells(), sumOp<label>())
      << " to " << mesh_.globalData().nTotalCells()
      << " cells." << endl;
    if (debug)
    {
      // const_cast<Time&>(mesh_.time())++;
      // Info<< "Time " << mesh_.time().timeName() << endl;
      cellWeights.write();
      mesh_.write();
    }
  }
  if (debug)
  {
    printMeshData(mesh_);
    Pout<< "    Pre distribute sum(cellWeights) "
      << sum(icellWeights)
      << " max(cellWeights) "
      << max(icellWeights)
      << endl;
  }
  labelList newDecomp = decomposerPtr_().decompose
  (
    mesh_,
    mesh_.cellCentres(),
    icellWeights
  );
  Info<< "    Redistributing background mesh cells" << endl;
  fvMeshDistribute distributor(mesh_, mergeDist_);
  autoPtr<mapDistributePolyMesh> mapDist = distributor.distribute(newDecomp);
  meshCutter_.distribute(mapDist);
  if (debug)
  {
    printMeshData(mesh_);
    Pout<< "    Post distribute sum(cellWeights) "
      << sum(icellWeights)
      << " max(cellWeights) "
      << max(icellWeights)
      << endl;
    // const_cast<Time&>(mesh_.time())++;
    // Info<< "Time " << mesh_.time().timeName() << endl;
    mesh_.write();
    cellWeights.write();
  }
  buildPatchAndTree();
  return mapDist;
}
bool mousse::backgroundMeshDecomposition::positionOnThisProcessor
(
  const point& pt
) const
{
//    return bFTreePtr_().findAnyOverlap(pt, 0.0);
  return (bFTreePtr_().getVolumeType(pt) == volumeType::INSIDE);
}
mousse::boolList mousse::backgroundMeshDecomposition::positionOnThisProcessor
(
  const List<point>& pts
) const
{
  boolList posProc(pts.size(), true);
  forAll(pts, pI)
  {
    posProc[pI] = positionOnThisProcessor(pts[pI]);
  }
  return posProc;
}
bool mousse::backgroundMeshDecomposition::overlapsThisProcessor
(
  const treeBoundBox& box
) const
{
//    return !procBounds().contains(box);
  return !bFTreePtr_().findBox(box).empty();
}
bool mousse::backgroundMeshDecomposition::overlapsThisProcessor
(
  const point& centre,
  const scalar radiusSqr
) const
{
  //return bFTreePtr_().findAnyOverlap(centre, radiusSqr);
  return bFTreePtr_().findNearest(centre, radiusSqr).hit();
}
mousse::pointIndexHit mousse::backgroundMeshDecomposition::findLine
(
  const point& start,
  const point& end
) const
{
  return bFTreePtr_().findLine(start, end);
}
mousse::pointIndexHit mousse::backgroundMeshDecomposition::findLineAny
(
  const point& start,
  const point& end
) const
{
  return bFTreePtr_().findLineAny(start, end);
}
mousse::labelList mousse::backgroundMeshDecomposition::processorNearestPosition
(
  const List<point>& pts
) const
{
  DynamicList<label> toCandidateProc;
  DynamicList<point> testPoints;
  labelList ptBlockStart(pts.size(), -1);
  labelList ptBlockSize(pts.size(), -1);
  label nTotalCandidates = 0;
  forAll(pts, pI)
  {
    const point& pt = pts[pI];
    label nCandidates = 0;
    forAll(allBackgroundMeshBounds_, procI)
    {
      // Candidate points may lie just outside a processor box, increase
      // test range by using overlaps rather than contains
      if (allBackgroundMeshBounds_[procI].overlaps(pt, sqr(SMALL*100)))
      {
        toCandidateProc.append(procI);
        testPoints.append(pt);
        nCandidates++;
      }
    }
    ptBlockStart[pI] = nTotalCandidates;
    ptBlockSize[pI] = nCandidates;
    nTotalCandidates += nCandidates;
  }
  // Needed for reverseDistribute
  label preDistributionToCandidateProcSize = toCandidateProc.size();
  autoPtr<mapDistribute> map(buildMap(toCandidateProc));
  map().distribute(testPoints);
  List<scalar> distanceSqrToCandidate(testPoints.size(), sqr(GREAT));
  // Test candidate points on candidate processors
  forAll(testPoints, tPI)
  {
    pointIndexHit info = bFTreePtr_().findNearest
    (
      testPoints[tPI],
      sqr(GREAT)
    );
    if (info.hit())
    {
      distanceSqrToCandidate[tPI] = magSqr
      (
        testPoints[tPI] - info.hitPoint()
      );
    }
  }
  map().reverseDistribute
  (
    preDistributionToCandidateProcSize,
    distanceSqrToCandidate
  );
  labelList ptNearestProc(pts.size(), -1);
  forAll(pts, pI)
  {
    // Extract the sub list of results for this point
    SubList<scalar> ptNearestProcResults
    (
      distanceSqrToCandidate,
      ptBlockSize[pI],
      ptBlockStart[pI]
    );
    scalar nearestProcDistSqr = GREAT;
    forAll(ptNearestProcResults, pPRI)
    {
      if (ptNearestProcResults[pPRI] < nearestProcDistSqr)
      {
        nearestProcDistSqr = ptNearestProcResults[pPRI];
        ptNearestProc[pI] = toCandidateProc[ptBlockStart[pI] + pPRI];
      }
    }
    if (debug)
    {
      Pout<< pts[pI] << " nearestProcDistSqr " << nearestProcDistSqr
        << " ptNearestProc[pI] " << ptNearestProc[pI] << endl;
    }
    if (ptNearestProc[pI] < 0)
    {
      FatalErrorIn
      (
        "mousse::labelList"
        "mousse::backgroundMeshDecomposition::processorNearestPosition"
        "("
          "const List<point>& pts"
        ") const"
      )
        << "The position " << pts[pI]
        << " did not find a nearest point on the background mesh."
        << exit(FatalError);
    }
  }
  return ptNearestProc;
}
mousse::List<mousse::List<mousse::pointIndexHit> >
mousse::backgroundMeshDecomposition::intersectsProcessors
(
  const List<point>& starts,
  const List<point>& ends,
  bool includeOwnProcessor
) const
{
  DynamicList<label> toCandidateProc;
  DynamicList<point> testStarts;
  DynamicList<point> testEnds;
  labelList segmentBlockStart(starts.size(), -1);
  labelList segmentBlockSize(starts.size(), -1);
  label nTotalCandidates = 0;
  forAll(starts, sI)
  {
    const point& s = starts[sI];
    const point& e = ends[sI];
    // Dummy point for treeBoundBox::intersects
    point p(vector::zero);
    label nCandidates = 0;
    forAll(allBackgroundMeshBounds_, procI)
    {
      // It is assumed that the sphere in question overlaps the source
      // processor, so don't test it, unless includeOwnProcessor is true
      if
      (
        (includeOwnProcessor || procI != Pstream::myProcNo())
       && allBackgroundMeshBounds_[procI].intersects(s, e, p)
      )
      {
        toCandidateProc.append(procI);
        testStarts.append(s);
        testEnds.append(e);
        nCandidates++;
      }
    }
    segmentBlockStart[sI] = nTotalCandidates;
    segmentBlockSize[sI] = nCandidates;
    nTotalCandidates += nCandidates;
  }
  // Needed for reverseDistribute
  label preDistributionToCandidateProcSize = toCandidateProc.size();
  autoPtr<mapDistribute> map(buildMap(toCandidateProc));
  map().distribute(testStarts);
  map().distribute(testEnds);
  List<pointIndexHit> segmentIntersectsCandidate(testStarts.size());
  // Test candidate segments on candidate processors
  forAll(testStarts, sI)
  {
    const point& s = testStarts[sI];
    const point& e = testEnds[sI];
    // If the sphere finds some elements of the patch, then it overlaps
    segmentIntersectsCandidate[sI] = bFTreePtr_().findLine(s, e);
  }
  map().reverseDistribute
  (
    preDistributionToCandidateProcSize,
    segmentIntersectsCandidate
  );
  List<List<pointIndexHit> > segmentHitProcs(starts.size());
  // Working storage for assessing processors
  DynamicList<pointIndexHit> tmpProcHits;
  forAll(starts, sI)
  {
    tmpProcHits.clear();
    // Extract the sub list of results for this point
    SubList<pointIndexHit> segmentProcResults
    (
      segmentIntersectsCandidate,
      segmentBlockSize[sI],
      segmentBlockStart[sI]
    );
    forAll(segmentProcResults, sPRI)
    {
      if (segmentProcResults[sPRI].hit())
      {
        tmpProcHits.append(segmentProcResults[sPRI]);
        tmpProcHits.last().setIndex
        (
          toCandidateProc[segmentBlockStart[sI] + sPRI]
        );
      }
    }
    segmentHitProcs[sI] = tmpProcHits;
  }
  return segmentHitProcs;
}
bool mousse::backgroundMeshDecomposition::overlapsOtherProcessors
(
  const point& centre,
  const scalar& radiusSqr
) const
{
  forAll(allBackgroundMeshBounds_, procI)
  {
    if (bFTreePtr_().findNearest(centre, radiusSqr).hit())
    {
      return true;
    }
  }
  return false;
}
mousse::labelList mousse::backgroundMeshDecomposition::overlapProcessors
(
  const point& centre,
  const scalar radiusSqr
) const
{
  DynamicList<label> toProc(Pstream::nProcs());
  forAll(allBackgroundMeshBounds_, procI)
  {
    // Test against the bounding box of the processor
    if
    (
      procI != Pstream::myProcNo()
    && allBackgroundMeshBounds_[procI].overlaps(centre, radiusSqr)
    )
    {
      // Expensive test
//            if (bFTreePtr_().findNearest(centre, radiusSqr).hit())
      {
        toProc.append(procI);
      }
    }
  }
  return toProc;
}
//mousse::labelListList mousse::backgroundMeshDecomposition::overlapsProcessors
//(
//    const List<point>& centres,
//    const List<scalar>& radiusSqrs,
//    const Delaunay& T,
//    bool includeOwnProcessor
//) const
//{
//    DynamicList<label> toCandidateProc;
//    DynamicList<point> testCentres;
//    DynamicList<scalar> testRadiusSqrs;
//    labelList sphereBlockStart(centres.size(), -1);
//    labelList sphereBlockSize(centres.size(), -1);
//
//    label nTotalCandidates = 0;
//
//    forAll(centres, sI)
//    {
//        const point& c = centres[sI];
//        scalar rSqr = radiusSqrs[sI];
//
//        label nCandidates = 0;
//
//        forAll(allBackgroundMeshBounds_, procI)
//        {
//            // It is assumed that the sphere in question overlaps the source
//            // processor, so don't test it, unless includeOwnProcessor is true
//            if
//            (
//                (includeOwnProcessor || procI != Pstream::myProcNo())
//             && allBackgroundMeshBounds_[procI].overlaps(c, rSqr)
//            )
//            {
//                if (bFTreePtr_().findNearest(c, rSqr).hit())
//                {
//                    toCandidateProc.append(procI);
//                    testCentres.append(c);
//                    testRadiusSqrs.append(rSqr);
//
//                    nCandidates++;
//                }
//            }
//        }
//
//        sphereBlockStart[sI] = nTotalCandidates;
//        sphereBlockSize[sI] = nCandidates;
//
//        nTotalCandidates += nCandidates;
//    }
//
//    // Needed for reverseDistribute
////    label preDistributionToCandidateProcSize = toCandidateProc.size();
////
////    autoPtr<mapDistribute> map(buildMap(toCandidateProc));
////
////    map().distribute(testCentres);
////    map().distribute(testRadiusSqrs);
//
//    // @todo This is faster, but results in more vertices being referred
//    boolList sphereOverlapsCandidate(testCentres.size(), true);
////    boolList sphereOverlapsCandidate(testCentres.size(), false);
////
////    // Test candidate spheres on candidate processors
////    forAll(testCentres, sI)
////    {
////        const point& c = testCentres[sI];
////        const scalar rSqr = testRadiusSqrs[sI];
////
////        const bool flagOverlap = bFTreePtr_().findNearest(c, rSqr).hit();
////
////        if (flagOverlap)
////        {
////            //if (vertexOctree.findAnyOverlap(c, rSqr))
//////            if (vertexOctree.findNearest(c, rSqr*1.001).hit())
//////            {
//////                sphereOverlapsCandidate[sI] = true;
//////            }
////
//////            Vertex_handle nearestVertex = T.nearest_vertex
//////            (
//////                toPoint<Point>(c)
//////            );
//////
//////            const scalar distSqr = magSqr
//////            (
//////                topoint(nearestVertex->point()) - c
//////            );
//////
//////            if (distSqr <= rSqr)
//////            {
//////                // If the sphere finds a nearest element of the patch,
//////                // then it overlaps
////                sphereOverlapsCandidate[sI] = true;
//////            }
////        }
////    }
//
////    map().reverseDistribute
////    (
////        preDistributionToCandidateProcSize,
////        sphereOverlapsCandidate
////    );
//
//    labelListList sphereProcs(centres.size());
//
//    // Working storage for assessing processors
//    DynamicList<label> tmpProcs;
//
//    forAll(centres, sI)
//    {
//        tmpProcs.clear();
//
//        // Extract the sub list of results for this point
//
//        SubList<bool> sphereProcResults
//        (
//            sphereOverlapsCandidate,
//            sphereBlockSize[sI],
//            sphereBlockStart[sI]
//        );
//
//        forAll(sphereProcResults, sPRI)
//        {
//            if (sphereProcResults[sPRI])
//            {
//                tmpProcs.append(toCandidateProc[sphereBlockStart[sI] + sPRI]);
//            }
//        }
//
//        sphereProcs[sI] = tmpProcs;
//    }
//
//    return sphereProcs;
//}
//mousse::labelListList mousse::backgroundMeshDecomposition::overlapProcessors
//(
//    const point& cc,
//    const scalar rSqr
//) const
//{
//    DynamicList<label> toCandidateProc;
//    label sphereBlockStart(-1);
//    label sphereBlockSize(-1);
//
//    label nCandidates = 0;
//
//    forAll(allBackgroundMeshBounds_, procI)
//    {
//        // It is assumed that the sphere in question overlaps the source
//        // processor, so don't test it, unless includeOwnProcessor is true
//        if
//        (
//            (includeOwnProcessor || procI != Pstream::myProcNo())
//         && allBackgroundMeshBounds_[procI].overlaps(cc, rSqr)
//        )
//        {
//            toCandidateProc.append(procI);
//
//            nCandidates++;
//        }
//    }
//
//    sphereBlockSize = nCandidates;
//    nTotalCandidates += nCandidates;
//
//    // Needed for reverseDistribute
//    label preDistributionToCandidateProcSize = toCandidateProc.size();
//
//    autoPtr<mapDistribute> map(buildMap(toCandidateProc));
//
//    map().distribute(testCentres);
//    map().distribute(testRadiusSqrs);
//
//    // @todo This is faster, but results in more vertices being referred
////    boolList sphereOverlapsCandidate(testCentres.size(), true);
//    boolList sphereOverlapsCandidate(testCentres.size(), false);
//
//    // Test candidate spheres on candidate processors
//    forAll(testCentres, sI)
//    {
//        const point& c = testCentres[sI];
//        const scalar rSqr = testRadiusSqrs[sI];
//
//        const bool flagOverlap = bFTreePtr_().findNearest(c, rSqr).hit();
//
//        if (flagOverlap)
//        {
//            //if (vertexOctree.findAnyOverlap(c, rSqr))
////            if (vertexOctree.findNearest(c, rSqr*1.001).hit())
////            {
////                sphereOverlapsCandidate[sI] = true;
////            }
//
////            Vertex_handle nearestVertex = T.nearest_vertex
////            (
////                toPoint<Point>(c)
////            );
////
////            const scalar distSqr = magSqr
////            (
////                topoint(nearestVertex->point()) - c
////            );
////
////            if (distSqr <= rSqr)
////            {
////                // If the sphere finds a nearest element of the patch, then
////                // it overlaps
//                sphereOverlapsCandidate[sI] = true;
////            }
//        }
//    }
//
//    map().reverseDistribute
//    (
//        preDistributionToCandidateProcSize,
//        sphereOverlapsCandidate
//    );
//
//    labelListList sphereProcs(centres.size());
//
//    // Working storage for assessing processors
//    DynamicList<label> tmpProcs;
//
//    forAll(centres, sI)
//    {
//        tmpProcs.clear();
//
//        // Extract the sub list of results for this point
//
//        SubList<bool> sphereProcResults
//        (
//            sphereOverlapsCandidate,
//            sphereBlockSize[sI],
//            sphereBlockStart[sI]
//        );
//
//        forAll(sphereProcResults, sPRI)
//        {
//            if (sphereProcResults[sPRI])
//            {
//                tmpProcs.append(toCandidateProc[sphereBlockStart[sI] + sPRI]);
//            }
//        }
//
//        sphereProcs[sI] = tmpProcs;
//    }
//
//    return sphereProcs;
//}
