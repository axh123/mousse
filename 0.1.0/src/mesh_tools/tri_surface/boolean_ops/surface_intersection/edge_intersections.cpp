// mousse: CFD toolbox
// Copyright (C) 2011-2014 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "edge_intersections.hpp"
#include "tri_surface_search.hpp"
#include "label_pair_lookup.hpp"
#include "ofstream.hpp"
#include "hash_set.hpp"
#include "tri_surface.hpp"
#include "point_index_hit.hpp"
#include "tree_data_tri_surface.hpp"
#include "indexed_octree.hpp"
#include "mesh_tools.hpp"
#include "plane.hpp"
#include "random.hpp"
#include "unit_conversion.hpp"
#include "tree_bound_box.hpp"
// Static Data Members
namespace mousse
{
DEFINE_TYPE_NAME_AND_DEBUG(edgeIntersections, 0);
scalar edgeIntersections::alignedCos_ = cos(degToRad(89.0));
}
// Private Member Functions 
void mousse::edgeIntersections::checkEdges(const triSurface& surf)
{
  const pointField& localPoints = surf.localPoints();
  const edgeList& edges = surf.edges();
  const labelListList& edgeFaces = surf.edgeFaces();
  treeBoundBox bb(localPoints);
  scalar minSize = SMALL * bb.minDim();
  FOR_ALL(edges, edgeI)
  {
    const edge& e = edges[edgeI];
    scalar eMag = e.mag(localPoints);
    if (eMag < minSize)
    {
      WARNING_IN
      (
        "mousse::edgeIntersections::checkEdges(const triSurface& surf)"
      )   << "Edge " << edgeI << " vertices " << e
        << " coords:" << localPoints[e[0]] << ' '
        << localPoints[e[1]] << " is very small compared to bounding"
        << " box dimensions " << bb << endl
        << "This might lead to problems in intersection"
        << endl;
    }
    if (edgeFaces[edgeI].size() == 1)
    {
      WARNING_IN
      (
        "mousse::edgeIntersections::checkEdges(const triSurface& surf)"
      )   << "Edge " << edgeI << " vertices " << e
        << " coords:" << localPoints[e[0]] << ' '
        << localPoints[e[1]] << " has only one face connected to it:"
        << edgeFaces[edgeI] << endl
        << "This might lead to problems in intersection"
        << endl;
    }
  }
}
// Update intersections for selected edges.
void mousse::edgeIntersections::intersectEdges
(
  const triSurface& surf1,
  const pointField& points1,          // surf1 meshPoints (not localPoints!)
  const triSurfaceSearch& querySurf2,
  const scalarField& surf1PointTol,   // surf1 tolerance per point
  const labelList& edgeLabels
)
{
  const triSurface& surf2 = querySurf2.surface();
  const vectorField& normals2 = surf2.faceNormals();
  const labelList& meshPoints = surf1.meshPoints();
  if (debug)
  {
    Pout<< "Calculating intersection of " << edgeLabels.size() << " edges"
      << " out of " << surf1.nEdges() << " with " << surf2.size()
      << " triangles ..." << endl;
  }
  pointField start(edgeLabels.size());
  pointField end(edgeLabels.size());
  vectorField edgeDirs(edgeLabels.size());
  // Go through all edges, calculate intersections
  FOR_ALL(edgeLabels, i)
  {
    label edgeI = edgeLabels[i];
    if (debug)// && (i % 1000 == 0))
    {
      Pout<< "Intersecting edge " << edgeI << " with surface" << endl;
    }
    const edge& e = surf1.edges()[edgeI];
    const point& pStart = points1[meshPoints[e.start()]];
    const point& pEnd = points1[meshPoints[e.end()]];
    const vector eVec(pEnd - pStart);
    const vector n(eVec/(mag(eVec) + VSMALL));
    // Start tracking somewhat before pStart and up to somewhat after p1.
    // Note that tolerances here are smaller than those used to classify
    // hit below.
    // This will cause this hit to be marked as degenerate and resolved
    // later on.
    start[i] = pStart - 0.5*surf1PointTol[e[0]]*n;
    end[i] = pEnd + 0.5*surf1PointTol[e[1]]*n;
    edgeDirs[i] = n;
  }
  List<List<pointIndexHit> > edgeIntersections;
  querySurf2.findLineAll
  (
    start,
    end,
    edgeIntersections
  );
  label nHits = 0;
  // Classify the hits
  FOR_ALL(edgeLabels, i)
  {
    const label edgeI = edgeLabels[i];
    labelList& intersectionTypes = classification_[edgeI];
    intersectionTypes.setSize(edgeIntersections[i].size(), -1);
    this->operator[](edgeI).transfer(edgeIntersections[i]);
    FOR_ALL(intersectionTypes, hitI)
    {
      const pointIndexHit& pHit = this->operator[](edgeI)[hitI];
      label& hitType = intersectionTypes[hitI];
      if (!pHit.hit())
      {
        continue;
      }
      const edge& e = surf1.edges()[edgeI];
      // Classify point on surface1 edge.
      if (mag(pHit.hitPoint() - start[i]) < surf1PointTol[e[0]])
      {
        // Intersection is close to edge start
        hitType = 0;
      }
      else if (mag(pHit.hitPoint() - end[i]) < surf1PointTol[e[1]])
      {
        // Intersection is close to edge end
        hitType = 1;
      }
      else if (mag(edgeDirs[i] & normals2[pHit.index()]) < alignedCos_)
      {
        // Edge is almost coplanar with the face
        Pout<< "Flat angle edge:" << edgeI
          << " face:" << pHit.index()
          << " cos:" << mag(edgeDirs[i] & normals2[pHit.index()])
          << endl;
        hitType = 2;
      }
      if (debug)
      {
        Info<< "    hit " << pHit << " classify = " << hitType << endl;
      }
      nHits++;
    }
  }
  if (debug)
  {
    Pout<< "Found " << nHits << " intersections of edges with surface ..."
      << endl;
  }
}
// If edgeI intersections are close to endpoint of edge shift endpoints
// slightly along edge
// Updates
// - points1 with new endpoint position
// - affectedEdges with all edges affected by moving the point
// Returns true if changed anything.
bool mousse::edgeIntersections::inlinePerturb
(
  const triSurface& surf1,
  const scalarField& surf1PointTol,   // surf1 tolerance per point
  const label edgeI,
  Random& rndGen,
  pointField& points1,
  boolList& affectedEdges
) const
{
  bool hasPerturbed = false;
  // Check if edge close to endpoint. Note that we only have to check
  // the intersection closest to the edge endpoints (i.e. first and last in
  // edgeEdgs)
  const labelList& edgeEnds = classification_[edgeI];
  if (edgeEnds.size())
  {
    bool perturbStart = false;
    bool perturbEnd = false;
    // Check first intersection.
    if (edgeEnds.first() == 0)
    {
      perturbStart = true;
    }
    if (edgeEnds.last() == 1)
    {
      perturbEnd = true;
    }
    if (perturbStart || perturbEnd)
    {
      const edge& e = surf1.edges()[edgeI];
      label v0 = surf1.meshPoints()[e[0]];
      label v1 = surf1.meshPoints()[e[1]];
      vector eVec(points1[v1] - points1[v0]);
      vector n = eVec/mag(eVec);
      if (perturbStart)
      {
        // Perturb with something (hopefully) larger than tolerance.
        scalar t = 4.0*(rndGen.scalar01() - 0.5);
        points1[v0] += t*surf1PointTol[e[0]]*n;
        const labelList& pEdges = surf1.pointEdges()[e[0]];
        FOR_ALL(pEdges, i)
        {
          affectedEdges[pEdges[i]] = true;
        }
      }
      if (perturbEnd)
      {
        // Perturb with something larger than tolerance.
        scalar t = 4.0*(rndGen.scalar01() - 0.5);
        points1[v1] += t*surf1PointTol[e[1]]*n;
        const labelList& pEdges = surf1.pointEdges()[e[1]];
        FOR_ALL(pEdges, i)
        {
          affectedEdges[pEdges[i]] = true;
        }
      }
      hasPerturbed = true;
    }
  }
  return hasPerturbed;
}
// Perturb single edge endpoint when perpendicular to face
bool mousse::edgeIntersections::rotatePerturb
(
  const triSurface& surf1,
  const scalarField& surf1PointTol,   // surf1 tolerance per point
  const label edgeI,
  Random& rndGen,
  pointField& points1,
  boolList& affectedEdges
) const
{
  const labelList& meshPoints = surf1.meshPoints();
  const labelList& edgeEnds = classification_[edgeI];
  bool hasPerturbed = false;
  FOR_ALL(edgeEnds, i)
  {
    if (edgeEnds[i] == 2)
    {
      const edge& e = surf1.edges()[edgeI];
      // Endpoint to modify. Choose either start or end.
      label pointI = e[rndGen.bit()];
      //label pointI = e[0];
      // Generate random vector slightly larger than tolerance.
      vector rndVec = rndGen.vector01() - vector(0.5, 0.5, 0.5);
      // Make sure rndVec only perp to edge
      vector n(points1[meshPoints[e[1]]] - points1[meshPoints[e[0]]]);
      scalar magN = mag(n) + VSMALL;
      n /= magN;
      rndVec -= n*(n & rndVec);
      // Normalize
      rndVec /= mag(rndVec) + VSMALL;
      // Scale to be moved by tolerance.
      rndVec *= 0.01*magN;
      Pout<< "rotating: shifting endpoint " << meshPoints[pointI]
        << " of edge:" << edgeI << " verts:"
        << points1[meshPoints[e[0]]] << ' '
        << points1[meshPoints[e[1]]]
        << " by " << rndVec
        << " tol:" << surf1PointTol[pointI] << endl;
      points1[meshPoints[pointI]] += rndVec;
      // Mark edges affected by change to point
      const labelList& pEdges = surf1.pointEdges()[pointI];
      FOR_ALL(pEdges, i)
      {
        affectedEdges[pEdges[i]] = true;
      }
      hasPerturbed = true;
      // Enough done for current edge; no need to test other intersections
      // of this edge.
      break;
    }
  }
  return hasPerturbed;
}
// Perturb edge when close to face
bool mousse::edgeIntersections::offsetPerturb
(
  const triSurface& surf1,
  const triSurface& surf2,
  const label edgeI,
  Random& rndGen,
  pointField& points1,
  boolList& affectedEdges
) const
{
  const labelList& meshPoints = surf1.meshPoints();
  const edge& e = surf1.edges()[edgeI];
  const List<pointIndexHit>& hits = operator[](edgeI);
  bool hasPerturbed = false;
  // For all hits on edge
  FOR_ALL(hits, i)
  {
    const pointIndexHit& pHit = hits[i];
    // Classify point on face of surface2
    label surf2FaceI = pHit.index();
    const triSurface::FaceType& f2 = surf2.localFaces()[surf2FaceI];
    const pointField& surf2Pts = surf2.localPoints();
    const point ctr = f2.centre(surf2Pts);
    label nearType, nearLabel;
    f2.nearestPointClassify(pHit.hitPoint(), surf2Pts, nearType, nearLabel);
    if (nearType == triPointRef::POINT || nearType == triPointRef::EDGE)
    {
      // Shift edge towards tri centre
      vector offset = 0.01*rndGen.scalar01()*(ctr - pHit.hitPoint());
      // shift e[0]
      points1[meshPoints[e[0]]] += offset;
      // Mark edges affected by change to e0
      const labelList& pEdges0 = surf1.pointEdges()[e[0]];
      FOR_ALL(pEdges0, i)
      {
        affectedEdges[pEdges0[i]] = true;
      }
      // shift e[1]
      points1[meshPoints[e[1]]] += offset;
      // Mark edges affected by change to e1
      const labelList& pEdges1 = surf1.pointEdges()[e[1]];
      FOR_ALL(pEdges1, i)
      {
        affectedEdges[pEdges1[i]] = true;
      }
      hasPerturbed = true;
      // No need to test any other hits on this edge
      break;
    }
  }
  return hasPerturbed;
}
// Constructors 
// Construct null
mousse::edgeIntersections::edgeIntersections()
:
  List<List<pointIndexHit> >(),
  classification_()
{}
// Construct from surface and tolerance
mousse::edgeIntersections::edgeIntersections
(
  const triSurface& surf1,
  const triSurfaceSearch& query2,
  const scalarField& surf1PointTol
)
:
  List<List<pointIndexHit> >(surf1.nEdges()),
  classification_(surf1.nEdges())
{
  checkEdges(surf1);
  checkEdges(query2.surface());
  // Current set of edges to test
  labelList edgesToTest(surf1.nEdges());
  // Start off with all edges
  FOR_ALL(edgesToTest, i)
  {
    edgesToTest[i] = i;
  }
  // Determine intersections for edgesToTest
  intersectEdges
  (
    surf1,
    surf1.points(), // surf1 meshPoints (not localPoints!)
    query2,
    surf1PointTol,
    edgesToTest
  );
}
// Construct from components
mousse::edgeIntersections::edgeIntersections
(
  const List<List<pointIndexHit> >& intersections,
  const labelListList& classification
)
:
  List<List<pointIndexHit> >(intersections),
  classification_(classification)
{}
// Static Functions 
mousse::scalarField mousse::edgeIntersections::minEdgeLength(const triSurface& surf)
{
  const pointField& localPoints = surf.localPoints();
  const labelListList& pointEdges = surf.pointEdges();
  const edgeList& edges = surf.edges();
  scalarField minLen(localPoints.size());
  FOR_ALL(minLen, pointI)
  {
    const labelList& pEdges = pointEdges[pointI];
    scalar minDist = GREAT;
    FOR_ALL(pEdges, i)
    {
      minDist = min(minDist, edges[pEdges[i]].mag(localPoints));
    }
    minLen[pointI] = minDist;
  }
  return minLen;
}
// Member Functions 
mousse::label mousse::edgeIntersections::removeDegenerates
(
  const label nIters,
  const triSurface& surf1,
  const triSurfaceSearch& query2,
  const scalarField& surf1PointTol,
  pointField& points1
)
{
  const triSurface& surf2 = query2.surface();
  Random rndGen(356574);
  // Current set of edges to (re)test
  labelList edgesToTest(surf1.nEdges());
  // Start off with all edges
  FOR_ALL(edgesToTest, i)
  {
    edgesToTest[i] = i;
  }
  label iter = 0;
  for (; iter < nIters; iter++)
  {
    // Go through all edges to (re)test and perturb points if they are
    // degenerate hits. Mark off edges that need to be recalculated.
    boolList affectedEdges(surf1.nEdges(), false);
    label nShifted = 0;
    label nRotated = 0;
    label nOffset = 0;
    FOR_ALL(edgesToTest, i)
    {
      label edgeI = edgesToTest[i];
      // If edge not already marked for retesting
      if (!affectedEdges[edgeI])
      {
        // 1. Check edges close to endpoint and perturb if necessary.
        bool shiftedEdgeEndPoints =
          inlinePerturb
          (
            surf1,
            surf1PointTol,
            edgeI,
            rndGen,
            points1,
            affectedEdges
          );
        nShifted += (shiftedEdgeEndPoints ? 1 : 0);
        if (!shiftedEdgeEndPoints)
        {
          bool rotatedEdge =
            rotatePerturb
            (
              surf1,
              surf1PointTol,
              edgeI,
              rndGen,
              points1,
              affectedEdges
            );
          nRotated += (rotatedEdge ? 1 : 0);
          if (!rotatedEdge)
          {
            // 2. we're sure now that the edge actually pierces the
            // face. Now check the face for intersections close its
            // points/edges
            bool offsetEdgePoints =
              offsetPerturb
              (
                surf1,
                surf2,
                edgeI,
                rndGen,
                points1,
                affectedEdges
              );
            nOffset += (offsetEdgePoints ? 1 : 0);
          }
        }
      }
    }
    if (debug)
    {
      Pout<< "Edges to test : " << nl
        << "    total:" << edgesToTest.size() << nl
        << "    resolved by:" << nl
        << "        shifting   : " << nShifted << nl
        << "        rotating   : " << nRotated << nl
        << "        offsetting : " << nOffset << nl
        << endl;
    }
    if (nShifted == 0 && nRotated == 0 && nOffset == 0)
    {
      // Nothing changed in current iteration. Current hit pattern is
      // without any degenerates.
      break;
    }
    // Repack affected edges
    labelList newEdgesToTest(surf1.nEdges());
    label newEdgeI = 0;
    FOR_ALL(affectedEdges, edgeI)
    {
      if (affectedEdges[edgeI])
      {
        newEdgesToTest[newEdgeI++] = edgeI;
      }
    }
    newEdgesToTest.setSize(newEdgeI);
    if (debug)
    {
      Pout<< "Edges to test:" << nl
        << "    was : " << edgesToTest.size() << nl
        << "    is  : " << newEdgesToTest.size() << nl
        << endl;
    }
    // Transfer and test.
    edgesToTest.transfer(newEdgesToTest);
    if (edgesToTest.empty())
    {
      FATAL_ERROR_IN("perturb") << "oops" << abort(FatalError);
    }
    // Re intersect moved edges.
    intersectEdges
    (
      surf1,
      points1,          // surf1 meshPoints (not localPoints!)
      query2,
      surf1PointTol,
      edgesToTest
    );
  }
  return iter;
}
void mousse::edgeIntersections::merge
(
  const edgeIntersections& subInfo,
  const labelList& edgeMap,
  const labelList& faceMap,
  const bool merge
)
{
  FOR_ALL(subInfo, subI)
  {
    const List<pointIndexHit>& subHits = subInfo[subI];
    const labelList& subClass = subInfo.classification()[subI];
    label edgeI = edgeMap[subI];
    List<pointIndexHit>& intersections = operator[](edgeI);
    labelList& intersectionTypes = classification_[edgeI];
    // Count unique hits. Assume edge can hit face only once
    label sz = 0;
    if (merge)
    {
      sz = intersections.size();
    }
    label nNew = 0;
    FOR_ALL(subHits, i)
    {
      const pointIndexHit& subHit = subHits[i];
      bool foundFace = false;
      for (label interI = 0; interI < sz; interI++)
      {
        if (intersections[interI].index() == faceMap[subHit.index()])
        {
          foundFace = true;
          break;
        }
      }
      if (!foundFace)
      {
        nNew++;
      }
    }
    intersections.setSize(sz+nNew);
    intersectionTypes.setSize(sz+nNew);
    nNew = sz;
    FOR_ALL(subHits, i)
    {
      const pointIndexHit& subHit = subHits[i];
      bool foundFace = false;
      for (label interI = 0; interI < sz; interI++)
      {
        if (intersections[interI].index() == faceMap[subHit.index()])
        {
          foundFace = true;
          break;
        }
      }
      if (!foundFace)
      {
        intersections[nNew] = pointIndexHit
        (
          subHit.hit(),
          subHit.rawPoint(),
          faceMap[subHit.index()]
        );
        intersectionTypes[nNew] = subClass[i];
        nNew++;
      }
    }
  }
}
