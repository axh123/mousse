// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "face.hpp"
#include "dynamic_list.hpp"
// Member Functions 
template<unsigned SizeInc, unsigned SizeMult, unsigned SizeDiv>
mousse::label mousse::face::triangles
(
  const pointField& points,
  DynamicList<face, SizeInc, SizeMult, SizeDiv>& triFaces
) const
{
  label triI = triFaces.size();
  label quadI = 0;
  faceList quadFaces;
  // adjust the addressable size (and allocate space if needed)
  triFaces.setSize(triI + nTriangles());
  return split(SPLITTRIANGLE, points, triI, quadI, triFaces, quadFaces);
}
template<class Type>
Type mousse::face::average
(
  const pointField& meshPoints,
  const Field<Type>& fld
) const
{
  // Calculate the average by breaking the face into triangles and
  // area-weighted averaging their averages
  // If the face is a triangle, do a direct calculation
  if (size() == 3)
  {
    return
      (1.0/3.0)
     *(
       fld[operator[](0)]
      + fld[operator[](1)]
      + fld[operator[](2)]
      );
  }
  label nPoints = size();
  point centrePoint = point::zero;
  Type cf = pTraits<Type>::zero;
  for (label pI=0; pI<nPoints; pI++)
  {
    centrePoint += meshPoints[operator[](pI)];
    cf += fld[operator[](pI)];
  }
  centrePoint /= nPoints;
  cf /= nPoints;
  scalar sumA = 0;
  Type sumAf = pTraits<Type>::zero;
  for (label pI=0; pI<nPoints; pI++)
  {
    // Calculate 3*triangle centre field value
    Type ttcf  =
    (
      fld[operator[](pI)]
     + fld[operator[]((pI + 1) % nPoints)]
     + cf
    );
    // Calculate 2*triangle area
    scalar ta = mousse::mag
    (
      (meshPoints[operator[](pI)] - centrePoint)
     ^ (meshPoints[operator[]((pI + 1) % nPoints)] - centrePoint)
    );
    sumA += ta;
    sumAf += ta*ttcf;
  }
  if (sumA > VSMALL)
  {
    return sumAf/(3*sumA);
  }
  else
  {
    return cf;
  }
}
