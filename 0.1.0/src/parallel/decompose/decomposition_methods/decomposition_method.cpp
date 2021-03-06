// mousse: CFD toolbox
// Copyright (C) 2011-2014 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "decomposition_method.hpp"
#include "global_index.hpp"
#include "sync_tools.hpp"
#include "tuple2.hpp"
#include "face_set.hpp"
#include "region_split.hpp"
#include "local_point_region.hpp"
#include "min_data.hpp"
#include "face_cell_wave.hpp"


// Static Data Members
namespace mousse {

DEFINE_TYPE_NAME_AND_DEBUG(decompositionMethod, 0);
DEFINE_RUN_TIME_SELECTION_TABLE(decompositionMethod, dictionary);

}


// Member Functions 
mousse::autoPtr<mousse::decompositionMethod> mousse::decompositionMethod::New
(
  const dictionary& decompositionDict
)
{
  word methodType{decompositionDict.lookup("method")};
  if (methodType == "scotch" && Pstream::parRun()) {
    methodType = "ptscotch";
  }
  Info << "Selecting decompositionMethod " << methodType << endl;
  dictionaryConstructorTable::iterator cstrIter =
    dictionaryConstructorTablePtr_->find(methodType);
  if (cstrIter == dictionaryConstructorTablePtr_->end()) {
    FATAL_ERROR_IN
    (
      "decompositionMethod::New"
      "(const dictionary& decompositionDict)"
    )
    << "Unknown decompositionMethod "
    << methodType << nl << nl
    << "Valid decompositionMethods are : " << endl
    << dictionaryConstructorTablePtr_->sortedToc()
    << exit(FatalError);
  }
  return autoPtr<decompositionMethod>(cstrIter()(decompositionDict));
}


mousse::labelList mousse::decompositionMethod::decompose
(
  const polyMesh& mesh,
  const pointField& points
)
{
  scalarField weights(points.size(), 1.0);
  return decompose(mesh, points, weights);
}


mousse::labelList mousse::decompositionMethod::decompose
(
  const polyMesh& mesh,
  const labelList& fineToCoarse,
  const pointField& coarsePoints,
  const scalarField& coarseWeights
)
{
  CompactListList<label> coarseCellCells;
  calcCellCells
    (
      mesh,
      fineToCoarse,
      coarsePoints.size(),
      true,                       // use global cell labels
      coarseCellCells
    );
  // Decompose based on agglomerated points
  labelList coarseDistribution
  {
    decompose(coarseCellCells(), coarsePoints, coarseWeights)
  };
  // Rework back into decomposition for original mesh_
  labelList fineDistribution{fineToCoarse.size()};
  FOR_ALL(fineDistribution, i) {
    fineDistribution[i] = coarseDistribution[fineToCoarse[i]];
  }
  return fineDistribution;
}


mousse::labelList mousse::decompositionMethod::decompose
(
  const polyMesh& mesh,
  const labelList& fineToCoarse,
  const pointField& coarsePoints
)
{
  scalarField cWeights{coarsePoints.size(), 1.0};
  return decompose(mesh, fineToCoarse, coarsePoints, cWeights);
}


mousse::labelList mousse::decompositionMethod::decompose
(
  const labelListList& globalCellCells,
  const pointField& cc
)
{
  scalarField cWeights{cc.size(), 1.0};
  return decompose(globalCellCells, cc, cWeights);
}


void mousse::decompositionMethod::calcCellCells
(
  const polyMesh& mesh,
  const labelList& agglom,
  const label nLocalCoarse,
  const bool parallel,
  CompactListList<label>& cellCells
)
{
  const labelList& faceOwner = mesh.faceOwner();
  const labelList& faceNeighbour = mesh.faceNeighbour();
  const polyBoundaryMesh& patches = mesh.boundaryMesh();
  // Create global cell numbers
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~
  globalIndex
    globalAgglom{nLocalCoarse, Pstream::msgType(), Pstream::worldComm, parallel};
  // Get agglomerate owner on other side of coupled faces
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  labelList globalNeighbour{mesh.nFaces()-mesh.nInternalFaces()};
  FOR_ALL(patches, patchI) {
    const polyPatch& pp = patches[patchI];
    if (pp.coupled() && (parallel || !isA<processorPolyPatch>(pp))) {
      label faceI = pp.start();
      label bFaceI = pp.start() - mesh.nInternalFaces();
      FOR_ALL(pp, i) {
        globalNeighbour[bFaceI] =
          globalAgglom.toGlobal(agglom[faceOwner[faceI]]);
        bFaceI++;
        faceI++;
      }
    }
  }
  // Get the cell on the other side of coupled patches
  syncTools::swapBoundaryFaceList(mesh, globalNeighbour);
  // Count number of faces (internal + coupled)
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Number of faces per coarse cell
  labelList nFacesPerCell(nLocalCoarse, 0);
  for (label faceI = 0; faceI < mesh.nInternalFaces(); faceI++) {
    label own = agglom[faceOwner[faceI]];
    label nei = agglom[faceNeighbour[faceI]];
    nFacesPerCell[own]++;
    nFacesPerCell[nei]++;
  }
  FOR_ALL(patches, patchI) {
    const polyPatch& pp = patches[patchI];
    if (pp.coupled() && (parallel || !isA<processorPolyPatch>(pp))) {
      label faceI = pp.start();
      label bFaceI = pp.start() - mesh.nInternalFaces();
      FOR_ALL(pp, i) {
        label own = agglom[faceOwner[faceI]];
        label globalNei = globalNeighbour[bFaceI];
        if (!globalAgglom.isLocal(globalNei)
            || globalAgglom.toLocal(globalNei) != own) {
          nFacesPerCell[own]++;
        }
        faceI++;
        bFaceI++;
      }
    }
  }
  // Fill in offset and data
  cellCells.setSize(nFacesPerCell);
  nFacesPerCell = 0;
  labelList& m = cellCells.m();
  const labelList& offsets = cellCells.offsets();
  // For internal faces is just offsetted owner and neighbour
  for (label faceI = 0; faceI < mesh.nInternalFaces(); faceI++) {
    label own = agglom[faceOwner[faceI]];
    label nei = agglom[faceNeighbour[faceI]];
    m[offsets[own] + nFacesPerCell[own]++] = globalAgglom.toGlobal(nei);
    m[offsets[nei] + nFacesPerCell[nei]++] = globalAgglom.toGlobal(own);
  }
  // For boundary faces is offsetted coupled neighbour
  FOR_ALL(patches, patchI) {
    const polyPatch& pp = patches[patchI];
    if (pp.coupled() && (parallel || !isA<processorPolyPatch>(pp))) {
      label faceI = pp.start();
      label bFaceI = pp.start()-mesh.nInternalFaces();
      FOR_ALL(pp, i) {
        label own = agglom[faceOwner[faceI]];
        label globalNei = globalNeighbour[bFaceI];
        if (!globalAgglom.isLocal(globalNei)
            || globalAgglom.toLocal(globalNei) != own) {
          m[offsets[own] + nFacesPerCell[own]++] = globalNei;
        }
        faceI++;
        bFaceI++;
      }
    }
  }
  // Check for duplicates connections between cells
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Done as postprocessing step since we now have cellCells.
  label newIndex = 0;
  labelHashSet nbrCells;
  if (cellCells.size() == 0) {
    return;
  }
  label startIndex = cellCells.offsets()[0];
  FOR_ALL(cellCells, cellI) {
    nbrCells.clear();
    nbrCells.insert(globalAgglom.toGlobal(cellI));
    label endIndex = cellCells.offsets()[cellI+1];
    for (label i = startIndex; i < endIndex; i++) {
      if (nbrCells.insert(cellCells.m()[i])) {
        cellCells.m()[newIndex++] = cellCells.m()[i];
      }
    }
    startIndex = endIndex;
    cellCells.offsets()[cellI+1] = newIndex;
  }
  cellCells.m().setSize(newIndex);
}


mousse::labelList mousse::decompositionMethod::decompose
(
  const polyMesh& mesh,
  const scalarField& cellWeights,
  //- Whether owner and neighbour should be on same processor
  //  (takes priority over explicitConnections)
  const boolList& blockedFace,
  //- Whether whole sets of faces (and point neighbours) need to be kept
  //  on single processor
  const PtrList<labelList>& specifiedProcessorFaces,
  const labelList& specifiedProcessor,
  //- Additional connections between boundary faces
  const List<labelPair>& explicitConnections
)
{
  // Any weights specified?
  label nWeights = returnReduce(cellWeights.size(), sumOp<label>());
  if (nWeights > 0 && cellWeights.size() != mesh.nCells()) {
    FATAL_ERROR_IN
    (
      "decompositionMethod::decompose\n"
      "(\n"
      "   const polyMesh&,\n"
      "   const scalarField&,\n"
      "   const boolList&,\n"
      "   const PtrList<labelList>&,\n"
      "   const labelList&,\n"
      "   const List<labelPair>&\n"
    )
    << "Number of weights " << cellWeights.size()
    << " differs from number of cells " << mesh.nCells()
    << exit(FatalError);
  }
  // Any processor sets?
  label nProcSets = 0;
  FOR_ALL(specifiedProcessorFaces, setI) {
    nProcSets += specifiedProcessorFaces[setI].size();
  }
  reduce(nProcSets, sumOp<label>());
  // Any non-mesh connections?
  label nConnections =
    returnReduce(explicitConnections.size(), sumOp<label>());
  // Any faces not blocked?
  label nUnblocked = 0;
  FOR_ALL(blockedFace, faceI) {
    if (!blockedFace[faceI]) {
      nUnblocked++;
    }
  }
  reduce(nUnblocked, sumOp<label>());
  // Either do decomposition on cell centres or on agglomeration
  labelList finalDecomp;
  if (nProcSets+nConnections+nUnblocked == 0) {
    // No constraints, possibly weights
    if (nWeights > 0) {
      finalDecomp = decompose(mesh, mesh.cellCentres(), cellWeights);
    } else {
      finalDecomp = decompose(mesh, mesh.cellCentres());
    }
  } else {
    if (debug) {
      Info << "Constrained decomposition:" << endl
        << "    faces with same owner and neighbour processor : "
        << nUnblocked << endl
        << "    baffle faces with same owner processor        : "
        << nConnections << endl
        << "    faces all on same processor                   : "
        << nProcSets << endl << endl;
    }
    // Determine local regions, separated by blockedFaces
    regionSplit localRegion(mesh, blockedFace, explicitConnections, false);
    if (debug) {
      Info << "Constrained decomposition:" << endl
        << "    split into " << localRegion.nLocalRegions()
        << " regions."
        << endl;
    }
    // Determine region cell centres
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // This just takes the first cell in the region. Otherwise the problem
    // is with cyclics - if we'd average the region centre might be
    // somewhere in the middle of the domain which might not be anywhere
    // near any of the cells.
    pointField regionCentres{localRegion.nLocalRegions(), point::max};
    FOR_ALL(localRegion, cellI) {
      label regionI = localRegion[cellI];
      if (regionCentres[regionI] == point::max) {
        regionCentres[regionI] = mesh.cellCentres()[cellI];
      }
    }
    // Do decomposition on agglomeration
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    scalarField regionWeights{localRegion.nLocalRegions(), 0};
    if (nWeights > 0) {
      FOR_ALL(localRegion, cellI) {
        label regionI = localRegion[cellI];
        regionWeights[regionI] += cellWeights[cellI];
      }
    } else {
      FOR_ALL(localRegion, cellI) {
        label regionI = localRegion[cellI];
        regionWeights[regionI] += 1.0;
      }
    }
    finalDecomp = decompose(mesh, localRegion, regionCentres, regionWeights);
    // Implement the explicitConnections since above decompose
    // does not know about them
    FOR_ALL(explicitConnections, i) {
      const labelPair& baffle = explicitConnections[i];
      label f0 = baffle.first();
      label f1 = baffle.second();
      if (!blockedFace[f0] && !blockedFace[f1]) {
        // Note: what if internal faces and owner and neighbour on
        // different processor? So for now just push owner side
        // proc
        const label procI = finalDecomp[mesh.faceOwner()[f0]];
        finalDecomp[mesh.faceOwner()[f1]] = procI;
        if (mesh.isInternalFace(f1)) {
          finalDecomp[mesh.faceNeighbour()[f1]] = procI;
        }
      } else if (blockedFace[f0] != blockedFace[f1]) {
        FATAL_ERROR_IN
        (
          "labelList decompose\n"
          "(\n"
          "    const polyMesh&,\n"
          "    const scalarField&,\n"
          "    const boolList&,\n"
          "    const PtrList<labelList>&,\n"
          "    const labelList&,\n"
          "    const List<labelPair>&\n"
          ")"
        )
        << "On explicit connection between faces " << f0
        << " and " << f1
        << " the two blockedFace status are not equal : "
        << blockedFace[f0] << " and " << blockedFace[f1]
        << exit(FatalError);
      }
    }
    // blockedFaces corresponding to processor faces need to be handled
    // separately since not handled by local regionSplit. We need to
    // walk now across coupled faces and make sure to move a whole
    // global region across
    if (Pstream::parRun()) {
      // Re-do regionSplit
      // Field on cells and faces.
      List<minData> cellData{mesh.nCells()};
      List<minData> faceData{mesh.nFaces()};
      // Take over blockedFaces by seeding a negative number
      // (so is always less than the decomposition)
      label nUnblocked = 0;
      FOR_ALL(blockedFace, faceI) {
        if (blockedFace[faceI]) {
          faceData[faceI] = minData(-123);
        } else {
          nUnblocked++;
        }
      }
      // Seed unblocked faces with destination processor
      labelList seedFaces{nUnblocked};
      List<minData> seedData{nUnblocked};
      nUnblocked = 0;
      FOR_ALL(blockedFace, faceI) {
        if (!blockedFace[faceI]) {
          label own = mesh.faceOwner()[faceI];
          seedFaces[nUnblocked] = faceI;
          seedData[nUnblocked] = minData(finalDecomp[own]);
          nUnblocked++;
        }
      }
      // Propagate information inwards
      FaceCellWave<minData> deltaCalc
        (
          mesh,
          seedFaces,
          seedData,
          faceData,
          cellData,
          mesh.globalData().nTotalCells()+1
        );
      // And extract
      FOR_ALL(finalDecomp, cellI) {
        if (cellData[cellI].valid(deltaCalc.data())) {
          finalDecomp[cellI] = cellData[cellI].data();
        }
      }
    }
    // For specifiedProcessorFaces rework the cellToProc to enforce
    // all on one processor since we can't guarantee that the input
    // to regionSplit was a single region.
    // E.g. faceSet 'a' with the cells split into two regions
    // by a notch formed by two walls
    //
    //          \   /
    //           \ /
    //    ---a----+-----a-----
    //
    //
    // Note that reworking the cellToProc might make the decomposition
    // unbalanced.
    FOR_ALL(specifiedProcessorFaces, setI) {
      const labelList& set = specifiedProcessorFaces[setI];
      label procI = specifiedProcessor[setI];
      if (procI == -1) {
        // If no processor specified use the one from the
        // 0th element
        procI = finalDecomp[mesh.faceOwner()[set[0]]];
      }
      FOR_ALL(set, fI) {
        const face& f = mesh.faces()[set[fI]];
        FOR_ALL(f, fp) {
          const labelList& pFaces = mesh.pointFaces()[f[fp]];
          FOR_ALL(pFaces, i) {
            label faceI = pFaces[i];
            finalDecomp[mesh.faceOwner()[faceI]] = procI;
            if (mesh.isInternalFace(faceI)) {
              finalDecomp[mesh.faceNeighbour()[faceI]] = procI;
            }
          }
        }
      }
    }
    if (debug && Pstream::parRun()) {
      labelList nbrDecomp;
      syncTools::swapBoundaryCellList(mesh, finalDecomp, nbrDecomp);
      const polyBoundaryMesh& patches = mesh.boundaryMesh();
      FOR_ALL(patches, patchI) {
        const polyPatch& pp = patches[patchI];
        if (pp.coupled()) {
          FOR_ALL(pp, i) {
            label faceI = pp.start()+i;
            label own = mesh.faceOwner()[faceI];
            label bFaceI = faceI-mesh.nInternalFaces();
            if (!blockedFace[faceI]) {
              label ownProc = finalDecomp[own];
              label nbrProc = nbrDecomp[bFaceI];
              if (ownProc != nbrProc) {
                FATAL_ERROR_IN("decompositionMethod::decompose()")
                  << "patch:" << pp.name()
                  << " face:" << faceI
                  << " at:" << mesh.faceCentres()[faceI]
                  << " ownProc:" << ownProc
                  << " nbrProc:" << nbrProc
                  << exit(FatalError);
              }
            }
          }
        }
      }
    }
  }
  return finalDecomp;
}


void mousse::decompositionMethod::setConstraints
(
  const polyMesh& mesh,
  boolList& blockedFace,
  PtrList<labelList>& specifiedProcessorFaces,
  labelList& specifiedProcessor,
  List<labelPair>& explicitConnections
)
{
  blockedFace.setSize(mesh.nFaces());
  blockedFace = true;
  //label nUnblocked = 0;
  specifiedProcessorFaces.clear();
  explicitConnections.clear();
  if (decompositionDict_.found("preservePatches")) {
    wordList pNames{decompositionDict_.lookup("preservePatches")};
    Info << nl
      << "Keeping owner of faces in patches " << pNames
      << " on same processor. This only makes sense for cyclics." << endl;
    const polyBoundaryMesh& patches = mesh.boundaryMesh();
    FOR_ALL(pNames, i) {
      const label patchI = patches.findPatchID(pNames[i]);
      if (patchI == -1) {
        FATAL_ERROR_IN("decompositionMethod::decompose(const polyMesh&)")
          << "Unknown preservePatch " << pNames[i]
          << endl << "Valid patches are " << patches.names()
          << exit(FatalError);
      }
      const polyPatch& pp = patches[patchI];
      FOR_ALL(pp, i) {
        if (blockedFace[pp.start() + i]) {
          blockedFace[pp.start() + i] = false;
          //nUnblocked++;
        }
      }
    }
  }
  if (decompositionDict_.found("preserveFaceZones")) {
    wordList zNames{decompositionDict_.lookup("preserveFaceZones")};
    Info << nl << "Keeping owner and neighbour of faces in zones " << zNames
      << " on same processor" << endl;
    const faceZoneMesh& fZones = mesh.faceZones();
    FOR_ALL(zNames, i) {
      label zoneI = fZones.findZoneID(zNames[i]);
      if (zoneI == -1) {
        FATAL_ERROR_IN("decompositionMethod::decompose(const polyMesh&)")
          << "Unknown preserveFaceZone " << zNames[i]
          << endl << "Valid faceZones are " << fZones.names()
          << exit(FatalError);
      }
      const faceZone& fz = fZones[zoneI];
      FOR_ALL(fz, i) {
        if (blockedFace[fz[i]]) {
          blockedFace[fz[i]] = false;
        }
      }
    }
  }
  bool preserveBaffles =
    decompositionDict_.lookupOrDefault("preserveBaffles", false);
  if (preserveBaffles) {
    Info << nl << "Keeping owner of faces in baffles "
      << " on same processor." << endl;
    explicitConnections = localPointRegion::findDuplicateFacePairs(mesh);
    FOR_ALL(explicitConnections, i) {
      blockedFace[explicitConnections[i].first()] = false;
      blockedFace[explicitConnections[i].second()] = false;
    }
  }
  if (decompositionDict_.found("preservePatches")
      || decompositionDict_.found("preserveFaceZones")
      || preserveBaffles) {
    syncTools::syncFaceList(mesh, blockedFace, andEqOp<bool>());
    //reduce(nUnblocked, sumOp<label>());
  }
  // Specified processor for group of cells connected to faces
  label nProcSets = 0;
  if (decompositionDict_.found("singleProcessorFaceSets")) {
    List<Tuple2<word, label>> zNameAndProcs
    {
      decompositionDict_.lookup("singleProcessorFaceSets")
    };
    specifiedProcessorFaces.setSize(zNameAndProcs.size());
    specifiedProcessor.setSize(zNameAndProcs.size());
    FOR_ALL(zNameAndProcs, setI) {
      Info << "Keeping all cells connected to faceSet "
        << zNameAndProcs[setI].first()
        << " on processor " << zNameAndProcs[setI].second() << endl;
      // Read faceSet
      faceSet fz{mesh, zNameAndProcs[setI].first()};
      specifiedProcessorFaces.set(setI, new labelList(fz.sortedToc()));
      specifiedProcessor[setI] = zNameAndProcs[setI].second();
      nProcSets += fz.size();
    }
    reduce(nProcSets, sumOp<label>());
    // Unblock all point connected faces
    // 1. Mark all points on specifiedProcessorFaces
    boolList procFacePoint{mesh.nPoints(), false};
    FOR_ALL(specifiedProcessorFaces, setI) {
      const labelList& set = specifiedProcessorFaces[setI];
      FOR_ALL(set, fI) {
        const face& f = mesh.faces()[set[fI]];
        FOR_ALL(f, fp) {
          procFacePoint[f[fp]] = true;
        }
      }
    }
    syncTools::syncPointList(mesh, procFacePoint, orEqOp<bool>(), false);
    // 2. Unblock all faces on procFacePoint
    FOR_ALL(procFacePoint, pointI) {
      if (procFacePoint[pointI]) {
        const labelList& pFaces = mesh.pointFaces()[pointI];
        FOR_ALL(pFaces, i) {
          blockedFace[pFaces[i]] = false;
        }
      }
    }
    syncTools::syncFaceList(mesh, blockedFace, andEqOp<bool>());
  }
}


mousse::labelList mousse::decompositionMethod::decompose
(
  const polyMesh& mesh,
  const scalarField& cellWeights
)
{
  boolList blockedFace;
  PtrList<labelList> specifiedProcessorFaces;
  labelList specifiedProcessor;
  List<labelPair> explicitConnections;
  setConstraints
    (
      mesh,
      blockedFace,
      specifiedProcessorFaces,
      specifiedProcessor,
      explicitConnections
    );
  // Construct decomposition method and either do decomposition on
  // cell centres or on agglomeration
  labelList finalDecomp =
    decompose
    (
      mesh,
      cellWeights,            // optional weights
      blockedFace,            // any cells to be combined
      specifiedProcessorFaces,// any whole cluster of cells to be kept
      specifiedProcessor,
      explicitConnections     // baffles
    );
  return finalDecomp;
}

