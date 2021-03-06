#ifndef TRI_SURFACE_TRI_SURFACE_TRI_SURFACE_HPP_
#define TRI_SURFACE_TRI_SURFACE_TRI_SURFACE_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::triSurface
// Description
//   Triangulated surface description with patch information.

#include "_primitive_patch.hpp"
#include "point_field.hpp"
#include "labelled_tri.hpp"
#include "bool_list.hpp"
#include "geometric_surface_patch_list.hpp"
#include "surface_patch_list.hpp"
#include "tri_face_list.hpp"


namespace mousse {

class Time;
class IFstream;


class triSurface
:
  public PrimitivePatch<labelledTri, ::mousse::List, pointField, point>
{
  // Private typedefs
  //- Typedefs for convenience
    typedef labelledTri Face;
    typedef PrimitivePatch
    <
      labelledTri,
      ::mousse::List,
      pointField,
      point
    >
    ParentType;
  // Private data
    //- The number of bytes in the STL header
    static const int STLheaderSize = 80;
    //- Patch information (face ordering nFaces/startFace only used
    //  during reading and writing)
    geometricSurfacePatchList patches_;
  // Demand driven private data.
    //- Edge-face addressing (sorted)
    mutable labelListList* sortedEdgeFacesPtr_;
    //- Label of face that 'owns' edge (i.e. e.vec() is righthanded walk
    //  along face)
    mutable labelList* edgeOwnerPtr_;
  // Private Member Functions
    //- Calculate sorted edgeFaces
    void calcSortedEdgeFaces() const;
    //- Calculate owner
    void calcEdgeOwner() const;
    //- Sort faces according to region. Returns patch list
    //  and sets faceMap to index of labelledTri inside *this.
    surfacePatchList calcPatches(labelList& faceMap) const;
    //- Sets default values for patches
    void setDefaultPatches();
    //- Function to stitch the triangles by removing duplicate points.
    //  Returns true if any points merged
    bool stitchTriangles
    (
      const scalar tol = SMALL,
      const bool verbose = false
    );
    //- Read in Foam format
    bool read(Istream&);
    //- Generic read routine. Chooses reader based on extension.
    bool read(const fileName&, const word& ext, const bool check = true);
    bool readSTL(const fileName&);
    bool readSTLASCII(const fileName&);
    bool readSTLBINARY(const fileName&);
    bool readGTS(const fileName&);
    bool readOBJ(const fileName&);
    bool readOFF(const fileName&);
    bool readTRI(const fileName&);
    bool readAC(const fileName&);
    bool readNAS(const fileName&);
    bool readVTK(const fileName&);
    //- Generic write routine. Chooses writer based on extension.
    void write(const fileName&, const word& ext, const bool sort) const;
    //- Write to Ostream in ASCII STL format.
    //  Each region becomes 'solid' 'endsolid' block.
    void writeSTLASCII(const bool writeSorted, Ostream&) const;
    //- Write to std::ostream in BINARY STL format
    void writeSTLBINARY(std::ostream&) const;
    //- Write to Ostream in GTS (Gnu Tri Surface library)
    //  format.
    void writeGTS(const bool writeSorted, Ostream&) const;
    //- Write to Ostream in OBJ (Lightwave) format.
    //  writeSorted=true: sort faces acc. to region and write as single
    //  group. =false: write in normal order.
    void writeOBJ(const bool writeSorted, Ostream&) const;
    //- Write to Ostream in OFF (Geomview) format.
    //  writeSorted=true: sort faces acc. to region and write as single
    //  group. =false: write in normal order.
    void writeOFF(const bool writeSorted, Ostream&) const;
    //- Write to VTK legacy format.
    void writeVTK(const bool writeSorted, Ostream&) const;
    //- Write to Ostream in TRI (AC3D) format
    //  Ac3d .tri format (unmerged triangle format)
    void writeTRI(const bool writeSorted, Ostream&) const;
    //- Write to Ostream in SMESH (tetgen) format
    void writeSMESH(const bool writeSorted, Ostream&) const;
    //- Write to Ostream in AC3D format. Always sorted by patch.
    void writeAC(Ostream&) const;
    //- For DX writing.
    void writeDX(const bool, Ostream&) const;
    void writeDXGeometry(const bool, Ostream&) const;
    void writeDXTrailer(Ostream&) const;
  // Static private functions
    //- Convert faces to labelledTri. All get same region.
    static List<labelledTri> convertToTri
    (
      const faceList&,
      const label defaultRegion = 0
    );
    //- Convert triFaces to labelledTri. All get same region.
    static List<labelledTri> convertToTri
    (
      const triFaceList&,
      const label defaultRegion = 0
    );
    //- Helper function to print triangle info
    static void printTriangle
    (
      Ostream&,
      const mousse::string& pre,
      const labelledTri&,
      const pointField&
    );
    //- Read non-comment line
    static string getLineNoComment(IFstream&);
protected:
  // Protected Member Functions
    //- Non-const access to global points
    pointField& storedPoints()
    {
      return const_cast<pointField&>(ParentType::points());
    }
    //- Non-const access to the faces
    List<Face>& storedFaces()
    {
      return static_cast<List<Face>&>(*this);
    }
public:
  // Public typedefs
    //- Placeholder only, but do not remove - it is needed for GeoMesh
    typedef bool BoundaryMesh;
    //- Runtime type information
    CLASS_NAME("triSurface");
  // Static
    //- Name of triSurface directory to use.
    static fileName triSurfInstance(const Time&);
  // Constructors
    //- Construct null
    triSurface();
    //- Construct from triangles, patches, points.
    triSurface
    (
      const List<labelledTri>&,
      const geometricSurfacePatchList&,
      const pointField&
    );
    //- Construct from triangles, patches, points. Reuse storage.
    triSurface
    (
      List<labelledTri>&,
      const geometricSurfacePatchList&,
      pointField&,
      const bool reUse
    );
    //- Construct from triangles, patches, points.
    triSurface
    (
      const Xfer<List<labelledTri> >&,
      const geometricSurfacePatchList&,
      const Xfer<List<point> >&
    );
    //- Construct from triangles, points. Set patchnames to default.
    triSurface(const List<labelledTri>&, const pointField&);
    //- Construct from triangles, points. Set region to 0 and default
    //  patchName.
    triSurface(const triFaceList&, const pointField&);
    //- Construct from file name (uses extension to determine type)
    triSurface(const fileName&);
    //- Construct from Istream
    triSurface(Istream&);
    //- Construct from objectRegistry
    triSurface(const Time& d);
    //- Construct as copy
    triSurface(const triSurface&);
  //- Destructor
  ~triSurface();
    void clearOut();
    void clearTopology();
    void clearPatchMeshAddr();
  // Member Functions
    // Access
      const geometricSurfacePatchList& patches() const
      {
        return patches_;
      }
      geometricSurfacePatchList& patches()
      {
        return patches_;
      }
      //- Return edge-face addressing sorted (for edges with more than
      //  2 faces) according to the angle around the edge.
      //  Orientation is anticlockwise looking from
      //  edge.vec(localPoints())
      const labelListList& sortedEdgeFaces() const;
      //- If 2 face neighbours: label of face where ordering of edge
      //  is consistent with righthand walk.
      //  If 1 neighbour: label of only face.
      //  If >2 neighbours: undetermined.
      const labelList& edgeOwner() const;
    // Edit
      //- Move points
      virtual void movePoints(const pointField&);
      //- Scale points. A non-positive factor is ignored
      virtual void scalePoints(const scalar);
      //- Check/remove duplicate/degenerate triangles
      void checkTriangles(const bool verbose);
      //- Check triply (or more) connected edges.
      void checkEdges(const bool verbose);
      //- Remove non-valid triangles
      void cleanup(const bool verbose);
      //- Fill faceZone with currentZone for every face reachable
      //  from faceI without crossing edge marked in borderEdge.
      //  Note: faceZone has to be sized nFaces before calling this fun.
      void markZone
      (
        const boolList& borderEdge,
        const label faceI,
        const label currentZone,
        labelList& faceZone
      ) const;
      //- (size and) fills faceZone with zone of face. Zone is area
      //  reachable by edge crossing without crossing borderEdge
      //  (bool for every edge in surface). Returns number of zones.
      label markZones
      (
        const boolList& borderEdge,
        labelList& faceZone
      ) const;
      //- 'Create' sub mesh, including only faces for which
      //  boolList entry is true
      //  Sets: pointMap: from new to old localPoints
      //        faceMap: new to old faces
      void subsetMeshMap
      (
        const boolList& include,
        labelList& pointMap,
        labelList& faceMap
      ) const;
      //- Return new surface. Returns pointMap, faceMap from
      //  subsetMeshMap
      triSurface subsetMesh
      (
        const boolList& include,
        labelList& pointMap,
        labelList& faceMap
      ) const;
    // Write
      //- Write to Ostream in simple FOAM format
      void write(Ostream&) const;
      //- Generic write routine. Chooses writer based on extension.
      void write(const fileName&, const bool sortByRegion = false) const;
      //- Write to database
      void write(const Time&) const;
      //- Write to Ostream in OpenDX format
      void writeDX(const scalarField&, Ostream&) const;
      void writeDX(const vectorField&, Ostream&) const;
      //- Write some statistics
      void writeStats(Ostream&) const;
  // Member operators
    void operator=(const triSurface&);
  // Ostream Operator
    friend Ostream& operator<<(Ostream&, const triSurface&);
};
}  // namespace mousse
#endif
