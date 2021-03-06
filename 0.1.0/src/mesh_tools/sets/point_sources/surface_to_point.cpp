// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "surface_to_point.hpp"
#include "poly_mesh.hpp"
#include "tri_surface_search.hpp"
#include "tri_surface.hpp"
#include "cpu_time.hpp"
#include "add_to_run_time_selection_table.hpp"


// Static Data Members
namespace mousse {

DEFINE_TYPE_NAME_AND_DEBUG(surfaceToPoint, 0);
ADD_TO_RUN_TIME_SELECTION_TABLE(topoSetSource, surfaceToPoint, word);
ADD_TO_RUN_TIME_SELECTION_TABLE(topoSetSource, surfaceToPoint, istream);

}

mousse::topoSetSource::addToUsageTable mousse::surfaceToPoint::usage_
{
  surfaceToPoint::typeName,
  "\n    Usage: surfaceToPoint <surface> <near> <inside> <outside>\n\n"
  "    <surface> name of triSurface\n"
  "    <near> scalar; include points with coordinate <= near to surface\n"
  "    <inside> boolean; whether to include points on opposite side of"
  " surface normal\n"
  "    <outside> boolean; whether to include points on this side of"
  " surface normal\n\n"
};


// Private Member Functions 
void mousse::surfaceToPoint::combine(topoSet& set, const bool add) const
{
  cpuTime timer;
  triSurface surf{surfName_};
  Info << "    Read surface from " << surfName_
    << " in = "<< timer.cpuTimeIncrement() << " s" << endl << endl;
  // Construct search engine on surface
  triSurfaceSearch querySurf{surf};
  if (includeInside_ || includeOutside_) {
    boolList pointInside{querySurf.calcInside(mesh_.points())};
    FOR_ALL(pointInside, pointI) {
      bool isInside = pointInside[pointI];
      if ((isInside && includeInside_) || (!isInside && includeOutside_)) {
        addOrDelete(set, pointI, add);
      }
    }
  }
  if (nearDist_ > 0) {
    const pointField& meshPoints = mesh_.points();
    // Box dimensions to search in octree.
    const vector span{nearDist_, nearDist_, nearDist_};
    FOR_ALL(meshPoints, pointI) {
      const point& pt = meshPoints[pointI];
      pointIndexHit inter = querySurf.nearest(pt, span);
      if (inter.hit() && (mag(inter.hitPoint() - pt) < nearDist_)) {
        addOrDelete(set, pointI, add);
      }
    }
  }
}


void mousse::surfaceToPoint::checkSettings() const
{
  if (nearDist_ < 0 && !includeInside_ && !includeOutside_) {
    FATAL_ERROR_IN("surfaceToPoint:checkSettings()")
      << "Illegal point selection specification."
      << " Result would be either all or no points." << endl
      << "Please set one of includeInside or includeOutside"
      << " to true, set nearDistance to a value > 0"
      << exit(FatalError);
  }
}


// Constructors 
mousse::surfaceToPoint::surfaceToPoint
(
  const polyMesh& mesh,
  const fileName& surfName,
  const scalar nearDist,
  const bool includeInside,
  const bool includeOutside
)
:
  topoSetSource{mesh},
  surfName_{surfName},
  nearDist_{nearDist},
  includeInside_{includeInside},
  includeOutside_{includeOutside}
{
  checkSettings();
}


mousse::surfaceToPoint::surfaceToPoint
(
  const polyMesh& mesh,
  const dictionary& dict
)
:
  topoSetSource{mesh},
  surfName_{fileName(dict.lookup("file")).expand()},
  nearDist_{readScalar(dict.lookup("nearDistance"))},
  includeInside_{readBool(dict.lookup("includeInside"))},
  includeOutside_{readBool(dict.lookup("includeOutside"))}
{
  checkSettings();
}


mousse::surfaceToPoint::surfaceToPoint
(
  const polyMesh& mesh,
  Istream& is
)
:
  topoSetSource{mesh},
  surfName_{checkIs(is)},
  nearDist_{readScalar(checkIs(is))},
  includeInside_{readBool(checkIs(is))},
  includeOutside_{readBool(checkIs(is))}
{
  checkSettings();
}


// Destructor 
mousse::surfaceToPoint::~surfaceToPoint()
{}


// Member Functions 
void mousse::surfaceToPoint::applyToSet
(
  const topoSetSource::setAction action,
  topoSet& set
) const
{
  if ( (action == topoSetSource::NEW) || (action == topoSetSource::ADD)) {
    Info << "    Adding points in relation to surface " << surfName_
      << " ..." << endl;
    combine(set, true);
  } else if (action == topoSetSource::DELETE) {
    Info << "    Removing points in relation to surface " << surfName_
      << " ..." << endl;
    combine(set, false);
  }
}
