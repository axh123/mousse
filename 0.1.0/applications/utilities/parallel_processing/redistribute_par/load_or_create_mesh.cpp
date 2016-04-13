// mousse: CFD toolbox
// Copyright (C) 2012-2014 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "load_or_create_mesh.hpp"
#include "processor_poly_patch.hpp"
#include "processor_cyclic_poly_patch.hpp"
#include "time.hpp"
#include "ioptr_list.hpp"

// Global Functions 
namespace mousse {
DEFINE_TEMPLATE_TYPE_NAME_AND_DEBUG(IOPtrList<entry>, 0);
}

// Read mesh if available. Otherwise create empty mesh with same non-proc
// patches as proc0 mesh. Requires all processors to have all patches
// (and in same order).
mousse::autoPtr<mousse::fvMesh> mousse::loadOrCreateMesh
(
  const IOobject& io
)
{
  fileName meshSubDir;
  if (io.name() == polyMesh::defaultRegion)
  {
    meshSubDir = polyMesh::meshSubDir;
  }
  else
  {
    meshSubDir = io.name()/polyMesh::meshSubDir;
  }
  // Scatter master patches
  PtrList<entry> patchEntries;
  if (Pstream::master())
  {
    // Read PtrList of dictionary as dictionary.
    const word oldTypeName = IOPtrList<entry>::typeName;
    const_cast<word&>(IOPtrList<entry>::typeName) = word::null;
    IOPtrList<entry> dictList
    {
      {
        "boundary",
        io.time().findInstance
        (
          meshSubDir,
          "boundary",
          IOobject::MUST_READ
        ),
        meshSubDir,
        io.db(),
        IOobject::MUST_READ,
        IOobject::NO_WRITE,
        false
      }
    };
    const_cast<word&>(IOPtrList<entry>::typeName) = oldTypeName;
    // Fake type back to what was in field
    const_cast<word&>(dictList.type()) = dictList.headerClassName();
    patchEntries.transfer(dictList);
    // Send patches
    for
    (
      int slave=Pstream::firstSlave();
      slave<=Pstream::lastSlave();
      slave++
    )
    {
      OPstream toSlave{Pstream::scheduled, slave};
      toSlave << patchEntries;
    }
  }
  else
  {
    // Receive patches
    IPstream fromMaster{Pstream::scheduled, Pstream::masterNo()};
    fromMaster >> patchEntries;
  }
  // Check who has a mesh
  const bool haveMesh = isDir(io.time().path()/io.instance()/meshSubDir);
  if (!haveMesh)
  {
    bool oldParRun = Pstream::parRun();
    Pstream::parRun() = false;
    // Create dummy mesh. Only used on procs that don't have mesh.
    IOobject noReadIO{io};
    noReadIO.readOpt() = IOobject::NO_READ;
    fvMesh dummyMesh
    {
      noReadIO,
      xferCopy(pointField()),
      xferCopy(faceList()),
      xferCopy(labelList()),
      xferCopy(labelList()),
      false
    };
    // Add patches
    List<polyPatch*> patches{patchEntries.size()};
    label nPatches = 0;
    FOR_ALL(patchEntries, patchI)
    {
      const entry& e = patchEntries[patchI];
      const word type{e.dict().lookup("type")};
      const word& name = e.keyword();
      if (type != processorPolyPatch::typeName
          && type != processorCyclicPolyPatch::typeName)
      {
        dictionary patchDict(e.dict());
        patchDict.set("nFaces", 0);
        patchDict.set("startFace", 0);
        patches[patchI] = polyPatch::New
        (
          name,
          patchDict,
          nPatches++,
          dummyMesh.boundaryMesh()
        ).ptr();
      }
    }
    patches.setSize(nPatches);
    dummyMesh.addFvPatches(patches, false);  // no parallel comms
    // Add some dummy zones so upon reading it does not read them
    // from the undecomposed case. Should be done as extra argument to
    // regIOobject::readStream?
    List<pointZone*> pz
    {
      1,
      new pointZone
      {
        "dummyPointZone",
        labelList(0),
        0,
        dummyMesh.pointZones()
      }
    };
    List<faceZone*> fz
    {
      1,
      new faceZone
      {
        "dummyFaceZone",
        labelList(0),
        boolList(0),
        0,
        dummyMesh.faceZones()
      }
    };
    List<cellZone*> cz
    {
      1,
      new cellZone
      {
        "dummyCellZone",
        labelList(0),
        0,
        dummyMesh.cellZones()
      }
    };
    dummyMesh.addZones(pz, fz, cz);
    dummyMesh.write();
    Pstream::parRun() = oldParRun;
  }
  autoPtr<fvMesh> meshPtr{new fvMesh{io}};
  fvMesh& mesh = meshPtr();
  // Sync patches
  // ~~~~~~~~~~~~
  if (!Pstream::master() && haveMesh)
  {
    // Check master names against mine
    const polyBoundaryMesh& patches = mesh.boundaryMesh();
    FOR_ALL(patchEntries, patchI)
    {
      const entry& e = patchEntries[patchI];
      const word type{e.dict().lookup("type")};
      const word& name = e.keyword();
      if (type == processorPolyPatch::typeName)
      {
        break;
      }
      if (patchI >= patches.size())
      {
        FATAL_ERROR_IN
        (
          "createMesh(const Time&, const fileName&, const bool)"
        )
        << "Non-processor patches not synchronised."
        << endl
        << "Processor " << Pstream::myProcNo()
        << " has only " << patches.size()
        << " patches, master has "
        << patchI
        << exit(FatalError);
      }
      if (type != patches[patchI].type() || name != patches[patchI].name())
      {
        FATAL_ERROR_IN
        (
          "createMesh(const Time&, const fileName&, const bool)"
        )
        << "Non-processor patches not synchronised."
        << endl
        << "Master patch " << patchI
        << " name:" << type
        << " type:" << type << endl
        << "Processor " << Pstream::myProcNo()
        << " patch " << patchI
        << " has name:" << patches[patchI].name()
        << " type:" << patches[patchI].type()
        << exit(FatalError);
      }
    }
  }
  // Determine zones
  // ~~~~~~~~~~~~~~~
  wordList pointZoneNames{mesh.pointZones().names()};
  Pstream::scatter(pointZoneNames);
  wordList faceZoneNames{mesh.faceZones().names()};
  Pstream::scatter(faceZoneNames);
  wordList cellZoneNames{mesh.cellZones().names()};
  Pstream::scatter(cellZoneNames);
  if (!haveMesh)
  {
    // Add the zones. Make sure to remove the old dummy ones first
    mesh.pointZones().clear();
    mesh.faceZones().clear();
    mesh.cellZones().clear();
    List<pointZone*> pz{pointZoneNames.size()};
    FOR_ALL(pointZoneNames, i)
    {
      pz[i] = new pointZone
      {
        pointZoneNames[i],
        labelList(0),
        i,
        mesh.pointZones()
      };
    }
    List<faceZone*> fz{faceZoneNames.size()};
    FOR_ALL(faceZoneNames, i)
    {
      fz[i] = new faceZone
      {
        faceZoneNames[i],
        labelList(0),
        boolList(0),
        i,
        mesh.faceZones()
      };
    }
    List<cellZone*> cz{cellZoneNames.size()};
    FOR_ALL(cellZoneNames, i)
    {
      cz[i] = new cellZone
      {
        cellZoneNames[i],
        labelList(0),
        i,
        mesh.cellZones()
      };
    }
    mesh.addZones(pz, fz, cz);
  }
  if (!haveMesh)
  {
    // We created a dummy mesh file above. Delete it.
    const fileName meshFiles = io.time().path()/io.instance()/meshSubDir;
    //Pout<< "Removing dummy mesh " << meshFiles << endl;
    mesh.removeFiles();
    rmDir(meshFiles);
  }
  // Force recreation of globalMeshData.
  mesh.clearOut();
  mesh.globalData();
  // Do some checks.
  // Check if the boundary definition is unique
  mesh.boundaryMesh().checkDefinition(true);
  // Check if the boundary processor patches are correct
  mesh.boundaryMesh().checkParallelSync(true);
  // Check names of zones are equal
  mesh.cellZones().checkDefinition(true);
  mesh.cellZones().checkParallelSync(true);
  mesh.faceZones().checkDefinition(true);
  mesh.faceZones().checkParallelSync(true);
  mesh.pointZones().checkDefinition(true);
  mesh.pointZones().checkParallelSync(true);
  return meshPtr;
}
