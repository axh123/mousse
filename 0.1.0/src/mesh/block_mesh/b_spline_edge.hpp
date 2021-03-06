#ifndef MESH_BLOCK_MESH_CURVED_EDGES_B_SPLINE_EDGE_HPP_
#define MESH_BLOCK_MESH_CURVED_EDGES_B_SPLINE_EDGE_HPP_

// mousse: CFD toolbox
// Copyright (C) 2014 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::BSplineEdge
// Description
//   A curvedEdge interface for B-splines.

#include "curved_edge.hpp"
#include "b_spline.hpp"


namespace mousse {

class BSplineEdge
:
  public curvedEdge,
  public BSpline
{
public:
  //- Runtime type information
  TYPE_NAME("BSpline");
  // Constructors
    //- Construct from components
    BSplineEdge
    (
      const pointField&,
      const label start,
      const label end,
      const pointField& internalPoints
    );
    //- Construct from Istream, setting pointsList
    BSplineEdge(const pointField&, Istream&);
    //- Disallow default bitwise copy construct
    BSplineEdge(const BSplineEdge&) = delete;
    //- Disallow default bitwise assignment
    BSplineEdge& operator=(const BSplineEdge&) = delete;
  //- Destructor
  virtual ~BSplineEdge();
  // Member Functions
    //- Return the point position corresponding to the curve parameter
    //  0 <= lambda <= 1
    virtual point position(const scalar) const;
    //- Return the length of the spline curve (not implemented)
    virtual scalar length() const;
};

}  // namespace mousse

#endif

