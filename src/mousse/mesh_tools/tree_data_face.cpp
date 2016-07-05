// mousse: CFD toolbox
// Copyright (C) 2011-2013 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "tree_data_face.hpp"
#include "poly_mesh.hpp"
#include "triangle_funcs.hpp"


// Static Data Members
namespace mousse {

DEFINE_TYPE_NAME_AND_DEBUG(treeDataFace, 0);
scalar treeDataFace::tolSqr = sqr(1e-6);

}


// Private Member Functions 
mousse::treeBoundBox mousse::treeDataFace::calcBb(const label faceI) const
{
  const pointField& points = mesh_.points();
  const face& f = mesh_.faces()[faceI];
  treeBoundBox bb{points[f[0]], points[f[0]]};
  for (label fp = 1; fp < f.size(); fp++) {
    const point& p = points[f[fp]];
    bb.min() = min(bb.min(), p);
    bb.max() = max(bb.max(), p);
  }
  return bb;
}


void mousse::treeDataFace::update()
{
  FOR_ALL(faceLabels_, i) {
    isTreeFace_.set(faceLabels_[i], 1);
  }
  if (cacheBb_) {
    bbs_.setSize(faceLabels_.size());
    FOR_ALL(faceLabels_, i) {
      bbs_[i] = calcBb(faceLabels_[i]);
    }
  }
}


// Constructors 
mousse::treeDataFace::treeDataFace
(
  const bool cacheBb,
  const primitiveMesh& mesh,
  const labelUList& faceLabels
)
:
  mesh_{mesh},
  faceLabels_{faceLabels},
  isTreeFace_{mesh.nFaces(), 0},
  cacheBb_{cacheBb}
{
  update();
}


mousse::treeDataFace::treeDataFace
(
  const bool cacheBb,
  const primitiveMesh& mesh,
  const Xfer<labelList>& faceLabels
)
:
  mesh_{mesh},
  faceLabels_{faceLabels},
  isTreeFace_{mesh.nFaces(), 0},
  cacheBb_{cacheBb}
{
  update();
}


mousse::treeDataFace::treeDataFace
(
  const bool cacheBb,
  const primitiveMesh& mesh
)
:
  mesh_{mesh},
  faceLabels_{identity(mesh_.nFaces())},
  isTreeFace_{mesh.nFaces(), 0},
  cacheBb_{cacheBb}
{
  update();
}


mousse::treeDataFace::treeDataFace
(
  const bool cacheBb,
  const polyPatch& patch
)
:
  mesh_{patch.boundaryMesh().mesh()},
  faceLabels_{identity(patch.size()) + patch.start()},
  isTreeFace_{mesh_.nFaces(), 0},
  cacheBb_{cacheBb}
{
  update();
}


mousse::treeDataFace::findNearestOp::findNearestOp
(
  const indexedOctree<treeDataFace>& tree
)
:
  tree_{tree}
{}


mousse::treeDataFace::findIntersectOp::findIntersectOp
(
  const indexedOctree<treeDataFace>& tree
)
:
  tree_{tree}
{}


// Member Functions 
mousse::pointField mousse::treeDataFace::shapePoints() const
{
  pointField cc{faceLabels_.size()};
  FOR_ALL(faceLabels_, i) {
    cc[i] = mesh_.faceCentres()[faceLabels_[i]];
  }
  return cc;
}


//- Get type (inside,outside,mixed,unknown) of point w.r.t. surface.
//  Only makes sense for closed surfaces.
mousse::volumeType mousse::treeDataFace::getVolumeType
(
  const indexedOctree<treeDataFace>& oc,
  const point& sample
) const
{
  // Need to determine whether sample is 'inside' or 'outside'
  // Done by finding nearest face. This gives back a face which is
  // guaranteed to contain nearest point. This point can be
  // - in interior of face: compare to face normal
  // - on edge of face: compare to edge normal
  // - on point of face: compare to point normal
  // Unfortunately the octree does not give us back the intersection point
  // or where on the face it has hit so we have to recreate all that
  // information.
  // Find nearest face to sample
  pointIndexHit info = oc.findNearest(sample, sqr(GREAT));
  if (info.index() == -1) {
    FATAL_ERROR_IN
    (
      "treeDataFace::getSampleType"
      "(indexedOctree<treeDataFace>&, const point&)"
    )
    << "Could not find " << sample << " in octree."
    << abort(FatalError);
  }
  // Get actual intersection point on face
  label faceI = faceLabels_[info.index()];
  if (debug & 2) {
    Pout << "getSampleType : sample:" << sample
      << " nearest face:" << faceI;
  }
  const pointField& points = mesh_.points();
  // Retest to classify where on face info is. Note: could be improved. We
  // already have point.
  const face& f = mesh_.faces()[faceI];
  const vector& area = mesh_.faceAreas()[faceI];
  const point& fc = mesh_.faceCentres()[faceI];
  pointHit curHit = f.nearestPoint(sample, points);
  const point& curPt = curHit.rawPoint();
  //
  // 1] Check whether sample is above face
  //
  if (curHit.hit()) {
    // Nearest point inside face. Compare to face normal.
    if (debug & 2) {
      Pout << " -> face hit:" << curPt
        << " comparing to face normal " << area << endl;
    }
    return indexedOctree<treeDataFace>::getSide(area, sample - curPt);
  }
  if (debug & 2) {
    Pout << " -> face miss:" << curPt;
  }
  //
  // 2] Check whether intersection is on one of the face vertices or
  //    face centre
  //
  const scalar typDimSqr = mag(area) + VSMALL;
  FOR_ALL(f, fp) {
    if ((magSqr(points[f[fp]] - curPt)/typDimSqr) >= tolSqr)
      continue;
    // Face intersection point equals face vertex fp
    // Calculate point normal (wrong: uses face normals instead of
    // triangle normals)
    const labelList& pFaces = mesh_.pointFaces()[f[fp]];
    vector pointNormal{vector::zero};
    FOR_ALL(pFaces, i) {
      if (isTreeFace_.get(pFaces[i]) != 1)
        continue;
      vector n = mesh_.faceAreas()[pFaces[i]];
      n /= mag(n) + VSMALL;
      pointNormal += n;
    }
    if (debug & 2) {
        Pout << " -> face point hit :" << points[f[fp]]
          << " point normal:" << pointNormal
          << " distance:"
          << magSqr(points[f[fp]] - curPt)/typDimSqr << endl;
    }
    return indexedOctree<treeDataFace>::getSide
    (
      pointNormal,
      sample - curPt
    );
  }
  if ((magSqr(fc - curPt)/typDimSqr) < tolSqr) {
    // Face intersection point equals face centre. Normal at face centre
    // is already average of face normals
    if (debug & 2) {
      Pout << " -> centre hit:" << fc
        << " distance:" << magSqr(fc - curPt)/typDimSqr << endl;
    }
    return indexedOctree<treeDataFace>::getSide(area,  sample - curPt);
  }
  //
  // 3] Get the 'real' edge the face intersection is on
  //
  const labelList& myEdges = mesh_.faceEdges()[faceI];
  FOR_ALL(myEdges, myEdgeI) {
    const edge& e = mesh_.edges()[myEdges[myEdgeI]];
    pointHit edgeHit =
      line<point, const point&>
      (
        points[e.start()],
        points[e.end()]
      ).nearestDist(sample);
    if ((magSqr(edgeHit.rawPoint() - curPt)/typDimSqr) >= tolSqr)
      continue;
    // Face intersection point lies on edge e
    // Calculate edge normal (wrong: uses face normals instead of
    // triangle normals)
    const labelList& eFaces = mesh_.edgeFaces()[myEdges[myEdgeI]];
    vector edgeNormal(vector::zero);
    FOR_ALL(eFaces, i) {
      if (isTreeFace_.get(eFaces[i]) != 1)
        continue;
      vector n = mesh_.faceAreas()[eFaces[i]];
      n /= mag(n) + VSMALL;
      edgeNormal += n;
    }
    if (debug & 2) {
      Pout << " -> real edge hit point:" << edgeHit.rawPoint()
        << " comparing to edge normal:" << edgeNormal
        << endl;
    }
    // Found face intersection point on this edge. Compare to edge
    // normal
    return indexedOctree<treeDataFace>::getSide
    (
      edgeNormal,
      sample - curPt
    );
  }
  //
  // 4] Get the internal edge the face intersection is on
  //
  FOR_ALL(f, fp) {
    pointHit edgeHit = line<point, const point&>
    (
      points[f[fp]],
      fc
    ).nearestDist(sample);
    if ((magSqr(edgeHit.rawPoint() - curPt)/typDimSqr) >= tolSqr)
      continue;
    // Face intersection point lies on edge between two face triangles
    // Calculate edge normal as average of the two triangle normals
    vector e = points[f[fp]] - fc;
    vector ePrev = points[f[f.rcIndex(fp)]] - fc;
    vector eNext = points[f[f.fcIndex(fp)]] - fc;
    vector nLeft = ePrev ^ e;
    nLeft /= mag(nLeft) + VSMALL;
    vector nRight = e ^ eNext;
    nRight /= mag(nRight) + VSMALL;
    if (debug & 2) {
      Pout << " -> internal edge hit point:" << edgeHit.rawPoint()
        << " comparing to edge normal "
        << 0.5*(nLeft + nRight)
        << endl;
    }
    // Found face intersection point on this edge. Compare to edge
    // normal
    return indexedOctree<treeDataFace>::getSide
    (
      0.5*(nLeft + nRight),
      sample - curPt
    );
  }
  if (debug & 2) {
    Pout << "Did not find sample " << sample
      << " anywhere related to nearest face " << faceI << endl
      << "Face:";
    FOR_ALL(f, fp) {
      Pout << "    vertex:" << f[fp] << "  coord:" << points[f[fp]]
        << endl;
    }
  }
  // Can't determine status of sample with respect to nearest face.
  // Either
  // - tolerances are wrong. (if e.g. face has zero area)
  // - or (more likely) surface is not closed.
  return volumeType::UNKNOWN;
}


// Check if any point on shape is inside cubeBb.
bool mousse::treeDataFace::overlaps
(
  const label index,
  const treeBoundBox& cubeBb
) const
{
  // 1. Quick rejection: bb does not intersect face bb at all
  if (cacheBb_) {
    if (!cubeBb.overlaps(bbs_[index])) {
      return false;
    }
  } else {
    if (!cubeBb.overlaps(calcBb(faceLabels_[index]))) {
      return false;
    }
  }
  const pointField& points = mesh_.points();
  // 2. Check if one or more face points inside
  label faceI = faceLabels_[index];
  const face& f = mesh_.faces()[faceI];
  if (cubeBb.containsAny(points, f)) {
    return true;
  }
  // 3. Difficult case: all points are outside but connecting edges might
  // go through cube. Use triangle-bounding box intersection.
  const point& fc = mesh_.faceCentres()[faceI];
  FOR_ALL(f, fp) {
    bool triIntersects = triangleFuncs::intersectBb
    (
      points[f[fp]],
      points[f[f.fcIndex(fp)]],
      fc,
      cubeBb
    );
    if (triIntersects) {
      return true;
    }
  }
  return false;
}


void mousse::treeDataFace::findNearestOp::operator()
(
  const labelUList& indices,
  const point& sample,
  scalar& nearestDistSqr,
  label& minIndex,
  point& nearestPoint
) const
{
  const treeDataFace& shape = tree_.shapes();
  FOR_ALL(indices, i) {
    const label index = indices[i];
    const face& f = shape.mesh().faces()[shape.faceLabels()[index]];
    pointHit nearHit = f.nearestPoint(sample, shape.mesh().points());
    scalar distSqr = sqr(nearHit.distance());
    if (distSqr < nearestDistSqr) {
      nearestDistSqr = distSqr;
      minIndex = index;
      nearestPoint = nearHit.rawPoint();
    }
  }
}


void mousse::treeDataFace::findNearestOp::operator()
(
  const labelUList& /*indices*/,
  const linePointRef& /*ln*/,
  treeBoundBox& /*tightest*/,
  label& /*minIndex*/,
  point& /*linePoint*/,
  point& /*nearestPoint*/
) const
{
  NOT_IMPLEMENTED
  (
    "treeDataFace::findNearestOp::operator()"
    "("
    "    const labelUList&,"
    "    const linePointRef&,"
    "    treeBoundBox&,"
    "    label&,"
    "    point&,"
    "    point&"
    ") const"
  );
}


bool mousse::treeDataFace::findIntersectOp::operator()
(
  const label index,
  const point& start,
  const point& end,
  point& intersectionPoint
) const
{
  const treeDataFace& shape = tree_.shapes();
  // Do quick rejection test
  if (shape.cacheBb_) {
    const treeBoundBox& faceBb = shape.bbs_[index];
    if ((faceBb.posBits(start) & faceBb.posBits(end)) != 0) {
      // start and end in same block outside of faceBb.
      return false;
    }
  }
  const label faceI = shape.faceLabels_[index];
  const vector dir(end - start);
  pointHit inter = shape.mesh_.faces()[faceI].intersection
  (
    start,
    dir,
    shape.mesh_.faceCentres()[faceI],
    shape.mesh_.points(),
    intersection::HALF_RAY
  );
  if (inter.hit() && inter.distance() <= 1) {
    // Note: no extra test on whether intersection is in front of us
    // since using half_ray
    intersectionPoint = inter.hitPoint();
    return true;
  }
  return false;
}
