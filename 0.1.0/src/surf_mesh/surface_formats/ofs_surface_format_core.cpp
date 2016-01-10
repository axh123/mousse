// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "ofs_surface_format_core.hpp"
#include "clock.hpp"
#include "ioobject.hpp"

// Private Member Functions
void mousse::fileFormats::OFSsurfaceFormatCore::writeHeader
(
  Ostream& os,
  const pointField& pointLst,
  const UList<surfZone>& zoneLst
)
{
  // just emit some information until we get a nice IOobject
  IOobject::writeBanner(os)
    << "// OpenFOAM Surface Format - written "
    << clock::dateTime().c_str() << nl
    << "// ~~~~~~~~~~~~~~~~~~~~~~~" << nl << nl
    << "// surfZones:" << nl;
  // treat a single zone as being unzoned
  if (zoneLst.size() <= 1)
  {
    os  << "0" << token::BEGIN_LIST << token::END_LIST << nl << nl;
  }
  else
  {
    os  << zoneLst.size() << nl << token::BEGIN_LIST << incrIndent << nl;
    FOR_ALL(zoneLst, zoneI)
    {
      zoneLst[zoneI].writeDict(os);
    }
    os  << decrIndent << token::END_LIST << nl << nl;
  }
  // Note: write with global point numbering
  IOobject::writeDivider(os)
    << "\n// points:" << nl << pointLst << nl;
  IOobject::writeDivider(os);
}
