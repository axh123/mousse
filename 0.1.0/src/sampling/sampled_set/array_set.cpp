// mousse: CFD toolbox
// Copyright (C) 2011-2013 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "array_set.hpp"
#include "sampled_set.hpp"
#include "mesh_search.hpp"
#include "dynamic_list.hpp"
#include "poly_mesh.hpp"
#include "add_to_run_time_selection_table.hpp"
#include "word.hpp"
#include "transform.hpp"


// Static Data Members
namespace mousse {

DEFINE_TYPE_NAME_AND_DEBUG(arraySet, 0);
ADD_TO_RUN_TIME_SELECTION_TABLE(sampledSet, arraySet, word);

}


// Private Member Functions 
void mousse::arraySet::calcSamples
(
  DynamicList<point>& samplingPts,
  DynamicList<label>& samplingCells,
  DynamicList<label>& samplingFaces,
  DynamicList<label>& samplingSegments,
  DynamicList<scalar>& samplingCurveDist
) const
{
  const meshSearch& queryMesh = searchEngine();
  label nTotalSamples(pointsDensity_.x()*pointsDensity_.y()*pointsDensity_.z());
  List<point> sampleCoords{nTotalSamples};
  const scalar deltax = spanBox_.x()/(pointsDensity_.x() + 1);
  const scalar deltay = spanBox_.y()/(pointsDensity_.y() + 1);
  const scalar deltaz = spanBox_.z()/(pointsDensity_.z() + 1);
  label p{0};
  for (label k=1; k<=pointsDensity_.z(); k++) {
    for (label j=1; j<=pointsDensity_.y(); j++) {
      for (label i=1; i<=pointsDensity_.x(); i++) {
        vector t{deltax*i , deltay*j, deltaz*k};
        sampleCoords[p] = coordSys_.origin() + t;
        p++;
      }
    }
  }
  FOR_ALL(sampleCoords, i) {
    sampleCoords[i] = transform(coordSys_.R().R(), sampleCoords[i]);
  }
  FOR_ALL(sampleCoords, sampleI) {
    label cellI = queryMesh.findCell(sampleCoords[sampleI]);
    if (cellI != -1) {
      samplingPts.append(sampleCoords[sampleI]);
      samplingCells.append(cellI);
      samplingFaces.append(-1);
      samplingSegments.append(0);
      samplingCurveDist.append(1.0 * sampleI);
    }
  }
}


void mousse::arraySet::genSamples()
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
mousse::arraySet::arraySet
(
  const word& name,
  const polyMesh& mesh,
  const meshSearch& searchEngine,
  const word& axis,
  const coordinateSystem& origin,
  const Vector<label>& pointsDensity,
  const Vector<scalar>& spanBox
)
:
  sampledSet{name, mesh, searchEngine, axis},
  coordSys_{origin},
  pointsDensity_{pointsDensity},
  spanBox_{spanBox}
{
  genSamples();
  if (debug) {
    write(Info);
  }
}


mousse::arraySet::arraySet
(
  const word& name,
  const polyMesh& mesh,
  const meshSearch& searchEngine,
  const dictionary& dict
)
:
  sampledSet{name, mesh, searchEngine, dict},
  coordSys_{dict},
  pointsDensity_{dict.lookup("pointsDensity")},
  spanBox_{dict.lookup("spanBox")}
{
  genSamples();
  if (debug) {
    write(Info);
  }
}


// Destructor 
mousse::arraySet::~arraySet()
{}

