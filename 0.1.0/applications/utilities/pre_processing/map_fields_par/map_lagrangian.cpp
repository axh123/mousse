// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "map_lagrangian_fields.hpp"
#include "passive_particle_cloud.hpp"
#include "mesh_search.hpp"


namespace mousse {

static const scalar perturbFactor = 1e-6;


// Special version of findCell that generates a cell guaranteed to be
// compatible with tracking.
static label findCell(const Cloud<passiveParticle>& cloud, const point& pt)
{
  label cellI = -1;
  label tetFaceI = -1;
  label tetPtI = -1;
  const polyMesh& mesh = cloud.pMesh();
  mesh.findCellFacePt(pt, cellI, tetFaceI, tetPtI);
  if (cellI >= 0) {
    return cellI;
  } else {
    // See if particle on face by finding nearest face and shifting
    // particle.
    meshSearch meshSearcher
    {
      mesh,
      polyMesh::FACE_PLANES    // no decomposition needed
    };
    label faceI = meshSearcher.findNearestBoundaryFace(pt);
    if (faceI >= 0) {
      const point& cc = mesh.cellCentres()[mesh.faceOwner()[faceI]];
      const point perturbPt = (1 - perturbFactor)*pt + perturbFactor*cc;
      mesh.findCellFacePt(perturbPt, cellI, tetFaceI, tetPtI);
      return cellI;
    }
  }
  return -1;
}


void mapLagrangian(const meshToMesh& interp)
{
  // Determine which particles are in meshTarget
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  const polyMesh& meshSource = interp.srcRegion();
  const polyMesh& meshTarget = interp.tgtRegion();
  const labelListList& sourceToTarget = interp.srcToTgtCellAddr();
  const pointField& targetCc = meshTarget.cellCentres();
  fileNameList cloudDirs
  {
    readDir(meshSource.time().timePath()/cloud::prefix, fileName::DIRECTORY)
  };
  FOR_ALL(cloudDirs, cloudI) {
    // Search for list of lagrangian objects for this time
    IOobjectList objects
    {
      meshSource,
      meshSource.time().timeName(),
      cloud::prefix/cloudDirs[cloudI]
    };
    IOobject* positionsPtr = objects.lookup(word("positions"));
    if (!positionsPtr)
      continue;
    Info << nl << "    processing cloud " << cloudDirs[cloudI] << endl;
    // Read positions & cell
    passiveParticleCloud sourceParcels
    {
      meshSource,
      cloudDirs[cloudI],
      false
    };
    Info << "    read " << sourceParcels.size()
      << " parcels from source mesh." << endl;
    // Construct empty target cloud
    passiveParticleCloud targetParcels
    {
      meshTarget,
      cloudDirs[cloudI],
      IDLList<passiveParticle>()
    };
    particle::TrackingData<passiveParticleCloud> td{targetParcels};
    label sourceParticleI = 0;
    // Indices of source particles that get added to targetParcels
    DynamicList<label> addParticles{sourceParcels.size()};
    // Unmapped particles
    labelHashSet unmappedSource{sourceParcels.size()};
    // Initial: track from fine-mesh cell centre to particle position
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // This requires there to be no boundary in the way.
    FOR_ALL_CONST_ITER(Cloud<passiveParticle>, sourceParcels, iter) {
      bool foundCell = false;
      // Assume that cell from read parcel is the correct one...
      if (iter().cell() >= 0) {
        const labelList& targetCells = sourceToTarget[iter().cell()];
        // Particle probably in one of the targetcells. Try
        // all by tracking from their cell centre to the parcel
        // position.
        FOR_ALL(targetCells, i) {
          // Track from its cellcentre to position to make sure.
          autoPtr<passiveParticle> newPtr
          {
            new passiveParticle
            {
              meshTarget,
              targetCc[targetCells[i]],
              targetCells[i]
            }
          };
          passiveParticle& newP = newPtr();
          label faceI = newP.track(iter().position(), td);
          if (faceI < 0 && newP.cell() >= 0) {
            // Hit position.
            foundCell = true;
            addParticles.append(sourceParticleI);
            targetParcels.addParticle(newPtr.ptr());
            break;
          }
        }
      }
      if (!foundCell) {
        // Store for closer analysis
        unmappedSource.insert(sourceParticleI);
      }
      sourceParticleI++;
    }
    Info << "    after meshToMesh addressing found "
      << targetParcels.size()
      << " parcels in target mesh." << endl;
    // Do closer inspection for unmapped particles
    if (unmappedSource.size()) {
      sourceParticleI = 0;
      FOR_ALL_ITER(Cloud<passiveParticle>, sourceParcels, iter) {
        if (unmappedSource.found(sourceParticleI)) {
          label targetCell = findCell(targetParcels, iter().position());
          if (targetCell >= 0) {
            unmappedSource.erase(sourceParticleI);
            addParticles.append(sourceParticleI);
            iter().cell() = targetCell;
            targetParcels.addParticle
            (
              sourceParcels.remove(&iter())
            );
          }
        }
        sourceParticleI++;
      }
    }
    addParticles.shrink();
    Info << "    after additional mesh searching found "
      << targetParcels.size() << " parcels in target mesh." << endl;
    if (addParticles.size()) {
      IOPosition<passiveParticleCloud>{targetParcels}.write();
      // addParticles now contains the indices of the sourceMesh
      // particles that were appended to the target mesh.
      // Map lagrangian fields
      // ~~~~~~~~~~~~~~~~~~~~~
      MapLagrangianFields<label>
      (
        cloudDirs[cloudI],
        objects,
        meshTarget,
        addParticles
      );
      MapLagrangianFields<scalar>
      (
        cloudDirs[cloudI],
        objects,
        meshTarget,
        addParticles
      );
      MapLagrangianFields<vector>
      (
        cloudDirs[cloudI],
        objects,
        meshTarget,
        addParticles
      );
      MapLagrangianFields<sphericalTensor>
      (
        cloudDirs[cloudI],
        objects,
        meshTarget,
        addParticles
      );
      MapLagrangianFields<symmTensor>
      (
        cloudDirs[cloudI],
        objects,
        meshTarget,
        addParticles
      );
      MapLagrangianFields<tensor>
      (
        cloudDirs[cloudI],
        objects,
        meshTarget,
        addParticles
      );
    }
  }
}

}  // namespace mousse

