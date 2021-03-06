#ifndef CONVERSION_MESH_TABLES_CELL_TABLE_HPP_
#define CONVERSION_MESH_TABLES_CELL_TABLE_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::cellTable
// Description
//   The cellTable persistent data saved as a Map<dictionary>.
//   The meshReader supports cellTable information.
//   The <tt>constant/cellTable</tt> file is an \c IOMap<dictionary> that is
//   used to save the information persistently. It contains the cellTable
//   information of the following form:
//   \verbatim
//     (
//       ID
//       {
//         Label           WORD;
//         MaterialType    WORD;
//         MaterialId      INT;
//         PorosityId      INT;
//         ColorIdx        INT;
//         ...
//       }
//     ...
//     )
//   \endverbatim
//   If the \a Label is missing, a value <tt>cellTable_{ID}</tt> will be
//   inferred. If the \a MaterialType is missing, the value @a fluid will
//   be inferred.

#include "poly_mesh.hpp"
#include "map.hpp"
#include "dictionary.hpp"
#include "label_list.hpp"
#include "word_re_list.hpp"


namespace mousse {

class cellTable
:
  public Map<dictionary>
{
  // Private data
   static const char* const defaultMaterial_;
  // Private Member Functions
    //- Map from cellTable ID => zone number
    Map<label> zoneMap() const;
    //- A contiguous list of cellTable names
    List<word> namesList() const;
    //- Add required entries - MaterialType
    void addDefaults();
    void setEntry(const label id, const word& keyWord, const word& value);
public:
  // Constructors
    //- Construct null
    cellTable();
    //- Construct read from registry, name. instance
    cellTable
    (
      const objectRegistry&,
      const word& name = "cellTable",
      const fileName& instance = "constant"
    );
    //- Disallow default bitwise copy construct
    cellTable(const cellTable&) = delete;
  //- Destructor
  ~cellTable();
  // Member Functions
    //- Append to the end, return index
    label append(const dictionary&);
    //- Return index corresponding to name
    //  returns -1 if not found
    label findIndex(const word& name) const;
    //- Return the name corresponding to id
    //  returns cellTable_ID if not otherwise defined
    word name(const label id) const;
    //- Return a Map of (id => name)
    Map<word> names() const;
    //- Return a Map of (id => names) selected by patterns
    Map<word> names(const UList<wordRe>& patterns) const;
    //- Return a Map of (id => name) for materialType
    //  (fluid | solid | shell)
    Map<word> selectType(const word& materialType) const;
    //- Return a Map of (id => name) for fluids
    Map<word> fluids() const;
    //- Return a Map of (id => name) for shells
    Map<word> shells() const;
    //- Return a Map of (id => name) for solids
    Map<word> solids() const;
    //- Return a Map of (id => fluid|solid|shell)
    Map<word> materialTypes() const;
    //- Assign material Type
    void setMaterial(const label, const word&);
    //- Assign name
    void setName(const label, const word&);
    //- Assign default name if not already set
    void setName(const label);
    //- Read constant/cellTable
    void readDict
    (
      const objectRegistry&,
      const word& name = "cellTable",
      const fileName& instance = "constant"
    );
    //- Write constant/cellTable for later reuse
    void writeDict
    (
      const objectRegistry&,
      const word& name = "cellTable",
      const fileName& instance = "constant"
    ) const;
  // Member Operators
    //- Assignment
    void operator=(const cellTable&);
    //- Assign from Map<dictionary>
    void operator=(const Map<dictionary>&);
    //- Assign from cellZones
    void operator=(const polyMesh&);
  // Friend Functions
    //- Classify tableIds into cellZones according to the cellTable
    void addCellZones(polyMesh&, const labelList& tableIds) const;
    //- Combine tableIds together
    //  each dictionary entry is a wordList
    void combine(const dictionary&, labelList& tableIds);
};

}  // namespace mousse

#endif

