#ifndef PARALLEL_DECOMPOSE_SCOTCH_DECOMP_SCOTCH_DECOMP_HPP_
#define PARALLEL_DECOMPOSE_SCOTCH_DECOMP_SCOTCH_DECOMP_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::scotchDecomp
// Description
//   Scotch domain decomposition.
//   When run in parallel will collect the whole graph on to the master,
//   decompose and send back. Run ptscotchDecomp for proper distributed
//   decomposition.

#include "decomposition_method.hpp"


namespace mousse {

class scotchDecomp
:
  public decompositionMethod
{
  // Private Member Functions
    //- Check and print error message
    static void check(const int, const char*);
    label decompose
    (
      const fileName& meshPath,
      const List<label>& adjncy,
      const List<label>& xadj,
      const scalarField& cWeights,
      List<label>& finalDecomp
    );
    //- Decompose non-parallel
    label decomposeOneProc
    (
      const fileName& meshPath,
      const List<label>& adjncy,
      const List<label>& xadj,
      const scalarField& cWeights,
      List<label>& finalDecomp
    );
public:
  //- Runtime type information
  TYPE_NAME("scotch");
  // Constructors
    //- Construct given the decomposition dictionary and mesh
    scotchDecomp(const dictionary& decompositionDict);
    //- Disallow default bitwise copy construct and assignment
    scotchDecomp& operator=(const scotchDecomp&) = delete;
    scotchDecomp(const scotchDecomp&) = delete;
  //- Destructor
  virtual ~scotchDecomp()
  {}
  // Member Functions
    virtual bool parallelAware() const
    {
      // Knows about coupled boundaries
      return true;
    }
    //- Inherit decompose from decompositionMethod
    using decompositionMethod::decompose;
    //- Return for every coordinate the wanted processor number. Use the
    //  mesh connectivity (if needed)
    //  Weights get normalised with minimum weight and truncated to
    //  convert into integer so e.g. 3.5 is seen as 3. The overall sum
    //  of weights might otherwise overflow.
    virtual labelList decompose
    (
      const polyMesh& mesh,
      const pointField& points,
      const scalarField& pointWeights
    );
    //- Return for every coordinate the wanted processor number. Gets
    //  passed agglomeration map (from fine to coarse cells) and coarse cell
    //  location. Can be overridden by decomposers that provide this
    //  functionality natively.
    //  See note on weights above.
    virtual labelList decompose
    (
      const polyMesh& mesh,
      const labelList& agglom,
      const pointField& regionPoints,
      const scalarField& regionWeights
    );
    //- Return for every coordinate the wanted processor number. Explicitly
    //  provided mesh connectivity.
    //  The connectivity is equal to mesh.cellCells() except for
    //  - in parallel the cell numbers are global cell numbers (starting
    //    from 0 at processor0 and then incrementing all through the
    //    processors)
    //  - the connections are across coupled patches
    //  See note on weights above.
    virtual labelList decompose
    (
      const labelListList& globalCellCells,
      const pointField& cc,
      const scalarField& cWeights
    );
};

}  // namespace mousse

#endif

