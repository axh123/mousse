// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#ifndef zone_i_ds_hpp_
#define zone_i_ds_hpp_
#include "dynamic_id.hpp"
#include "cell_zone_mesh_fwd.hpp"
#include "face_zone_mesh_fwd.hpp"
#include "point_zone_mesh_fwd.hpp"
namespace mousse
{
  //- mousse::cellZoneID
  typedef DynamicID<cellZoneMesh> cellZoneID;
  //- mousse::faceZoneID
  typedef DynamicID<faceZoneMesh> faceZoneID;
  //- mousse::pointZoneID
  typedef DynamicID<pointZoneMesh> pointZoneID;
}
#endif
