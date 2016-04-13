// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "cell_features.hpp"
#include "primitive_mesh.hpp"
#include "hash_set.hpp"
#include "map.hpp"
#include "demand_driven_data.hpp"
#include "list_ops.hpp"
#include "mesh_tools.hpp"


// Private Member Functions 
// Return true if edge start and end are on increasing face vertices. (edge is
// guaranteed to be on face)
bool mousse::cellFeatures::faceAlignedEdge(const label faceI, const label edgeI)
const
{
  const edge& e = mesh_.edges()[edgeI];
  const face& f = mesh_.faces()[faceI];
  FOR_ALL(f, fp) {
    if (f[fp] == e.start()) {
      label fp1 = f.fcIndex(fp);
      return f[fp1] == e.end();
    }
  }
  FATAL_ERROR_IN
  (
    "cellFeatures::faceAlignedEdge(const label, const label)"
  )
  << "Can not find edge " << mesh_.edges()[edgeI]
  << " on face " << faceI << abort(FatalError);
  return false;
}


// Return edge in featureEdge that uses vertI and is on same superface
// but is not edgeI
mousse::label mousse::cellFeatures::nextEdge
(
  const Map<label>& toSuperFace,
  const label superFaceI,
  const label thisEdgeI,
  const label thisVertI
) const
{
  const labelList& pEdges = mesh_.pointEdges()[thisVertI];
  FOR_ALL(pEdges, pEdgeI) {
    label edgeI = pEdges[pEdgeI];
    if ((edgeI == thisEdgeI) || !featureEdge_.found(edgeI))
      continue;
    // Check that edge is used by a face on same superFace
    const labelList& eFaces = mesh_.edgeFaces()[edgeI];
    FOR_ALL(eFaces, eFaceI) {
      label faceI = eFaces[eFaceI];
      if (meshTools::faceOnCell(mesh_, cellI_, faceI)
          && (toSuperFace[faceI] == superFaceI)) {
        return edgeI;
      }
    }
  }
  FATAL_ERROR_IN
  (
    "cellFeatures::nextEdge(const label, const Map<label>"
    ", const labelHashSet&, const label, const label, const label)"
  )
  << "Can not find edge in " << featureEdge_ << " connected to edge "
  << thisEdgeI << " at vertex " << thisVertI << endl
  << "This might mean that the externalEdges do not form a closed loop"
  << abort(FatalError);
  return -1;
}


// Return true if angle between faces using it is larger than certain value.
bool mousse::cellFeatures::isCellFeatureEdge
(
  const scalar minCos,
  const label edgeI
) const
{
  // Get the two faces using this edge
  label face0;
  label face1;
  meshTools::getEdgeFaces(mesh_, cellI_, edgeI, face0, face1);
  // Check the angle between them by comparing the face normals.
  vector n0 = mesh_.faceAreas()[face0];
  n0 /= mag(n0);
  vector n1 = mesh_.faceAreas()[face1];
  n1 /= mag(n1);
  scalar cosAngle = n0 & n1;
  const edge& e = mesh_.edges()[edgeI];
  const face& f0 = mesh_.faces()[face0];
  label face0Start = findIndex(f0, e.start());
  label face0End   = f0.fcIndex(face0Start);
  const face& f1 = mesh_.faces()[face1];
  label face1Start = findIndex(f1, e.start());
  label face1End   = f1.fcIndex(face1Start);
  if (((f0[face0End] == e.end())
       && (f1[face1End] != e.end()))
      || ((f0[face0End] != e.end())
          && (f1[face1End] == e.end()))) {
  }
  else {
    cosAngle = -cosAngle;
  }
  if (cosAngle < minCos) {
    return true;
  } else {
    return false;
  }
}


// Recursively mark (on toSuperFace) all face reachable across non-feature
// edges.
void mousse::cellFeatures::walkSuperFace
(
  const label faceI,
  const label superFaceI,
  Map<label>& toSuperFace
) const
{
  if (!toSuperFace.found(faceI))
    return;

  toSuperFace.insert(faceI, superFaceI);
  const labelList& fEdges = mesh_.faceEdges()[faceI];
  FOR_ALL(fEdges, fEdgeI)
  {
    label edgeI = fEdges[fEdgeI];
    if (featureEdge_.found(edgeI))
      continue;
    label face0;
    label face1;
    meshTools::getEdgeFaces(mesh_, cellI_, edgeI, face0, face1);
    if (face0 == faceI) {
      face0 = face1;
    }
    walkSuperFace
    (
      face0,
      superFaceI,
      toSuperFace
    );
  }
}


void mousse::cellFeatures::calcSuperFaces() const
{
  // Determine superfaces by edge walking across non-feature edges
  const labelList& cFaces = mesh_.cells()[cellI_];
  // Mapping from old to super face:
  //    <not found> : not visited
  //    >=0 : superFace
  Map<label> toSuperFace{10*cFaces.size()};
  label superFaceI = 0;
  FOR_ALL(cFaces, cFaceI) {
    label faceI = cFaces[cFaceI];
    if (toSuperFace.found(faceI))
      continue;
    walkSuperFace
    (
      faceI,
      superFaceI,
      toSuperFace
    );
    superFaceI++;
  }
  // Construct superFace-to-oldface mapping.
  faceMap_.setSize(superFaceI);
  FOR_ALL(cFaces, cFaceI) {
    label faceI = cFaces[cFaceI];
    faceMap_[toSuperFace[faceI]].append(faceI);
  }
  FOR_ALL(faceMap_, superI) {
    faceMap_[superI].shrink();
  }
  // Construct superFaces
  facesPtr_ = new faceList(superFaceI);
  faceList& faces = *facesPtr_;
  FOR_ALL(cFaces, cFaceI) {
    label faceI = cFaces[cFaceI];
    label superFaceI = toSuperFace[faceI];
    if (!faces[superFaceI].empty())
      continue;
    // Superface not yet constructed.
    // Find starting feature edge on face.
    label startEdgeI = -1;
    const labelList& fEdges = mesh_.faceEdges()[faceI];
    FOR_ALL(fEdges, fEdgeI) {
      label edgeI = fEdges[fEdgeI];
      if (featureEdge_.found(edgeI)) {
        startEdgeI = edgeI;
        break;
      }
    }
    if (startEdgeI == -1)
      continue;
    // Walk point-edge-point along feature edges
    DynamicList<label> superFace{10*mesh_.faces()[faceI].size()};
    const edge& e = mesh_.edges()[startEdgeI];
    // Walk either start-end or end-start depending on orientation
    // of face. SuperFace will have cellI as owner.
    bool flipOrientation =
      (mesh_.faceOwner()[faceI] == cellI_) ^ (faceAlignedEdge(faceI, startEdgeI));
    label startVertI = -1;
    startVertI = (flipOrientation) ? e.end() : e.start();
    label edgeI = startEdgeI;
    label vertI = e.otherVertex(startVertI);
    do {
      label newEdgeI = nextEdge
      (
        toSuperFace,
        superFaceI,
        edgeI,
        vertI
      );
      // Determine angle between edges.
      if (isFeaturePoint(edgeI, newEdgeI)) {
        superFace.append(vertI);
      }
      edgeI = newEdgeI;
      if (vertI == startVertI) {
        break;
      }
      vertI = mesh_.edges()[edgeI].otherVertex(vertI);
    } while (true);
    if (superFace.size() <= 2) {
      WARNING_IN("cellFeatures::calcSuperFaces")
        << " Can not collapse faces " << faceMap_[superFaceI]
        << " into one big face on cell " << cellI_ << endl
        << "Try decreasing minCos:" << minCos_ << endl;
    } else {
      faces[superFaceI].transfer(superFace);
    }
  }
}


// Constructors 

// Construct from components
mousse::cellFeatures::cellFeatures
(
  const primitiveMesh& mesh,
  const scalar minCos,
  const label cellI
)
:
  mesh_{mesh},
  minCos_{minCos},
  cellI_{cellI},
  featureEdge_{10*mesh.cellEdges()[cellI].size()},
  facesPtr_{NULL},
  faceMap_{0}
{
  const labelList& cEdges = mesh_.cellEdges()[cellI_];
  FOR_ALL(cEdges, cEdgeI) {
    label edgeI = cEdges[cEdgeI];
    if (isCellFeatureEdge(minCos_, edgeI)) {
      featureEdge_.insert(edgeI);
    }
  }
}


// Destructor 
mousse::cellFeatures::~cellFeatures()
{
  deleteDemandDrivenData(facesPtr_);
}


// Member Functions 
bool mousse::cellFeatures::isFeaturePoint(const label edge0, const label edge1)
const
{
  if ((edge0 < 0)
      || (edge0 >= mesh_.nEdges())
      || (edge1 < 0)
      || (edge1 >= mesh_.nEdges())) {
    FATAL_ERROR_IN
    (
      "cellFeatures::isFeatureVertex(const label, const label)"
    )
    << "Illegal edge labels : edge0:" << edge0 << " edge1:" << edge1
    << abort(FatalError);
  }
  const edge& e0 = mesh_.edges()[edge0];
  vector e0Vec = e0.vec(mesh_.points());
  e0Vec /= mag(e0Vec);
  const edge& e1 = mesh_.edges()[edge1];
  vector e1Vec = e1.vec(mesh_.points());
  e1Vec /= mag(e1Vec);
  scalar cosAngle;
  if ((e0.start() == e1.end()) || (e0.end() == e1.start())) {
    // Same direction
    cosAngle = e0Vec & e1Vec;
  } else if ((e0.start() == e1.start()) || (e0.end() == e1.end())) {
    // back on back
    cosAngle = - e0Vec & e1Vec;
  } else {
    cosAngle = GREAT;   // satisfy compiler
    FATAL_ERROR_IN
    (
      "cellFeatures::isFeaturePoint(const label, const label"
      ", const label)"
    )
    << "Edges do not share common vertex. e0:" << e0
    << " e1:" << e1 << abort(FatalError);
  }
  return (cosAngle < minCos_);
}


bool mousse::cellFeatures::isFeatureVertex(const label faceI, const label vertI)
const
{
  if ((faceI < 0)
      || (faceI >= mesh_.nFaces())
      || (vertI < 0)
      || (vertI >= mesh_.nPoints())) {
    FATAL_ERROR_IN
    (
      "cellFeatures::isFeatureVertex(const label, const label)"
    )
    << "Illegal face " << faceI << " or vertex " << vertI
    << abort(FatalError);
  }
  const labelList& pEdges = mesh_.pointEdges()[vertI];
  label edge0 = -1;
  label edge1 = -1;
  FOR_ALL(pEdges, pEdgeI) {
    label edgeI = pEdges[pEdgeI];
    if (!meshTools::edgeOnFace(mesh_, faceI, edgeI))
      continue;
    if (edge0 == -1) {
      edge0 = edgeI;
    } else {
      edge1 = edgeI;
      // Found the two edges.
      break;
    }
  }
  if (edge1 == -1) {
    FATAL_ERROR_IN
    (
      "cellFeatures::isFeatureVertex(const label, const label)"
    )
    << "Did not find two edges sharing vertex " << vertI
    << " on face " << faceI << " vertices:" << mesh_.faces()[faceI]
    << abort(FatalError);
  }
  return isFeaturePoint(edge0, edge1);
}

