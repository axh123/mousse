// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "closed_tri_surface_mesh.hpp"
#include "add_to_run_time_selection_table.hpp"


// Static Data Members
namespace mousse {

DEFINE_TYPE_NAME_AND_DEBUG(closedTriSurfaceMesh, 0);
ADD_TO_RUN_TIME_SELECTION_TABLE(searchableSurface, closedTriSurfaceMesh, dict);

}


// Constructors 
mousse::closedTriSurfaceMesh::closedTriSurfaceMesh
(
  const IOobject& io,
  const triSurface& s
)
:
  triSurfaceMesh{io, s}
{}


mousse::closedTriSurfaceMesh::closedTriSurfaceMesh(const IOobject& io)
:
  triSurfaceMesh{io}
{}


mousse::closedTriSurfaceMesh::closedTriSurfaceMesh
(
  const IOobject& io,
  const dictionary& dict
)
:
  triSurfaceMesh{io, dict}
{}


// Destructor 
mousse::closedTriSurfaceMesh::~closedTriSurfaceMesh()
{}

