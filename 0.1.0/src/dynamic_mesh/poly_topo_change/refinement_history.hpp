#ifndef DYNAMIC_MESH_POLY_TOPO_CHANGE_POLY_TOPO_CHANGE_REFINEMENT_HISTORY_HPP_
#define DYNAMIC_MESH_POLY_TOPO_CHANGE_POLY_TOPO_CHANGE_REFINEMENT_HISTORY_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::refinementHistory
// Description
//   All refinement history. Used in unrefinement.
//   - visibleCells: valid for the current mesh and contains per cell -1
//   (cell unrefined) or an index into splitCells_.
//   - splitCells: for every split contains the parent (also index into
//    splitCells) and optionally a subsplit as 8 indices into splitCells.
//    Note that the numbers in splitCells are not cell labels, they are purely
//    indices into splitCells.
//   E.g. 2 cells, cell 1 gets refined so end up with 9 cells:
//   \verbatim
//     // splitCells
//     9
//     (
//     -1 (1 2 3 4 5 6 7 8)
//     0 0()
//     0 0()
//     0 0()
//     0 0()
//     0 0()
//     0 0()
//     0 0()
//     0 0()
//     )
//     // visibleCells
//     9(-1 1 2 3 4 5 6 7 8)
//   \endverbatim
//   So cell0 (visibleCells=-1) is unrefined.
//   Cells 1-8 have all valid splitCells entries which are:
//    - parent:0
//    - subsplits:0()
//   The parent 0 refers back to the splitcell entries.

#include "dynamic_list.hpp"
#include "label_list.hpp"
#include "fixed_list.hpp"
#include "sl_list.hpp"
#include "auto_ptr.hpp"
#include "reg_ioobject.hpp"


namespace mousse {

// Forward declaration of classes
class mapPolyMesh;
class mapDistributePolyMesh;


class refinementHistory
:
  public regIOobject
{
public:
  class splitCell8
  {
  public:
    // Index to original splitCell this cell was refined off from
    // -1: top level cell
    // -2: free splitCell (so should also be in freeSplitCells_)
    label parent_;
    //- Cells this cell was refined into
    autoPtr<FixedList<label, 8> > addedCellsPtr_;
    //- Construct null (parent = -1)
    splitCell8();
    //- Construct from parent
    splitCell8(const label parent);
    //- Construct from Istream
    splitCell8(Istream& is);
    //- Construct as deep copy
    splitCell8(const splitCell8&);
    //- Copy operator since autoPtr otherwise 'steals' storage.
    void operator=(const splitCell8& s)
    {
      // Check for assignment to self
      if (this == &s) {
        FATAL_ERROR_IN("splitCell8::operator=(const mousse::splitCell8&)")
          << "Attempted assignment to self"
          << abort(FatalError);
      }
      parent_ = s.parent_;
      addedCellsPtr_.reset
      (
        s.addedCellsPtr_.valid()
        ? new FixedList<label, 8>{s.addedCellsPtr_()}
        : NULL
      );
    }
    bool operator==(const splitCell8& s) const
    {
      if (addedCellsPtr_.valid() != s.addedCellsPtr_.valid()) {
        return false;
      } else if (parent_ != s.parent_) {
        return false;
      } else if (addedCellsPtr_.valid()) {
        return addedCellsPtr_() == s.addedCellsPtr_();
      } else {
        return true;
      }
    }
    bool operator!=(const splitCell8& s) const
    {
      return !operator==(s);
    }
    friend Istream& operator>>(Istream&, splitCell8&);
    friend Ostream& operator<<(Ostream&, const splitCell8&);
  };
private:
  // Private data
    //- Storage for splitCells
    DynamicList<splitCell8> splitCells_;
    //- Unused indices in splitCells
    DynamicList<label> freeSplitCells_;
    //- Currently visible cells. Indices into splitCells.
    labelList visibleCells_;
  // Private Member Functions
    //- Debug write
    static void writeEntry
    (
      const List<splitCell8>&,
      const splitCell8&
    );
    //- Debug write
    static void writeDebug
    (
      const labelList&,
      const List<splitCell8>&
    );
    //- Check consistency of structure, i.e. indices into splitCells_.
    void checkIndices() const;
    //- Allocate a splitCell. Return index in splitCells_.
    label allocateSplitCell(const label parent, const label i);
    //- Free a splitCell.
    void freeSplitCell(const label index);
    //- Mark entry in splitCells. Recursively mark its parent and subs.
    void markSplit
    (
      const label,
      labelList& oldToNew,
      DynamicList<splitCell8>&
    ) const;
    void countProc
    (
      const label index,
      const label newProcNo,
      labelList& splitCellProc,
      labelList& splitCellNum
    ) const;
public:
  // Declare name of the class and its debug switch
  TYPE_NAME("refinementHistory");
  // Constructors
    //- Construct (read) given an IOobject
    refinementHistory(const IOobject&);
    //- Construct (read) or construct null
    refinementHistory
    (
      const IOobject&,
      const List<splitCell8>& splitCells,
      const labelList& visibleCells
    );
    //- Construct (read) or construct from initial number of cells
    //  (all visible)
    refinementHistory(const IOobject&, const label nCells);
    //- Construct as copy
    refinementHistory(const IOobject&, const refinementHistory&);
    //- Construct from Istream
    refinementHistory(const IOobject&, Istream&);
  // Member Functions
    //- Per cell in the current mesh (i.e. visible) either -1 (unrefined)
    //  or an index into splitCells.
    const labelList& visibleCells() const { return visibleCells_; }
    //- Storage for splitCell8s.
    const DynamicList<splitCell8>& splitCells() const { return splitCells_; }
    //- Cache of unused indices in splitCells
    const DynamicList<label>& freeSplitCells() const { return freeSplitCells_; }
    //- Is there unrefinement history. Note that this will fall over if
    //  there are 0 cells in the mesh. But this gives problems with
    //  lots of other programs anyway.
    bool active() const { return visibleCells_.size() > 0; }
    //- Get parent of cell
    label parentIndex(const label cellI) const
    {
      label index = visibleCells_[cellI];
      if (index < 0) {
        FATAL_ERROR_IN("refinementHistory::parentIndex(const label)")
          << "Cell " << cellI << " is not visible"
          << abort(FatalError);
      }
      return splitCells_[index].parent_;
    }
    //- Store splitting of cell into 8
    void storeSplit
    (
      const label cellI,
      const labelList& addedCells
    );
    //- Store combining 8 cells into master
    void combineCells
    (
      const label masterCellI,
      const labelList& combinedCells
    );
    //- Update numbering for mesh changes
    void updateMesh(const mapPolyMesh&);
    //- Update numbering for subsetting
    void subset
    (
      const labelList& pointMap,
      const labelList& faceMap,
      const labelList& cellMap
    );
    //- Update local numbering for mesh redistribution.
    //  Can only distribute clusters sent across in one go; cannot
    //  handle parts recombined in multiple passes.
    void distribute(const mapDistributePolyMesh&);
    //- Compact splitCells_. Removes all freeSplitCells_ elements.
    void compact();
    //- Extend/shrink storage. additional visibleCells_ elements get
    //  set to -1.
    void resize(const label nCells);
    //- Debug write
    void writeDebug() const;
    //- ReadData function required for regIOobject read operation
    virtual bool readData(Istream&);
    //- WriteData function required for regIOobject write operation
    virtual bool writeData(Ostream&) const;
  // IOstream Operators
    friend Istream& operator>>(Istream&, refinementHistory&);
    friend Ostream& operator<<(Ostream&, const refinementHistory&);
};
}  // namespace mousse
#endif
