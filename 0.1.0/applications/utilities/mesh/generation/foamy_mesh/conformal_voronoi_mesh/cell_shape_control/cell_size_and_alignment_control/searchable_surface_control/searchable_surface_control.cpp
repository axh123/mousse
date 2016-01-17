// mousse: CFD toolbox
// Copyright (C) 2012-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "searchable_surface_control.hpp"
#include "add_to_run_time_selection_table.hpp"
#include "cell_size_function.hpp"
#include "tri_surface_mesh.hpp"
#include "searchable_box.hpp"
#include "tetrahedron.hpp"
#include "vector_tools.hpp"
#include "quaternion.hpp"
// Static Data Members
namespace mousse
{
defineTypeNameAndDebug(searchableSurfaceControl, 0);
addToRunTimeSelectionTable
(
  cellSizeAndAlignmentControl,
  searchableSurfaceControl,
  dictionary
);
}
// Private Member Functions 
//mousse::tensor mousse::surfaceControl::requiredAlignment
//(
//    const mousse::point& pt,
//    const vectorField& ptNormals
//) const
//{
////    pointIndexHit surfHit;
////    label hitSurface;
////
////    geometryToConformTo_.findSurfaceNearest
////    (
////        pt,
////        sqr(GREAT),
////        surfHit,
////        hitSurface
////    );
////
////    if (!surfHit.hit())
////    {
////        FatalErrorIn
////        (
////            "mousse::tensor mousse::conformalVoronoiMesh::requiredAlignment"
////        )
////            << "findSurfaceNearest did not find a hit across the surfaces."
////            << exit(FatalError) << endl;
////    }
////
//////     Primary alignment
////
////    vectorField norm(1);
////
////    allGeometry_[hitSurface].getNormal
////    (
////        List<pointIndexHit>(1, surfHit),
////        norm
////    );
////
////    const vector np = norm[0];
////
////    const tensor Rp = rotationTensor(vector(0,0,1), np);
////
////    return (Rp);
//
////    Info<< "Point : " << pt << endl;
////    forAll(ptNormals, pnI)
////    {
////        Info<< "    normal " << pnI << " : " << ptNormals[pnI] << endl;
////    }
//
//    vector np = ptNormals[0];
//
//    const tensor Rp = rotationTensor(vector(0,0,1), np);
//
//    vector na = vector::zero;
//
//    scalar smallestAngle = GREAT;
//
//    for (label pnI = 1; pnI < ptNormals.size(); ++pnI)
//    {
//        const vector& nextNormal = ptNormals[pnI];
//
//        const scalar cosPhi = vectorTools::cosPhi(np, nextNormal);
//
//        if (mag(cosPhi) < smallestAngle)
//        {
//            na = nextNormal;
//            smallestAngle = cosPhi;
//        }
//    }
//
//    // Secondary alignment
//    vector ns = np ^ na;
//
//    if (mag(ns) < SMALL)
//    {
//        WarningIn("conformalVoronoiMesh::requiredAlignment")
//            << "Parallel normals detected in spoke search." << nl
//            << "point: " << pt << nl
//            << "np   : " << np << nl
//            << "na   : " << na << nl
//            << "ns   : " << ns << nl
//            << endl;
//
//        ns = np;
//    }
//
//    ns /= mag(ns);
//
//    tensor Rs = rotationTensor((Rp & vector(0,1,0)), ns);
//
////    Info<< "Point " << pt << nl
////        << "      np : " << np << nl
////        << "      ns : " << ns << nl
////        << "      Rp : " << Rp << nl
////        << "      Rs : " << Rs << nl
////        << "    Rs&Rp: " << (Rs & Rp) << endl;
//
//    return (Rs & Rp);
//}
// Constructors 
mousse::searchableSurfaceControl::searchableSurfaceControl
(
  const Time& runTime,
  const word& name,
  const dictionary& controlFunctionDict,
  const conformationSurfaces& geometryToConformTo,
  const scalar& defaultCellSize
)
:
  cellSizeAndAlignmentControl
  (
    runTime,
    name,
    controlFunctionDict,
    geometryToConformTo,
    defaultCellSize
  ),
  surfaceName_(controlFunctionDict.lookupOrDefault<word>("surface", name)),
  searchableSurface_(geometryToConformTo.geometry()[surfaceName_]),
  geometryToConformTo_(geometryToConformTo),
  cellSizeFunctions_(1),
  regionToCellSizeFunctions_(searchableSurface_.regions().size(), -1),
  maxPriority_(-1)
{
  Info<< indent << "Master settings:" << endl;
  Info<< incrIndent;
  cellSizeFunctions_.set
  (
    0,
    cellSizeFunction::New
    (
      controlFunctionDict,
      searchableSurface_,
      defaultCellSize_,
      labelList()
    )
  );
  Info<< decrIndent;
  PtrList<cellSizeFunction> regionCellSizeFunctions;
  DynamicList<label> defaultCellSizeRegions;
  label nRegionCellSizeFunctions = 0;
  // Loop over regions - if any entry is not specified they should
  // inherit values from the parent surface.
  if (controlFunctionDict.found("regions"))
  {
    const dictionary& regionsDict = controlFunctionDict.subDict("regions");
    const wordList& regionNames = searchableSurface_.regions();
    label nRegions = regionsDict.size();
    regionCellSizeFunctions.setSize(nRegions);
    defaultCellSizeRegions.setCapacity(nRegions);
    forAll(regionNames, regionI)
    {
      const word& regionName = regionNames[regionI];
      label regionID = geometryToConformTo_.geometry().findSurfaceRegionID
      (
        this->name(),
        regionName
      );
      if (regionsDict.found(regionName))
      {
        // Get the dictionary for region
        const dictionary& regionDict = regionsDict.subDict(regionName);
        Info<< indent << "Region " << regionName
          << " (ID = " << regionID << ")" << " settings:"
          << endl;
        Info<< incrIndent;
        regionCellSizeFunctions.set
        (
          nRegionCellSizeFunctions,
          cellSizeFunction::New
          (
            regionDict,
            searchableSurface_,
            defaultCellSize_,
            labelList(1, regionID)
          )
        );
        Info<< decrIndent;
        regionToCellSizeFunctions_[regionID] = nRegionCellSizeFunctions;
        nRegionCellSizeFunctions++;
      }
      else
      {
        // Add to default list
        defaultCellSizeRegions.append(regionID);
      }
    }
  }
  if (defaultCellSizeRegions.empty() && !regionCellSizeFunctions.empty())
  {
    cellSizeFunctions_.transfer(regionCellSizeFunctions);
  }
  else if (nRegionCellSizeFunctions > 0)
  {
    regionCellSizeFunctions.setSize(nRegionCellSizeFunctions + 1);
    regionCellSizeFunctions.set
    (
      nRegionCellSizeFunctions,
      cellSizeFunction::New
      (
        controlFunctionDict,
        searchableSurface_,
        defaultCellSize_,
        labelList()
      )
    );
    const wordList& regionNames = searchableSurface_.regions();
    forAll(regionNames, regionI)
    {
      if (regionToCellSizeFunctions_[regionI] == -1)
      {
        regionToCellSizeFunctions_[regionI] = nRegionCellSizeFunctions;
      }
    }
    cellSizeFunctions_.transfer(regionCellSizeFunctions);
  }
  else
  {
    const wordList& regionNames = searchableSurface_.regions();
    forAll(regionNames, regionI)
    {
      if (regionToCellSizeFunctions_[regionI] == -1)
      {
        regionToCellSizeFunctions_[regionI] = 0;
      }
    }
  }
  forAll(cellSizeFunctions_, funcI)
  {
    const label funcPriority = cellSizeFunctions_[funcI].priority();
    if (funcPriority > maxPriority_)
    {
      maxPriority_ = funcPriority;
    }
  }
  // Sort controlFunctions_ by maxPriority
  SortableList<label> functionPriorities(cellSizeFunctions_.size());
  forAll(cellSizeFunctions_, funcI)
  {
    functionPriorities[funcI] = cellSizeFunctions_[funcI].priority();
  }
  functionPriorities.reverseSort();
  labelList invertedFunctionPriorities =
    invert(functionPriorities.size(), functionPriorities.indices());
  cellSizeFunctions_.reorder(invertedFunctionPriorities);
  Info<< nl << indent << "There are " << cellSizeFunctions_.size()
    << " region control functions" << endl;
}
// Destructor 
mousse::searchableSurfaceControl::~searchableSurfaceControl()
{}
// Member Functions 
void mousse::searchableSurfaceControl::initialVertices
(
  pointField& pts,
  scalarField& sizes,
  triadField& alignments
) const
{
  pts = searchableSurface_.points();
  sizes.setSize(pts.size());
  alignments.setSize(pts.size());
  const scalar nearFeatDistSqrCoeff = 1e-8;
  forAll(pts, pI)
  {
    // Is the point in the extendedFeatureEdgeMesh? If so get the
    // point normal, otherwise get the surface normal from
    // searchableSurface
    pointIndexHit info;
    label infoFeature;
    geometryToConformTo_.findFeaturePointNearest
    (
      pts[pI],
      nearFeatDistSqrCoeff,
      info,
      infoFeature
    );
    scalar limitedCellSize = GREAT;
    autoPtr<triad> pointAlignment;
    if (info.hit())
    {
      const extendedFeatureEdgeMesh& features =
        geometryToConformTo_.features()[infoFeature];
      vectorField norms = features.featurePointNormals(info.index());
      // Create a triad from these norms.
      pointAlignment.set(new triad());
      forAll(norms, nI)
      {
        pointAlignment() += norms[nI];
      }
      pointAlignment().normalize();
      pointAlignment().orthogonalize();
    }
    else
    {
      geometryToConformTo_.findEdgeNearest
      (
        pts[pI],
        nearFeatDistSqrCoeff,
        info,
        infoFeature
      );
      if (info.hit())
      {
        const extendedFeatureEdgeMesh& features =
          geometryToConformTo_.features()[infoFeature];
        vectorField norms = features.edgeNormals(info.index());
        // Create a triad from these norms.
        pointAlignment.set(new triad());
        forAll(norms, nI)
        {
          pointAlignment() += norms[nI];
        }
        pointAlignment().normalize();
        pointAlignment().orthogonalize();
      }
      else
      {
        pointField ptField(1, pts[pI]);
        scalarField distField(1, nearFeatDistSqrCoeff);
        List<pointIndexHit> infoList(1, pointIndexHit());
        searchableSurface_.findNearest(ptField, distField, infoList);
        vectorField normals(1);
        searchableSurface_.getNormal(infoList, normals);
        if (mag(normals[0]) < SMALL)
        {
          normals[0] = vector(1, 1, 1);
        }
        pointAlignment.set(new triad(normals[0]));
        if (infoList[0].hit())
        {
          // Limit cell size
          const vector vN =
            infoList[0].hitPoint()
           - 2.0*normals[0]*defaultCellSize_;
          List<pointIndexHit> intersectionList;
          searchableSurface_.findLineAny
          (
            ptField,
            pointField(1, vN),
            intersectionList
          );
        }
//                if (intersectionList[0].hit())
//                {
//                    scalar dist =
//                        mag(intersectionList[0].hitPoint() - pts[pI]);
//
//                    limitedCellSize = dist/2.0;
//                }
      }
    }
    label priority = -1;
    if (!cellSize(pts[pI], sizes[pI], priority))
    {
      sizes[pI] = defaultCellSize_;
//            FatalErrorIn
//            (
//                "mousse::searchableSurfaceControl::initialVertices"
//                "(pointField&, scalarField&, tensorField&)"
//            )   << "Could not calculate cell size"
//                << abort(FatalError);
    }
    sizes[pI] = min(limitedCellSize, sizes[pI]);
    alignments[pI] = pointAlignment();
  }
}
void mousse::searchableSurfaceControl::cellSizeFunctionVertices
(
  DynamicList<mousse::point>& pts,
  DynamicList<scalar>& sizes
) const
{
  const tmp<pointField> tmpPoints = searchableSurface_.points();
  const pointField& points = tmpPoints();
  const scalar nearFeatDistSqrCoeff = 1e-8;
  pointField ptField(1, vector::min);
  scalarField distField(1, nearFeatDistSqrCoeff);
  List<pointIndexHit> infoList(1, pointIndexHit());
  vectorField normals(1);
  labelList region(1, label(-1));
  forAll(points, pI)
  {
    // Is the point in the extendedFeatureEdgeMesh? If so get the
    // point normal, otherwise get the surface normal from
    // searchableSurface
    ptField[0] = points[pI];
    searchableSurface_.findNearest(ptField, distField, infoList);
    if (infoList[0].hit())
    {
      searchableSurface_.getNormal(infoList, normals);
      searchableSurface_.getRegion(infoList, region);
      const cellSizeFunction& sizeFunc =
        sizeFunctions()[regionToCellSizeFunctions_[region[0]]];
      pointField extraPts;
      scalarField extraSizes;
      sizeFunc.sizeLocations
      (
        infoList[0],
        normals[0],
        extraPts,
        extraSizes
      );
      pts.append(extraPts);
      sizes.append(extraSizes);
    }
  }
}
bool mousse::searchableSurfaceControl::cellSize
(
  const mousse::point& pt,
  scalar& cellSize,
  label& priority
) const
{
  bool anyFunctionFound = false;
  forAll(sizeFunctions(), funcI)
  {
    const cellSizeFunction& sizeFunc = sizeFunctions()[funcI];
    if (sizeFunc.priority() < priority)
    {
      continue;
    }
    scalar sizeI = -1;
    if (sizeFunc.cellSize(pt, sizeI))
    {
      anyFunctionFound = true;
      if (sizeFunc.priority() == priority)
      {
        if (sizeI < cellSize)
        {
          cellSize = sizeI;
        }
      }
      else
      {
        cellSize = sizeI;
        priority = sizeFunc.priority();
      }
      if (debug)
      {
        Info<< "    sizeI " << sizeI
          <<"    minSize " << cellSize << endl;
      }
    }
  }
  return anyFunctionFound;
}
