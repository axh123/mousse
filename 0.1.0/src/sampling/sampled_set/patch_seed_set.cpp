// mousse: CFD toolbox
// Copyright (C) 2012-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "patch_seed_set.hpp"
#include "poly_mesh.hpp"
#include "add_to_run_time_selection_table.hpp"
#include "tree_bound_box.hpp"
#include "tree_data_face.hpp"
#include "time.hpp"
#include "mesh_tools.hpp"
//#include "random.hpp"
// For 'facePoint' helper function only
#include "mapped_patch_base.hpp"
#include "pstream_reduce_ops.hpp"


// Static Data Members
namespace mousse {

DEFINE_TYPE_NAME_AND_DEBUG(patchSeedSet, 0);
ADD_TO_RUN_TIME_SELECTION_TABLE(sampledSet, patchSeedSet, word);

}


// Private Member Functions 
void mousse::patchSeedSet::calcSamples
(
  DynamicList<point>& samplingPts,
  DynamicList<label>& samplingCells,
  DynamicList<label>& samplingFaces,
  DynamicList<label>& samplingSegments,
  DynamicList<scalar>& samplingCurveDist
)
{
  if (debug) {
    Info << "patchSeedSet : sampling on patches :" << endl;
  }
  // Construct search tree for all patch faces.
  label sz = 0;
  FOR_ALL_CONST_ITER(labelHashSet, patchSet_, iter) {
    const polyPatch& pp = mesh().boundaryMesh()[iter.key()];
    sz += pp.size();
    if (debug) {
      Info << "    " << pp.name() << " size " << pp.size() << endl;
    }
  }
  labelList patchFaces{sz};
  sz = 0;
  FOR_ALL_CONST_ITER(labelHashSet, patchSet_, iter) {
    const polyPatch& pp = mesh().boundaryMesh()[iter.key()];
    FOR_ALL(pp, i) {
      patchFaces[sz++] = pp.start()+i;
    }
  }
  label totalSize = returnReduce(sz, sumOp<label>());
  // Shuffle and truncate if in random mode
  if (maxPoints_ < totalSize) {
    // Check what fraction of maxPoints_ I need to generate locally.
    label myMaxPoints = static_cast<label>(scalar(sz)/totalSize*maxPoints_);
    rndGenPtr_.reset(new Random{123456});
    Random& rndGen = rndGenPtr_();
    labelList subset = identity(sz);
    for (label iter = 0; iter < 4; iter++) {
      FOR_ALL(subset, i) {
        label j = rndGen.integer(0, subset.size()-1);
        Swap(subset[i], subset[j]);
      }
    }
    // Truncate
    subset.setSize(myMaxPoints);
    // Subset patchFaces
    patchFaces = UIndirectList<label>{patchFaces, subset}();
    if (debug) {
      Pout << "In random mode : selected " << patchFaces.size()
        << " faces out of " << sz << endl;
    }
  }
  // Get points on patchFaces.
  globalIndex globalSampleNumbers{patchFaces.size()};
  samplingPts.setCapacity(patchFaces.size());
  samplingCells.setCapacity(patchFaces.size());
  samplingFaces.setCapacity(patchFaces.size());
  samplingSegments.setCapacity(patchFaces.size());
  samplingCurveDist.setCapacity(patchFaces.size());
  // For calculation of min-decomp tet base points
  (void)mesh().tetBasePtIs();
  FOR_ALL(patchFaces, i) {
    label faceI = patchFaces[i];
    pointIndexHit info = mappedPatchBase::facePoint(mesh(), faceI,
                                                    polyMesh::FACE_DIAG_TRIS);
    label cellI = mesh().faceOwner()[faceI];
    if (info.hit()) {
      // Move the point into the cell
      const point& cc = mesh().cellCentres()[cellI];
      samplingPts.append(info.hitPoint() + 1e-1*(cc-info.hitPoint()));
    } else {
      samplingPts.append(info.rawPoint());
    }
    samplingCells.append(cellI);
    samplingFaces.append(faceI);
    samplingSegments.append(0);
    samplingCurveDist.append(globalSampleNumbers.toGlobal(i));
  }
}


void mousse::patchSeedSet::genSamples()
{
  // Storage for sample points
  DynamicList<point> samplingPts;
  DynamicList<label> samplingCells;
  DynamicList<label> samplingFaces;
  DynamicList<label> samplingSegments;
  DynamicList<scalar> samplingCurveDist;
  calcSamples
  (
    samplingPts,
    samplingCells,
    samplingFaces,
    samplingSegments,
    samplingCurveDist
  );
  samplingPts.shrink();
  samplingCells.shrink();
  samplingFaces.shrink();
  samplingSegments.shrink();
  samplingCurveDist.shrink();
  setSamples
  (
    samplingPts,
    samplingCells,
    samplingFaces,
    samplingSegments,
    samplingCurveDist
  );
}


// Constructors 
mousse::patchSeedSet::patchSeedSet
(
  const word& name,
  const polyMesh& mesh,
  const meshSearch& searchEngine,
  const dictionary& dict
)
:
  sampledSet{name, mesh, searchEngine, dict},
  patchSet_
  {
    mesh.boundaryMesh().patchSet(wordReList{dict.lookup("patches")})
  },
  maxPoints_{readLabel(dict.lookup("maxPoints"))}
{
  genSamples();
  if (debug) {
    write(Info);
  }
}


// Destructor 
mousse::patchSeedSet::~patchSeedSet()
{}

