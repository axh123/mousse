#ifndef POST_PROCESSING_FUNCTION_OBJECTS_FIELD_STREAM_LINE_STREAM_LINE_HPP_
#define POST_PROCESSING_FUNCTION_OBJECTS_FIELD_STREAM_LINE_STREAM_LINE_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011-2013 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::streamLine
// Group
//   grpFieldFunctionObjects
// Description
//   This function object generates streamline data by sampling a set of
//   user-specified fields along a particle track, transported by a
//   user-specified velocity field.
//   Example of function object specification:
//   \verbatim
//   streamLine1
//   {
//     type            streamLine;
//     functionObjectLibs ("libfieldFunctionObjects.so");
//     ...
//     setFormat       vtk;
//     UName           U;
//     trackForward    yes;
//     fields
//     (
//       U
//       p
//     );
//     lifeTime        10000;
//     trackLength     1e-3;
//     nSubCycle       5;
//     cloudName       particleTracks;
//     seedSampleSet   uniform;
//     uniformCoeffs
//     {
//       type        uniform;
//       axis        x;  //distance;
//       start       (-0.0205 0.0001 0.00001);
//       end         (-0.0205 0.0005 0.00001);
//       nPoints     100;
//     }
//   }
//   \endverbatim
//   \heading Function object usage
//   \table
//     Property     | Description             | Required    | Default value
//     type         | type name: streamLine   | yes         |
//     setFormat    | output data type        | yes         |
//     UName        | tracking velocity field name | yes    |
//     fields       | fields to sample        | yes         |
//     lifetime     | maximum number of particle tracking steps | yes |
//     trackLength  | tracking segment length | no          |
//     nSubCycle    | number of tracking steps per cell | no|
//     cloudName    | cloud name to use       | yes         |
//     seedSampleSet| seeding method (see below)| yes       |
//   \endtable
//   \linebreak
//   Where \c seedSampleSet is typically one of
//   \plaintable
//     uniform | uniform particle seeding
//     cloud   | cloud of points
//     triSurfaceMeshPointSet | points according to a tri-surface mesh
//   \endplaintable
// Note
//   When specifying the track resolution, the \c trackLength OR \c nSubCycle
//   option should be used
// SeeAlso
//   mousse::functionObject
//   mousse::OutputFilterFunctionObject
//   mousse::sampledSet
//   mousse::wallBoundedStreamLine

#include "vol_fields_fwd.hpp"
#include "point_field_fwd.hpp"
#include "switch.hpp"
#include "dynamic_list.hpp"
#include "scalar_list.hpp"
#include "vector_list.hpp"
#include "poly_mesh.hpp"
#include "writer.hpp"
#include "indirect_primitive_patch.hpp"


namespace mousse {

// Forward declaration of classes
class objectRegistry;
class dictionary;
class mapPolyMesh;
class meshSearch;
class sampledSet;


class streamLine
{
  // Private data
    //- Input dictionary
    dictionary dict_;
    //- Name of this set of field averages.
    word name_;
    //- Database this class is registered to
    const objectRegistry& obr_;
    //- Load fields from files (not from objectRegistry)
    bool loadFromFiles_;
    //- On/off switch
    bool active_;
    //- List of fields to sample
    wordList fields_;
    //- Field to transport particle with
    word UName_;
    //- Interpolation scheme to use
    word interpolationScheme_;
    //- Whether to use +u or -u
    bool trackForward_;
    //- Maximum lifetime (= number of cells) of particle
    label lifeTime_;
    //- Number of subcycling steps
    label nSubCycle_;
    //- Track length
    scalar trackLength_;
    //- Optional specified name of particles
    word cloudName_;
    //- Type of seed
    word seedSet_;
    //- Names of scalar fields
    wordList scalarNames_;
    //- Names of vector fields
    wordList vectorNames_;
    // Demand driven
      //- Mesh searching enigne
      autoPtr<meshSearch> meshSearchPtr_;
      //- Seed set engine
      autoPtr<sampledSet> sampledSetPtr_;
      //- Axis of the sampled points to output
      word sampledSetAxis_;
      //- File writer for scalar data
      autoPtr<writer<scalar>> scalarFormatterPtr_;
      //- File writer for vector data
      autoPtr<writer<vector>> vectorFormatterPtr_;
    // Generated data
      //- All tracks. Per particle the points it passed through
      DynamicList<List<point>> allTracks_;
      //- Per scalarField, per particle, the sampled value.
      List<DynamicList<scalarList>> allScalars_;
      //- Per scalarField, per particle, the sampled value.
      List<DynamicList<vectorList>> allVectors_;
    //- Construct patch out of all wall patch faces
    autoPtr<indirectPrimitivePatch> wallPatch() const;
    //- Do all seeding and tracking
    void track();
public:
  //- Runtime type information
  TYPE_NAME("streamLine");
  // Constructors
    //- Construct for given objectRegistry and dictionary.
    //  Allow the possibility to load fields from files
    streamLine
    (
      const word& name,
      const objectRegistry&,
      const dictionary&,
      const bool loadFromFiles = false
    );
    //- Disallow default bitwise copy construct
    streamLine(const streamLine&) = delete;
    //- Disallow default bitwise assignment
    streamLine& operator=(const streamLine&) = delete;
  //- Destructor
  virtual ~streamLine();
  // Member Functions
    //- Return name of the set of field averages
    virtual const word& name() const { return name_; }
    //- Read the field average data
    virtual void read(const dictionary&);
    //- Execute the averaging
    virtual void execute();
    //- Execute the averaging at the final time-loop, currently does nothing
    virtual void end();
    //- Called when time was set at the end of the Time::operator++
    virtual void timeSet();
    //- Calculate the field average data and write
    virtual void write();
    //- Update for changes of mesh
    virtual void updateMesh(const mapPolyMesh&);
    //- Update for mesh point-motion
    virtual void movePoints(const polyMesh&);
    ////- Update for changes of mesh due to readUpdate
    //virtual void readUpdate(const polyMesh::readUpdateState state);
};

}  // namespace mousse

#endif

