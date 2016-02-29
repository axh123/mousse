#ifndef MESH_TOOLS_TRI_SURFACE_BOOLEAN_OPS_INTERSECTED_SURFACE_INTERSECTED_SURFACE_HPP_
#define MESH_TOOLS_TRI_SURFACE_BOOLEAN_OPS_INTERSECTED_SURFACE_INTERSECTED_SURFACE_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::intersectedSurface
// Description
//   Given triSurface and intersection creates the intersected
//   (properly triangulated) surface.
//   (note: intersection is the list of points and edges 'shared'
//   by two surfaces)
//   Algorithm:
//   - from the intersection get the points created on the edges of the surface
//   - split the edges of the surface
//   - construct a new edgeList with (in this order) the edges from the
//    intersection ('cuts', i.e. the edges shared with the other surface)
//    and the (split) edges from the original triangles (from 0 ..
//    nSurfaceEdges)
//   - construct face-edge addressing for above edges
//   - for each face do a right-handed walk to reconstruct faces (splitFace)
//   - retriangulate resulting faces (might be non-convex so use
//    faceTriangulation which does proper bisection)
//   The resulting surface will have the points from the surface first
//   in the point list (0 .. nSurfacePoints-1)
//   Note: problematic are the cut-edges which are completely inside a face.
//   These will not be visited by a edge-point-edge walk. These are handled by
//   resplitFace which first connects the 'floating' edges to triangle edges
//   with two extra edges and then tries the splitting again. Seems to work
//   (mostly). Will probably fail for boundary edge (edge with only face).
//   Note: points are compact, i.e. points().size() == localPoints().size()
//   (though points() probably not localPoints())
// SourceFiles
//   intersected_surface.cpp
#include "tri_surface.hpp"
#include "map.hpp"
#include "type_info.hpp"
namespace mousse
{
// Forward declaration of classes
class surfaceIntersection;
class edgeSurface;
class intersectedSurface
:
  public triSurface
{
public:
    static const label UNVISITED;
    static const label STARTTOEND;
    static const label ENDTOSTART;
    static const label BOTH;
private:
  // Private data
    //- Edges which are part of intersection
    labelList intersectionEdges_;
    //- From new to original faces
    labelList faceMap_;
    //- What are surface points: 0 .. nSurfacePoints_-1
    label nSurfacePoints_;
  // Static Member Functions
    //- Debug:Dump edges to stream. Mantains vertex numbering
    static void writeOBJ
    (
      const pointField& points,
      const edgeList& edges,
      Ostream& os
    );
    //- Debug:Dump selected edges to stream. Mantains vertex numbering
    static void writeOBJ
    (
      const pointField& points,
      const edgeList& edges,
      const labelList& faceEdges,
      Ostream& os
    );
    //- Debug:Dump selected edges to stream. Renumbers vertices to
    //  local ordering.
    static void writeLocalOBJ
    (
      const pointField& points,
      const edgeList& edges,
      const labelList& faceEdges,
      const fileName&
    );
    //- Debug:Write whole pointField and face to stream
    static void writeOBJ
    (
      const pointField& points,
      const face& f,
      Ostream& os
    );
    //- Debug:Print visited status
    static void printVisit
    (
      const edgeList& edges,
      const labelList& edgeLabels,
      const Map<label>& visited
    );
    //- Check if the two vertices that f0 and f1 share are in the same
    //  order on both faces.
    static bool sameEdgeOrder
    (
      const labelledTri& fA,
      const labelledTri& fB
    );
    //- Increment data for key. (start from 0 if not found)
    static void incCount
    (
      Map<label>& visited,
      const label key,
      const label offset
    );
    //- Calculate point-edge addressing for single face only.
    static Map<DynamicList<label> > calcPointEdgeAddressing
    (
      const edgeSurface&,
      const label faceI
    );
    //- Choose edge out of candidates (facePointEdges) according to
    //  angle with previous edge.
    static label nextEdge
    (
      const edgeSurface& eSurf,
      const Map<label>& visited,
      const label faceI,
      const vector& n,                // original triangle normal
      const Map<DynamicList<label> >& facePointEdges,
      const label prevEdgeI,
      const label prevVertI
    );
    //- Walk path along edges in face. Used by splitFace.
    static face walkFace
    (
      const edgeSurface& eSurf,
      const label faceI,
      const vector& n,
      const Map<DynamicList<label> >& facePointEdges,
      const label startEdgeI,
      const label startVertI,
      Map<label>& visited
    );
    //- For resplitFace: find nearest (to pt) fully visited point. Return
    //  point and distance.
    static void findNearestVisited
    (
      const edgeSurface& eSurf,
      const label faceI,
      const Map<DynamicList<label> >& facePointEdges,
      const Map<label>& pointVisited,
      const point& pt,
      const label excludeFaceI,
      label& minVertI,
      scalar& minDist
    );
    //- Fallback for if splitFace fails to connect all.
    static faceList resplitFace
    (
      const triSurface& surf,
      const label faceI,
      const Map<DynamicList<label> >& facePointEdges,
      const Map<label>& visited,
      edgeSurface& eSurf
    );
    //- Main face splitting routine. Gets overall points and edges and
    //  owners and face-local edgeLabels. Returns list of faces.
    static faceList splitFace
    (
      const triSurface& surf,
      const label faceI,
      edgeSurface& eSurf
    );
  // Private Member Functions
public:
  CLASS_NAME("intersectedSurface");
  // Constructors
    //- Construct null
    intersectedSurface();
    //- Construct from surface
    intersectedSurface(const triSurface& surf);
    //- Construct from surface and intersection. isFirstSurface is needed
    //  to determine which side of face pairs stored in the intersection
    //  to address. Should be in the same order as how the intersection was
    //  constructed.
    intersectedSurface
    (
      const triSurface& surf,
      const bool isFirstSurface,
      const surfaceIntersection& inter
    );
  // Member Functions
    //- Labels of edges in *this which originate from 'cuts'
    const labelList& intersectionEdges() const
    {
      return intersectionEdges_;
    }
    //- New to old
    const labelList& faceMap() const
    {
      return faceMap_;
    }
    //- Number of points from original surface
    label nSurfacePoints() const
    {
      return nSurfacePoints_;
    }
    //- Is point coming from original surface?
    bool isSurfacePoint(const label pointI) const
    {
      return pointI < nSurfacePoints_;
    }
};
}  // namespace mousse
#endif
