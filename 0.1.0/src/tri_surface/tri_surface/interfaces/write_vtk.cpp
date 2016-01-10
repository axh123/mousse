// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "tri_surface.hpp"
namespace mousse
{
// Member Functions 
void triSurface::writeVTK(const bool writeSorted, Ostream& os) const
{
  // Write header
  os  << "# vtk DataFile Version 2.0" << nl
    << "triSurface" << nl
    << "ASCII" << nl
    << "DATASET POLYDATA"
    << nl;
  const pointField& ps = points();
  os  << "POINTS " << ps.size() << " float" << nl;
  // Write vertex coords
  FOR_ALL(ps, pointi)
  {
    if (pointi > 0 && (pointi % 10) == 0)
    {
      os  << nl;
    }
    else
    {
      os  << ' ';
    }
    os  << ps[pointi].x() << ' '
      << ps[pointi].y() << ' '
      << ps[pointi].z();
  }
  os  << nl;
  os  << "POLYGONS " << size() << ' ' << 4*size() << nl;
  labelList faceMap;
  surfacePatchList myPatches(calcPatches(faceMap));
  if (writeSorted)
  {
    label faceIndex = 0;
    FOR_ALL(myPatches, patchI)
    {
      // Print all faces belonging to this patch
      for
      (
        label patchFaceI = 0;
        patchFaceI < myPatches[patchI].size();
        patchFaceI++
      )
      {
        if (faceIndex > 0 && (faceIndex % 10) == 0)
        {
          os  << nl;
        }
        else
        {
          os  << ' ';
        }
        const label faceI = faceMap[faceIndex++];
        os  << "3 "
          << operator[](faceI)[0] << ' '
          << operator[](faceI)[1] << ' '
          << operator[](faceI)[2];
      }
    }
    os  << nl;
    // Print region numbers
    os  << "CELL_DATA " << size() << nl;
    os  << "FIELD attributes 1" << nl;
    os  << "region 1 " << size() << " float" << nl;
    faceIndex = 0;
    FOR_ALL(myPatches, patchI)
    {
      for
      (
        label patchFaceI = 0;
        patchFaceI < myPatches[patchI].size();
        patchFaceI++
      )
      {
        if (faceIndex > 0 && (faceIndex % 10) == 0)
        {
          os  << nl;
        }
        else
        {
          os  << ' ';
        }
        const label faceI = faceMap[faceIndex++];
        os  << operator[](faceI).region();
      }
    }
    os  << nl;
  }
  else
  {
    FOR_ALL(*this, faceI)
    {
      if (faceI > 0 && (faceI % 10) == 0)
      {
        os  << nl;
      }
      else
      {
        os  << ' ';
      }
      os  << "3 "
        << operator[](faceI)[0] << ' '
        << operator[](faceI)[1] << ' '
        << operator[](faceI)[2];
    }
    os  << nl;
    os  << "CELL_DATA " << size() << nl;
    os  << "FIELD attributes 1" << nl;
    os  << "region 1 " << size() << " float" << nl;
    FOR_ALL(*this, faceI)
    {
      if (faceI > 0 && (faceI % 10) == 0)
      {
        os  << nl;
      }
      else
      {
        os  << ' ';
      }
      os  << operator[](faceI).region();
    }
    os  << nl;
  }
}
}  // namespace mousse
