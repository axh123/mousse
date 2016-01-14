// mousse: CFD toolbox
// Copyright (C) 2011-2012 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::inverseFaceDistanceDiffusivity
// Description
//   Inverse distance to the given patches motion diffusivity.
// SourceFiles
//   inverse_face_distance_diffusivity.cpp
#ifndef inverse_face_distance_diffusivity_hpp_
#define inverse_face_distance_diffusivity_hpp_
#include "uniform_diffusivity.hpp"
namespace mousse
{
class inverseFaceDistanceDiffusivity
:
  public uniformDiffusivity
{
  // Private data
    //- Patches selected to base the distance on
    wordList patchNames_;
public:
  //- Runtime type information
  TYPE_NAME("inverseFaceDistance");
  // Constructors
    //- Construct for the given fvMesh and data Istream
    inverseFaceDistanceDiffusivity(const fvMesh& mesh, Istream& mdData);
    //- Disallow default bitwise copy construct
    inverseFaceDistanceDiffusivity
    (
      const inverseFaceDistanceDiffusivity&
    ) = delete;
    //- Disallow default bitwise assignment
    inverseFaceDistanceDiffusivity& operator=
    (
      const inverseFaceDistanceDiffusivity&
    );
  //- Destructor
  virtual ~inverseFaceDistanceDiffusivity();
  // Member Functions
    //- Correct the motion diffusivity
    virtual void correct();
};
}  // namespace mousse
#endif
