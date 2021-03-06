#ifndef ENGINE_ENGINE_MESH_LAYERED_ENGINE_MESH_LAYERED_ENGINE_MESH_HPP_
#define ENGINE_ENGINE_MESH_LAYERED_ENGINE_MESH_LAYERED_ENGINE_MESH_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::layeredEngineMesh
// Description
//   mousse::layeredEngineMesh

#include "engine_mesh.hpp"
#include "dimensioned_scalar.hpp"


namespace mousse {

class layeredEngineMesh
:
  public engineMesh
{
  // Private data
    dimensionedScalar pistonLayers_;
public:
  //- Runtime type information
  TYPE_NAME("layered");
  // Constructors
    //- Construct from IOobject
    layeredEngineMesh(const IOobject& io);
    //- Disallow default bitwise copy construct
    layeredEngineMesh(const layeredEngineMesh&) = delete;
    //- Disallow default bitwise assignment
    layeredEngineMesh& operator=(const layeredEngineMesh&) = delete;
  //- Destructor
  ~layeredEngineMesh();
  // Member Functions
    // Edit
      void move();
};

}  // namespace mousse

#endif

