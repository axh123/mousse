// mousse: CFD toolbox
// Copyright (C) 2011-2013 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "arg_list.hpp"
#include "time.hpp"
#include "poly_topo_change.hpp"
#include "poly_modify_face.hpp"
#include "poly_add_face.hpp"
#include "read_fields.hpp"
#include "vol_fields.hpp"
#include "surface_fields.hpp"
#include "fv_mesh_mapper.hpp"
#include "face_selection.hpp"
#include "fv_mesh_tools.hpp"
#include "_read_fields.hpp"


using namespace mousse;


label addPatch
(
  fvMesh& mesh,
  const word& patchName,
  const word& groupName,
  const dictionary& patchDict
)
{
  const polyBoundaryMesh& pbm = mesh.boundaryMesh();
  if (pbm.findPatchID(patchName) == -1) {
    autoPtr<polyPatch> ppPtr
    {
      polyPatch::New
      (
        patchName,
        patchDict,
        0,
        pbm
      )
    };
    polyPatch& pp = ppPtr();
    if (!groupName.empty() && !pp.inGroup(groupName)) {
      pp.inGroups().append(groupName);
    }
    // Add patch, create calculated everywhere
    fvMeshTools::addPatch
    (
      mesh,
      pp,
      dictionary(),   // do not set specialised patchFields
      calculatedFvPatchField<scalar>::typeName,
      true            // parallel sync'ed addition
    );
  } else {
    Info << "Patch '" << patchName
      << "' already exists.  Only "
      << "moving patch faces - type will remain the same"
      << endl;
  }
  return pbm.findPatchID(patchName);
}


void modifyOrAddFace
(
  polyTopoChange& meshMod,
  const face& f,
  const label faceI,
  const label own,
  const bool flipFaceFlux,
  const label newPatchI,
  const label zoneID,
  const bool zoneFlip,
  PackedBoolList& modifiedFace
)
{
  if (!modifiedFace[faceI]) {
    // First usage of face. Modify.
    meshMod.setAction
    (
      polyModifyFace
      {
        f,                          // modified face
        faceI,                      // label of face
        own,                        // owner
        -1,                         // neighbour
        flipFaceFlux,               // face flip
        newPatchI,                  // patch for face
        false,                      // remove from zone
        zoneID,                     // zone for face
        zoneFlip                    // face flip in zone
      }
    );
    modifiedFace[faceI] = 1;
  } else {
    // Second or more usage of face. Add.
    meshMod.setAction
    (
      polyAddFace
      {
        f,                          // modified face
        own,                        // owner
        -1,                         // neighbour
        -1,                         // master point
        -1,                         // master edge
        faceI,                      // master face
        flipFaceFlux,               // face flip
        newPatchI,                  // patch for face
        zoneID,                     // zone for face
        zoneFlip                    // face flip in zone
      }
    );
  }
}


// Create faces for fZone faces. Usually newMasterPatches, newSlavePatches
// only size one but can be more for duplicate baffle sets
void createFaces
(
  const bool internalFacesOnly,
  const fvMesh& mesh,
  const faceZone& fZone,
  const labelList& newMasterPatches,
  const labelList& newSlavePatches,
  polyTopoChange& meshMod,
  PackedBoolList& modifiedFace,
  label& nModified
)
{
  const polyBoundaryMesh& pbm = mesh.boundaryMesh();
  FOR_ALL(newMasterPatches, i) {
    // Pass 1. Do selected side of zone
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    for (label faceI = 0; faceI < mesh.nInternalFaces(); faceI++) {
      label zoneFaceI = fZone.whichFace(faceI);
      if (zoneFaceI == -1)
        continue;
      if (!fZone.flipMap()[zoneFaceI]) {
        // Use owner side of face
        modifyOrAddFace
        (
          meshMod,
          mesh.faces()[faceI],    // modified face
          faceI,                  // label of face
          mesh.faceOwner()[faceI],// owner
          false,                  // face flip
          newMasterPatches[i],    // patch for face
          fZone.index(),          // zone for face
          false,                  // face flip in zone
          modifiedFace            // modify or add status
        );
      } else {
        // Use neighbour side of face.
        // To keep faceZone pointing out of original neighbour
        // we don't need to set faceFlip since that cell
        // now becomes the owner
        modifyOrAddFace
        (
          meshMod,
          mesh.faces()[faceI].reverseFace(),  // modified face
          faceI,                      // label of face
          mesh.faceNeighbour()[faceI],// owner
          true,                       // face flip
          newMasterPatches[i],        // patch for face
          fZone.index(),              // zone for face
          false,                      // face flip in zone
          modifiedFace                // modify or add status
        );
      }
      nModified++;
    }
    // Pass 2. Do other side of zone
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    for (label faceI = 0; faceI < mesh.nInternalFaces(); faceI++) {
      label zoneFaceI = fZone.whichFace(faceI);
      if (zoneFaceI == -1)
        continue;
      if (!fZone.flipMap()[zoneFaceI]) {
        // Use neighbour side of face
        modifyOrAddFace
        (
          meshMod,
          mesh.faces()[faceI].reverseFace(),  // modified face
          faceI,                          // label of face
          mesh.faceNeighbour()[faceI],    // owner
          true,                           // face flip
          newSlavePatches[i],             // patch for face
          fZone.index(),                  // zone for face
          true,                           // face flip in zone
          modifiedFace                    // modify or add
        );
      } else {
        // Use owner side of face
        modifyOrAddFace
        (
          meshMod,
          mesh.faces()[faceI],    // modified face
          faceI,                  // label of face
          mesh.faceOwner()[faceI],// owner
          false,                  // face flip
          newSlavePatches[i],     // patch for face
          fZone.index(),          // zone for face
          true,                   // face flip in zone
          modifiedFace            // modify or add status
        );
      }
    }
    // Modify any boundary faces
    // ~~~~~~~~~~~~~~~~~~~~~~~~~
    // Normal boundary:
    // - move to new patch. Might already be back-to-back baffle
    // you want to add cyclic to. Do warn though.
    //
    // Processor boundary:
    // - do not move to cyclic
    // - add normal patches though.
    // For warning once per patch.
    labelHashSet patchWarned;
    FOR_ALL(pbm, patchI) {
      const polyPatch& pp = pbm[patchI];
      label newPatchI = newMasterPatches[i];
      if (pp.coupled() && pbm[newPatchI].coupled()) {
        // Do not allow coupled faces to be moved to different
        // coupled patches.
      } else if (pp.coupled() || !internalFacesOnly) {
        FOR_ALL(pp, i) {
          label faceI = pp.start()+i;
          label zoneFaceI = fZone.whichFace(faceI);
          if (zoneFaceI == -1)
            continue;
          if (patchWarned.insert(patchI)) {
            WARNING_IN("createFaces(..)")
              << "Found boundary face (in patch "
              << pp.name()
              << ") in faceZone " << fZone.name()
              << " to convert to baffle patch "
              << pbm[newPatchI].name()
              << endl
              << "    Run with -internalFacesOnly option"
              << " if you don't wish to convert"
              << " boundary faces." << endl;
          }
          modifyOrAddFace
          (
            meshMod,
            mesh.faces()[faceI],        // modified face
            faceI,                      // label of face
            mesh.faceOwner()[faceI],    // owner
            false,                      // face flip
            newPatchI,                  // patch for face
            fZone.index(),              // zone for face
            fZone.flipMap()[zoneFaceI], // face flip in zone
            modifiedFace                // modify or add
          );
          nModified++;
        }
      }
    }
  }
}


int main(int argc, char *argv[])
{
  argList::addNote
  (
    "Makes internal faces into boundary faces.\n"
    "Does not duplicate points."
  );
  #include "add_dict_option.inc"
  #include "add_overwrite_option.inc"
  #include "add_dict_option.inc"
  #include "add_region_option.inc"
  #include "set_root_case.inc"
  #include "create_time.inc"
  runTime.functionObjects().off();
  #include "create_named_mesh.inc"
  const bool overwrite = args.optionFound("overwrite");
  const word oldInstance = mesh.pointsInstance();
  const word dictName{"createBafflesDict"};
  #include "set_system_mesh_dictionary_io.inc"
  Switch internalFacesOnly{false};
  Switch noFields{false};
  PtrList<faceSelection> selectors;

  {
    Info << "Reading baffle criteria from " << dictName << nl << endl;
    IOdictionary dict{dictIO};
    dict.lookup("internalFacesOnly") >> internalFacesOnly;
    noFields = dict.lookupOrDefault("noFields", false);
    const dictionary& selectionsDict = dict.subDict("baffles");
    label n = 0;
    FOR_ALL_CONST_ITER(dictionary, selectionsDict, iter) {
      if (iter().isDict()) {
        n++;
      }
    }
    selectors.setSize(n);
    n = 0;
    FOR_ALL_CONST_ITER(dictionary, selectionsDict, iter) {
      if (iter().isDict()) {
        selectors.set
        (
          n++,
          faceSelection::New(iter().keyword(), mesh, iter().dict())
        );
      }
    }
  }
  if (internalFacesOnly) {
    Info << "Not converting faces on non-coupled patches." << nl << endl;
  }
  // Read objects in time directory
  IOobjectList objects{mesh, runTime.timeName()};
  // Read vol fields.
  Info << "Reading geometric fields" << nl << endl;
  PtrList<volScalarField> vsFlds;
  if (!noFields)
    ReadFields(mesh, objects, vsFlds);
  PtrList<volVectorField> vvFlds;
  if (!noFields)
    ReadFields(mesh, objects, vvFlds);
  PtrList<volSphericalTensorField> vstFlds;
  if (!noFields)
    ReadFields(mesh, objects, vstFlds);
  PtrList<volSymmTensorField> vsymtFlds;
  if (!noFields)
    ReadFields(mesh, objects, vsymtFlds);
  PtrList<volTensorField> vtFlds;
  if (!noFields)
    ReadFields(mesh, objects, vtFlds);
  // Read surface fields.
  PtrList<surfaceScalarField> ssFlds;
  if (!noFields)
    ReadFields(mesh, objects, ssFlds);
  PtrList<surfaceVectorField> svFlds;
  if (!noFields)
    ReadFields(mesh, objects, svFlds);
  PtrList<surfaceSphericalTensorField> sstFlds;
  if (!noFields)
    ReadFields(mesh, objects, sstFlds);
  PtrList<surfaceSymmTensorField> ssymtFlds;
  if (!noFields)
    ReadFields(mesh, objects, ssymtFlds);
  PtrList<surfaceTensorField> stFlds;
  if (!noFields)
    ReadFields(mesh, objects, stFlds);
  // Creating (if necessary) faceZones
  FOR_ALL(selectors, selectorI) {
    const word& name = selectors[selectorI].name();
    if (mesh.faceZones().findZoneID(name) != -1)
      continue;
    mesh.faceZones().clearAddressing();
    label sz = mesh.faceZones().size();
    labelList addr{0};
    boolList flip{0};
    mesh.faceZones().setSize(sz+1);
    mesh.faceZones().set
    (
      sz,
      new faceZone{name, addr, flip, sz, mesh.faceZones()}
    );
  }
  // Select faces
  // ~~~~~~~~~~~~
  //- Per face zoneID it is in and flip status.
  labelList faceToZoneID{mesh.nFaces(), -1};
  boolList faceToFlip{mesh.nFaces(), false};
  FOR_ALL(selectors, selectorI) {
    const word& name = selectors[selectorI].name();
    label zoneID = mesh.faceZones().findZoneID(name);
    selectors[selectorI].select(zoneID, faceToZoneID, faceToFlip);
  }
  // Add faces to faceZones
  labelList nFaces{mesh.faceZones().size(), 0};
  FOR_ALL(faceToZoneID, faceI) {
    label zoneID = faceToZoneID[faceI];
    if (zoneID != -1) {
      nFaces[zoneID]++;
    }
  }
  FOR_ALL(selectors, selectorI) {
    const word& name = selectors[selectorI].name();
    label zoneID = mesh.faceZones().findZoneID(name);
    label& n = nFaces[zoneID];
    labelList addr(n);
    boolList flip(n);
    n = 0;
    FOR_ALL(faceToZoneID, faceI) {
      label zone = faceToZoneID[faceI];
      if (zone == zoneID) {
        addr[n] = faceI;
        flip[n] = faceToFlip[faceI];
        n++;
      }
    }
    Info << "Created zone " << name
      << " at index " << zoneID
      << " with " << n << " faces" << endl;
    mesh.faceZones().set
    (
      zoneID,
      new faceZone{name, addr, flip, zoneID, mesh.faceZones()}
    );
  }
  // Count patches to add
  HashSet<word> bafflePatches;

  {
    FOR_ALL(selectors, selectorI) {
      const dictionary& dict = selectors[selectorI].dict();
      if (dict.found("patches")) {
        const dictionary& patchSources = dict.subDict("patches");
        FOR_ALL_CONST_ITER(dictionary, patchSources, iter) {
          const word patchName(iter().dict()["name"]);
          bafflePatches.insert(patchName);
        }
      } else {
        const word masterName = selectors[selectorI].name() + "_master";
        bafflePatches.insert(masterName);
        const word slaveName = selectors[selectorI].name() + "_slave";
        bafflePatches.insert(slaveName);
      }
    }
  }
  // Create baffles
  // ~~~~~~~~~~~~~~
  // Is done in multiple steps
  // - create patches with 'calculated' patchFields
  // - move faces into these patches
  // - change the patchFields to the wanted type
  // This order is done so e.g. fixedJump works:
  // - you cannot create patchfields at the same time as patches since
  //   they do an evaluate upon construction
  // - you want to create the patchField only after you have faces
  //   so you don't get the 'create-from-nothing' mapping problem.
  // Pass 1: add patches
  // ~~~~~~~~~~~~~~~~~~~
  //HashSet<word> addedPatches;

  {
    const polyBoundaryMesh& pbm = mesh.boundaryMesh();
    FOR_ALL(selectors, selectorI) {
      const dictionary& dict = selectors[selectorI].dict();
      const word& groupName = selectors[selectorI].name();
      if (dict.found("patches")) {
        const dictionary& patchSources = dict.subDict("patches");
        FOR_ALL_CONST_ITER(dictionary, patchSources, iter) {
          const word patchName{iter().dict()["name"]};
          if (pbm.findPatchID(patchName) == -1) {
            dictionary patchDict = iter().dict();
            patchDict.set("nFaces", 0);
            patchDict.set("startFace", 0);
            // Note: do not set coupleGroup if constructed from
            //       baffles so you have freedom specifying it
            //       yourself.
            //patchDict.set("coupleGroup", groupName);
            addPatch(mesh, patchName, groupName, patchDict);
          } else {
            Info << "Patch '" << patchName
              << "' already exists.  Only "
              << "moving patch faces - type will remain the same"
              << endl;
          }
        }
      } else {
        const dictionary& patchSource = dict.subDict("patchPairs");
        const word masterName = groupName + "_master";
        const word slaveName = groupName + "_slave";
        word groupNameMaster = groupName;
        word groupNameSlave = groupName;
        dictionary patchDictMaster{patchSource};
        patchDictMaster.set("nFaces", 0);
        patchDictMaster.set("startFace", 0);
        patchDictMaster.set("coupleGroup", groupName);
        dictionary patchDictSlave{patchDictMaster};
        // Note: This is added for the particular case where we want
        // master and slave in different groupNames
        // (ie 3D thermal baffles)
        Switch sameGroup{patchSource.lookupOrDefault("sameGroup", true)};
        if (!sameGroup) {
          groupNameMaster = groupName + "Group_master";
          groupNameSlave = groupName + "Group_slave";
          patchDictMaster.set("coupleGroup", groupNameMaster);
          patchDictSlave.set("coupleGroup", groupNameSlave);
        }
        addPatch(mesh, masterName, groupNameMaster, patchDictMaster);
        addPatch(mesh, slaveName, groupNameSlave, patchDictSlave);
      }
    }
  }
  // Make sure patches and zoneFaces are synchronised across couples
  mesh.boundaryMesh().checkParallelSync(true);
  mesh.faceZones().checkParallelSync(true);
  // Mesh change container
  polyTopoChange meshMod{mesh};
  const polyBoundaryMesh& pbm = mesh.boundaryMesh();
  // Do the actual changes. Note:
  // - loop in incrementing face order (not necessary if faceZone ordered).
  //   Preserves any existing ordering on patch faces.
  // - two passes, do non-flip faces first and flip faces second. This
  //   guarantees that when e.g. creating a cyclic all faces from one
  //   side come first and faces from the other side next.
  // Whether first use of face (modify) or consecutive (add)
  PackedBoolList modifiedFace{mesh.nFaces()};
  label nModified = 0;
  FOR_ALL(selectors, selectorI) {
    const word& name = selectors[selectorI].name();
    label zoneID = mesh.faceZones().findZoneID(name);
    const faceZone& fZone = mesh.faceZones()[zoneID];
    const dictionary& dict = selectors[selectorI].dict();
    DynamicList<label> newMasterPatches;
    DynamicList<label> newSlavePatches;
    if (dict.found("patches")) {
      const dictionary& patchSources = dict.subDict("patches");
      bool master = true;
      FOR_ALL_CONST_ITER(dictionary, patchSources, iter) {
        const word patchName(iter().dict()["name"]);
        label patchI = pbm.findPatchID(patchName);
        if (master) {
          newMasterPatches.append(patchI);
        } else {
          newSlavePatches.append(patchI);
        }
        master = !master;
      }
    } else {
      const word masterName = selectors[selectorI].name() + "_master";
      newMasterPatches.append(pbm.findPatchID(masterName));
      const word slaveName = selectors[selectorI].name() + "_slave";
      newSlavePatches.append(pbm.findPatchID(slaveName));
    }
    createFaces
    (
      internalFacesOnly,
      mesh,
      fZone,
      newMasterPatches,
      newSlavePatches,
      meshMod,
      modifiedFace,
      nModified
    );
  }
  Info << "Converted " << returnReduce(nModified, sumOp<label>())
    << " faces into boundary faces in patches "
    << bafflePatches.sortedToc() << nl << endl;
  if (!overwrite) {
    runTime++;
  }
  // Change the mesh. Change points directly (no inflation).
  autoPtr<mapPolyMesh> map = meshMod.changeMesh(mesh, false);
  // Update fields
  mesh.updateMesh(map);
  // Correct boundary faces mapped-out-of-nothing.
  // This is just a hack to correct the value field.
  {
    fvMeshMapper mapper{mesh, map};
    bool hasWarned = false;
    FOR_ALL_CONST_ITER(HashSet<word>, bafflePatches, iter) {
      label patchI = mesh.boundaryMesh().findPatchID(iter.key());
      const fvPatchMapper& pm = mapper.boundaryMap()[patchI];
      if (pm.sizeBeforeMapping() == 0) {
        if (!hasWarned) {
          hasWarned = true;
          WARNING_IN(args.executable())
            << "Setting field on boundary faces to zero." << endl
            << "You might have to edit these fields." << endl;
        }
        fvMeshTools::zeroPatchFields(mesh, patchI);
      }
    }
  }
  // Pass 2: change patchFields
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~
  {
    const polyBoundaryMesh& pbm = mesh.boundaryMesh();
    FOR_ALL(selectors, selectorI) {
      const dictionary& dict = selectors[selectorI].dict();
      if (dict.found("patches")) {
        const dictionary& patchSources = dict.subDict("patches");
        FOR_ALL_CONST_ITER(dictionary, patchSources, iter) {
          const word patchName(iter().dict()["name"]);
          label patchI = pbm.findPatchID(patchName);
          if (!iter().dict().found("patchFields"))
            continue;
          const dictionary& patchFieldsDict =
            iter().dict().subDict("patchFields");
          fvMeshTools::setPatchFields
          (
            mesh,
            patchI,
            patchFieldsDict
          );
        }
      } else {
        const dictionary& patchSource = dict.subDict("patchPairs");
        Switch sameGroup{patchSource.lookupOrDefault("sameGroup", true)};
        const word& groupName = selectors[selectorI].name();
        if (!patchSource.found("patchFields"))
          continue;
        dictionary patchFieldsDict = patchSource.subDict("patchFields");
        if (sameGroup) {
          // Add coupleGroup to all entries
          FOR_ALL_ITER(dictionary, patchFieldsDict, iter) {
            if (iter().isDict()) {
              dictionary& dict = iter().dict();
              dict.set("coupleGroup", groupName);
            }
          }
          const labelList& patchIDs =
            pbm.groupPatchIDs()[groupName];
          FOR_ALL(patchIDs, i) {
            fvMeshTools::setPatchFields
            (
              mesh,
              patchIDs[i],
              patchFieldsDict
            );
          }
        } else {
          const word masterPatchName(groupName + "_master");
          const word slavePatchName(groupName + "_slave");
          label patchIMaster = pbm.findPatchID(masterPatchName);
          label patchISlave = pbm.findPatchID(slavePatchName);
          fvMeshTools::setPatchFields
          (
            mesh,
            patchIMaster,
            patchFieldsDict
          );
          fvMeshTools::setPatchFields
          (
            mesh,
            patchISlave,
            patchFieldsDict
          );
        }
      }
    }
  }
  // Move mesh (since morphing might not do this)
  if (map().hasMotionPoints()) {
    mesh.movePoints(map().preMotionPoints());
  }
  if (overwrite) {
    mesh.setInstance(oldInstance);
  }
  Info << "Writing mesh to " << runTime.timeName() << endl;
  mesh.write();
  Info << "End\n" << endl;
  return 0;
}
