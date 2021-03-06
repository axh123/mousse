#ifndef TRI_SURFACE_GEOMETRIC_SURFACE_PATCH_HPP_
#define TRI_SURFACE_GEOMETRIC_SURFACE_PATCH_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::geometricSurfacePatch
// Description
//   The geometricSurfacePatch is like patchIdentifier but for surfaces.
//   Holds type, name and index.

#include "word.hpp"
#include "label.hpp"
#include "type_info.hpp"


namespace mousse {

class dictionary;
class geometricSurfacePatch
{
  // Private data
    //- Type name of patch
    word geometricType_;
    //- Name of patch
    word name_;
    //- Index of patch in boundary
    label index_;
public:
  //- Runtime type information
  CLASS_NAME("geometricSurfacePatch");
  // Constructors
    //- Construct null
    geometricSurfacePatch();
    //- Construct from components
    geometricSurfacePatch
    (
      const word& geometricType,
      const word& name,
      const label index
    );
    //- Construct from Istream
    geometricSurfacePatch(Istream&, const label index);
    //- Construct from dictionary
    geometricSurfacePatch
    (
      const word& name,
      const dictionary& dict,
      const label index
    );
  // Member Functions
    //- Return name
    const word& name() const
    {
      return name_;
    }
    //- Return name
    word& name()
    {
      return name_;
    }
    //- Return the type of the patch
    const word& geometricType() const
    {
      return geometricType_;
    }
    //- Return the type of the patch
    word& geometricType()
    {
      return geometricType_;
    }
    //- Return the index of this patch in the boundaryMesh
    label index() const
    {
      return index_;
    }
    //- Return the index of this patch in the boundaryMesh
    label& index()
    {
      return index_;
    }
    //- Write
    void write(Ostream&) const;
    //- Write dictionary
    void writeDict(Ostream&) const;
  // Member Operators
    bool operator!=(const geometricSurfacePatch&) const;
    //- compare.
    bool operator==(const geometricSurfacePatch&) const;
  // Ostream Operator
    friend Ostream& operator<<(Ostream&, const geometricSurfacePatch&);
    friend Istream& operator>>(Istream&, geometricSurfacePatch&);
};

}  // namespace mousse
#endif
