// mousse: CFD toolbox
// Copyright (C) 2011-2014 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "fv_mesh_distribute.hpp"
#include "pstream_combine_reduce_ops.hpp"
#include "fv_mesh_adder.hpp"
#include "face_couple_info.hpp"
#include "processor_fv_patch_field.hpp"
#include "processor_fvs_patch_field.hpp"
#include "processor_cyclic_poly_patch.hpp"
#include "processor_cyclic_fv_patch_field.hpp"
#include "poly_topo_change.hpp"
#include "remove_cells.hpp"
#include "poly_modify_face.hpp"
#include "poly_remove_point.hpp"
#include "map_distribute_poly_mesh.hpp"
#include "surface_fields.hpp"
#include "sync_tools.hpp"
#include "compact_list_list.hpp"
#include "fv_mesh_tools.hpp"
// Static Data Members
namespace mousse
{
DEFINE_TYPE_NAME_AND_DEBUG(fvMeshDistribute, 0);
}
// Private Member Functions 
mousse::labelList mousse::fvMeshDistribute::select
(
  const bool selectEqual,
  const labelList& values,
  const label value
)
{
  label n = 0;
  FOR_ALL(values, i)
  {
    if (selectEqual == (values[i] == value))
    {
      n++;
    }
  }
  labelList indices(n);
  n = 0;
  FOR_ALL(values, i)
  {
    if (selectEqual == (values[i] == value))
    {
      indices[n++] = i;
    }
  }
  return indices;
}
// Check all procs have same names and in exactly same order.
void mousse::fvMeshDistribute::checkEqualWordList
(
  const string& msg,
  const wordList& lst
)
{
  List<wordList> allNames(Pstream::nProcs());
  allNames[Pstream::myProcNo()] = lst;
  Pstream::gatherList(allNames);
  Pstream::scatterList(allNames);
  for (label procI = 1; procI < Pstream::nProcs(); procI++)
  {
    if (allNames[procI] != allNames[0])
    {
      FATAL_ERROR_IN("fvMeshDistribute::checkEqualWordList(..)")
        << "When checking for equal " << msg.c_str() << " :" << endl
        << "processor0 has:" << allNames[0] << endl
        << "processor" << procI << " has:" << allNames[procI] << endl
        << msg.c_str() << " need to be synchronised on all processors."
        << exit(FatalError);
    }
  }
}
mousse::wordList mousse::fvMeshDistribute::mergeWordList(const wordList& procNames)
{
  List<wordList> allNames(Pstream::nProcs());
  allNames[Pstream::myProcNo()] = procNames;
  Pstream::gatherList(allNames);
  Pstream::scatterList(allNames);
  HashSet<word> mergedNames;
  FOR_ALL(allNames, procI)
  {
    FOR_ALL(allNames[procI], i)
    {
      mergedNames.insert(allNames[procI][i]);
    }
  }
  return mergedNames.toc();
}
// Print some info on mesh.
void mousse::fvMeshDistribute::printMeshInfo(const fvMesh& mesh)
{
  Pout<< "Primitives:" << nl
    << "    points       :" << mesh.nPoints() << nl
    << "    bb           :" << boundBox(mesh.points(), false) << nl
    << "    internalFaces:" << mesh.nInternalFaces() << nl
    << "    faces        :" << mesh.nFaces() << nl
    << "    cells        :" << mesh.nCells() << nl;
  const fvBoundaryMesh& patches = mesh.boundary();
  Pout<< "Patches:" << endl;
  FOR_ALL(patches, patchI)
  {
    const polyPatch& pp = patches[patchI].patch();
    Pout<< "    " << patchI << " name:" << pp.name()
      << " size:" << pp.size()
      << " start:" << pp.start()
      << " type:" << pp.type()
      << endl;
  }
  if (mesh.pointZones().size())
  {
    Pout<< "PointZones:" << endl;
    FOR_ALL(mesh.pointZones(), zoneI)
    {
      const pointZone& pz = mesh.pointZones()[zoneI];
      Pout<< "    " << zoneI << " name:" << pz.name()
        << " size:" << pz.size()
        << endl;
    }
  }
  if (mesh.faceZones().size())
  {
    Pout<< "FaceZones:" << endl;
    FOR_ALL(mesh.faceZones(), zoneI)
    {
      const faceZone& fz = mesh.faceZones()[zoneI];
      Pout<< "    " << zoneI << " name:" << fz.name()
        << " size:" << fz.size()
        << endl;
    }
  }
  if (mesh.cellZones().size())
  {
    Pout<< "CellZones:" << endl;
    FOR_ALL(mesh.cellZones(), zoneI)
    {
      const cellZone& cz = mesh.cellZones()[zoneI];
      Pout<< "    " << zoneI << " name:" << cz.name()
        << " size:" << cz.size()
        << endl;
    }
  }
}
void mousse::fvMeshDistribute::printCoupleInfo
(
  const primitiveMesh& mesh,
  const labelList& sourceFace,
  const labelList& sourceProc,
  const labelList& /*sourcePatch*/,
  const labelList& sourceNewNbrProc
)
{
  Pout<< nl
    << "Current coupling info:"
    << endl;
  FOR_ALL(sourceFace, bFaceI)
  {
    label meshFaceI = mesh.nInternalFaces() + bFaceI;
    Pout<< "    meshFace:" << meshFaceI
      << " fc:" << mesh.faceCentres()[meshFaceI]
      << " connects to proc:" << sourceProc[bFaceI]
      << "/face:" << sourceFace[bFaceI]
      << " which will move to proc:" << sourceNewNbrProc[bFaceI]
      << endl;
  }
}
// Finds (non-empty) patch that exposed internal and proc faces can be put into.
mousse::label mousse::fvMeshDistribute::findNonEmptyPatch() const
{
  const polyBoundaryMesh& patches = mesh_.boundaryMesh();
  label nonEmptyPatchI = -1;
  FOR_ALL_REVERSE(patches, patchI)
  {
    const polyPatch& pp = patches[patchI];
    if (!isA<emptyPolyPatch>(pp) && !pp.coupled())
    {
      nonEmptyPatchI = patchI;
      break;
    }
  }
  if (nonEmptyPatchI == -1)
  {
    FATAL_ERROR_IN("fvMeshDistribute::findNonEmptyPatch() const")
      << "Cannot find a patch which is neither of type empty nor"
      << " coupled in patches " << patches.names() << endl
      << "There has to be at least one such patch for"
      << " distribution to work" << abort(FatalError);
  }
  if (debug)
  {
    Pout<< "findNonEmptyPatch : using patch " << nonEmptyPatchI
      << " name:" << patches[nonEmptyPatchI].name()
      << " type:" << patches[nonEmptyPatchI].type()
      << " to put exposed faces into." << endl;
  }
  // Do additional test for processor patches intermingled with non-proc
  // patches.
  label procPatchI = -1;
  FOR_ALL(patches, patchI)
  {
    if (isA<processorPolyPatch>(patches[patchI]))
    {
      procPatchI = patchI;
    }
    else if (procPatchI != -1)
    {
      FATAL_ERROR_IN("fvMeshDistribute::findNonEmptyPatch() const")
        << "Processor patches should be at end of patch list."
        << endl
        << "Have processor patch " << procPatchI
        << " followed by non-processor patch " << patchI
        << " in patches " << patches.names()
        << abort(FatalError);
    }
  }
  return nonEmptyPatchI;
}
// Delete all processor patches. Move any processor faces into the last
// non-processor patch.
mousse::autoPtr<mousse::mapPolyMesh> mousse::fvMeshDistribute::deleteProcPatches
(
  const label destinationPatch
)
{
  // New patchID per boundary faces to be repatched. Is -1 (no change)
  // or new patchID
  labelList newPatchID(mesh_.nFaces() - mesh_.nInternalFaces(), -1);
  label nProcPatches = 0;
  FOR_ALL(mesh_.boundaryMesh(), patchI)
  {
    const polyPatch& pp = mesh_.boundaryMesh()[patchI];
    if (isA<processorPolyPatch>(pp))
    {
      if (debug)
      {
        Pout<< "Moving all faces of patch " << pp.name()
          << " into patch " << destinationPatch
          << endl;
      }
      label offset = pp.start() - mesh_.nInternalFaces();
      FOR_ALL(pp, i)
      {
        newPatchID[offset+i] = destinationPatch;
      }
      nProcPatches++;
    }
  }
  // Note: order of boundary faces been kept the same since the
  // destinationPatch is at the end and we have visited the patches in
  // incremental order.
  labelListList dummyFaceMaps;
  autoPtr<mapPolyMesh> map = repatch(newPatchID, dummyFaceMaps);
  // Delete (now empty) processor patches.
  {
    labelList oldToNew(identity(mesh_.boundaryMesh().size()));
    label newI = 0;
    // Non processor patches first
    FOR_ALL(mesh_.boundaryMesh(), patchI)
    {
      if (!isA<processorPolyPatch>(mesh_.boundaryMesh()[patchI]))
      {
        oldToNew[patchI] = newI++;
      }
    }
    label nNonProcPatches = newI;
    // Processor patches as last
    FOR_ALL(mesh_.boundaryMesh(), patchI)
    {
      if (isA<processorPolyPatch>(mesh_.boundaryMesh()[patchI]))
      {
        oldToNew[patchI] = newI++;
      }
    }
    fvMeshTools::reorderPatches(mesh_, oldToNew, nNonProcPatches, false);
  }
  return map;
}
// Repatch the mesh.
mousse::autoPtr<mousse::mapPolyMesh> mousse::fvMeshDistribute::repatch
(
  const labelList& newPatchID,         // per boundary face -1 or new patchID
  labelListList& constructFaceMap
)
{
  polyTopoChange meshMod(mesh_);
  FOR_ALL(newPatchID, bFaceI)
  {
    if (newPatchID[bFaceI] != -1)
    {
      label faceI = mesh_.nInternalFaces() + bFaceI;
      label zoneID = mesh_.faceZones().whichZone(faceI);
      bool zoneFlip = false;
      if (zoneID >= 0)
      {
        const faceZone& fZone = mesh_.faceZones()[zoneID];
        zoneFlip = fZone.flipMap()[fZone.whichFace(faceI)];
      }
      meshMod.setAction
      (
        polyModifyFace
        (
          mesh_.faces()[faceI],       // modified face
          faceI,                      // label of face
          mesh_.faceOwner()[faceI],   // owner
          -1,                         // neighbour
          false,                      // face flip
          newPatchID[bFaceI],         // patch for face
          false,                      // remove from zone
          zoneID,                     // zone for face
          zoneFlip                    // face flip in zone
        )
      );
    }
  }
  // Do mapping of fields from one patchField to the other ourselves since
  // is currently not supported by updateMesh.
  // Store boundary fields (we only do this for surfaceFields)
  PtrList<FieldField<fvsPatchField, scalar> > sFlds;
  saveBoundaryFields<scalar, surfaceMesh>(sFlds);
  PtrList<FieldField<fvsPatchField, vector> > vFlds;
  saveBoundaryFields<vector, surfaceMesh>(vFlds);
  PtrList<FieldField<fvsPatchField, sphericalTensor> > sptFlds;
  saveBoundaryFields<sphericalTensor, surfaceMesh>(sptFlds);
  PtrList<FieldField<fvsPatchField, symmTensor> > sytFlds;
  saveBoundaryFields<symmTensor, surfaceMesh>(sytFlds);
  PtrList<FieldField<fvsPatchField, tensor> > tFlds;
  saveBoundaryFields<tensor, surfaceMesh>(tFlds);
  // Change the mesh (no inflation). Note: parallel comms allowed.
  //
  // NOTE: there is one very particular problem with this ordering.
  // We first create the processor patches and use these to merge out
  // shared points (see mergeSharedPoints below). So temporarily points
  // and edges do not match!
  autoPtr<mapPolyMesh> map = meshMod.changeMesh(mesh_, false, true);
  // Update fields. No inflation, parallel sync.
  mesh_.updateMesh(map);
  // Map patch fields using stored boundary fields. Note: assumes order
  // of fields has not changed in object registry!
  mapBoundaryFields<scalar, surfaceMesh>(map, sFlds);
  mapBoundaryFields<vector, surfaceMesh>(map, vFlds);
  mapBoundaryFields<sphericalTensor, surfaceMesh>(map, sptFlds);
  mapBoundaryFields<symmTensor, surfaceMesh>(map, sytFlds);
  mapBoundaryFields<tensor, surfaceMesh>(map, tFlds);
  // Move mesh (since morphing does not do this)
  if (map().hasMotionPoints())
  {
    mesh_.movePoints(map().preMotionPoints());
  }
  // Adapt constructMaps.
  if (debug)
  {
    label index = findIndex(map().reverseFaceMap(), -1);
    if (index != -1)
    {
      FATAL_ERROR_IN
      (
        "fvMeshDistribute::repatch(const labelList&, labelListList&)"
      )   << "reverseFaceMap contains -1 at index:"
        << index << endl
        << "This means that the repatch operation was not just"
        << " a shuffle?" << abort(FatalError);
    }
  }
  FOR_ALL(constructFaceMap, procI)
  {
    inplaceRenumber(map().reverseFaceMap(), constructFaceMap[procI]);
  }
  return map;
}
// Detect shared points. Need processor patches to be present.
// Background: when adding bits of mesh one can get points which
// share the same position but are only detectable to be topologically
// the same point when doing parallel analysis. This routine will
// merge those points.
mousse::autoPtr<mousse::mapPolyMesh> mousse::fvMeshDistribute::mergeSharedPoints
(
  labelListList& constructPointMap
)
{
  // Find out which sets of points get merged and create a map from
  // mesh point to unique point.
  Map<label> pointToMaster
  (
    fvMeshAdder::findSharedPoints
    (
      mesh_,
      mergeTol_
    )
  );
  if (returnReduce(pointToMaster.size(), sumOp<label>()) == 0)
  {
    return autoPtr<mapPolyMesh>(NULL);
  }
  polyTopoChange meshMod(mesh_);
  fvMeshAdder::mergePoints(mesh_, pointToMaster, meshMod);
  // Change the mesh (no inflation). Note: parallel comms allowed.
  autoPtr<mapPolyMesh> map = meshMod.changeMesh(mesh_, false, true);
  // Update fields. No inflation, parallel sync.
  mesh_.updateMesh(map);
  // Adapt constructMaps for merged points.
  FOR_ALL(constructPointMap, procI)
  {
    labelList& constructMap = constructPointMap[procI];
    FOR_ALL(constructMap, i)
    {
      label oldPointI = constructMap[i];
      label newPointI = map().reversePointMap()[oldPointI];
      if (newPointI < -1)
      {
        constructMap[i] = -newPointI-2;
      }
      else if (newPointI >= 0)
      {
        constructMap[i] = newPointI;
      }
      else
      {
        FATAL_ERROR_IN("fvMeshDistribute::mergeSharedPoints()")
          << "Problem. oldPointI:" << oldPointI
          << " newPointI:" << newPointI << abort(FatalError);
      }
    }
  }
  return map;
}
// Construct the local environment of all boundary faces.
void mousse::fvMeshDistribute::getNeighbourData
(
  const labelList& distribution,
  labelList& sourceFace,
  labelList& sourceProc,
  labelList& sourcePatch,
  labelList& sourceNewNbrProc
) const
{
  label nBnd = mesh_.nFaces() - mesh_.nInternalFaces();
  sourceFace.setSize(nBnd);
  sourceProc.setSize(nBnd);
  sourcePatch.setSize(nBnd);
  sourceNewNbrProc.setSize(nBnd);
  const polyBoundaryMesh& patches = mesh_.boundaryMesh();
  // Get neighbouring meshFace labels and new processor of coupled boundaries.
  labelList nbrFaces(nBnd, -1);
  labelList nbrNewNbrProc(nBnd, -1);
  FOR_ALL(patches, patchI)
  {
    const polyPatch& pp = patches[patchI];
    if (pp.coupled())
    {
      label offset = pp.start() - mesh_.nInternalFaces();
      // Mesh labels of faces on this side
      FOR_ALL(pp, i)
      {
        label bndI = offset + i;
        nbrFaces[bndI] = pp.start()+i;
      }
      // Which processor they will end up on
      SubList<label>(nbrNewNbrProc, pp.size(), offset).assign
      (
        UIndirectList<label>(distribution, pp.faceCells())()
      );
    }
  }
  // Exchange the boundary data
  syncTools::swapBoundaryFaceList(mesh_, nbrFaces);
  syncTools::swapBoundaryFaceList(mesh_, nbrNewNbrProc);
  FOR_ALL(patches, patchI)
  {
    const polyPatch& pp = patches[patchI];
    label offset = pp.start() - mesh_.nInternalFaces();
    if (isA<processorPolyPatch>(pp))
    {
      const processorPolyPatch& procPatch =
        refCast<const processorPolyPatch>(pp);
      // Check which of the two faces we store.
      if (procPatch.owner())
      {
        // Use my local face labels
        FOR_ALL(pp, i)
        {
          label bndI = offset + i;
          sourceFace[bndI] = pp.start()+i;
          sourceProc[bndI] = Pstream::myProcNo();
          sourceNewNbrProc[bndI] = nbrNewNbrProc[bndI];
        }
      }
      else
      {
        // Use my neighbours face labels
        FOR_ALL(pp, i)
        {
          label bndI = offset + i;
          sourceFace[bndI] = nbrFaces[bndI];
          sourceProc[bndI] = procPatch.neighbProcNo();
          sourceNewNbrProc[bndI] = nbrNewNbrProc[bndI];
        }
      }
      label patchI = -1;
      if (isA<processorCyclicPolyPatch>(pp))
      {
        patchI = refCast<const processorCyclicPolyPatch>
        (
          pp
        ).referPatchID();
      }
      FOR_ALL(pp, i)
      {
        label bndI = offset + i;
        sourcePatch[bndI] = patchI;
      }
    }
    else if (isA<cyclicPolyPatch>(pp))
    {
      const cyclicPolyPatch& cpp = refCast<const cyclicPolyPatch>(pp);
      if (cpp.owner())
      {
        FOR_ALL(pp, i)
        {
          label bndI = offset + i;
          sourceFace[bndI] = pp.start()+i;
          sourceProc[bndI] = Pstream::myProcNo();
          sourcePatch[bndI] = patchI;
          sourceNewNbrProc[bndI] = nbrNewNbrProc[bndI];
        }
      }
      else
      {
        FOR_ALL(pp, i)
        {
          label bndI = offset + i;
          sourceFace[bndI] = nbrFaces[bndI];
          sourceProc[bndI] = Pstream::myProcNo();
          sourcePatch[bndI] = patchI;
          sourceNewNbrProc[bndI] = nbrNewNbrProc[bndI];
        }
      }
    }
    else
    {
      // Normal (physical) boundary
      FOR_ALL(pp, i)
      {
        label bndI = offset + i;
        sourceFace[bndI] = -1;
        sourceProc[bndI] = -1;
        sourcePatch[bndI] = patchI;
        sourceNewNbrProc[bndI] = -1;
      }
    }
  }
}
// Subset the neighbourCell/neighbourProc fields
void mousse::fvMeshDistribute::subsetBoundaryData
(
  const fvMesh& mesh,
  const labelList& faceMap,
  const labelList& cellMap,
  const labelList& oldDistribution,
  const labelList& oldFaceOwner,
  const labelList& oldFaceNeighbour,
  const label oldInternalFaces,
  const labelList& sourceFace,
  const labelList& sourceProc,
  const labelList& sourcePatch,
  const labelList& sourceNewNbrProc,
  labelList& subFace,
  labelList& subProc,
  labelList& subPatch,
  labelList& subNewNbrProc
)
{
  subFace.setSize(mesh.nFaces() - mesh.nInternalFaces());
  subProc.setSize(mesh.nFaces() - mesh.nInternalFaces());
  subPatch.setSize(mesh.nFaces() - mesh.nInternalFaces());
  subNewNbrProc.setSize(mesh.nFaces() - mesh.nInternalFaces());
  FOR_ALL(subFace, newBFaceI)
  {
    label newFaceI = newBFaceI + mesh.nInternalFaces();
    label oldFaceI = faceMap[newFaceI];
    // Was oldFaceI internal face? If so which side did we get.
    if (oldFaceI < oldInternalFaces)
    {
      subFace[newBFaceI] = oldFaceI;
      subProc[newBFaceI] = Pstream::myProcNo();
      subPatch[newBFaceI] = -1;
      label oldOwn = oldFaceOwner[oldFaceI];
      label oldNei = oldFaceNeighbour[oldFaceI];
      if (oldOwn == cellMap[mesh.faceOwner()[newFaceI]])
      {
        // We kept the owner side. Where does the neighbour move to?
        subNewNbrProc[newBFaceI] = oldDistribution[oldNei];
      }
      else
      {
        // We kept the neighbour side.
        subNewNbrProc[newBFaceI] = oldDistribution[oldOwn];
      }
    }
    else
    {
      // Was boundary face. Take over boundary information
      label oldBFaceI = oldFaceI - oldInternalFaces;
      subFace[newBFaceI] = sourceFace[oldBFaceI];
      subProc[newBFaceI] = sourceProc[oldBFaceI];
      subPatch[newBFaceI] = sourcePatch[oldBFaceI];
      subNewNbrProc[newBFaceI] = sourceNewNbrProc[oldBFaceI];
    }
  }
}
// Find cells on mesh whose faceID/procID match the neighbour cell/proc of
// domainMesh. Store the matching face.
void mousse::fvMeshDistribute::findCouples
(
  const primitiveMesh& mesh,
  const labelList& sourceFace,
  const labelList& sourceProc,
  const labelList& sourcePatch,
  const label /*domain*/,
  const primitiveMesh& domainMesh,
  const labelList& domainFace,
  const labelList& domainProc,
  const labelList& domainPatch,
  labelList& masterCoupledFaces,
  labelList& slaveCoupledFaces
)
{
  // Store domain neighbour as map so we can easily look for pair
  // with same face+proc.
  HashTable<label, labelPair, labelPair::Hash<> > map(domainFace.size());
  FOR_ALL(domainProc, bFaceI)
  {
    if (domainProc[bFaceI] != -1 && domainPatch[bFaceI] == -1)
    {
      map.insert
      (
        labelPair(domainFace[bFaceI], domainProc[bFaceI]),
        bFaceI
      );
    }
  }
  // Try to match mesh data.
  masterCoupledFaces.setSize(domainFace.size());
  slaveCoupledFaces.setSize(domainFace.size());
  label coupledI = 0;
  FOR_ALL(sourceFace, bFaceI)
  {
    if (sourceProc[bFaceI] != -1 && sourcePatch[bFaceI] == -1)
    {
      labelPair myData(sourceFace[bFaceI], sourceProc[bFaceI]);
      HashTable<label, labelPair, labelPair::Hash<> >::const_iterator
        iter = map.find(myData);
      if (iter != map.end())
      {
        label nbrBFaceI = iter();
        masterCoupledFaces[coupledI] = mesh.nInternalFaces() + bFaceI;
        slaveCoupledFaces[coupledI] =
          domainMesh.nInternalFaces()
         + nbrBFaceI;
        coupledI++;
      }
    }
  }
  masterCoupledFaces.setSize(coupledI);
  slaveCoupledFaces.setSize(coupledI);
  if (debug)
  {
    Pout<< "findCouples : found " << coupledI
      << " faces that will be stitched" << nl << endl;
  }
}
// Map data on boundary faces to new mesh (resulting from adding two meshes)
mousse::labelList mousse::fvMeshDistribute::mapBoundaryData
(
  const primitiveMesh& mesh,      // mesh after adding
  const mapAddedPolyMesh& map,
  const labelList& boundaryData0, // mesh before adding
  const label nInternalFaces1,
  const labelList& boundaryData1  // added mesh
)
{
  labelList newBoundaryData(mesh.nFaces() - mesh.nInternalFaces());
  FOR_ALL(boundaryData0, oldBFaceI)
  {
    label newFaceI = map.oldFaceMap()[oldBFaceI + map.nOldInternalFaces()];
    // Face still exists (is necessary?) and still boundary face
    if (newFaceI >= 0 && newFaceI >= mesh.nInternalFaces())
    {
      newBoundaryData[newFaceI - mesh.nInternalFaces()] =
        boundaryData0[oldBFaceI];
    }
  }
  FOR_ALL(boundaryData1, addedBFaceI)
  {
    label newFaceI = map.addedFaceMap()[addedBFaceI + nInternalFaces1];
    if (newFaceI >= 0 && newFaceI >= mesh.nInternalFaces())
    {
      newBoundaryData[newFaceI - mesh.nInternalFaces()] =
        boundaryData1[addedBFaceI];
    }
  }
  return newBoundaryData;
}
// Remove cells. Add all exposed faces to patch oldInternalPatchI
mousse::autoPtr<mousse::mapPolyMesh> mousse::fvMeshDistribute::doRemoveCells
(
  const labelList& cellsToRemove,
  const label oldInternalPatchI
)
{
  // Mesh change engine
  polyTopoChange meshMod(mesh_);
  // Cell removal topo engine. Do NOT synchronize parallel since
  // we are doing a local cell removal.
  removeCells cellRemover(mesh_, false);
  // Get all exposed faces
  labelList exposedFaces(cellRemover.getExposedFaces(cellsToRemove));
  // Insert the topo changes
  cellRemover.setRefinement
  (
    cellsToRemove,
    exposedFaces,
    labelList(exposedFaces.size(), oldInternalPatchI),  // patch for exposed
                              // faces.
    meshMod
  );
  // Change the mesh. No inflation. Note: no parallel comms allowed.
  autoPtr<mapPolyMesh> map = meshMod.changeMesh(mesh_, false, false);
  // Update fields
  mesh_.updateMesh(map);
  // Move mesh (since morphing does not do this)
  if (map().hasMotionPoints())
  {
    mesh_.movePoints(map().preMotionPoints());
  }
  return map;
}
// Delete and add processor patches. Changes mesh and returns per neighbour proc
// the processor patchID.
void mousse::fvMeshDistribute::addProcPatches
(
  const labelList& nbrProc,       // processor that neighbour is now on
  const labelList& referPatchID,  // patchID (or -1) I originated from
  List<Map<label> >& procPatchID
)
{
  // Now use the neighbourFace/Proc to repatch the mesh. These lists
  // contain for all current boundary faces the global patchID (for non-proc
  // patch) or the processor.
  procPatchID.setSize(Pstream::nProcs());
  FOR_ALL(nbrProc, bFaceI)
  {
    label procI = nbrProc[bFaceI];
    if (procI != -1 && procI != Pstream::myProcNo())
    {
      if (!procPatchID[procI].found(referPatchID[bFaceI]))
      {
        // No patch for neighbour yet. Is either a normal processor
        // patch or a processorCyclic patch.
        if (referPatchID[bFaceI] == -1)
        {
          // Ordinary processor boundary
          const word patchName =
            "procBoundary"
           + name(Pstream::myProcNo())
           + "to"
           + name(procI);
          processorPolyPatch pp
          (
            patchName,
            0,              // size
            mesh_.nFaces(),
            mesh_.boundaryMesh().size(),
            mesh_.boundaryMesh(),
            Pstream::myProcNo(),
            nbrProc[bFaceI]
          );
          procPatchID[procI].insert
          (
            referPatchID[bFaceI],
            fvMeshTools::addPatch
            (
              mesh_,
              pp,
              dictionary(),   // optional per field patchField
              processorFvPatchField<scalar>::typeName,
              false           // not parallel sync
            )
          );
        }
        else
        {
          const coupledPolyPatch& pcPatch
            = refCast<const coupledPolyPatch>
             (
               mesh_.boundaryMesh()[referPatchID[bFaceI]]
             );
          // Processor boundary originating from cyclic
          const word& cycName = pcPatch.name();
          const word patchName =
            "procBoundary"
           + name(Pstream::myProcNo())
           + "to"
           + name(procI)
           + "through"
           + cycName;
          processorCyclicPolyPatch pp
          (
            patchName,
            0,              // size
            mesh_.nFaces(),
            mesh_.boundaryMesh().size(),
            mesh_.boundaryMesh(),
            Pstream::myProcNo(),
            nbrProc[bFaceI],
            cycName,
            pcPatch.transform()
          );
          procPatchID[procI].insert
          (
            referPatchID[bFaceI],
            fvMeshTools::addPatch
            (
              mesh_,
              pp,
              dictionary(),   // optional per field patchField
              processorCyclicFvPatchField<scalar>::typeName,
              false           // not parallel sync
            )
          );
        }
      }
    }
  }
}
// Get boundary faces to be repatched. Is -1 or new patchID
mousse::labelList mousse::fvMeshDistribute::getBoundaryPatch
(
  const labelList& nbrProc,               // new processor per boundary face
  const labelList& referPatchID,          // patchID (or -1) I originated from
  const List<Map<label> >& procPatchID    // per proc the new procPatches
)
{
  labelList patchIDs(nbrProc);
  FOR_ALL(nbrProc, bFaceI)
  {
    if (nbrProc[bFaceI] == Pstream::myProcNo())
    {
      label origPatchI = referPatchID[bFaceI];
      patchIDs[bFaceI] = origPatchI;
    }
    else if (nbrProc[bFaceI] != -1)
    {
      label origPatchI = referPatchID[bFaceI];
      patchIDs[bFaceI] = procPatchID[nbrProc[bFaceI]][origPatchI];
    }
    else
    {
      patchIDs[bFaceI] = -1;
    }
  }
  return patchIDs;
}
// Send mesh and coupling data.
void mousse::fvMeshDistribute::sendMesh
(
  const label domain,
  const fvMesh& mesh,
  const wordList& pointZoneNames,
  const wordList& faceZoneNames,
  const wordList& cellZoneNames,
  const labelList& sourceFace,
  const labelList& sourceProc,
  const labelList& sourcePatch,
  const labelList& sourceNewNbrProc,
  Ostream& toDomain
)
{
  if (debug)
  {
    Pout<< "Sending to domain " << domain << nl
      << "    nPoints:" << mesh.nPoints() << nl
      << "    nFaces:" << mesh.nFaces() << nl
      << "    nCells:" << mesh.nCells() << nl
      << "    nPatches:" << mesh.boundaryMesh().size() << nl
      << endl;
  }
  // Assume sparse, possibly overlapping point zones. Get contents
  // in merged-zone indices.
  CompactListList<label> zonePoints;
  {
    const pointZoneMesh& pointZones = mesh.pointZones();
    labelList rowSizes(pointZoneNames.size(), 0);
    FOR_ALL(pointZoneNames, nameI)
    {
      label myZoneID = pointZones.findZoneID(pointZoneNames[nameI]);
      if (myZoneID != -1)
      {
        rowSizes[nameI] = pointZones[myZoneID].size();
      }
    }
    zonePoints.setSize(rowSizes);
    FOR_ALL(pointZoneNames, nameI)
    {
      label myZoneID = pointZones.findZoneID(pointZoneNames[nameI]);
      if (myZoneID != -1)
      {
        zonePoints[nameI].assign(pointZones[myZoneID]);
      }
    }
  }
  // Assume sparse, possibly overlapping face zones
  CompactListList<label> zoneFaces;
  CompactListList<bool> zoneFaceFlip;
  {
    const faceZoneMesh& faceZones = mesh.faceZones();
    labelList rowSizes(faceZoneNames.size(), 0);
    FOR_ALL(faceZoneNames, nameI)
    {
      label myZoneID = faceZones.findZoneID(faceZoneNames[nameI]);
      if (myZoneID != -1)
      {
        rowSizes[nameI] = faceZones[myZoneID].size();
      }
    }
    zoneFaces.setSize(rowSizes);
    zoneFaceFlip.setSize(rowSizes);
    FOR_ALL(faceZoneNames, nameI)
    {
      label myZoneID = faceZones.findZoneID(faceZoneNames[nameI]);
      if (myZoneID != -1)
      {
        zoneFaces[nameI].assign(faceZones[myZoneID]);
        zoneFaceFlip[nameI].assign(faceZones[myZoneID].flipMap());
      }
    }
  }
  // Assume sparse, possibly overlapping cell zones
  CompactListList<label> zoneCells;
  {
    const cellZoneMesh& cellZones = mesh.cellZones();
    labelList rowSizes(cellZoneNames.size(), 0);
    FOR_ALL(cellZoneNames, nameI)
    {
      label myZoneID = cellZones.findZoneID(cellZoneNames[nameI]);
      if (myZoneID != -1)
      {
        rowSizes[nameI] = cellZones[myZoneID].size();
      }
    }
    zoneCells.setSize(rowSizes);
    FOR_ALL(cellZoneNames, nameI)
    {
      label myZoneID = cellZones.findZoneID(cellZoneNames[nameI]);
      if (myZoneID != -1)
      {
        zoneCells[nameI].assign(cellZones[myZoneID]);
      }
    }
  }
  ////- Assume full cell zones
  //labelList cellZoneID;
  //if (hasCellZones)
  //{
  //    cellZoneID.setSize(mesh.nCells());
  //    cellZoneID = -1;
  //
  //    const cellZoneMesh& cellZones = mesh.cellZones();
  //
  //    FOR_ALL(cellZones, zoneI)
  //    {
  //        UIndirectList<label>(cellZoneID, cellZones[zoneI]) = zoneI;
  //    }
  //}
  // Send
  toDomain
    << mesh.points()
    << CompactListList<label, face>(mesh.faces())
    << mesh.faceOwner()
    << mesh.faceNeighbour()
    << mesh.boundaryMesh()
    << zonePoints
    << zoneFaces
    << zoneFaceFlip
    << zoneCells
    << sourceFace
    << sourceProc
    << sourcePatch
    << sourceNewNbrProc;
  if (debug)
  {
    Pout<< "Started sending mesh to domain " << domain
      << endl;
  }
}
// Receive mesh. Opposite of sendMesh
mousse::autoPtr<mousse::fvMesh> mousse::fvMeshDistribute::receiveMesh
(
  const label /*domain*/,
  const wordList& pointZoneNames,
  const wordList& faceZoneNames,
  const wordList& cellZoneNames,
  const Time& runTime,
  labelList& domainSourceFace,
  labelList& domainSourceProc,
  labelList& domainSourcePatch,
  labelList& domainSourceNewNbrProc,
  Istream& fromNbr
)
{
  pointField domainPoints(fromNbr);
  faceList domainFaces = CompactListList<label, face>(fromNbr)();
  labelList domainAllOwner(fromNbr);
  labelList domainAllNeighbour(fromNbr);
  PtrList<entry> patchEntries(fromNbr);
  CompactListList<label> zonePoints(fromNbr);
  CompactListList<label> zoneFaces(fromNbr);
  CompactListList<bool> zoneFaceFlip(fromNbr);
  CompactListList<label> zoneCells(fromNbr);
  fromNbr
    >> domainSourceFace
    >> domainSourceProc
    >> domainSourcePatch
    >> domainSourceNewNbrProc;
  // Construct fvMesh
  autoPtr<fvMesh> domainMeshPtr
  (
    new fvMesh
    (
      IOobject
      (
        fvMesh::defaultRegion,
        runTime.timeName(),
        runTime,
        IOobject::NO_READ
      ),
      xferMove(domainPoints),
      xferMove(domainFaces),
      xferMove(domainAllOwner),
      xferMove(domainAllNeighbour),
      false                   // no parallel comms
    )
  );
  fvMesh& domainMesh = domainMeshPtr();
  List<polyPatch*> patches(patchEntries.size());
  FOR_ALL(patchEntries, patchI)
  {
    patches[patchI] = polyPatch::New
    (
      patchEntries[patchI].keyword(),
      patchEntries[patchI].dict(),
      patchI,
      domainMesh.boundaryMesh()
    ).ptr();
  }
  // Add patches; no parallel comms
  domainMesh.addFvPatches(patches, false);
  // Construct zones
  List<pointZone*> pZonePtrs(pointZoneNames.size());
  FOR_ALL(pZonePtrs, i)
  {
    pZonePtrs[i] = new pointZone
    (
      pointZoneNames[i],
      zonePoints[i],
      i,
      domainMesh.pointZones()
    );
  }
  List<faceZone*> fZonePtrs(faceZoneNames.size());
  FOR_ALL(fZonePtrs, i)
  {
    fZonePtrs[i] = new faceZone
    (
      faceZoneNames[i],
      zoneFaces[i],
      zoneFaceFlip[i],
      i,
      domainMesh.faceZones()
    );
  }
  List<cellZone*> cZonePtrs(cellZoneNames.size());
  FOR_ALL(cZonePtrs, i)
  {
    cZonePtrs[i] = new cellZone
    (
      cellZoneNames[i],
      zoneCells[i],
      i,
      domainMesh.cellZones()
    );
  }
  domainMesh.addZones(pZonePtrs, fZonePtrs, cZonePtrs);
  return domainMeshPtr;
}
// Constructors 
// Construct from components
mousse::fvMeshDistribute::fvMeshDistribute(fvMesh& mesh, const scalar mergeTol)
:
  mesh_(mesh),
  mergeTol_(mergeTol)
{}
// Member Functions 
mousse::labelList mousse::fvMeshDistribute::countCells
(
  const labelList& distribution
)
{
  labelList nCells(Pstream::nProcs(), 0);
  FOR_ALL(distribution, cellI)
  {
    label newProc = distribution[cellI];
    if (newProc < 0 || newProc >= Pstream::nProcs())
    {
      FATAL_ERROR_IN("fvMeshDistribute::distribute(const labelList&)")
        << "Distribution should be in range 0.." << Pstream::nProcs()-1
        << endl
        << "At index " << cellI << " distribution:" << newProc
        << abort(FatalError);
    }
    nCells[newProc]++;
  }
  return nCells;
}
mousse::autoPtr<mousse::mapDistributePolyMesh> mousse::fvMeshDistribute::distribute
(
  const labelList& distribution
)
{
  // Some checks on distribution
  if (distribution.size() != mesh_.nCells())
  {
    FATAL_ERROR_IN("fvMeshDistribute::distribute(const labelList&)")
      << "Size of distribution:"
      << distribution.size() << " mesh nCells:" << mesh_.nCells()
      << abort(FatalError);
  }
  const polyBoundaryMesh& patches = mesh_.boundaryMesh();
  // Check all processors have same non-proc patches in same order.
  if (patches.checkParallelSync(true))
  {
    FATAL_ERROR_IN("fvMeshDistribute::distribute(const labelList&)")
      << "This application requires all non-processor patches"
      << " to be present in the same order on all patches" << nl
      << "followed by the processor patches (which of course are unique)."
      << nl
      << "Local patches:" << mesh_.boundaryMesh().names()
      << abort(FatalError);
  }
  // Save some data for mapping later on
  const label nOldPoints(mesh_.nPoints());
  const label nOldFaces(mesh_.nFaces());
  const label nOldCells(mesh_.nCells());
  labelList oldPatchStarts(patches.size());
  labelList oldPatchNMeshPoints(patches.size());
  FOR_ALL(patches, patchI)
  {
    oldPatchStarts[patchI] = patches[patchI].start();
    oldPatchNMeshPoints[patchI] = patches[patchI].nPoints();
  }
  // Short circuit trivial case.
  if (!Pstream::parRun())
  {
    // Collect all maps and return
    return autoPtr<mapDistributePolyMesh>
    (
      new mapDistributePolyMesh
      (
        mesh_,
        nOldPoints,
        nOldFaces,
        nOldCells,
        oldPatchStarts.xfer(),
        oldPatchNMeshPoints.xfer(),
        labelListList(1, identity(mesh_.nPoints())).xfer(),//subPointMap
        labelListList(1, identity(mesh_.nFaces())).xfer(), //subFaceMap
        labelListList(1, identity(mesh_.nCells())).xfer(), //subCellMap
        labelListList(1, identity(patches.size())).xfer(), //subPatchMap
        labelListList(1, identity(mesh_.nPoints())).xfer(),//pointMap
        labelListList(1, identity(mesh_.nFaces())).xfer(), //faceMap
        labelListList(1, identity(mesh_.nCells())).xfer(), //cellMap
        labelListList(1, identity(patches.size())).xfer()  //patchMap
      )
    );
  }
  // Collect any zone names
  const wordList pointZoneNames(mergeWordList(mesh_.pointZones().names()));
  const wordList faceZoneNames(mergeWordList(mesh_.faceZones().names()));
  const wordList cellZoneNames(mergeWordList(mesh_.cellZones().names()));
  // Local environment of all boundary faces
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // A face is uniquely defined by
  //  - proc
  //  - local face no
  //
  // To glue the parts of meshes which can get sent from anywhere we
  // need to know on boundary faces what the above tuple on both sides is.
  // So we need to maintain:
  //  - original face
  //  - original processor id (= trivial)
  // For coupled boundaries (where the faces are 'duplicate') we take the
  // lowest numbered processor as the data to store.
  //
  // Additionally to create the procboundaries we need to know where the owner
  // cell on the other side now is: newNeighbourProc.
  //
  // physical boundary:
  //     sourceProc = -1
  //     sourceNewNbrProc = -1
  //     sourceFace = -1
  //     sourcePatch = patchID
  // processor boundary:
  //     sourceProc = proc (on owner side)
  //     sourceNewNbrProc = distribution of coupled cell
  //     sourceFace = face (on owner side)
  //     sourcePatch = -1
  // ?cyclic:
  // ?    sourceProc = proc
  // ?    sourceNewNbrProc = distribution of coupled cell
  // ?    sourceFace = face (on owner side)
  // ?    sourcePatch = patchID
  // processor-cyclic boundary:
  //     sourceProc = proc (on owner side)
  //     sourceNewNbrProc = distribution of coupled cell
  //     sourceFace = face (on owner side)
  //     sourcePatch = patchID
  labelList sourcePatch;
  labelList sourceFace;
  labelList sourceProc;
  labelList sourceNewNbrProc;
  getNeighbourData
  (
    distribution,
    sourceFace,
    sourceProc,
    sourcePatch,
    sourceNewNbrProc
  );
  // Remove meshPhi. Since this would otherwise disappear anyway
  // during topo changes and we have to guarantee that all the fields
  // can be sent.
  mesh_.clearOut();
  mesh_.resetMotion();
  // Get data to send. Make sure is synchronised
  const wordList volScalars(mesh_.names(volScalarField::typeName));
  checkEqualWordList("volScalarFields", volScalars);
  const wordList volVectors(mesh_.names(volVectorField::typeName));
  checkEqualWordList("volVectorFields", volVectors);
  const wordList volSphereTensors
  (
    mesh_.names(volSphericalTensorField::typeName)
  );
  checkEqualWordList("volSphericalTensorFields", volSphereTensors);
  const wordList volSymmTensors(mesh_.names(volSymmTensorField::typeName));
  checkEqualWordList("volSymmTensorFields", volSymmTensors);
  const wordList volTensors(mesh_.names(volTensorField::typeName));
  checkEqualWordList("volTensorField", volTensors);
  const wordList surfScalars(mesh_.names(surfaceScalarField::typeName));
  checkEqualWordList("surfaceScalarFields", surfScalars);
  const wordList surfVectors(mesh_.names(surfaceVectorField::typeName));
  checkEqualWordList("surfaceVectorFields", surfVectors);
  const wordList surfSphereTensors
  (
    mesh_.names(surfaceSphericalTensorField::typeName)
  );
  checkEqualWordList("surfaceSphericalTensorFields", surfSphereTensors);
  const wordList surfSymmTensors
  (
    mesh_.names(surfaceSymmTensorField::typeName)
  );
  checkEqualWordList("surfaceSymmTensorFields", surfSymmTensors);
  const wordList surfTensors(mesh_.names(surfaceTensorField::typeName));
  checkEqualWordList("surfaceTensorFields", surfTensors);
  // Find patch to temporarily put exposed and processor faces into.
  label oldInternalPatchI = findNonEmptyPatch();
  // Delete processor patches, starting from the back. Move all faces into
  // oldInternalPatchI.
  labelList repatchFaceMap;
  {
    autoPtr<mapPolyMesh> repatchMap = deleteProcPatches(oldInternalPatchI);
    // Store face map (only face ordering that changed)
    repatchFaceMap = repatchMap().faceMap();
    // Reorder all boundary face data (sourceProc, sourceFace etc.)
    labelList bFaceMap
    (
      SubList<label>
      (
        repatchMap().reverseFaceMap(),
        mesh_.nFaces() - mesh_.nInternalFaces(),
        mesh_.nInternalFaces()
      )
     - mesh_.nInternalFaces()
    );
    inplaceReorder(bFaceMap, sourceFace);
    inplaceReorder(bFaceMap, sourceProc);
    inplaceReorder(bFaceMap, sourcePatch);
    inplaceReorder(bFaceMap, sourceNewNbrProc);
  }
  // Print a bit.
  if (debug)
  {
    Pout<< nl << "MESH WITH PROC PATCHES DELETED:" << endl;
    printMeshInfo(mesh_);
    printFieldInfo<volScalarField>(mesh_);
    printFieldInfo<volVectorField>(mesh_);
    printFieldInfo<volSphericalTensorField>(mesh_);
    printFieldInfo<volSymmTensorField>(mesh_);
    printFieldInfo<volTensorField>(mesh_);
    printFieldInfo<surfaceScalarField>(mesh_);
    printFieldInfo<surfaceVectorField>(mesh_);
    printFieldInfo<surfaceSphericalTensorField>(mesh_);
    printFieldInfo<surfaceSymmTensorField>(mesh_);
    printFieldInfo<surfaceTensorField>(mesh_);
    Pout<< nl << endl;
  }
  // Maps from subsetted mesh (that is sent) back to original maps
  labelListList subCellMap(Pstream::nProcs());
  labelListList subFaceMap(Pstream::nProcs());
  labelListList subPointMap(Pstream::nProcs());
  labelListList subPatchMap(Pstream::nProcs());
  // Maps from subsetted mesh to reconstructed mesh
  labelListList constructCellMap(Pstream::nProcs());
  labelListList constructFaceMap(Pstream::nProcs());
  labelListList constructPointMap(Pstream::nProcs());
  labelListList constructPatchMap(Pstream::nProcs());
  // Find out schedule
  // ~~~~~~~~~~~~~~~~~
  labelListList nSendCells(Pstream::nProcs());
  nSendCells[Pstream::myProcNo()] = countCells(distribution);
  Pstream::gatherList(nSendCells);
  Pstream::scatterList(nSendCells);
  // Allocate buffers
  PstreamBuffers pBufs(Pstream::nonBlocking);
  // What to send to neighbouring domains
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  bool oldParRun = UPstream::parRun();
  UPstream::parRun() = false;
  FOR_ALL(nSendCells[Pstream::myProcNo()], recvProc)
  {
    if
    (
      recvProc != Pstream::myProcNo()
    && nSendCells[Pstream::myProcNo()][recvProc] > 0
    )
    {
      // Send to recvProc
      if (debug)
      {
        Pout<< nl
          << "SUBSETTING FOR DOMAIN " << recvProc
          << " cells to send:"
          << nSendCells[Pstream::myProcNo()][recvProc]
          << nl << endl;
      }
      // Pstream for sending mesh and fields
      //OPstream str(Pstream::blocking, recvProc);
      UOPstream str(recvProc, pBufs);
      // Mesh subsetting engine
      fvMeshSubset subsetter(mesh_);
      // Subset the cells of the current domain.
      subsetter.setLargeCellSubset
      (
        distribution,
        recvProc,
        oldInternalPatchI,  // oldInternalFaces patch
        false               // no parallel sync
      );
      subCellMap[recvProc] = subsetter.cellMap();
      subFaceMap[recvProc] = renumber
      (
        repatchFaceMap,
        subsetter.faceMap()
      );
      subPointMap[recvProc] = subsetter.pointMap();
      subPatchMap[recvProc] = subsetter.patchMap();
      // Subset the boundary fields (owner/neighbour/processor)
      labelList procSourceFace;
      labelList procSourceProc;
      labelList procSourcePatch;
      labelList procSourceNewNbrProc;
      subsetBoundaryData
      (
        subsetter.subMesh(),
        subsetter.faceMap(),        // from subMesh to mesh
        subsetter.cellMap(),        //      ,,      ,,
        distribution,               // old mesh distribution
        mesh_.faceOwner(),          // old owner
        mesh_.faceNeighbour(),
        mesh_.nInternalFaces(),
        sourceFace,
        sourceProc,
        sourcePatch,
        sourceNewNbrProc,
        procSourceFace,
        procSourceProc,
        procSourcePatch,
        procSourceNewNbrProc
      );
      // Send to neighbour
      sendMesh
      (
        recvProc,
        subsetter.subMesh(),
        pointZoneNames,
        faceZoneNames,
        cellZoneNames,
        procSourceFace,
        procSourceProc,
        procSourcePatch,
        procSourceNewNbrProc,
        str
      );
      sendFields<volScalarField>(recvProc, volScalars, subsetter, str);
      sendFields<volVectorField>(recvProc, volVectors, subsetter, str);
      sendFields<volSphericalTensorField>
      (
        recvProc,
        volSphereTensors,
        subsetter,
        str
      );
      sendFields<volSymmTensorField>
      (
        recvProc,
        volSymmTensors,
        subsetter,
        str
      );
      sendFields<volTensorField>(recvProc, volTensors, subsetter, str);
      sendFields<surfaceScalarField>
      (
        recvProc,
        surfScalars,
        subsetter,
        str
      );
      sendFields<surfaceVectorField>
      (
        recvProc,
        surfVectors,
        subsetter,
        str
      );
      sendFields<surfaceSphericalTensorField>
      (
        recvProc,
        surfSphereTensors,
        subsetter,
        str
      );
      sendFields<surfaceSymmTensorField>
      (
        recvProc,
        surfSymmTensors,
        subsetter,
        str
      );
      sendFields<surfaceTensorField>
      (
        recvProc,
        surfTensors,
        subsetter,
        str
      );
    }
  }
  UPstream::parRun() = oldParRun;
  // Start sending&receiving from buffers
  pBufs.finishedSends();
  // Subset the part that stays
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~
  {
    // Save old mesh maps before changing mesh
    const labelList oldFaceOwner(mesh_.faceOwner());
    const labelList oldFaceNeighbour(mesh_.faceNeighbour());
    const label oldInternalFaces = mesh_.nInternalFaces();
    // Remove cells.
    autoPtr<mapPolyMesh> subMap
    (
      doRemoveCells
      (
        select(false, distribution, Pstream::myProcNo()),
        oldInternalPatchI
      )
    );
    // Addressing from subsetted mesh
    subCellMap[Pstream::myProcNo()] = subMap().cellMap();
    subFaceMap[Pstream::myProcNo()] = renumber
    (
      repatchFaceMap,
      subMap().faceMap()
    );
    subPointMap[Pstream::myProcNo()] = subMap().pointMap();
    subPatchMap[Pstream::myProcNo()] = identity(patches.size());
    // Initialize all addressing into current mesh
    constructCellMap[Pstream::myProcNo()] = identity(mesh_.nCells());
    constructFaceMap[Pstream::myProcNo()] = identity(mesh_.nFaces());
    constructPointMap[Pstream::myProcNo()] = identity(mesh_.nPoints());
    constructPatchMap[Pstream::myProcNo()] = identity(patches.size());
    // Subset the mesh data: neighbourCell/neighbourProc
    // fields
    labelList domainSourceFace;
    labelList domainSourceProc;
    labelList domainSourcePatch;
    labelList domainSourceNewNbrProc;
    subsetBoundaryData
    (
      mesh_,                          // new mesh
      subMap().faceMap(),             // from new to original mesh
      subMap().cellMap(),
      distribution,                   // distribution before subsetting
      oldFaceOwner,                   // owner before subsetting
      oldFaceNeighbour,               // neighbour        ,,
      oldInternalFaces,               // nInternalFaces   ,,
      sourceFace,
      sourceProc,
      sourcePatch,
      sourceNewNbrProc,
      domainSourceFace,
      domainSourceProc,
      domainSourcePatch,
      domainSourceNewNbrProc
    );
    sourceFace.transfer(domainSourceFace);
    sourceProc.transfer(domainSourceProc);
    sourcePatch.transfer(domainSourcePatch);
    sourceNewNbrProc.transfer(domainSourceNewNbrProc);
  }
  // Print a bit.
  if (debug)
  {
    Pout<< nl << "STARTING MESH:" << endl;
    printMeshInfo(mesh_);
    printFieldInfo<volScalarField>(mesh_);
    printFieldInfo<volVectorField>(mesh_);
    printFieldInfo<volSphericalTensorField>(mesh_);
    printFieldInfo<volSymmTensorField>(mesh_);
    printFieldInfo<volTensorField>(mesh_);
    printFieldInfo<surfaceScalarField>(mesh_);
    printFieldInfo<surfaceVectorField>(mesh_);
    printFieldInfo<surfaceSphericalTensorField>(mesh_);
    printFieldInfo<surfaceSymmTensorField>(mesh_);
    printFieldInfo<surfaceTensorField>(mesh_);
    Pout<< nl << endl;
  }
  // Receive and add what was sent
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  oldParRun = UPstream::parRun();
  UPstream::parRun() = false;
  FOR_ALL(nSendCells, sendProc)
  {
    // Did processor sendProc send anything to me?
    if
    (
      sendProc != Pstream::myProcNo()
    && nSendCells[sendProc][Pstream::myProcNo()] > 0
    )
    {
      if (debug)
      {
        Pout<< nl
          << "RECEIVING FROM DOMAIN " << sendProc
          << " cells to receive:"
          << nSendCells[sendProc][Pstream::myProcNo()]
          << nl << endl;
      }
      // Pstream for receiving mesh and fields
      UIPstream str(sendProc, pBufs);
      // Receive from sendProc
      labelList domainSourceFace;
      labelList domainSourceProc;
      labelList domainSourcePatch;
      labelList domainSourceNewNbrProc;
      autoPtr<fvMesh> domainMeshPtr;
      PtrList<volScalarField> vsf;
      PtrList<volVectorField> vvf;
      PtrList<volSphericalTensorField> vsptf;
      PtrList<volSymmTensorField> vsytf;
      PtrList<volTensorField> vtf;
      PtrList<surfaceScalarField> ssf;
      PtrList<surfaceVectorField> svf;
      PtrList<surfaceSphericalTensorField> ssptf;
      PtrList<surfaceSymmTensorField> ssytf;
      PtrList<surfaceTensorField> stf;
      // Opposite of sendMesh
      {
        domainMeshPtr = receiveMesh
        (
          sendProc,
          pointZoneNames,
          faceZoneNames,
          cellZoneNames,
          const_cast<Time&>(mesh_.time()),
          domainSourceFace,
          domainSourceProc,
          domainSourcePatch,
          domainSourceNewNbrProc,
          str
        );
        fvMesh& domainMesh = domainMeshPtr();
        // Force construction of various on mesh.
        //(void)domainMesh.globalData();
        // Receive fields. Read as single dictionary because
        // of problems reading consecutive fields from single stream.
        dictionary fieldDicts(str);
        receiveFields<volScalarField>
        (
          sendProc,
          volScalars,
          domainMesh,
          vsf,
          fieldDicts.subDict(volScalarField::typeName)
        );
        receiveFields<volVectorField>
        (
          sendProc,
          volVectors,
          domainMesh,
          vvf,
          fieldDicts.subDict(volVectorField::typeName)
        );
        receiveFields<volSphericalTensorField>
        (
          sendProc,
          volSphereTensors,
          domainMesh,
          vsptf,
          fieldDicts.subDict(volSphericalTensorField::typeName)
        );
        receiveFields<volSymmTensorField>
        (
          sendProc,
          volSymmTensors,
          domainMesh,
          vsytf,
          fieldDicts.subDict(volSymmTensorField::typeName)
        );
        receiveFields<volTensorField>
        (
          sendProc,
          volTensors,
          domainMesh,
          vtf,
          fieldDicts.subDict(volTensorField::typeName)
        );
        receiveFields<surfaceScalarField>
        (
          sendProc,
          surfScalars,
          domainMesh,
          ssf,
          fieldDicts.subDict(surfaceScalarField::typeName)
        );
        receiveFields<surfaceVectorField>
        (
          sendProc,
          surfVectors,
          domainMesh,
          svf,
          fieldDicts.subDict(surfaceVectorField::typeName)
        );
        receiveFields<surfaceSphericalTensorField>
        (
          sendProc,
          surfSphereTensors,
          domainMesh,
          ssptf,
          fieldDicts.subDict(surfaceSphericalTensorField::typeName)
        );
        receiveFields<surfaceSymmTensorField>
        (
          sendProc,
          surfSymmTensors,
          domainMesh,
          ssytf,
          fieldDicts.subDict(surfaceSymmTensorField::typeName)
        );
        receiveFields<surfaceTensorField>
        (
          sendProc,
          surfTensors,
          domainMesh,
          stf,
          fieldDicts.subDict(surfaceTensorField::typeName)
        );
      }
      const fvMesh& domainMesh = domainMeshPtr();
      constructCellMap[sendProc] = identity(domainMesh.nCells());
      constructFaceMap[sendProc] = identity(domainMesh.nFaces());
      constructPointMap[sendProc] = identity(domainMesh.nPoints());
      constructPatchMap[sendProc] =
        identity(domainMesh.boundaryMesh().size());
      // Print a bit.
      if (debug)
      {
        Pout<< nl << "RECEIVED MESH FROM:" << sendProc << endl;
        printMeshInfo(domainMesh);
        printFieldInfo<volScalarField>(domainMesh);
        printFieldInfo<volVectorField>(domainMesh);
        printFieldInfo<volSphericalTensorField>(domainMesh);
        printFieldInfo<volSymmTensorField>(domainMesh);
        printFieldInfo<volTensorField>(domainMesh);
        printFieldInfo<surfaceScalarField>(domainMesh);
        printFieldInfo<surfaceVectorField>(domainMesh);
        printFieldInfo<surfaceSphericalTensorField>(domainMesh);
        printFieldInfo<surfaceSymmTensorField>(domainMesh);
        printFieldInfo<surfaceTensorField>(domainMesh);
      }
      // Now this mesh we received (from sendProc) needs to be merged
      // with the current mesh. On the current mesh we have for all
      // boundaryfaces the original face and processor. See if we can
      // match these up to the received domainSourceFace and
      // domainSourceProc.
      labelList masterCoupledFaces;
      labelList slaveCoupledFaces;
      findCouples
      (
        mesh_,
        sourceFace,
        sourceProc,
        sourcePatch,
        sendProc,
        domainMesh,
        domainSourceFace,
        domainSourceProc,
        domainSourcePatch,
        masterCoupledFaces,
        slaveCoupledFaces
      );
      // Generate additional coupling info (points, edges) from
      // faces-that-match
      faceCoupleInfo couples
      (
        mesh_,
        masterCoupledFaces,
        domainMesh,
        slaveCoupledFaces,
        mergeTol_,              // merge tolerance
        true,                   // faces align
        true,                   // couples are ordered already
        false
      );
      // Add domainMesh to mesh
      // ~~~~~~~~~~~~~~~~~~~~~~
      autoPtr<mapAddedPolyMesh> map = fvMeshAdder::add
      (
        mesh_,
        domainMesh,
        couples,
        false           // no parallel comms
      );
      // Update mesh data: sourceFace,sourceProc for added
      // mesh.
      sourceFace = mapBoundaryData
      (
        mesh_,
        map(),
        sourceFace,
        domainMesh.nInternalFaces(),
        domainSourceFace
      );
      sourceProc = mapBoundaryData
      (
        mesh_,
        map(),
        sourceProc,
        domainMesh.nInternalFaces(),
        domainSourceProc
      );
      sourcePatch = mapBoundaryData
      (
        mesh_,
        map(),
        sourcePatch,
        domainMesh.nInternalFaces(),
        domainSourcePatch
      );
      sourceNewNbrProc = mapBoundaryData
      (
        mesh_,
        map(),
        sourceNewNbrProc,
        domainMesh.nInternalFaces(),
        domainSourceNewNbrProc
      );
      // Update all addressing so xxProcAddressing points to correct item
      // in masterMesh.
      const labelList& oldCellMap = map().oldCellMap();
      const labelList& oldFaceMap = map().oldFaceMap();
      const labelList& oldPointMap = map().oldPointMap();
      const labelList& oldPatchMap = map().oldPatchMap();
      FOR_ALL(constructPatchMap, procI)
      {
        if (procI != sendProc && constructPatchMap[procI].size())
        {
          // Processor already in mesh (either myProcNo or received)
          inplaceRenumber(oldCellMap, constructCellMap[procI]);
          inplaceRenumber(oldFaceMap, constructFaceMap[procI]);
          inplaceRenumber(oldPointMap, constructPointMap[procI]);
          inplaceRenumber(oldPatchMap, constructPatchMap[procI]);
        }
      }
      // Added processor
      inplaceRenumber(map().addedCellMap(), constructCellMap[sendProc]);
      inplaceRenumber(map().addedFaceMap(), constructFaceMap[sendProc]);
      inplaceRenumber(map().addedPointMap(), constructPointMap[sendProc]);
      inplaceRenumber(map().addedPatchMap(), constructPatchMap[sendProc]);
      if (debug)
      {
        Pout<< nl << "MERGED MESH FROM:" << sendProc << endl;
        printMeshInfo(mesh_);
        printFieldInfo<volScalarField>(mesh_);
        printFieldInfo<volVectorField>(mesh_);
        printFieldInfo<volSphericalTensorField>(mesh_);
        printFieldInfo<volSymmTensorField>(mesh_);
        printFieldInfo<volTensorField>(mesh_);
        printFieldInfo<surfaceScalarField>(mesh_);
        printFieldInfo<surfaceVectorField>(mesh_);
        printFieldInfo<surfaceSphericalTensorField>(mesh_);
        printFieldInfo<surfaceSymmTensorField>(mesh_);
        printFieldInfo<surfaceTensorField>(mesh_);
        Pout<< nl << endl;
      }
    }
  }
  UPstream::parRun() = oldParRun;
  // Print a bit.
  if (debug)
  {
    Pout<< nl << "REDISTRIBUTED MESH:" << endl;
    printMeshInfo(mesh_);
    printFieldInfo<volScalarField>(mesh_);
    printFieldInfo<volVectorField>(mesh_);
    printFieldInfo<volSphericalTensorField>(mesh_);
    printFieldInfo<volSymmTensorField>(mesh_);
    printFieldInfo<volTensorField>(mesh_);
    printFieldInfo<surfaceScalarField>(mesh_);
    printFieldInfo<surfaceVectorField>(mesh_);
    printFieldInfo<surfaceSphericalTensorField>(mesh_);
    printFieldInfo<surfaceSymmTensorField>(mesh_);
    printFieldInfo<surfaceTensorField>(mesh_);
    Pout<< nl << endl;
  }
  // Add processorPatches
  // ~~~~~~~~~~~~~~~~~~~~
  // Per neighbour processor, per originating patch, the patchID
  // For faces resulting from internal faces or normal processor patches
  // the originating patch is -1. For cyclics this is the cyclic patchID.
  List<Map<label> > procPatchID;
  // Add processor and processorCyclic patches.
  addProcPatches(sourceNewNbrProc, sourcePatch, procPatchID);
  // Put faces into correct patch. Note that we now have proper
  // processorPolyPatches again so repatching will take care of coupled face
  // ordering.
  // Get boundary faces to be repatched. Is -1 or new patchID
  labelList newPatchID
  (
    getBoundaryPatch
    (
      sourceNewNbrProc,
      sourcePatch,
      procPatchID
    )
  );
  // Change patches. Since this might change ordering of coupled faces
  // we also need to adapt our constructMaps.
  // NOTE: there is one very particular problem with this structure.
  // We first create the processor patches and use these to merge out
  // shared points (see mergeSharedPoints below). So temporarily points
  // and edges do not match!
  repatch(newPatchID, constructFaceMap);
  // See if any geometrically shared points need to be merged. Note: does
  // parallel comms. After this points and edges should again be consistent.
  mergeSharedPoints(constructPointMap);
  // Bit of hack: processorFvPatchField does not get reset since created
  // from nothing so explicitly reset.
  initPatchFields<volScalarField, processorFvPatchField<scalar> >
  (
    pTraits<scalar>::zero
  );
  initPatchFields<volVectorField, processorFvPatchField<vector> >
  (
    pTraits<vector>::zero
  );
  initPatchFields
  <
    volSphericalTensorField,
    processorFvPatchField<sphericalTensor>
  >
  (
    pTraits<sphericalTensor>::zero
  );
  initPatchFields<volSymmTensorField, processorFvPatchField<symmTensor> >
  (
    pTraits<symmTensor>::zero
  );
  initPatchFields<volTensorField, processorFvPatchField<tensor> >
  (
    pTraits<tensor>::zero
  );
  initPatchFields<surfaceScalarField, processorFvsPatchField<scalar> >
  (
    pTraits<scalar>::zero
  );
  initPatchFields<surfaceVectorField, processorFvsPatchField<vector> >
  (
    pTraits<vector>::zero
  );
  initPatchFields
  <
    surfaceSphericalTensorField,
    processorFvsPatchField<sphericalTensor>
  >
  (
    pTraits<sphericalTensor>::zero
  );
  initPatchFields
  <
    surfaceSymmTensorField,
    processorFvsPatchField<symmTensor>
  >
  (
    pTraits<symmTensor>::zero
  );
  initPatchFields<surfaceTensorField, processorFvsPatchField<tensor> >
  (
    pTraits<tensor>::zero
  );
  mesh_.setInstance(mesh_.time().timeName());
  // Print a bit
  if (debug)
  {
    Pout<< nl << "FINAL MESH:" << endl;
    printMeshInfo(mesh_);
    printFieldInfo<volScalarField>(mesh_);
    printFieldInfo<volVectorField>(mesh_);
    printFieldInfo<volSphericalTensorField>(mesh_);
    printFieldInfo<volSymmTensorField>(mesh_);
    printFieldInfo<volTensorField>(mesh_);
    printFieldInfo<surfaceScalarField>(mesh_);
    printFieldInfo<surfaceVectorField>(mesh_);
    printFieldInfo<surfaceSphericalTensorField>(mesh_);
    printFieldInfo<surfaceSymmTensorField>(mesh_);
    printFieldInfo<surfaceTensorField>(mesh_);
    Pout<< nl << endl;
  }
  // Collect all maps and return
  return autoPtr<mapDistributePolyMesh>
  (
    new mapDistributePolyMesh
    (
      mesh_,
      nOldPoints,
      nOldFaces,
      nOldCells,
      oldPatchStarts.xfer(),
      oldPatchNMeshPoints.xfer(),
      subPointMap.xfer(),
      subFaceMap.xfer(),
      subCellMap.xfer(),
      subPatchMap.xfer(),
      constructPointMap.xfer(),
      constructFaceMap.xfer(),
      constructCellMap.xfer(),
      constructPatchMap.xfer()
    )
  );
}
