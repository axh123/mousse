// mousse: CFD toolbox
// Copyright (C) 2011-2014 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::dynamicRefineFvMesh
// Description
//   A fvMesh with built-in refinement.
//   Determines which cells to refine/unrefine and does all in update().
//     // How often to refine
//     refineInterval  1;
//     // Field to be refinement on
//     field           alpha.water;
//     // Refine field inbetween lower..upper
//     lowerRefineLevel 0.001;
//     upperRefineLevel 0.999;
//     // If value < unrefineLevel (default=GREAT) unrefine
//     //unrefineLevel   10;
//     // Have slower than 2:1 refinement
//     nBufferLayers   1;
//     // Refine cells only up to maxRefinement levels
//     maxRefinement   2;
//     // Stop refinement if maxCells reached
//     maxCells        200000;
//     // Flux field and corresponding velocity field. Fluxes on changed
//     // faces get recalculated by interpolating the velocity. Use 'none'
//     // on surfaceScalarFields that do not need to be reinterpolated, use
//     // NaN to detect use of mapped variable
//     correctFluxes
//     (
//       (phi none)  //NaN)   //none)
//       (nHatf none)   //none)
//       (rho*phi none)   //none)
//       (ghf none)  //NaN)   //none)
//     );
//     // Write the refinement level as a volScalarField
//     dumpLevel       true;
// SourceFiles
//   dynamic_refine_fv_mesh.cpp
#ifndef dynamic_refine_fv_mesh_hpp_
#define dynamic_refine_fv_mesh_hpp_
#include "dynamic_fv_mesh.hpp"
#include "hex_ref8.hpp"
#include "packed_bool_list.hpp"
#include "switch.hpp"
namespace mousse
{
class dynamicRefineFvMesh
:
  public dynamicFvMesh
{
protected:
    //- Mesh cutting engine
    hexRef8 meshCutter_;
    //- Dump cellLevel for postprocessing
    Switch dumpLevel_;
    //- Fluxes to map
    HashTable<word> correctFluxes_;
    //- Number of refinement/unrefinement steps done so far.
    label nRefinementIterations_;
    //- Protected cells (usually since not hexes)
    PackedBoolList protectedCell_;
  // Private Member Functions
    //- Count set/unset elements in packedlist.
    static label count(const PackedBoolList&, const unsigned int);
    //- Calculate cells that cannot be refined since would trigger
    //  refinement of protectedCell_ (since 2:1 refinement cascade)
    void calculateProtectedCells(PackedBoolList& unrefineableCell) const;
    //- Read the projection parameters from dictionary
    void readDict();
    //- Refine cells. Update mesh and fields.
    autoPtr<mapPolyMesh> refine(const labelList&);
    //- Unrefine cells. Gets passed in centre points of cells to combine.
    autoPtr<mapPolyMesh> unrefine(const labelList&);
    // Selection of cells to un/refine
      //- Calculates approximate value for refinement level so
      //  we don't go above maxCell
      scalar getRefineLevel
      (
        const label maxCells,
        const label maxRefinement,
        const scalar refineLevel,
        const scalarField&
      ) const;
      //- Get per cell max of connected point
      scalarField maxPointField(const scalarField&) const;
      //- Get point max of connected cell
      scalarField maxCellField(const volScalarField&) const;
      scalarField cellToPoint(const scalarField& vFld) const;
      scalarField error
      (
        const scalarField& fld,
        const scalar minLevel,
        const scalar maxLevel
      ) const;
      //- Select candidate cells for refinement
      virtual void selectRefineCandidates
      (
        const scalar lowerRefineLevel,
        const scalar upperRefineLevel,
        const scalarField& vFld,
        PackedBoolList& candidateCell
      ) const;
      //- Subset candidate cells for refinement
      virtual labelList selectRefineCells
      (
        const label maxCells,
        const label maxRefinement,
        const PackedBoolList& candidateCell
      ) const;
      //- Select points that can be unrefined.
      virtual labelList selectUnrefinePoints
      (
        const scalar unrefineLevel,
        const PackedBoolList& markedCell,
        const scalarField& pFld
      ) const;
      //- Extend markedCell with cell-face-cell.
      void extendMarkedCells(PackedBoolList& markedCell) const;
      //- Check all cells have 8 anchor points
      void checkEightAnchorPoints
      (
        PackedBoolList& protectedCell,
        label& nProtected
      ) const;
public:
  //- Runtime type information
  TYPE_NAME("dynamicRefineFvMesh");
  // Constructors
    //- Construct from IOobject
    explicit dynamicRefineFvMesh(const IOobject& io);
    //- Disallow default bitwise copy construct
    dynamicRefineFvMesh(const dynamicRefineFvMesh&) = delete;
    //- Disallow default bitwise assignment
    dynamicRefineFvMesh& operator=(const dynamicRefineFvMesh&) = delete;
  //- Destructor
  virtual ~dynamicRefineFvMesh();
  // Member Functions
    //- Direct access to the refinement engine
    const hexRef8& meshCutter() const
    {
      return meshCutter_;
    }
    //- Cells which should not be refined/unrefined
    const PackedBoolList& protectedCell() const
    {
      return protectedCell_;
    }
    //- Cells which should not be refined/unrefined
    PackedBoolList& protectedCell()
    {
      return protectedCell_;
    }
    //- Update the mesh for both mesh motion and topology change
    virtual bool update();
  // Writing
    //- Write using given format, version and compression
    virtual bool writeObject
    (
      IOstream::streamFormat fmt,
      IOstream::versionNumber ver,
      IOstream::compressionType cmp
    ) const;
};
}  // namespace mousse
#endif
