#ifndef UTILITIES_MISCELLANEOUS_POST_CHANNEL_CHANNEL_INDEX_HPP_
#define UTILITIES_MISCELLANEOUS_POST_CHANNEL_CHANNEL_INDEX_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::channelIndex
// Description
//   Does averaging of fields over layers of cells. Assumes layered mesh.

#include "region_split.hpp"
#include "direction.hpp"
#include "scalar_field.hpp"
#include "poly_mesh.hpp"


namespace mousse {

class channelIndex
{
  // Private data
    static const NamedEnum<vector::components, 3> vectorComponentsNames_;
    //- Is mesh symmetric
    const bool symmetric_;
    //- Direction to sort
    const direction dir_;
    //- Per cell the global region
    autoPtr<regionSplit> cellRegion_;
    //- Per global region the number of cells (scalarField so we can use
    //  field algebra)
    scalarField regionCount_;
    //- From sorted region back to unsorted global region
    labelList sortMap_;
    //- Sorted component of cell centres
    scalarField y_;
  // Private Member Functions
    void walkOppositeFaces
    (
      const polyMesh& mesh,
      const labelList& startFaces,
      boolList& blockedFace
    );
    void calcLayeredRegions
    (
      const polyMesh& mesh,
      const labelList& startFaces
    );
public:
  // Constructors
    //- Construct from dictionary
    channelIndex(const polyMesh&, const dictionary&);
    //- Construct from supplied starting faces
    channelIndex
    (
      const polyMesh& mesh,
      const labelList& startFaces,
      const bool symmetric,
      const direction dir
    );
    //- Disallow default bitwise copy construct and assignment
    channelIndex(const channelIndex&) = delete;
    void operator=(const channelIndex&) = delete;
  // Member Functions
    // Access
      //- Sum field per region
      template<class T>
      Field<T> regionSum(const Field<T>& cellField) const;
      //- Collapse a field to a line
      template<class T>
      Field<T> collapse
      (
        const Field<T>& vsf,
        const bool asymmetric=false
      ) const;
      //- Return the field of Y locations from the cell centres
      const scalarField& y() const
      {
        return y_;
      }
};

}  // namespace mousse

#include "channel_index.ipp"

#endif
