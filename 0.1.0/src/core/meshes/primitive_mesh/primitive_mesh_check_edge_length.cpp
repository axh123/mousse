// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "primitive_mesh.hpp"
#include "pstream_reduce_ops.hpp"


bool mousse::primitiveMesh::checkEdgeLength
(
  const bool report,
  const scalar reportLenSqr,
  labelHashSet* setPtr
) const
{
  const pointField& points = this->points();
  const faceList& faces = this->faces();
  scalar minLenSqr = sqr(GREAT);
  scalar maxLenSqr = -sqr(GREAT);
  labelHashSet smallEdgeSet{nPoints()/100};
  FOR_ALL(faces, faceI) {
    const face& f = faces[faceI];
    FOR_ALL(f, fp) {
      label fp1 = f.fcIndex(fp);
      scalar magSqrE = magSqr(points[f[fp]] - points[f[fp1]]);
      if (magSqrE < reportLenSqr) {
        smallEdgeSet.insert(f[fp]);
        smallEdgeSet.insert(f[fp1]);
      }
      minLenSqr = min(minLenSqr, magSqrE);
      maxLenSqr = max(maxLenSqr, magSqrE);
    }
  }
  reduce(minLenSqr, minOp<scalar>());
  reduce(maxLenSqr, maxOp<scalar>());
  label nSmall = smallEdgeSet.size();
  reduce(nSmall, sumOp<label>());
  if (setPtr) {
    setPtr->transfer(smallEdgeSet);
  }
  if (nSmall > 0) {
    if (report) {
      Info << "   *Edges too small, min/max edge length = "
        << sqrt(minLenSqr) << " " << sqrt(maxLenSqr)
        << ", number too small: " << nSmall << endl;
    }
    return true;
  } else {
    if (report) {
      Info << "    Min/max edge length = "
        << sqrt(minLenSqr) << " " << sqrt(maxLenSqr)
        << " OK." << endl;
    }
    return false;
  }
}

