// mousse: CFD toolbox
// Copyright (C) 2011-2012 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "ensight_set_writer.hpp"
#include "coord_set.hpp"
#include "ofstream.hpp"
#include "add_to_run_time_selection_table.hpp"
#include "iomanip.hpp"
#include "mousse_version.hpp"


// Constructors 
template<class Type>
mousse::ensightSetWriter<Type>::ensightSetWriter()
:
  writer<Type>{}
{}


// Destructor 
template<class Type>
mousse::ensightSetWriter<Type>::~ensightSetWriter()
{}


// Member Functions 
template<class Type>
mousse::fileName mousse::ensightSetWriter<Type>::getFileName
(
  const coordSet& points,
  const wordList& valueSetNames
) const
{
  return
    this->getBaseName(points, valueSetNames) + ".case";
}


template<class Type>
void mousse::ensightSetWriter<Type>::write
(
  const coordSet& points,
  const wordList& valueSetNames,
  const List<const Field<Type>*>& valueSets,
  Ostream& os
) const
{
  const fileName base{os.name().lessExt()};
  const fileName meshFile{base + ".mesh"};
  // Write .case file
  os << "FORMAT" << nl
    << "type: ensight gold" << nl
    << nl
    << "GEOMETRY" << nl
    << "model:        1     " << meshFile.name().c_str() << nl
    << nl
    << "VARIABLE"
    << nl;
  FOR_ALL(valueSetNames, setI) {
    fileName dataFile{base + ".***." + valueSetNames[setI]};
    os.setf(ios_base::left);
    os << pTraits<Type>::typeName
      << " per node:            1       "
      << setw(15) << valueSetNames[setI]
      << " " << dataFile.name().c_str()
      << nl;
  }
  os << nl
    << "TIME" << nl
    << "time set:                      1" << nl
    << "number of steps:               1" << nl
    << "filename start number:         0" << nl
    << "filename increment:            1" << nl
    << "time values:" << nl
    << "0.00000e+00" << nl;

  // Write .mesh file
  {
    string desc = string("written by mousse-") + mousse::mousse_version;
    OFstream os{meshFile};
    os.setf(ios_base::scientific, ios_base::floatfield);
    os.precision(5);
    os << "EnSight Geometry File" << nl
      << desc.c_str() << nl
      << "node id assign" << nl
      << "element id assign" << nl
      << "part" << nl
      << setw(10) << 1 << nl
      << "internalMesh" << nl
      << "coordinates" << nl
      << setw(10) << points.size() << nl;
    for (direction cmpt = 0; cmpt < vector::nComponents; cmpt++) {
      FOR_ALL(points, pointI) {
        const scalar comp = points[pointI][cmpt];
        if (mag(comp) >= scalar(floatScalarVSMALL)) {
          os  << setw(12) << comp << nl;
        } else {
          os  << setw(12) << scalar(0) << nl;
        }
      }
    }
    os << "point" << nl
      << setw(10) << points.size() << nl;
    FOR_ALL(points, pointI) {
      os  << setw(10) << pointI+1 << nl;
    }
  }

  // Write data files
  FOR_ALL(valueSetNames, setI) {
    fileName dataFile{base + ".000." + valueSetNames[setI]};
    OFstream os{dataFile};
    os.setf(ios_base::scientific, ios_base::floatfield);
    os.precision(5);

    {
      os << pTraits<Type>::typeName << nl
        << "part" << nl
        << setw(10) << 1 << nl
        << "coordinates" << nl;
      for (direction cmpt = 0; cmpt < pTraits<Type>::nComponents; cmpt++) {
        const scalarField fld{valueSets[setI]->component(cmpt)};
        FOR_ALL(fld, i) {
          if (mag(fld[i]) >= scalar(floatScalarVSMALL)) {
            os  << setw(12) << fld[i] << nl;
          } else {
            os  << setw(12) << scalar(0) << nl;
          }
        }
      }
    }
  }
}


template<class Type>
void mousse::ensightSetWriter<Type>::write
(
  const bool writeTracks,
  const PtrList<coordSet>& tracks,
  const wordList& valueSetNames,
  const List<List<Field<Type> > >& valueSets,
  Ostream& os
) const
{
  const fileName base{os.name().lessExt()};
  const fileName meshFile{base + ".mesh"};
  // Write .case file
  os << "FORMAT" << nl
    << "type: ensight gold" << nl
    << nl
    << "GEOMETRY" << nl
    << "model:        1     " << meshFile.name().c_str() << nl
    << nl
    << "VARIABLE"
    << nl;
  FOR_ALL(valueSetNames, setI) {
    fileName dataFile{base + ".***." + valueSetNames[setI]};
    os.setf(ios_base::left);
    os << pTraits<Type>::typeName
      << " per node:            1       "
      << setw(15) << valueSetNames[setI]
      << " " << dataFile.name().c_str()
      << nl;
  }
  os << nl
    << "TIME" << nl
    << "time set:                      1" << nl
    << "number of steps:               1" << nl
    << "filename start number:         0" << nl
    << "filename increment:            1" << nl
    << "time values:" << nl
    << "0.00000e+00" << nl;

  // Write .mesh file
  {
    string desc = string{"written by mousse-"} + mousse::mousse_version;
    OFstream os{meshFile};
    os.setf(ios_base::scientific, ios_base::floatfield);
    os.precision(5);
    os << "EnSight Geometry File" << nl
      << desc.c_str() << nl
      << "node id assign" << nl
      << "element id assign" << nl;
    FOR_ALL(tracks, trackI) {
      const coordSet& points = tracks[trackI];
      os << "part" << nl
        << setw(10) << trackI+1 << nl
        << "internalMesh" << nl
        << "coordinates" << nl
        << setw(10) << points.size() << nl;
      for (direction cmpt = 0; cmpt < vector::nComponents; cmpt++) {
        FOR_ALL(points, pointI) {
          const scalar comp = points[pointI][cmpt];
          if (mag(comp) >= scalar(floatScalarVSMALL)) {
            os  << setw(12) << comp << nl;
          } else {
            os  << setw(12) << scalar(0) << nl;
          }
        }
      }
      if (writeTracks) {
        os << "bar2" << nl
          << setw(10) << points.size()-1 << nl;
        for (label i = 0; i < points.size()-1; i++) {
          os << setw(10) << i+1
            << setw(10) << i+2
            << nl;
        }
      }
    }
  }
  // Write data files
  FOR_ALL(valueSetNames, setI) {
    fileName dataFile{base + ".000." + valueSetNames[setI]};
    OFstream os{dataFile};
    os.setf(ios_base::scientific, ios_base::floatfield);
    os.precision(5);

    {
      os << pTraits<Type>::typeName << nl;
      const List<Field<Type>>& fieldVals = valueSets[setI];
      FOR_ALL(fieldVals, trackI) {
        os << "part" << nl
          << setw(10) << trackI+1 << nl
          << "coordinates" << nl;
        for
        (
          direction cmpt = 0;
          cmpt < pTraits<Type>::nComponents;
          cmpt++
        ) {
          const scalarField fld{fieldVals[trackI].component(cmpt)};
          FOR_ALL(fld, i) {
            if (mag(fld[i]) >= scalar(floatScalarVSMALL)) {
              os  << setw(12) << fld[i] << nl;
            } else {
              os  << setw(12) << scalar(0) << nl;
            }
          }
        }
      }
    }
  }
}

