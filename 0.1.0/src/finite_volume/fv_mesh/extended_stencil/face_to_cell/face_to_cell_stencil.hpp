#ifndef FINITE_VOLUME_FV_MESH_EXTENDED_STENCIL_FACE_TO_CELL_FACE_TO_CELL_STENCIL_HPP_
#define FINITE_VOLUME_FV_MESH_EXTENDED_STENCIL_FACE_TO_CELL_FACE_TO_CELL_STENCIL_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011-2013 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::faceToCellStencil
// Description
//   baseclass for extended cell centred addressing. Contains per cell a
//   list of neighbouring faces in global addressing.
// SourceFiles
//   face_to_cell_stencil.cpp
#include "global_index.hpp"
#include "bool_list.hpp"
#include "hash_set.hpp"
#include "indirect_primitive_patch.hpp"
namespace mousse
{
class polyMesh;
class faceToCellStencil
:
  public labelListList
{
  // Private data
    const polyMesh& mesh_;
    //- Global numbering for faces
    const globalIndex globalNumbering_;
public:
  // Constructors
    //- Construct from mesh
    explicit faceToCellStencil(const polyMesh&);
  // Member Functions
    const polyMesh& mesh() const
    {
      return mesh_;
    }
    //- Global numbering for faces
    const globalIndex& globalNumbering() const
    {
      return globalNumbering_;
    }
};
}  // namespace mousse
#endif
