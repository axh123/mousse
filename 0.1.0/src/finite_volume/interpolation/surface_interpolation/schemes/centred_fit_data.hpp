#ifndef FINITE_VOLUME_INTERPOLATION_SURFACE_INTERPOLATION_SCHEMES_CENTRED_FIT_DATA_HPP_
#define FINITE_VOLUME_INTERPOLATION_SURFACE_INTERPOLATION_SCHEMES_CENTRED_FIT_DATA_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::CentredFitData
// Description
//   Data for the quadratic fit correction interpolation scheme

#include "fit_data.hpp"
#include "extended_centred_cell_to_face_stencil.hpp"


namespace mousse {

template<class Polynomial>
class CentredFitData
:
  public FitData
  <
    CentredFitData<Polynomial>,
    extendedCentredCellToFaceStencil,
    Polynomial
  >
{
  // Private data
    //- For each cell in the mesh store the values which multiply the
    //  values of the stencil to obtain the gradient for each direction
    List<scalarList> coeffs_;
  // Private Member Functions
    //- Calculate the fit for the all the mesh faces
    //  and set the coefficients
    void calcFit();
public:
  TYPE_NAME("CentredFitData");
  // Constructors
    //- Construct from components
    CentredFitData
    (
      const fvMesh& mesh,
      const extendedCentredCellToFaceStencil& stencil,
      const scalar linearLimitFactor,
      const scalar centralWeight
    );
  //- Destructor
  virtual ~CentredFitData()
  {}
  // Member functions
    //- Return reference to fit coefficients
    const List<scalarList>& coeffs() const
    {
      return coeffs_;
    }
};

}  // namespace mousse

#include "centred_fit_data.ipp"

#endif
