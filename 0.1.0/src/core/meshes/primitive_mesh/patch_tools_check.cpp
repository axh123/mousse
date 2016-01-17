// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "patch_tools.hpp"

template
<
  class Face,
  template<class> class FaceList,
  class PointField,
  class PointType
>
bool
mousse::PatchTools::checkOrientation
(
  const PrimitivePatch<Face, FaceList, PointField, PointType>& p,
  const bool report,
  labelHashSet* setPtr
)
{
  bool foundError = false;
  // Check edge normals, face normals, point normals.
  FOR_ALL(p.faceEdges(), faceI)
  {
    const labelList& edgeLabels = p.faceEdges()[faceI];
    bool valid = true;
    if (edgeLabels.size() < 3)
    {
      if (report)
      {
        Info<< "Face[" << faceI << "] " << p[faceI]
          << " has fewer than 3 edges. Edges: " << edgeLabels
          << endl;
      }
      valid = false;
    }
    else
    {
      FOR_ALL(edgeLabels, i)
      {
        if (edgeLabels[i] < 0 || edgeLabels[i] >= p.nEdges())
        {
          if (report)
          {
            Info<< "edge number " << edgeLabels[i]
              << " on face " << faceI
              << " out-of-range\n"
              << "This usually means the input surface has "
              << "edges with more than 2 faces connected."
              << endl;
          }
          valid = false;
        }
      }
    }
    if (!valid)
    {
      foundError = true;
      continue;
    }
    //
    //- Compute normal from 3 points, use the first as the origin
    // minor warpage should not be a problem
    const Face& f = p[faceI];
    const point& p0 = p.points()[f[0]];
    const point& p1 = p.points()[f[1]];
    const point& p2 = p.points()[f.last()];
    const vector pointNormal{(p1 - p0) ^ (p2 - p0)};
    if ((pointNormal & p.faceNormals()[faceI]) < 0)
    {
      foundError = false;
      if (report)
      {
        Info
          << "Normal calculated from points inconsistent"
          << " with faceNormal" << nl
          << "face: " << f << nl
          << "points: " << p0 << ' ' << p1 << ' ' << p2 << nl
          << "pointNormal:" << pointNormal << nl
          << "faceNormal:" << p.faceNormals()[faceI] << endl;
      }
    }
  }
  FOR_ALL(p.edges(), edgeI)
  {
    const edge& e = p.edges()[edgeI];
    const labelList& neighbouringFaces = p.edgeFaces()[edgeI];
    if (neighbouringFaces.size() == 2)
    {
      // we use localFaces() since edges() are LOCAL
      // these are both already available
      const Face& faceA = p.localFaces()[neighbouringFaces[0]];
      const Face& faceB = p.localFaces()[neighbouringFaces[1]];
      // If the faces are correctly oriented, the edges must go in
      // different directions on connected faces.
      if (faceA.edgeDirection(e) == faceB.edgeDirection(e))
      {
        if (report)
        {
          Info<< "face orientation incorrect." << nl
            << "localEdge[" << edgeI << "] " << e
            << " between faces:" << nl
            << "  face[" << neighbouringFaces[0] << "] "
            << p[neighbouringFaces[0]]
            << "   localFace: " << faceA
            << nl
            << "  face[" << neighbouringFaces[1] << "] "
            << p[neighbouringFaces[1]]
            << "   localFace: " << faceB
            << endl;
        }
        if (setPtr)
        {
          setPtr->insert(edgeI);
        }
        foundError = true;
      }
    }
    else if (neighbouringFaces.size() != 1)
    {
      if (report)
      {
        Info
          << "Wrong number of edge neighbours." << nl
          << "edge[" << edgeI << "] " << e
          << " with points:" << p.localPoints()[e.start()]
          << ' ' << p.localPoints()[e.end()]
          << " has neighbouringFaces:" << neighbouringFaces << endl;
      }
      if (setPtr)
      {
        setPtr->insert(edgeI);
      }
      foundError = true;
    }
  }

  return foundError;
}
