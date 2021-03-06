#ifndef UTILITIES_MESH_GENERATION_FOAMY_MESH_CONFORMAL_VORONOI_MESH_SEARCHABLE_PLATE_FEATURES_HPP_
#define UTILITIES_MESH_GENERATION_FOAMY_MESH_CONFORMAL_VORONOI_MESH_SEARCHABLE_PLATE_FEATURES_HPP_

// mousse: CFD toolbox
// Copyright (C) 2013-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::searchablePlateFeatures

#include "searchable_surface_features.hpp"


namespace mousse {

class searchablePlateFeatures
:
  public searchableSurfaceFeatures
{
private:
    //- To initialise edges.
    static edgeList calcEdges(const label[4][2]);
  // Private Member Data
    //- Which side of the plate to mesh
    const extendedFeatureEdgeMesh::sideVolumeType mode_;
public:
  //- Runtime type information
  TYPE_NAME("searchablePlateFeatures");
  // Static data members
    //- Edge to point addressing
    static const edgeList edges;
  // Constructors
    //- Construct from searchable surface and dictionary
    searchablePlateFeatures
    (
      const searchableSurface& surface,
      const dictionary& dict
    );
    //- Disallow default bitwise copy construct
    searchablePlateFeatures(const searchablePlateFeatures&) = delete;
    //- Disallow default bitwise assignment
    void operator=(const searchablePlateFeatures&) = delete;
  //- Destructor
  virtual ~searchablePlateFeatures();
  // Member Functions
    //- Return true for a searchable plate having features
    virtual bool hasFeatures() const
    {
      return true;
    }
    //- Return an extendedFeatureEdgeMesh containing the features
    virtual autoPtr<extendedFeatureEdgeMesh> features() const;
};

}  // namespace mousse

#endif

