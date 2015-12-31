// mousse: CFD toolbox
// Copyright (C) 2012-2014 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::targetCoeffTrim
// Description
//   Target trim forces/coefficients
//   Solves:
//     c^old + J.d(theta) = c^target
//   Where:
//     n     = time level
//     c     = coefficient vector (thrust force, roll moment, pitch moment)
//     theta = pitch angle vector (collective, roll, pitch)
//     J     = Jacobian [3x3] matrix
//   The trimmed pitch angles are found via solving the above with a
//   Newton-Raphson iterative method.  The solver tolerance can be user-input,
//   using the 'tol' entry.
//   If coefficients are requested (useCoeffs = true), the force and moments
//   are normalised using:
//             force
//     c = ---------------------------------
//       alpha*pi*rho*(omega^2)*(radius^4)
//   and
//             moment
//     c = ---------------------------------
//       alpha*pi*rho*(omega^2)*(radius^5)
//   Where:
//     alpha = user-input conversion coefficient (default = 1)
//     rho   = desity
//     omega = rotor angulr velocity
//     pi    = mathematical pi
// SourceFiles
//   target_coeff_trim.cpp
#ifndef target_coeff_trim_hpp_
#define target_coeff_trim_hpp_
#include "trim_model.hpp"
#include "tensor.hpp"
#include "vector.hpp"
namespace mousse
{
class targetCoeffTrim
:
  public trimModel
{
protected:
  // Protected data
    //- Number of iterations between calls to 'correct'
    label calcFrequency_;
    //- Flag to indicate whether to solve coeffs (true) or forces (false)
    bool useCoeffs_;
    //- Target coefficient vector (thrust force, roll moment, pitch moment)
    vector target_;
    //- Pitch angles (collective, roll, pitch) [rad]
    vector theta_;
    //- Maximum number of iterations in trim routine
    label nIter_;
    //- Convergence tolerance
    scalar tol_;
    //- Under-relaxation coefficient
    scalar relax_;
    //- Perturbation angle used to determine jacobian
    scalar dTheta_;
    //- Coefficient to allow for conversion between US and EU definitions
    scalar alpha_;
  // Protected member functions
    //- Calculate the rotor force and moment coefficients vector
    template<class RhoFieldType>
    vector calcCoeffs
    (
      const RhoFieldType& rho,
      const vectorField& U,
      const scalarField& alphag,
      vectorField& force
    ) const;
    //- Correct the model
    template<class RhoFieldType>
    void correctTrim
    (
      const RhoFieldType& rho,
      const vectorField& U,
      vectorField& force
    );
public:
  //- Run-time type information
  TypeName("targetCoeffTrim");
  //- Constructor
  targetCoeffTrim(const fv::rotorDiskSource& rotor, const dictionary& dict);
  //- Destructor
  virtual ~targetCoeffTrim();
  // Member functions
    //- Read
    void read(const dictionary& dict);
    //- Return the geometric angle of attack [rad]
    virtual tmp<scalarField> thetag() const;
    //- Correct the model
    virtual void correct
    (
      const vectorField& U,
      vectorField& force
    );
    //- Correct the model for compressible flow
    virtual void correct
    (
      const volScalarField rho,
      const vectorField& U,
      vectorField& force
    );
};
}  // namespace mousse
#endif
