// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "fv.hpp"
#include "hash_table.hpp"
#include "linear.hpp"
namespace mousse
{
namespace fv
{
// Constructors 
template<class Type>
convectionScheme<Type>::convectionScheme(const convectionScheme& cs)
:
  refCount(),
  mesh_(cs.mesh_)
{}
// Selectors
template<class Type>
tmp<convectionScheme<Type> > convectionScheme<Type>::New
(
  const fvMesh& mesh,
  const surfaceScalarField& faceFlux,
  Istream& schemeData
)
{
  if (fv::debug)
  {
    Info<< "convectionScheme<Type>::New"
       "(const fvMesh&, const surfaceScalarField&, Istream&) : "
       "constructing convectionScheme<Type>"
      << endl;
  }
  if (schemeData.eof())
  {
    FatalIOErrorIn
    (
      "convectionScheme<Type>::New"
      "(const fvMesh&, const surfaceScalarField&, Istream&)",
      schemeData
    )   << "Convection scheme not specified" << endl << endl
      << "Valid convection schemes are :" << endl
      << IstreamConstructorTablePtr_->sortedToc()
      << exit(FatalIOError);
  }
  const word schemeName(schemeData);
  typename IstreamConstructorTable::iterator cstrIter =
    IstreamConstructorTablePtr_->find(schemeName);
  if (cstrIter == IstreamConstructorTablePtr_->end())
  {
    FatalIOErrorIn
    (
      "convectionScheme<Type>::New"
      "(const fvMesh&, const surfaceScalarField&, Istream&)",
      schemeData
    )   << "Unknown convection scheme " << schemeName << nl << nl
      << "Valid convection schemes are :" << endl
      << IstreamConstructorTablePtr_->sortedToc()
      << exit(FatalIOError);
  }
  return cstrIter()(mesh, faceFlux, schemeData);
}
template<class Type>
tmp<convectionScheme<Type> > convectionScheme<Type>::New
(
  const fvMesh& mesh,
  const typename multivariateSurfaceInterpolationScheme<Type>::
    fieldTable& fields,
  const surfaceScalarField& faceFlux,
  Istream& schemeData
)
{
  if (fv::debug)
  {
    Info<< "convectionScheme<Type>::New"
       "(const fvMesh&, "
       "const typename multivariateSurfaceInterpolationScheme<Type>"
       "::fieldTable&, const surfaceScalarField&, Istream&) : "
       "constructing convectionScheme<Type>"
      << endl;
  }
  if (schemeData.eof())
  {
    FatalIOErrorIn
    (
      "convectionScheme<Type>::New"
       "(const fvMesh&, "
       "const typename multivariateSurfaceInterpolationScheme<Type>"
       "::fieldTable&, const surfaceScalarField&, Istream&)",
      schemeData
    )   << "Convection scheme not specified" << endl << endl
      << "Valid convection schemes are :" << endl
      << MultivariateConstructorTablePtr_->sortedToc()
      << exit(FatalIOError);
  }
  const word schemeName(schemeData);
  typename MultivariateConstructorTable::iterator cstrIter =
    MultivariateConstructorTablePtr_->find(schemeName);
  if (cstrIter == MultivariateConstructorTablePtr_->end())
  {
    FatalIOErrorIn
    (
      "convectionScheme<Type>::New"
      "(const fvMesh&, "
      "const typename multivariateSurfaceInterpolationScheme<Type>"
      "::fieldTable&, const surfaceScalarField&, Istream&)",
      schemeData
    )   << "Unknown convection scheme " << schemeName << nl << nl
      << "Valid convection schemes are :" << endl
      << MultivariateConstructorTablePtr_->sortedToc()
      << exit(FatalIOError);
  }
  return cstrIter()(mesh, fields, faceFlux, schemeData);
}
// Destructor 
template<class Type>
convectionScheme<Type>::~convectionScheme()
{}
// Member Operators 
template<class Type>
void convectionScheme<Type>::operator=(const convectionScheme<Type>& cs)
{
  if (this == &cs)
  {
    FatalErrorIn
    (
      "convectionScheme<Type>::operator=(const convectionScheme<Type>&)"
    )   << "attempted assignment to self"
      << abort(FatalError);
  }
}
}  // namespace fv
}  // namespace mousse
