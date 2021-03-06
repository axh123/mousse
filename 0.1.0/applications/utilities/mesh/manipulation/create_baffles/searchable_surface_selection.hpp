#ifndef UTILITIES_MESH_MANIPULATION_CREATE_BAFFLES_FACE_SELECTION_SEARCHABLE_SURFACE_SELECTION_HPP_
#define UTILITIES_MESH_MANIPULATION_CREATE_BAFFLES_FACE_SELECTION_SEARCHABLE_SURFACE_SELECTION_HPP_

// mousse: CFD toolbox
// Copyright (C) 2012 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::faceSelections::searchableSurfaceSelection
// Description
//   Selects all (internal or coupled) faces intersecting the searchableSurface.

#include "face_selection.hpp"


namespace mousse {

class searchableSurface;

namespace faceSelections {

class searchableSurfaceSelection
:
  public faceSelection
{
  // Private data
    autoPtr<searchableSurface> surfacePtr_;
public:
  //- Runtime type information
  TYPE_NAME("searchableSurface");
  // Constructors
    //- Construct from dictionary
    searchableSurfaceSelection
    (
      const word& name,
      const fvMesh& mesh,
      const dictionary& dict
    );
    //- Clone
    autoPtr<faceSelection> clone() const
    {
      NOT_IMPLEMENTED("autoPtr<faceSelection> clone() const");
      return autoPtr<faceSelection>{nullptr};
    }
  //- Destructor
  virtual ~searchableSurfaceSelection();
  // Member Functions
    virtual void select(const label zoneID, labelList&, boolList&) const;
};

}  // namespace faceSelections
}  // namespace mousse

#endif

