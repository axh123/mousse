#ifndef MESH_TOOLS_SETS_FACE_ZONE_SOURCES_SETS_TO_FACE_ZONE_SETS_TO_FACE_ZONE_HPP_
#define MESH_TOOLS_SETS_FACE_ZONE_SOURCES_SETS_TO_FACE_ZONE_SETS_TO_FACE_ZONE_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011-2013 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::setsToFaceZone
// Description
//   A topoSetSource to select faces based on usage in a faceSet and cellSet

#include "topo_set_source.hpp"
#include "switch.hpp"


namespace mousse {

class setsToFaceZone
:
  public topoSetSource
{
  // Private data
    //- Add usage string
    static addToUsageTable usage_;
    //- Name of set to use
    const word faceSetName_;
    //- Name of set to use
    const word cellSetName_;
    //- Whether cellSet is slave cells or master cells
    const Switch flip_;
public:
  //- Runtime type information
  TYPE_NAME("setsToFaceZone");
  // Constructors
    //- Construct from components
    setsToFaceZone
    (
      const polyMesh& mesh,
      const word& faceSetName,
      const word& cellSetName,
      const Switch& flip
    );
    //- Construct from dictionary
    setsToFaceZone
    (
      const polyMesh& mesh,
      const dictionary& dict
    );
    //- Construct from Istream
    setsToFaceZone
    (
      const polyMesh& mesh,
      Istream&
    );
  //- Destructor
  virtual ~setsToFaceZone();
  // Member Functions
    virtual sourceType setType() const
    {
      return FACEZONESOURCE;
    }
    virtual void applyToSet
    (
      const topoSetSource::setAction action,
      topoSet&
    ) const;
};
}  // namespace mousse
#endif
