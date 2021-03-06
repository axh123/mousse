#ifndef FINITE_VOLUME_INTERPOLATION_SURFACE_INTERPOLATION_SCHEMES_CUBIC_HPP_
#define FINITE_VOLUME_INTERPOLATION_SURFACE_INTERPOLATION_SCHEMES_CUBIC_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::cubic
// Description
//   Cubic interpolation scheme class derived from linear and returns
//   linear weighting factors but also applies an explicit correction.

#include "surface_fields.hpp"
#include "linear.hpp"
#include "gauss_grad.hpp"
#include "time.hpp"


namespace mousse {

template<class Type>
class cubic
:
  public linear<Type>
{
  // Private Member Functions
public:
  //- Runtime type information
  TYPE_NAME("cubic");
  // Constructors
    //- Construct from mesh
    cubic(const fvMesh& mesh)
    :
      linear<Type>{mesh}
    {}
    //- Construct from mesh and Istream
    cubic
    (
      const fvMesh& mesh,
      Istream&
    )
    :
      linear<Type>{mesh}
    {}
    //- Construct from mesh, faceFlux and Istream
    cubic
    (
      const fvMesh& mesh,
      const surfaceScalarField&,
      Istream&
    )
    :
      linear<Type>{mesh}
    {}
    //- Disallow default bitwise copy construct
    cubic(const cubic&) = delete;
    //- Disallow default bitwise assignment
    cubic& operator=(const cubic&) = delete;
  // Member Functions
    //- Return true if this scheme uses an explicit correction
    virtual bool corrected() const
    {
      return true;
    }
    //- Return the explicit correction to the face-interpolate
    virtual tmp<GeometricField<Type, fvsPatchField, surfaceMesh> >
    correction
    (
      const GeometricField<Type, fvPatchField, volMesh>& vf
    ) const
    {
      const fvMesh& mesh = this->mesh();
      // calculate the appropriate interpolation factors
      const surfaceScalarField& lambda = mesh.weights();
      const surfaceScalarField kSc
      {
        lambda*(scalar(1) - lambda*(scalar(3) - scalar(2)*lambda))
      };
      const surfaceScalarField kVecP{sqr(scalar(1) - lambda)*lambda};
      const surfaceScalarField kVecN{sqr(lambda)*(lambda - scalar(1))};
      tmp<GeometricField<Type, fvsPatchField, surfaceMesh> > tsfCorr
      {
        new GeometricField<Type, fvsPatchField, surfaceMesh>
        {
          {
            "cubic::correction(" + vf.name() +')',
            mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE,
            false
          },
          surfaceInterpolationScheme<Type>::interpolate(vf, kSc, -kSc)
        }
      };
      GeometricField<Type, fvsPatchField, surfaceMesh>& sfCorr = tsfCorr();
      for (direction cmpt=0; cmpt<pTraits<Type>::nComponents; cmpt++) {
        sfCorr.replace
        (
          cmpt,
          sfCorr.component(cmpt)
          + (
            surfaceInterpolationScheme
            <
              typename outerProduct
              <
                vector,
                typename pTraits<Type>::cmptType
              >::type
            >::interpolate
            (
              fv::gaussGrad
              <typename pTraits<Type>::cmptType>(mesh).grad(vf.component(cmpt)),
              kVecP,
              kVecN
            ) & mesh.Sf()
          )/mesh.magSf()/mesh.surfaceInterpolation::deltaCoeffs()
        );
      }
      FOR_ALL(sfCorr.boundaryField(), pi) {
        if (!sfCorr.boundaryField()[pi].coupled()) {
          sfCorr.boundaryField()[pi] = pTraits<Type>::zero;
        }
      }
      return tsfCorr;
    }
};

}  // namespace mousse

#endif

