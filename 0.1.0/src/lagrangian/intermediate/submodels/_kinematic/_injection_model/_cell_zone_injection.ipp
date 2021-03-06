// mousse: CFD toolbox
// Copyright (C) 2011-2014 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "_cell_zone_injection.hpp"
#include "mathematical_constants.hpp"
#include "poly_mesh_tet_decomposition.hpp"
#include "global_index.hpp"
#include "pstream.hpp"


// Private Member Functions 
template<class CloudType>
void mousse::CellZoneInjection<CloudType>::setPositions
(
  const labelList& cellZoneCells
)
{
  const fvMesh& mesh = this->owner().mesh();
  const scalarField& V = mesh.V();
  const label nCells = cellZoneCells.size();
  cachedRandom& rnd = this->owner().rndGen();
  DynamicList<vector> positions{nCells};          // initial size only
  DynamicList<label> injectorCells{nCells};       // initial size only
  DynamicList<label> injectorTetFaces{nCells};    // initial size only
  DynamicList<label> injectorTetPts{nCells};      // initial size only
  scalar newParticlesTotal = 0.0;
  label addParticlesTotal = 0;
  FOR_ALL(cellZoneCells, i) {
    const label cellI = cellZoneCells[i];
    // Calc number of particles to add
    const scalar newParticles = V[cellI]*numberDensity_;
    newParticlesTotal += newParticles;
    label addParticles = floor(newParticles);
    addParticlesTotal += addParticles;
    const scalar diff = newParticlesTotal - addParticlesTotal;
    if (diff > 1) {
      label corr = floor(diff);
      addParticles += corr;
      addParticlesTotal += corr;
    }
    // Construct cell tet indices
    const List<tetIndices> cellTetIs =
      polyMeshTetDecomposition::cellTetIndices(mesh, cellI);
    // Construct cell tet volume fractions
    scalarList cTetVFrac{cellTetIs.size(), 0.0};
    for (label tetI = 1; tetI < cellTetIs.size() - 1; tetI++) {
      cTetVFrac[tetI] =
        cTetVFrac[tetI-1] + cellTetIs[tetI].tet(mesh).mag()/V[cellI];
    }
    cTetVFrac.last() = 1.0;
    // Set new particle position and cellId
    for (label pI = 0; pI < addParticles; pI++) {
      const scalar volFrac = rnd.sample01<scalar>();
      label tetI = 0;
      FOR_ALL(cTetVFrac, vfI) {
        if (cTetVFrac[vfI] > volFrac) {
          tetI = vfI;
          break;
        }
      }
      positions.append(cellTetIs[tetI].tet(mesh).randomPoint(rnd));
      injectorCells.append(cellI);
      injectorTetFaces.append(cellTetIs[tetI].face());
      injectorTetPts.append(cellTetIs[tetI].tetPt());
    }
  }
  // Parallel operation manipulations
  globalIndex globalPositions{positions.size()};
  List<vector> allPositions{globalPositions.size(), point::max};
  List<label> allInjectorCells{globalPositions.size(), -1};
  List<label> allInjectorTetFaces{globalPositions.size(), -1};
  List<label> allInjectorTetPts{globalPositions.size(), -1};
  // Gather all positions on to all processors
  SubList<vector>
  {
    allPositions,
    globalPositions.localSize(Pstream::myProcNo()),
    globalPositions.offset(Pstream::myProcNo())
  }.assign(positions);
  Pstream::listCombineGather(allPositions, minEqOp<point>());
  Pstream::listCombineScatter(allPositions);
  // Gather local cell tet and tet-point Ids, but leave non-local ids set -1
  SubList<label>
  {
    allInjectorCells,
    globalPositions.localSize(Pstream::myProcNo()),
    globalPositions.offset(Pstream::myProcNo())
  }.assign(injectorCells);
  SubList<label>
  {
    allInjectorTetFaces,
    globalPositions.localSize(Pstream::myProcNo()),
    globalPositions.offset(Pstream::myProcNo())
  }.assign(injectorTetFaces);
  SubList<label>
  {
    allInjectorTetPts,
    globalPositions.localSize(Pstream::myProcNo()),
    globalPositions.offset(Pstream::myProcNo())
  }.assign(injectorTetPts);
  // Transfer data
  positions_.transfer(allPositions);
  injectorCells_.transfer(allInjectorCells);
  injectorTetFaces_.transfer(allInjectorTetFaces);
  injectorTetPts_.transfer(allInjectorTetPts);
  if (debug) {
    OFstream points{"points.obj"};
    FOR_ALL(positions_, i) {
      meshTools::writeOBJ(points, positions_[i]);
    }
  }
}


// Constructors 
template<class CloudType>
mousse::CellZoneInjection<CloudType>::CellZoneInjection
(
  const dictionary& dict,
  CloudType& owner,
  const word& modelName
)
:
  InjectionModel<CloudType>{dict, owner, modelName, typeName},
  cellZoneName_{this->coeffDict().lookup("cellZone")},
  numberDensity_{readScalar(this->coeffDict().lookup("numberDensity"))},
  positions_{},
  injectorCells_{},
  injectorTetFaces_{},
  injectorTetPts_{},
  diameters_{},
  U0_{this->coeffDict().lookup("U0")},
  sizeDistribution_
  {
    distributionModels::distributionModel::New
    (
      this->coeffDict().subDict("sizeDistribution"), owner.rndGen()
    )
  }
{
  updateMesh();
}


template<class CloudType>
mousse::CellZoneInjection<CloudType>::CellZoneInjection
(
  const CellZoneInjection<CloudType>& im
)
:
  InjectionModel<CloudType>{im},
  cellZoneName_{im.cellZoneName_},
  numberDensity_{im.numberDensity_},
  positions_{im.positions_},
  injectorCells_{im.injectorCells_},
  injectorTetFaces_{im.injectorTetFaces_},
  injectorTetPts_{im.injectorTetPts_},
  diameters_{im.diameters_},
  U0_{im.U0_},
  sizeDistribution_{im.sizeDistribution_().clone().ptr()}
{}


// Destructor 
template<class CloudType>
mousse::CellZoneInjection<CloudType>::~CellZoneInjection()
{}


// Member Functions 
template<class CloudType>
void mousse::CellZoneInjection<CloudType>::updateMesh()
{
  // Set/cache the injector cells
  const fvMesh& mesh = this->owner().mesh();
  const label zoneI = mesh.cellZones().findZoneID(cellZoneName_);
  if (zoneI < 0) {
    FATAL_ERROR_IN("mousse::CellZoneInjection<CloudType>::updateMesh()")
      << "Unknown cell zone name: " << cellZoneName_
      << ". Valid cell zones are: " << mesh.cellZones().names()
      << nl << exit(FatalError);
  }
  const labelList& cellZoneCells = mesh.cellZones()[zoneI];
  const label nCells = cellZoneCells.size();
  const scalar nCellsTotal = returnReduce(nCells, sumOp<label>());
  const scalar VCells = sum(scalarField(mesh.V(), cellZoneCells));
  const scalar VCellsTotal = returnReduce(VCells, sumOp<scalar>());
  Info << "    cell zone size      = " << nCellsTotal << endl;
  Info << "    cell zone volume    = " << VCellsTotal << endl;
  if ((nCellsTotal == 0) || (VCellsTotal*numberDensity_ < 1)) {
    WARNING_IN("mousse::CellZoneInjection<CloudType>::updateMesh()")
      << "Number of particles to be added to cellZone " << cellZoneName_
      << " is zero" << endl;
  } else {
    setPositions(cellZoneCells);
    Info << "    number density      = " << numberDensity_ << nl
      << "    number of particles = " << positions_.size() << endl;
    // Construct parcel diameters
    diameters_.setSize(positions_.size());
    FOR_ALL(diameters_, i) {
      diameters_[i] = sizeDistribution_->sample();
    }
  }
  // Determine volume of particles to inject
  this->volumeTotal_ = sum(pow3(diameters_))*constant::mathematical::pi/6.0;
}


template<class CloudType>
mousse::scalar mousse::CellZoneInjection<CloudType>::timeEnd() const
{
  // Not used
  return this->SOI_;
}


template<class CloudType>
mousse::label mousse::CellZoneInjection<CloudType>::parcelsToInject
(
  const scalar time0,
  const scalar time1
)
{
  if ((0.0 >= time0) && (0.0 < time1)) {
    return positions_.size();
  } else {
    return 0;
  }
}


template<class CloudType>
mousse::scalar mousse::CellZoneInjection<CloudType>::volumeToInject
(
  const scalar time0,
  const scalar time1
)
{
  // All parcels introduced at SOI
  if ((0.0 >= time0) && (0.0 < time1)) {
    return this->volumeTotal_;
  } else {
    return 0.0;
  }
}


template<class CloudType>
void mousse::CellZoneInjection<CloudType>::setPositionAndCell
(
  const label parcelI,
  const label /*nParcels*/,
  const scalar /*time*/,
  vector& position,
  label& cellOwner,
  label& tetFaceI,
  label& tetPtI
)
{
  position = positions_[parcelI];
  cellOwner = injectorCells_[parcelI];
  tetFaceI = injectorTetFaces_[parcelI];
  tetPtI = injectorTetPts_[parcelI];
}


template<class CloudType>
void mousse::CellZoneInjection<CloudType>::setProperties
(
  const label parcelI,
  const label,
  const scalar,
  typename CloudType::parcelType& parcel
)
{
  // set particle velocity
  parcel.U() = U0_;
  // set particle diameter
  parcel.d() = diameters_[parcelI];
}


template<class CloudType>
bool mousse::CellZoneInjection<CloudType>::fullyDescribed() const
{
  return false;
}


template<class CloudType>
bool mousse::CellZoneInjection<CloudType>::validInjection(const label)
{
  return true;
}

