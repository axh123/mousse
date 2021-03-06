#ifndef MESH_TOOLS_SEARCHABLE_SURFACE_SEARCHABLE_PLATE_HPP_
#define MESH_TOOLS_SEARCHABLE_SURFACE_SEARCHABLE_PLATE_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::searchablePlate
// Description
//   Searching on finite plate. Plate has to be aligned with coordinate
//   axes.
//   Plate defined as origin and span. One of the components of span has
//   to be 0 which defines the normal direction. E.g.
//   span    = (Sx Sy 0)     // plate in x-y plane
//   origin  = (Ox Oy Oz)
//   now plane is from (Ox Oy Oz) to (Ox+Sx Oy+Sy Oz)

#include "searchable_surface.hpp"
#include "tree_bound_box.hpp"


namespace mousse {

// Forward declaration of classes
class searchablePlate
:
  public searchableSurface
{
private:
  // Private Member Data
    const point origin_;
    const vector span_;
    //- Coordinate direction which is normal
    const direction normalDir_;
    mutable wordList regions_;
  // Private Member Functions
    //- Calculate normal direction from span
    static direction calcNormal(const point&);
    //- Inherit findNearest from searchableSurface
    using searchableSurface::findNearest;
    pointIndexHit findNearest
    (
      const point& sample,
      const scalar nearestDistSqr
    ) const;
    pointIndexHit findLine
    (
      const point& start,
      const point& end
    ) const;
public:
  //- Runtime type information
  TYPE_NAME("searchablePlate");
  // Constructors
    //- Construct from components
    searchablePlate
    (
      const IOobject& io,
      const point& origin,
      const vector& span
    );
    //- Construct from dictionary (used by searchableSurface)
    searchablePlate
    (
      const IOobject& io,
      const dictionary& dict
    );
    //- Disallow default bitwise copy construct
    searchablePlate(const searchablePlate&) = delete;
    //- Disallow default bitwise assignment
    searchablePlate& operator=(const searchablePlate&) = delete;
  //- Destructor
  virtual ~searchablePlate();
  // Member Functions
    virtual const wordList& regions() const;
    //- Whether supports volume type below
    virtual bool hasVolumeType() const
    {
      return false;
    }
    //- Range of local indices that can be returned.
    virtual label size() const
    {
      return 1;
    }
    //- Get representative set of element coordinates
    //  Usually the element centres (should be of length size()).
    virtual tmp<pointField> coordinates() const;
    //- Get bounding spheres (centre and radius squared), one per element.
    //  Any point on element is guaranteed to be inside.
    virtual void boundingSpheres
    (
      pointField& centres,
      scalarField& radiusSqr
    ) const;
    //- Get the points that define the surface.
    virtual tmp<pointField> points() const;
    //- Does any part of the surface overlap the supplied bound box?
    virtual bool overlaps(const boundBox& bb) const;
    // Multiple point queries.
      virtual void findNearest
      (
        const pointField& sample,
        const scalarField& nearestDistSqr,
        List<pointIndexHit>&
      ) const;
      virtual void findLine
      (
        const pointField& start,
        const pointField& end,
        List<pointIndexHit>&
      ) const;
      virtual void findLineAny
      (
        const pointField& start,
        const pointField& end,
        List<pointIndexHit>&
      ) const;
      //- Get all intersections in order from start to end.
      virtual void findLineAll
      (
        const pointField& start,
        const pointField& end,
        List<List<pointIndexHit> >&
      ) const;
      //- From a set of points and indices get the region
      virtual void getRegion
      (
        const List<pointIndexHit>&,
        labelList& region
      ) const;
      //- From a set of points and indices get the normal
      virtual void getNormal
      (
        const List<pointIndexHit>&,
        vectorField& normal
      ) const;
      //- Determine type (inside/outside/mixed) for point. unknown if
      //  cannot be determined (e.g. non-manifold surface)
      virtual void getVolumeType
      (
        const pointField&,
        List<volumeType>&
      ) const;
    // regIOobject implementation
      bool writeData(Ostream&) const
      {
        NOT_IMPLEMENTED("searchablePlate::writeData(Ostream&) const");
        return false;
      }
};
}  // namespace mousse
#endif
