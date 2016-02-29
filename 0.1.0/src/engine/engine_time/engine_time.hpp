#ifndef ENGINE_ENGINE_TIME_ENGINE_TIME_HPP_
#define ENGINE_ENGINE_TIME_ENGINE_TIME_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::engineTime
// Description
//   Manage time in terms of engine RPM and crank-angle.
//   When engineTime is in effect, the userTime is reported in degrees
//   crank-angle instead of in seconds. The RPM to be used is specified in
//   \c constant/engineGeometry. If only a time conversion is required,
//   the geometric engine parameters can be dropped or set to zero.
//   For example,
//   \verbatim
//     rpm             rpm  [0 0 -1 0 0]  2000;
//     conRodLength    conRodLength  [0 1 0 0 0] 0.0;
//     bore            bore          [0 1 0 0 0] 0.0;
//     stroke          stroke        [0 1 0 0 0] 0.0;
//     clearance       clearance     [0 1 0 0 0] 0.0;
//   \endverbatim
// Note
//  The engineTime can currently only be selected at compile-time.
// SourceFiles
//   engine_time.cpp
#include "time.hpp"
#include "dictionary.hpp"
#include "dimensioned_scalar.hpp"
namespace mousse
{
class engineTime
:
  public Time
{
  // Private data
    IOdictionary dict_;
    //- RPM is required
    dimensionedScalar rpm_;
    //- Optional engine geometry parameters
    dimensionedScalar conRodLength_;
    dimensionedScalar bore_;
    dimensionedScalar stroke_;
    dimensionedScalar clearance_;
  // Private Member Functions
    //- Adjust read time values
    void timeAdjustment();
public:
  // Constructors
    //- Construct from objectRegistry arguments
    engineTime
    (
      const word& name,
      const fileName& rootPath,
      const fileName& caseName,
      const fileName& systemName = "system",
      const fileName& constantName = "constant",
      const fileName& dictName = "engineGeometry"
    );
    //- Disallow default bitwise copy construct
    engineTime(const engineTime&) = delete;
    //- Disallow default bitwise assignment
    engineTime& operator=(const engineTime&) = delete;
  //- Destructor
  virtual ~engineTime()
  {}
  // Member Functions
    // Conversion
      //- Convert degrees to seconds (for given engine speed in RPM)
      scalar degToTime(const scalar theta) const;
      //- Convert seconds to degrees (for given engine speed in RPM)
      scalar timeToDeg(const scalar t) const;
      //- Calculate the piston position from the engine geometry
      //  and given crank angle.
      scalar pistonPosition(const scalar theta) const;
    // Access
      //- Return the engine geometry dictionary
      const IOdictionary& engineDict() const
      {
        return dict_;
      }
      //- Return the engines current operating RPM
      const dimensionedScalar& rpm() const
      {
        return rpm_;
      }
      //- Return the engines connecting-rod length
      const dimensionedScalar& conRodLength() const
      {
        return conRodLength_;
      }
      //- Return the engines bore
      const dimensionedScalar& bore() const
      {
        return bore_;
      }
      //- Return the engines stroke
      const dimensionedScalar& stroke() const
      {
        return stroke_;
      }
      //- Return the engines clearance-gap
      const dimensionedScalar& clearance() const
      {
        return clearance_;
      }
      //- Return current crank-angle
      scalar theta() const;
      //- Return current crank-angle translated to a single revolution
      //  (value between -180 and 180 with 0 = top dead centre)
      scalar thetaRevolution() const;
      //- Return crank-angle increment
      scalar deltaTheta() const;
      //- Return current piston position
      dimensionedScalar pistonPosition() const;
      //- Return piston displacement for current time step
      dimensionedScalar pistonDisplacement() const;
      //- Return piston speed for current time step
      dimensionedScalar pistonSpeed() const;
    // Member functions overriding the virtual functions in time
      //- Convert the user-time (CA deg) to real-time (s).
      virtual scalar userTimeToTime(const scalar theta) const;
      //- Convert the real-time (s) into user-time (CA deg)
      virtual scalar timeToUserTime(const scalar t) const;
      //- Read the control dictionary and set the write controls etc.
      virtual void readDict();
    // Edit
      //- Read the controlDict and set all the parameters
      virtual bool read();
};
}  // namespace mousse
#endif
