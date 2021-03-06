#ifndef FINITE_VOLUME_FV_MESH_EXTENDED_STENCIL_CELL_TO_FACE_UPWIND_CPC_CELL_TO_FACE_STENCIL_OBJECT_HPP_
#define FINITE_VOLUME_FV_MESH_EXTENDED_STENCIL_CELL_TO_FACE_UPWIND_CPC_CELL_TO_FACE_STENCIL_OBJECT_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::upwindCPCCellToFaceStencilObject
// Description
// SourceFiles
#include "extended_upwind_cell_to_face_stencil.hpp"
#include "cpc_cell_to_face_stencil.hpp"
#include "_mesh_object.hpp"
namespace mousse
{
class upwindCPCCellToFaceStencilObject
:
  public MeshObject
  <
    fvMesh,
    TopologicalMeshObject,
    upwindCPCCellToFaceStencilObject
  >,
  public extendedUpwindCellToFaceStencil
{
public:
  TYPE_NAME("upwindCPCCellToFaceStencil");
  // Constructors
    //- Construct from uncompacted face stencil
    upwindCPCCellToFaceStencilObject
    (
      const fvMesh& mesh,
      const bool pureUpwind,
      const scalar minOpposedness
    )
    :
      MeshObject
      <
        fvMesh,
        mousse::TopologicalMeshObject,
        upwindCPCCellToFaceStencilObject
      >(mesh),
      extendedUpwindCellToFaceStencil
      (
        CPCCellToFaceStencil(mesh),
        pureUpwind,
        minOpposedness
      )
    {
      if (extendedCellToFaceStencil::debug)
      {
        Info<< "Generated off-centred stencil " << type()
          << nl << endl;
        writeStencilStats(Info, ownStencil(), ownMap());
      }
    }
  //- Destructor
  virtual ~upwindCPCCellToFaceStencilObject()
  {}
};
}  // namespace mousse
#endif
