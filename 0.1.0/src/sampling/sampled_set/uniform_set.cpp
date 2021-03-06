// mousse: CFD toolbox
// Copyright (C) 2011-2014 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "uniform_set.hpp"
#include "mesh_search.hpp"
#include "dynamic_list.hpp"
#include "poly_mesh.hpp"
#include "add_to_run_time_selection_table.hpp"


// Static Data Members
namespace mousse {

DEFINE_TYPE_NAME_AND_DEBUG(uniformSet, 0);
ADD_TO_RUN_TIME_SELECTION_TABLE(sampledSet, uniformSet, word);

}


// Private Member Functions 
bool mousse::uniformSet::nextSample
(
  const point& currentPt,
  const vector& offset,
  const scalar smallDist,
  point& samplePt,
  label& sampleI
) const
{
  bool pointFound = false;
  const vector normOffset = offset/mag(offset);
  samplePt += offset;
  sampleI++;
  for (; sampleI < nPoints_; sampleI++) {
    scalar s = (samplePt - currentPt) & normOffset;
    if (s > -smallDist) {
      // samplePt is close to or beyond currentPt -> use it
      pointFound = true;
      break;
    }
    samplePt += offset;
  }
  return pointFound;
}


bool mousse::uniformSet::trackToBoundary
(
  passiveParticleCloud& particleCloud,
  passiveParticle& singleParticle,
  point& samplePt,
  label& sampleI,
  DynamicList<point>& samplingPts,
  DynamicList<label>& samplingCells,
  DynamicList<label>& samplingFaces,
  DynamicList<scalar>& samplingCurveDist
) const
{
  // distance vector between sampling points
  const vector offset = (end_ - start_)/(nPoints_ - 1);
  const vector smallVec = tol*offset;
  const scalar smallDist = mag(smallVec);
  // Alias
  const point& trackPt = singleParticle.position();
  particle::TrackingData<passiveParticleCloud> trackData(particleCloud);
  while(true) {
    // Find next samplePt on/after trackPt. Update samplePt, sampleI
    if (!nextSample(trackPt, offset, smallDist, samplePt, sampleI)) {
      // no more samples.
      if (debug) {
        Pout << "trackToBoundary : Reached end : samplePt now:"
          << samplePt << "  sampleI now:" << sampleI << endl;
      }
      return false;
    }
    if (mag(samplePt - trackPt) < smallDist) {
      // trackPt corresponds with samplePt. Store and use next samplePt
      if (debug) {
        Pout << "trackToBoundary : samplePt corresponds to trackPt : "
          << "  trackPt:" << trackPt << "  samplePt:" << samplePt
          << endl;
      }
      samplingPts.append(trackPt);
      samplingCells.append(singleParticle.cell());
      samplingFaces.append(-1);
      samplingCurveDist.append(mag(trackPt - start_));
      // go to next samplePt
      if (!nextSample(trackPt, offset, smallDist, samplePt, sampleI)) {
        // no more samples.
        if (debug) {
          Pout << "trackToBoundary : Reached end : "
            << "  samplePt now:" << samplePt
            << "  sampleI now:" << sampleI
            << endl;
        }
        return false;
      }
    }
    if (debug) {
      Pout << "Searching along trajectory from "
        << "  trackPt:" << trackPt
        << "  trackCellI:" << singleParticle.cell()
        << "  to:" << samplePt << endl;
    }
    point oldPos = trackPt;
    label facei = -1;
    do {
      singleParticle.stepFraction() = 0;
      singleParticle.track(samplePt, trackData);
      if (debug) {
        Pout
          << "Result of tracking "
          << "  trackPt:" << trackPt
          << "  trackCellI:" << singleParticle.cell()
          << "  trackFaceI:" << singleParticle.face()
          << "  onBoundary:" << singleParticle.onBoundary()
          << "  samplePt:" << samplePt
          << "  smallDist:" << smallDist
          << endl;
      }
    }
    while (!singleParticle.onBoundary()
           && (mag(trackPt - oldPos) < smallDist));
    if (singleParticle.onBoundary()) {
      //Pout<< "trackToBoundary : reached boundary" << endl;
      if (mag(trackPt - samplePt) < smallDist) {
        // Reached samplePt on boundary
        samplingPts.append(trackPt);
        samplingCells.append(singleParticle.cell());
        samplingFaces.append(facei);
        samplingCurveDist.append(mag(trackPt - start_));
      }
      return true;
    }
    // Reached samplePt in cell or on internal face
    samplingPts.append(trackPt);
    samplingCells.append(singleParticle.cell());
    samplingFaces.append(-1);
    samplingCurveDist.append(mag(trackPt - start_));
    // go to next samplePt
  }
}


void mousse::uniformSet::calcSamples
(
  DynamicList<point>& samplingPts,
  DynamicList<label>& samplingCells,
  DynamicList<label>& samplingFaces,
  DynamicList<label>& samplingSegments,
  DynamicList<scalar>& samplingCurveDist
) const
{
  // distance vector between sampling points
  if ((nPoints_ < 2) || (mag(end_ - start_) < SMALL)) {
    FATAL_ERROR_IN("uniformSet::calcSamples()")
      << "Incorrect sample specification. Either too few points or"
      << " start equals end point." << endl
      << "nPoints:" << nPoints_
      << "  start:" << start_
      << "  end:" << end_
      << exit(FatalError);
  }
  const vector offset = (end_ - start_)/(nPoints_ - 1);
  const vector normOffset = offset/mag(offset);
  const vector smallVec = tol*offset;
  const scalar smallDist = mag(smallVec);
  // Force calculation of cloud addressing on all processors
  const bool oldMoving = const_cast<polyMesh&>(mesh()).moving(false);
  passiveParticleCloud particleCloud(mesh());
  // Get all boundary intersections
  List<pointIndexHit> bHits =
    searchEngine().intersections(start_ - smallVec, end_ + smallVec);
  point bPoint{GREAT, GREAT, GREAT};
  label bFaceI = -1;
  if (bHits.size()) {
    bPoint = bHits[0].hitPoint();
    bFaceI = bHits[0].index();
  }
  // Get first tracking point. Use bPoint, bFaceI if provided.
  point trackPt;
  label trackCellI = -1;
  label trackFaceI = -1;
  bool isSample = getTrackingPoint(offset, start_, bPoint, bFaceI, trackPt,
                                   trackCellI, trackFaceI);
  if (trackCellI == -1) {
    // Line start_ - end_ does not intersect domain at all.
    // (or is along edge)
    // Set points and cell/face labels to empty lists
    return;
  }
  if (isSample) {
    samplingPts.append(start_);
    samplingCells.append(trackCellI);
    samplingFaces.append(trackFaceI);
    samplingCurveDist.append(0.0);
  }
  //
  // Track until hit end of all boundary intersections
  //
  // current segment number
  label segmentI = 0;
  // starting index of current segment in samplePts
  label startSegmentI = 0;
  label sampleI = 0;
  point samplePt = start_;
  // index in bHits; current boundary intersection
  label bHitI = 1;
  while(true) {
    // Initialize tracking starting from trackPt
    passiveParticle singleParticle(mesh(), trackPt, trackCellI);
    bool reachedBoundary = trackToBoundary(particleCloud, singleParticle,
                                           samplePt, sampleI, samplingPts,
                                           samplingCells, samplingFaces,
                                           samplingCurveDist);
    // fill sampleSegments
    for (label i = samplingPts.size() - 1; i >= startSegmentI; --i) {
      samplingSegments.append(segmentI);
    }
    if (!reachedBoundary) {
      if (debug) {
        Pout << "calcSamples : Reached end of samples: "
          << "  samplePt now:" << samplePt
          << "  sampleI now:" << sampleI
          << endl;
      }
      break;
    }
    bool foundValidB = false;
    while (bHitI < bHits.size()) {
      scalar dist =
        (bHits[bHitI].hitPoint() - singleParticle.position()) & normOffset;
      if (debug) {
        Pout << "Finding next boundary : "
          << "bPoint:" << bHits[bHitI].hitPoint()
          << "  tracking:" << singleParticle.position()
          << "  dist:" << dist
          << endl;
      }
      if (dist > smallDist) {
        // hitpoint is past tracking position
        foundValidB = true;
        break;
      } else {
        bHitI++;
      }
    }
    if (!foundValidB) {
      // No valid boundary intersection found beyond tracking position
      break;
    }
    // Update starting point for tracking
    trackFaceI = bFaceI;
    trackPt = pushIn(bPoint, trackFaceI);
    trackCellI = getBoundaryCell(trackFaceI);
    segmentI++;
    startSegmentI = samplingPts.size();
  }
  const_cast<polyMesh&>(mesh()).moving(oldMoving);
}


void mousse::uniformSet::genSamples()
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
mousse::uniformSet::uniformSet
(
  const word& name,
  const polyMesh& mesh,
  const meshSearch& searchEngine,
  const word& axis,
  const point& start,
  const point& end,
  const label nPoints
)
:
  sampledSet{name, mesh, searchEngine, axis},
  start_{start},
  end_{end},
  nPoints_{nPoints}
{
  genSamples();
  if (debug) {
    write(Pout);
  }
}


mousse::uniformSet::uniformSet
(
  const word& name,
  const polyMesh& mesh,
  const meshSearch& searchEngine,
  const dictionary& dict
)
:
  sampledSet{name, mesh, searchEngine, dict},
  start_{dict.lookup("start")},
  end_{dict.lookup("end")},
  nPoints_{readLabel(dict.lookup("nPoints"))}
{
  genSamples();
  if (debug) {
    write(Pout);
  }
}


// Destructor 
mousse::uniformSet::~uniformSet()
{}

