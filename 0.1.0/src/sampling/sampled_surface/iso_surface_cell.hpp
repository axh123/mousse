#ifndef SAMPLING_SAMPLED_SURFACE_ISO_SURFACE_CELL_HPP_
#define SAMPLING_SAMPLED_SURFACE_ISO_SURFACE_CELL_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011-2013 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::isoSurfaceCell
// Description
//   A surface formed by the iso value.
//   After "Polygonising A Scalar Field Using Tetrahedrons", Paul Bourke
//   (http://paulbourke.net/geometry/polygonise) and
//   "Regularised Marching Tetrahedra: improved iso-surface extraction",
//   G.M. Treece, R.W. Prager and A.H. Gee.
//   See isoSurface. This is a variant which does tetrahedrisation from
//   triangulation of face to cell centre instead of edge of face to two
//   neighbouring cell centres. This gives much lower quality triangles
//   but they are local to a cell.

#include "tri_surface.hpp"
#include "label_pair.hpp"
#include "point_index_hit.hpp"
#include "packed_bool_list.hpp"


namespace mousse {

class polyMesh;


class isoSurfaceCell
:
  public triSurface
{
  // Private data
    enum segmentCutType
    {
      NEARFIRST,      // intersection close to e.first()
      NEARSECOND,     //  ,,                   e.second()
      NOTNEAR         // no intersection
    };
    enum cellCutType
    {
      NOTCUT,     // not cut
      SPHERE,     // all edges to cell centre cut
      CUT         // normal cut
    };
    //- Reference to mesh
    const polyMesh& mesh_;
    const scalarField& cVals_;
    const scalarField& pVals_;
    //- isoSurfaceCell value
    const scalar iso_;
    //- When to merge points
    const scalar mergeDistance_;
    //- Whether cell might be cut
    List<cellCutType> cellCutType_;
    //- Estimated number of cut cells
    label nCutCells_;
    //- For every triangle the original cell in mesh
    labelList meshCells_;
    //- For every unmerged triangle point the point in the triSurface
    labelList triPointMergeMap_;
  // Private Member Functions
    //- Get location of iso value as fraction inbetween s0,s1
    scalar isoFraction
    (
      const scalar s0,
      const scalar s1
    ) const;
    //- Does any edge of triangle cross iso value?
    bool isTriCut
    (
      const triFace& tri,
      const scalarField& pointValues
    ) const;
    //- Determine whether cell is cut
    cellCutType calcCutType
    (
      const PackedBoolList&,
      const scalarField& cellValues,
      const scalarField& pointValues,
      const label
    ) const;
    void calcCutTypes
    (
      const PackedBoolList&,
      const scalarField& cellValues,
      const scalarField& pointValues
    );
    static labelPair findCommonPoints
    (
      const labelledTri&,
      const labelledTri&
    );
    static point calcCentre(const triSurface&);
    pointIndexHit collapseSurface
    (
      const label cellI,
      pointField& localPoints,
      DynamicList<labelledTri, 64>& localTris
    ) const;
    //- Determine per cc whether all near cuts can be snapped to single
    //  point.
    void calcSnappedCc
    (
      const PackedBoolList& isTet,
      const scalarField& cVals,
      const scalarField& pVals,
      DynamicList<point>& triPoints,
      labelList& snappedCc
    ) const;
    //- Generate triangles for face connected to pointI
    void genPointTris
    (
      const scalarField& cellValues,
      const scalarField& pointValues,
      const label pointI,
      const label faceI,
      const label cellI,
      DynamicList<point, 64>& localTriPoints
    ) const;
    //- Generate triangles for tet connected to pointI
    void genPointTris
    (
      const scalarField& pointValues,
      const label pointI,
      const label faceI,
      const label cellI,
      DynamicList<point, 64>& localTriPoints
    ) const;
    //- Determine per point whether all near cuts can be snapped to single
    //  point.
    void calcSnappedPoint
    (
      const PackedBoolList& isTet,
      const scalarField& cVals,
      const scalarField& pVals,
      DynamicList<point>& triPoints,
      labelList& snappedPoint
    ) const;
    //- Generate single point by interpolation or snapping
    template<class Type>
    Type generatePoint
    (
      const DynamicList<Type>& snappedPoints,
      const scalar s0,
      const Type& p0,
      const label p0Index,
      const scalar s1,
      const Type& p1,
      const label p1Index
    ) const;
    template<class Type>
    void generateTriPoints
    (
      const DynamicList<Type>& snapped,
      const scalar isoVal0,
      const scalar s0,
      const Type& p0,
      const label p0Index,
      const scalar isoVal1,
      const scalar s1,
      const Type& p1,
      const label p1Index,
      const scalar isoVal2,
      const scalar s2,
      const Type& p2,
      const label p2Index,
      const scalar isoVal3,
      const scalar s3,
      const Type& p3,
      const label p3Index,
      DynamicList<Type>& points
    ) const;
    template<class Type>
    void generateTriPoints
    (
      const scalarField& cVals,
      const scalarField& pVals,
      const Field<Type>& cCoords,
      const Field<Type>& pCoords,
      const DynamicList<Type>& snappedPoints,
      const labelList& snappedCc,
      const labelList& snappedPoint,
      DynamicList<Type>& triPoints,
      DynamicList<label>& triMeshCells
    ) const;
    triSurface stitchTriPoints
    (
      const bool checkDuplicates,
      const List<point>& triPoints,
      labelList& triPointReverseMap,  // unmerged to merged point
      labelList& triMap               // merged to unmerged triangle
    ) const;
    //- Check single triangle for (topological) validity
    static bool validTri(const triSurface&, const label);
    //- Determine edge-face addressing
    void calcAddressing
    (
      const triSurface& surf,
      List<FixedList<label, 3> >& faceEdges,
      labelList& edgeFace0,
      labelList& edgeFace1,
      Map<labelList>& edgeFacesRest
    ) const;
    //- Is triangle (given by 3 edges) not fully connected?
    static bool danglingTriangle
    (
      const FixedList<label, 3>& fEdges,
      const labelList& edgeFace1
    );
    //- Mark all non-fully connected triangles
    static label markDanglingTriangles
    (
      const List<FixedList<label, 3> >& faceEdges,
      const labelList& edgeFace0,
      const labelList& edgeFace1,
      const Map<labelList>& edgeFacesRest,
      boolList& keepTriangles
    );
    static triSurface subsetMesh
    (
      const triSurface& s,
      const labelList& newToOldFaces,
      labelList& oldToNewPoints,
      labelList& newToOldPoints
    );
    //- Combine all triangles inside a cell into a minimal triangulation
    void combineCellTriangles();
public:
  //- Runtime type information
  TYPE_NAME("isoSurfaceCell");
  // Constructors
    //- Construct from dictionary
    isoSurfaceCell
    (
      const polyMesh& mesh,
      const scalarField& cellValues,
      const scalarField& pointValues,
      const scalar iso,
      const bool regularise,
      const scalar mergeTol = 1e-6    // fraction of bounding box
    );
  // Member Functions
    //- For every face original cell in mesh
    const labelList& meshCells() const
    {
      return meshCells_;
    }
    //- For every unmerged triangle point the point in the triSurface
    const labelList triPointMergeMap() const
    {
      return triPointMergeMap_;
    }
    //- Interpolates cCoords,pCoords. Takes the original fields
    //  used to create the iso surface.
    template<class Type>
    tmp<Field<Type> > interpolate
    (
      const scalarField& cVals,
      const scalarField& pVals,
      const Field<Type>& cCoords,
      const Field<Type>& pCoords
    ) const;
    //- Interpolates cCoords,pCoords.
    template<class Type>
    tmp<Field<Type> > interpolate
    (
      const Field<Type>& cCoords,
      const Field<Type>& pCoords
    ) const;
};
}  // namespace mousse

#include "iso_surface_cell.ipp"

#endif
