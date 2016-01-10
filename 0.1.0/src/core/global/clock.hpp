// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::clock
// Description
//   Read access to the system clock with formatting.
// SourceFiles
//   clock.cpp
#ifndef clock_hpp_
#define clock_hpp_
#include <ctime>
namespace mousse
{
class string;
class clock
{
  // Private data
    //- Start time in seconds
    time_t startTime_;
    //- Time when clockTimeIncrement() was last called
    mutable time_t lastTime_;
    //- Latest time from either elapsedClockTime() or clockTimeIncrement()
    mutable time_t newTime_;
    //- Names of the months
    static const char *monthNames[];
public:
  // Constructors
    //- Null constructor which stores the start time
    clock();
  // Member Functions
    //- Get the current clock time in seconds
    static time_t getTime();
    //- Return the current wall-clock date as a raw struct
    static const struct tm rawDate();
    //- Return the current wall-clock date/time as a string
    //  format according to ISO-8601 (yyyy-mm-ddThh:mm:ss)
    static string dateTime();
    //- Return the current wall-clock date as a string
    static string date();
    //- Return the current wall-clock time as a string
    static string clockTime();
    //- Returns wall-clock time from clock instantiation
    time_t elapsedClockTime() const;
    //- Returns wall-clock time from last call of clockTimeIncrement()
    time_t clockTimeIncrement() const;
};
}  // namespace mousse
#endif
