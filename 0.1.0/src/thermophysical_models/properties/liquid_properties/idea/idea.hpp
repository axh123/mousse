// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::IDEA
// Description
//   The IDEA fuel is constructed by adding 30% alphaMethylNaphthalene
//   with 70% n-decane.
//   The new properties have been calculated by adding the values in these
//   proportions and making a least square fit, using the same NSRDS-eq.
//   so that Y = 0.3*Y_naphthalene + 0.7*Y_decane
//   The valid Temperature range for n-decane is normally 243.51 - 617.70 K
//   and for the naphthalene it is 242.67 - 772.04 K
//   The least square fit was done in the interval 244 - 617 K
//   The critical temperature was taken to be 618.074 K, since this
//   is the 'c'-value in the rho-equation, which corresponds to Tcrit,
//   This value was then used in the fit for the NSRDS6-eq, which uses Tcrit.
//   (important for the latent heat and surface tension)
//   The molecular weights are 142.20 and 142.285 and for the IDEA fuel
//   it is thus 142.26 ( approximately 0.3*142.2 + 0.7*142.285 )
//   Critical pressure was set to the lowest one (n-Decane)
//   Critical volume... also the lowest one (naphthalene) 0.523 m^3/kmol
//   Second Virial Coefficient is n-Decane
// SourceFiles
//   idea.cpp
#ifndef idea_hpp_
#define idea_hpp_
#include "liquid_properties.hpp"
#include "nsrds_func0.hpp"
#include "nsrds_func1.hpp"
#include "nsrds_func2.hpp"
#include "nsrds_func3.hpp"
#include "nsrds_func4.hpp"
#include "nsrds_func5.hpp"
#include "nsrds_func6.hpp"
#include "nsrds_func7.hpp"
#include "api_diff_coef_func.hpp"
namespace mousse
{
class IDEA
:
  public liquidProperties
{
  // Private data
    NSRDSfunc5 rho_;
    NSRDSfunc1 pv_;
    NSRDSfunc6 hl_;
    NSRDSfunc0 Cp_;
    NSRDSfunc0 h_;
    NSRDSfunc7 Cpg_;
    NSRDSfunc4 B_;
    NSRDSfunc1 mu_;
    NSRDSfunc2 mug_;
    NSRDSfunc0 K_;
    NSRDSfunc2 Kg_;
    NSRDSfunc6 sigma_;
    APIdiffCoefFunc D_;
public:
  //- Runtime type information
  TYPE_NAME("IDEA");
  // Constructors
    //- Construct null
    IDEA();
    // Construct from components
    IDEA
    (
      const liquidProperties& l,
      const NSRDSfunc5& density,
      const NSRDSfunc1& vapourPressure,
      const NSRDSfunc6& heatOfVapourisation,
      const NSRDSfunc0& heatCapacity,
      const NSRDSfunc0& enthalpy,
      const NSRDSfunc7& idealGasHeatCapacity,
      const NSRDSfunc4& secondVirialCoeff,
      const NSRDSfunc1& dynamicViscosity,
      const NSRDSfunc2& vapourDynamicViscosity,
      const NSRDSfunc0& thermalConductivity,
      const NSRDSfunc2& vapourThermalConductivity,
      const NSRDSfunc6& surfaceTension,
      const APIdiffCoefFunc& vapourDiffussivity
    );
    //- Construct from Istream
    IDEA(Istream& is);
    //- Construct from dictionary
    IDEA(const dictionary& dict);
    //- Construct copy
    IDEA(const IDEA& liq);
    //- Construct and return clone
    virtual autoPtr<liquidProperties> clone() const
    {
      return autoPtr<liquidProperties>(new IDEA(*this));
    }
  // Member Functions
    //- Liquid density [kg/m^3]
    inline scalar rho(scalar p, scalar T) const;
    //- Vapour pressure [Pa]
    inline scalar pv(scalar p, scalar T) const;
    //- Heat of vapourisation [J/kg]
    inline scalar hl(scalar p, scalar T) const;
    //- Liquid heat capacity [J/(kg K)]
    inline scalar Cp(scalar p, scalar T) const;
    //- Liquid Enthalpy [J/(kg)]
    inline scalar h(scalar p, scalar T) const;
    //- Ideal gas heat capacity [J/(kg K)]
    inline scalar Cpg(scalar p, scalar T) const;
    //- Second Virial Coefficient [m^3/kg]
    inline scalar B(scalar p, scalar T) const;
    //- Liquid viscosity [Pa s]
    inline scalar mu(scalar p, scalar T) const;
    //- Vapour viscosity [Pa s]
    inline scalar mug(scalar p, scalar T) const;
    //- Liquid thermal conductivity  [W/(m K)]
    inline scalar K(scalar p, scalar T) const;
    //- Vapour thermal conductivity  [W/(m K)]
    inline scalar Kg(scalar p, scalar T) const;
    //- Surface tension [N/m]
    inline scalar sigma(scalar p, scalar T) const;
    //- Vapour diffussivity [m2/s]
    inline scalar D(scalar p, scalar T) const;
    //- Vapour diffussivity [m2/s] with specified binary pair
    inline scalar D(scalar p, scalar T, scalar Wb) const;
  // I-O
    //- Write the function coefficients
    void writeData(Ostream& os) const
    {
      liquidProperties::writeData(os); os << nl;
      rho_.writeData(os); os << nl;
      pv_.writeData(os); os << nl;
      hl_.writeData(os); os << nl;
      Cp_.writeData(os); os << nl;
      h_.writeData(os); os << nl;
      Cpg_.writeData(os); os << nl;
      mu_.writeData(os); os << nl;
      mug_.writeData(os); os << nl;
      K_.writeData(os); os << nl;
      Kg_.writeData(os); os << nl;
      sigma_.writeData(os); os << nl;
      D_.writeData(os); os << endl;
    }
    //- Ostream Operator
    friend Ostream& operator<<(Ostream& os, const IDEA& l)
    {
      l.writeData(os);
      return os;
    }
};
}  // namespace mousse
#include "ideai.hpp"
#endif
