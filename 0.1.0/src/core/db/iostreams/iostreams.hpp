// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#ifndef iostreams_hpp_
#define iostreams_hpp_

#include "isstream.hpp"
#include "prefix_osstream.hpp"
#include "token.hpp"
#include "char.hpp"
#include "int.hpp"
#include "uint.hpp"
#if defined(darwin64) && defined(__clang__)
#include "ulong.hpp"
#include "long.hpp"
#endif

// Global predefined streams for standard input, output
namespace mousse
{
  extern ISstream Sin;
  extern OSstream Sout;
  extern OSstream Serr;
  extern prefixOSstream Pout;
  extern prefixOSstream Perr;
}
#endif
