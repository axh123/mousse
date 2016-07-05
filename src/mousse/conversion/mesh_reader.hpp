#ifndef CONVERSION_MESH_READER_MESH_READER_HPP_
#define CONVERSION_MESH_READER_MESH_READER_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::meshReader
// Description
//   This class supports creating polyMeshes with baffles.
//   The derived classes are responsible for providing the protected data.
//   This implementation is somewhat messy, but could/should be restructured
//   to provide a more generalized reader (at the moment it has been written
//   for converting pro-STAR data).
//   The meshReader supports cellTable information (see new user's guide entry).
// Note
//   The boundary definitions are given as cell/face.

#include "poly_mesh.hpp"
#include "hash_table.hpp"
#include "iostream.hpp"
#include "cell_table.hpp"


namespace mousse {

class meshReader
{
protected:
  //- Identify cell faces in terms of cell Id and face Id
  class cellFaceIdentifier
  {
  public:
    // Public data
      //- Cell Id
      label cell;
      //- Face Id
      label face;
    // Constructors
      //- Construct null
      cellFaceIdentifier() : cell(-1), face(-1) {}
      //- Construct from cell/face components
      cellFaceIdentifier(label c, label f) : cell(c), face(f) {}
    // Check
      //- Used if cell or face are non-negative
      bool used() const
      {
        return (cell >= 0 && face >= 0);
      }
      //- Unused if cell or face are negative
      bool unused() const
      {
        return (cell < 0 || face < 0);
      }
    // Member Operators
      bool operator!=(const cellFaceIdentifier& cf) const
      {
        return (cell != cf.cell || face != cf.face);
      }
      bool operator==(const cellFaceIdentifier& cf) const
      {
        return (cell == cf.cell && face == cf.face);
      }
    // IOstream Operators
      friend Ostream& operator<<
      (
        Ostream& os,
        const cellFaceIdentifier& cf
      )
      {
        os << "(" << cf.cell << "/" << cf.face << ")";
        return os;
      }
    };
private:
  // Private data
    //- Point-cell addressing. Used for topological analysis
    // Warning. This point cell addressing list potentially contains
    // duplicate cell entries. Use additional checking
    mutable labelListList* pointCellsPtr_;
    //- Number of internal faces for polyMesh
    label nInternalFaces_;
    //- Polyhedral mesh boundary patch start indices and dimensions
    labelList patchStarts_;
    labelList patchSizes_;
    //- Association between two faces
    List<labelPair> interfaces_;
    //- List of cells/faces id pairs for each baffle
    List<List<cellFaceIdentifier> > baffleIds_;
    //- Global face list for polyMesh
    faceList meshFaces_;
    //- Cells as polyhedra for polyMesh
    cellList cellPolys_;
    //- Face sets for monitoring
    HashTable<List<label>, word, string::hash> monitoringSets_;
  // Private Member Functions
    //- Calculate pointCells
    void calcPointCells() const;
    const labelListList& pointCells() const;
    //- Make polyhedral cells and global faces if the mesh is polyhedral
    void createPolyCells();
    //- Add in boundary face
    void addPolyBoundaryFace
    (
      const label cellId,
      const label cellFaceId,
      const label nCreatedFaces
    );
    //- Add in boundary face
    void addPolyBoundaryFace
    (
      const cellFaceIdentifier& identifier,
      const label nCreatedFaces
    );
    //- Add cellZones based on cellTable Id
    void addCellZones(polyMesh&) const;
    //- Add faceZones based on monitoring boundary conditions
    void addFaceZones(polyMesh&) const;
    //- Make polyhedral boundary from shape boundary
    // (adds more faces to the face list)
    void createPolyBoundary();
    //- Add polyhedral boundary
    List<polyPatch*> polyBoundaryPatches(const polyMesh&);
    //- Clear extra storage before creation of the mesh to remove
    //  a memory peak
    void clearExtraStorage();
    void writeInterfaces(const objectRegistry&) const;
    //- Write List<label> in constant/polyMesh
    void writeMeshLabelList
    (
      const objectRegistry& registry,
      const word& propertyName,
      const labelList& list,
      IOstream::streamFormat fmt = IOstream::ASCII
    ) const;
    //- Return list of faces for every cell
    faceListList& cellFaces() const
    {
      return const_cast<faceListList&>(cellFaces_);
    }
protected:
  // Protected data
    //- Pointers to cell shape models
    static const cellModel* unknownModel;
    static const cellModel* tetModel;
    static const cellModel* pyrModel;
    static const cellModel* prismModel;
    static const cellModel* hexModel;
    //- Referenced filename
    fileName geometryFile_;
    //- Geometry scaling
    scalar scaleFactor_;
    //- Points supporting the mesh
    pointField points_;
    //- Lookup original Cell number for a given cell
    labelList origCellId_;
    //- Identify boundary faces by cells and their faces
    //  for each patch
    List<List<cellFaceIdentifier> > boundaryIds_;
    //- Boundary patch types
    wordList patchTypes_;
    //- Boundary patch names
    wordList patchNames_;
    //- Boundary patch physical types
    wordList patchPhysicalTypes_;
    //- List of faces for every cell
    faceListList cellFaces_;
    //- List of each baffle face
    faceList baffleFaces_;
    //- Cell table id for each cell
    labelList cellTableId_;
    //- Cell table persistent data saved as a dictionary
    cellTable cellTable_;
  // Member Functions
    //- Subclasses are required to supply this information
    virtual bool readGeometry(const scalar scaleFactor = 1.0) = 0;
public:
  // Static Members
    //- Warn about repeated names
    static void warnDuplicates(const word& context, const wordList&);
  // Constructors
    //- Construct from fileName
    meshReader(const fileName&, const scalar scaleFactor = 1.0);
    //- Disallow default bitwise copy construct
    meshReader(const meshReader&) = delete;
    //- Disallow default bitwise assignment
    meshReader& operator=(const meshReader&) = delete;
  //- Destructor
  virtual ~meshReader();
  // Member Functions
    //- Create and return polyMesh
    virtual autoPtr<polyMesh> mesh(const objectRegistry&);
    //- Write auxiliary information
    void writeAux(const objectRegistry&) const;
    //- Write mesh
    void writeMesh
    (
      const polyMesh&,
      IOstream::streamFormat fmt = IOstream::BINARY
    ) const;
};

}  // namespace mousse

#endif
