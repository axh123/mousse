// mousse: CFD toolbox
// Copyright (C) 2011-2014 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "refinement_surfaces.hpp"
#include "time.hpp"
#include "searchable_surfaces.hpp"
#include "shell_surfaces.hpp"
#include "tri_surface_mesh.hpp"
#include "label_pair.hpp"
#include "searchable_surfaces_queries.hpp"
#include "uptr_list.hpp"
#include "volume_type.hpp"


// Constructors 
mousse::refinementSurfaces::refinementSurfaces
(
  const searchableSurfaces& allGeometry,
  const dictionary& surfacesDict,
  const label gapLevelIncrement
)
:
  allGeometry_{allGeometry},
  surfaces_{surfacesDict.size()},
  names_{surfacesDict.size()},
  surfZones_{surfacesDict.size()},
  regionOffset_{surfacesDict.size()}
{
  // Wildcard specification : loop over all surface, all regions
  // and try to find a match.
  // Count number of surfaces.
  label surfI = 0;
  FOR_ALL(allGeometry_.names(), geomI) {
    const word& geomName = allGeometry_.names()[geomI];
    if (surfacesDict.found(geomName)) {
      surfI++;
    }
  }
  // Size lists
  surfaces_.setSize(surfI);
  names_.setSize(surfI);
  surfZones_.setSize(surfI);
  regionOffset_.setSize(surfI);
  labelList globalMinLevel{surfI, 0};
  labelList globalMaxLevel{surfI, 0};
  labelList globalLevelIncr{surfI, 0};
  scalarField globalAngle{surfI, -GREAT};
  PtrList<dictionary> globalPatchInfo{surfI};
  List<Map<label>> regionMinLevel{surfI};
  List<Map<label>> regionMaxLevel{surfI};
  List<Map<label>> regionLevelIncr{surfI};
  List<Map<scalar>> regionAngle{surfI};
  List<Map<autoPtr<dictionary>>> regionPatchInfo{surfI};
  HashSet<word> unmatchedKeys{surfacesDict.toc()};
  surfI = 0;
  FOR_ALL(allGeometry_.names(), geomI) {
    const word& geomName = allGeometry_.names()[geomI];
    const entry* ePtr = surfacesDict.lookupEntryPtr(geomName, false, true);
    if (!ePtr)
      continue;
    const dictionary& dict = ePtr->dict();
    unmatchedKeys.erase(ePtr->keyword());
    names_[surfI] = geomName;
    surfaces_[surfI] = geomI;
    const labelPair refLevel(dict.lookup("level"));
    globalMinLevel[surfI] = refLevel[0];
    globalMaxLevel[surfI] = refLevel[1];
    globalLevelIncr[surfI] =
      dict.lookupOrDefault("gapLevelIncrement", gapLevelIncrement);
    if (globalMinLevel[surfI] < 0
        || globalMaxLevel[surfI] < globalMinLevel[surfI]
        || globalLevelIncr[surfI] < 0) {
      FATAL_IO_ERROR_IN
      (
        "refinementSurfaces::refinementSurfaces"
        "(const searchableSurfaces&, const dictionary>&",
        dict
      )
      << "Illegal level specification for surface "
      << names_[surfI]
      << " : minLevel:" << globalMinLevel[surfI]
      << " maxLevel:" << globalMaxLevel[surfI]
      << " levelIncrement:" << globalLevelIncr[surfI]
      << exit(FatalIOError);
    }
    const searchableSurface& surface = allGeometry_[surfaces_[surfI]];
    // Surface zones
    surfZones_.set(surfI, new surfaceZonesInfo{surface, dict});
    // Global perpendicular angle
    if (dict.found("patchInfo")) {
      globalPatchInfo.set
        (
          surfI,
          dict.subDict("patchInfo").clone()
        );
    }
    dict.readIfPresent("perpendicularAngle", globalAngle[surfI]);
    if (dict.found("regions")) {
      const dictionary& regionsDict = dict.subDict("regions");
      const wordList& regionNames = surface.regions();
      FOR_ALL(regionNames, regionI) {
        if (regionsDict.found(regionNames[regionI])) {
          // Get the dictionary for region
          const dictionary& regionDict =
            regionsDict.subDict(regionNames[regionI]);
          const labelPair refLevel{regionDict.lookup("level")};
          regionMinLevel[surfI].insert(regionI, refLevel[0]);
          regionMaxLevel[surfI].insert(regionI, refLevel[1]);
          label levelIncr =
            regionDict.lookupOrDefault("gapLevelIncrement", gapLevelIncrement);
          regionLevelIncr[surfI].insert(regionI, levelIncr);
          if (refLevel[0] < 0 || refLevel[1] < refLevel[0] || levelIncr < 0) {
            FATAL_IO_ERROR_IN
            (
              "refinementSurfaces::refinementSurfaces"
              "(const searchableSurfaces&, const dictionary&",
              dict
            )
            << "Illegal level specification for surface "
            << names_[surfI] << " region "
            << regionNames[regionI]
            << " : minLevel:" << refLevel[0]
            << " maxLevel:" << refLevel[1]
            << " levelIncrement:" << levelIncr
            << exit(FatalIOError);
          }
          if (regionDict.found("perpendicularAngle")) {
            regionAngle[surfI].insert
            (
              regionI,
              readScalar(regionDict.lookup("perpendicularAngle"))
            );
          }
          if (regionDict.found("patchInfo")) {
            regionPatchInfo[surfI].insert
            (
              regionI,
              regionDict.subDict("patchInfo").clone()
            );
          }
        }
      }
    }
    surfI++;
  }
  if (unmatchedKeys.size() > 0) {
    IO_WARNING_IN
    (
      "refinementSurfaces::refinementSurfaces(..)",
      surfacesDict
    )
    << "Not all entries in refinementSurfaces dictionary were used."
    << " The following entries were not used : "
    << unmatchedKeys.sortedToc()
    << endl;
  }
  // Calculate local to global region offset
  label nRegions = 0;
  FOR_ALL(surfaces_, surfI) {
    regionOffset_[surfI] = nRegions;
    nRegions += allGeometry_[surfaces_[surfI]].regions().size();
  }
  // Rework surface specific information into information per global region
  minLevel_.setSize(nRegions);
  minLevel_ = 0;
  maxLevel_.setSize(nRegions);
  maxLevel_ = 0;
  gapLevel_.setSize(nRegions);
  gapLevel_ = -1;
  perpendicularAngle_.setSize(nRegions);
  perpendicularAngle_ = -GREAT;
  patchInfo_.setSize(nRegions);
  FOR_ALL(globalMinLevel, surfI) {
    label nRegions = allGeometry_[surfaces_[surfI]].regions().size();
    // Initialise to global (i.e. per surface)
    for (label i = 0; i < nRegions; i++) {
      label globalRegionI = regionOffset_[surfI] + i;
      minLevel_[globalRegionI] = globalMinLevel[surfI];
      maxLevel_[globalRegionI] = globalMaxLevel[surfI];
      gapLevel_[globalRegionI] =
        maxLevel_[globalRegionI] + globalLevelIncr[surfI];
      perpendicularAngle_[globalRegionI] = globalAngle[surfI];
      if (globalPatchInfo.set(surfI)) {
        patchInfo_.set
        (
          globalRegionI,
          globalPatchInfo[surfI].clone()
        );
      }
    }
    // Overwrite with region specific information
    FOR_ALL_CONST_ITER(Map<label>, regionMinLevel[surfI], iter) {
      label globalRegionI = regionOffset_[surfI] + iter.key();
      minLevel_[globalRegionI] = iter();
      maxLevel_[globalRegionI] = regionMaxLevel[surfI][iter.key()];
      gapLevel_[globalRegionI] =
        maxLevel_[globalRegionI] + regionLevelIncr[surfI][iter.key()];
    }
    FOR_ALL_CONST_ITER(Map<scalar>, regionAngle[surfI], iter) {
      label globalRegionI = regionOffset_[surfI] + iter.key();
      perpendicularAngle_[globalRegionI] = regionAngle[surfI][iter.key()];
    }
    const Map<autoPtr<dictionary>>& localInfo = regionPatchInfo[surfI];
    FOR_ALL_CONST_ITER(Map<autoPtr<dictionary>>, localInfo, iter)
    {
      label globalRegionI = regionOffset_[surfI] + iter.key();
      patchInfo_.set(globalRegionI, iter()().clone());
    }
  }
}


mousse::refinementSurfaces::refinementSurfaces
(
  const searchableSurfaces& allGeometry,
  const labelList& surfaces,
  const wordList& names,
  const PtrList<surfaceZonesInfo>& surfZones,
  const labelList& regionOffset,
  const labelList& minLevel,
  const labelList& maxLevel,
  const labelList& gapLevel,
  const scalarField& perpendicularAngle,
  PtrList<dictionary>& patchInfo
)
:
  allGeometry_{allGeometry},
  surfaces_{surfaces},
  names_{names},
  surfZones_{surfZones},
  regionOffset_{regionOffset},
  minLevel_{minLevel},
  maxLevel_{maxLevel},
  gapLevel_{gapLevel},
  perpendicularAngle_{perpendicularAngle},
  patchInfo_{patchInfo.size()}
{
  FOR_ALL(patchInfo_, pI) {
    if (patchInfo.set(pI)) {
      patchInfo_.set(pI, patchInfo.set(pI, NULL));
    }
  }
}


// Member Functions 

// Precalculate the refinement level for every element of the searchable
// surface.
void mousse::refinementSurfaces::setMinLevelFields
(
  const shellSurfaces& shells
)
{
  FOR_ALL(surfaces_, surfI) {
    const searchableSurface& geom = allGeometry_[surfaces_[surfI]];
    // Precalculation only makes sense if there are different regions
    // (so different refinement levels possible) and there are some
    // elements. Possibly should have 'enough' elements to have fine
    // enough resolution but for now just make sure we don't catch e.g.
    // searchableBox (size=6)
    if (geom.regions().size() <= 1 || geom.globalSize() <= 10)
      continue;
    // Representative local coordinates and bounding sphere
    pointField ctrs;
    scalarField radiusSqr;
    geom.boundingSpheres(ctrs, radiusSqr);
    labelList minLevelField{ctrs.size(), -1};

    {
      // Get the element index in a roundabout way. Problem is e.g.
      // distributed surface where local indices differ from global
      // ones (needed for getRegion call)
      List<pointIndexHit> info;
      geom.findNearest(ctrs, radiusSqr, info);
      // Get per element the region
      labelList region;
      geom.getRegion(info, region);
      // From the region get the surface-wise refinement level
      FOR_ALL(minLevelField, i) {
        if (info[i].hit()) {  //Note: should not be necessary
          minLevelField[i] = minLevel(surfI, region[i]);
        }
      }
    }
    // Find out if triangle inside shell with higher level
    // What level does shell want to refine fc to?
    labelList shellLevel;
    shells.findHigherLevel(ctrs, minLevelField, shellLevel);
    FOR_ALL(minLevelField, i) {
      minLevelField[i] = max(minLevelField[i], shellLevel[i]);
    }
    // Store minLevelField on surface
    const_cast<searchableSurface&>(geom).setField(minLevelField);
  }
}


// Find intersections of edge. Return -1 or first surface with higher minLevel
// number.
void mousse::refinementSurfaces::findHigherIntersection
(
  const pointField& start,
  const pointField& end,
  const labelList& currentLevel,   // current cell refinement level
  labelList& surfaces,
  labelList& surfaceLevel
) const
{
  surfaces.setSize(start.size());
  surfaces = -1;
  surfaceLevel.setSize(start.size());
  surfaceLevel = -1;
  if (surfaces_.empty()) {
    return;
  }
  if (surfaces_.size() == 1) {
    // Optimisation: single segmented surface. No need to duplicate
    // point storage.
    label surfI = 0;
    const searchableSurface& geom = allGeometry_[surfaces_[surfI]];
    // Do intersection test
    List<pointIndexHit> intersectionInfo{start.size()};
    geom.findLineAny(start, end, intersectionInfo);
    // See if a cached level field available
    labelList minLevelField;
    geom.getField(intersectionInfo, minLevelField);
    bool haveLevelField =
      (returnReduce(minLevelField.size(), sumOp<label>()) > 0);
    if (!haveLevelField && geom.regions().size() == 1) {
      minLevelField =
        labelList
        {
          intersectionInfo.size(),
          minLevel(surfI, 0)
        };
      haveLevelField = true;
    }
    if (haveLevelField) {
      FOR_ALL(intersectionInfo, i) {
        if (intersectionInfo[i].hit() && minLevelField[i] > currentLevel[i]) {
          surfaces[i] = surfI;    // index of surface
          surfaceLevel[i] = minLevelField[i];
        }
      }
      return;
    }
  }
  // Work arrays
  pointField p0{start};
  pointField p1{end};
  labelList intersectionToPoint{identity(start.size())};
  List<pointIndexHit> intersectionInfo{start.size()};
  FOR_ALL(surfaces_, surfI) {
    const searchableSurface& geom = allGeometry_[surfaces_[surfI]];
    // Do intersection test
    geom.findLineAny(p0, p1, intersectionInfo);
    // See if a cached level field available
    labelList minLevelField;
    geom.getField(intersectionInfo, minLevelField);
    // Copy all hits into arguments, In-place compact misses.
    label missI = 0;
    FOR_ALL(intersectionInfo, i) {
      // Get the minLevel for the point
      label minLocalLevel = -1;
      if (intersectionInfo[i].hit()) {
        // Check if minLevelField for this surface.
        if (minLevelField.size()) {
          minLocalLevel = minLevelField[i];
        } else {
          // Use the min level for the surface instead. Assume
          // single region 0.
          minLocalLevel = minLevel(surfI, 0);
        }
      }
      label pointI = intersectionToPoint[i];
      if (minLocalLevel > currentLevel[pointI]) {
        // Mark point for refinement
        surfaces[pointI] = surfI;
        surfaceLevel[pointI] = minLocalLevel;
      } else {
        p0[missI] = start[pointI];
        p1[missI] = end[pointI];
        intersectionToPoint[missI] = pointI;
        missI++;
      }
    }
    // All done? Note that this decision should be synchronised
    if (returnReduce(missI, sumOp<label>()) == 0) {
      break;
    }
    // Trim misses
    p0.setSize(missI);
    p1.setSize(missI);
    intersectionToPoint.setSize(missI);
    intersectionInfo.setSize(missI);
  }
}


void mousse::refinementSurfaces::findAllHigherIntersections
(
  const pointField& start,
  const pointField& end,
  const labelList& currentLevel,   // current cell refinement level
  const labelList& globalRegionLevel,
  List<vectorList>& surfaceNormal,
  labelListList& surfaceLevel
) const
{
  surfaceLevel.setSize(start.size());
  surfaceNormal.setSize(start.size());
  if (surfaces_.empty()) {
    return;
  }
  // Work arrays
  List<List<pointIndexHit>> hitInfo;
  labelList pRegions;
  vectorField pNormals;
  FOR_ALL(surfaces_, surfI) {
    const searchableSurface& surface = allGeometry_[surfaces_[surfI]];
    surface.findLineAll(start, end, hitInfo);
    // Repack hits for surface into flat list
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // To avoid overhead of calling getRegion for every point
    label n = 0;
    FOR_ALL(hitInfo, pointI) {
      n += hitInfo[pointI].size();
    }
    List<pointIndexHit> surfInfo{n};
    labelList pointMap{n};
    n = 0;
    FOR_ALL(hitInfo, pointI) {
      const List<pointIndexHit>& pHits = hitInfo[pointI];
      FOR_ALL(pHits, i) {
        surfInfo[n] = pHits[i];
        pointMap[n] = pointI;
        n++;
      }
    }
    labelList surfRegion{n};
    vectorField surfNormal{n};
    surface.getRegion(surfInfo, surfRegion);
    surface.getNormal(surfInfo, surfNormal);
    surfInfo.clear();
    // Extract back into pointwise
    FOR_ALL(surfRegion, i) {
      label region = globalRegion(surfI, surfRegion[i]);
      label pointI = pointMap[i];
      if (globalRegionLevel[region] > currentLevel[pointI]) {
        // Append to pointI info
        label sz = surfaceNormal[pointI].size();
        surfaceNormal[pointI].setSize(sz + 1);
        surfaceNormal[pointI][sz] = surfNormal[i];
        surfaceLevel[pointI].setSize(sz + 1);
        surfaceLevel[pointI][sz] = globalRegionLevel[region];
      }
    }
  }
}


void mousse::refinementSurfaces::findAllHigherIntersections
(
  const pointField& start,
  const pointField& end,
  const labelList& currentLevel,   // current cell refinement level
  const labelList& globalRegionLevel,
  List<pointList>& surfaceLocation,
  List<vectorList>& surfaceNormal,
  labelListList& surfaceLevel
) const
{
  surfaceLevel.setSize(start.size());
  surfaceNormal.setSize(start.size());
  surfaceLocation.setSize(start.size());
  if (surfaces_.empty()) {
    return;
  }
  // Work arrays
  List<List<pointIndexHit>> hitInfo;
  labelList pRegions;
  vectorField pNormals;
  FOR_ALL(surfaces_, surfI) {
    const searchableSurface& surface = allGeometry_[surfaces_[surfI]];
    surface.findLineAll(start, end, hitInfo);
    // Repack hits for surface into flat list
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // To avoid overhead of calling getRegion for every point
    label n = 0;
    FOR_ALL(hitInfo, pointI) {
      n += hitInfo[pointI].size();
    }
    List<pointIndexHit> surfInfo{n};
    labelList pointMap{n};
    n = 0;
    FOR_ALL(hitInfo, pointI) {
      const List<pointIndexHit>& pHits = hitInfo[pointI];
      FOR_ALL(pHits, i) {
        surfInfo[n] = pHits[i];
        pointMap[n] = pointI;
        n++;
      }
    }
    labelList surfRegion{n};
    vectorField surfNormal{n};
    surface.getRegion(surfInfo, surfRegion);
    surface.getNormal(surfInfo, surfNormal);
    // Extract back into pointwise
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~
    FOR_ALL(surfRegion, i) {
      label region = globalRegion(surfI, surfRegion[i]);
      label pointI = pointMap[i];
      if (globalRegionLevel[region] > currentLevel[pointI]) {
        // Append to pointI info
        label sz = surfaceNormal[pointI].size();
        surfaceLocation[pointI].setSize(sz + 1);
        surfaceLocation[pointI][sz] = surfInfo[i].hitPoint();
        surfaceNormal[pointI].setSize(sz + 1);
        surfaceNormal[pointI][sz] = surfNormal[i];
        surfaceLevel[pointI].setSize(sz + 1);
        surfaceLevel[pointI][sz] = globalRegionLevel[region];
      }
    }
  }
}


void mousse::refinementSurfaces::findNearestIntersection
(
  const labelList& surfacesToTest,
  const pointField& start,
  const pointField& end,
  labelList& surface1,
  List<pointIndexHit>& hit1,
  labelList& region1,
  labelList& surface2,
  List<pointIndexHit>& hit2,
  labelList& region2
) const
{
  // 1. intersection from start to end
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Initialize arguments
  surface1.setSize(start.size());
  surface1 = -1;
  hit1.setSize(start.size());
  region1.setSize(start.size());
  // Current end of segment to test.
  pointField nearest{end};
  // Work array
  List<pointIndexHit> nearestInfo{start.size()};
  labelList region;
  FOR_ALL(surfacesToTest, testI) {
    label surfI = surfacesToTest[testI];
    const searchableSurface& surface = allGeometry_[surfaces_[surfI]];
    // See if any intersection between start and current nearest
    surface.findLine
      (
        start,
        nearest,
        nearestInfo
      );
    surface.getRegion
      (
        nearestInfo,
        region
      );
    FOR_ALL(nearestInfo, pointI) {
      if (nearestInfo[pointI].hit()) {
        hit1[pointI] = nearestInfo[pointI];
        surface1[pointI] = surfI;
        region1[pointI] = region[pointI];
        nearest[pointI] = hit1[pointI].hitPoint();
      }
    }
  }
  // 2. intersection from end to last intersection
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Find the nearest intersection from end to start. Note that we initialize
  // to the first intersection (if any).
  surface2 = surface1;
  hit2 = hit1;
  region2 = region1;
  // Set current end of segment to test.
  FOR_ALL(nearest, pointI) {
    if (hit1[pointI].hit()) {
      nearest[pointI] = hit1[pointI].hitPoint();
    } else {
      // Disable testing by setting to end.
      nearest[pointI] = end[pointI];
    }
  }
  FOR_ALL(surfacesToTest, testI) {
    label surfI = surfacesToTest[testI];
    const searchableSurface& surface = allGeometry_[surfaces_[surfI]];
    // See if any intersection between end and current nearest
    surface.findLine
      (
        end,
        nearest,
        nearestInfo
      );
    surface.getRegion
      (
        nearestInfo,
        region
      );
    FOR_ALL(nearestInfo, pointI) {
      if (nearestInfo[pointI].hit()) {
        hit2[pointI] = nearestInfo[pointI];
        surface2[pointI] = surfI;
        region2[pointI] = region[pointI];
        nearest[pointI] = hit2[pointI].hitPoint();
      }
    }
  }
  // Make sure that if hit1 has hit something, hit2 will have at least the
  // same point (due to tolerances it might miss its end point)
  FOR_ALL(hit1, pointI) {
    if (hit1[pointI].hit() && !hit2[pointI].hit()) {
      hit2[pointI] = hit1[pointI];
      surface2[pointI] = surface1[pointI];
      region2[pointI] = region1[pointI];
    }
  }
}


void mousse::refinementSurfaces::findNearestIntersection
(
  const labelList& surfacesToTest,
  const pointField& start,
  const pointField& end,
  labelList& surface1,
  List<pointIndexHit>& hit1,
  labelList& region1,
  vectorField& normal1,
  labelList& surface2,
  List<pointIndexHit>& hit2,
  labelList& region2,
  vectorField& normal2
) const
{
  // 1. intersection from start to end
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Initialize arguments
  surface1.setSize(start.size());
  surface1 = -1;
  hit1.setSize(start.size());
  region1.setSize(start.size());
  region1 = -1;
  normal1.setSize(start.size());
  normal1 = vector::zero;
  // Current end of segment to test.
  pointField nearest{end};
  // Work array
  List<pointIndexHit> nearestInfo{start.size()};
  labelList region;
  vectorField normal;
  FOR_ALL(surfacesToTest, testI) {
    label surfI = surfacesToTest[testI];
    const searchableSurface& geom = allGeometry_[surfaces_[surfI]];
    // See if any intersection between start and current nearest
    geom.findLine(start, nearest, nearestInfo);
    geom.getRegion(nearestInfo, region);
    geom.getNormal(nearestInfo, normal);
    FOR_ALL(nearestInfo, pointI) {
      if (nearestInfo[pointI].hit()) {
        hit1[pointI] = nearestInfo[pointI];
        surface1[pointI] = surfI;
        region1[pointI] = region[pointI];
        normal1[pointI] = normal[pointI];
        nearest[pointI] = hit1[pointI].hitPoint();
      }
    }
  }
  // 2. intersection from end to last intersection
  // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  // Find the nearest intersection from end to start. Note that we initialize
  // to the first intersection (if any).
  surface2 = surface1;
  hit2 = hit1;
  region2 = region1;
  normal2 = normal1;
  // Set current end of segment to test.
  FOR_ALL(nearest, pointI) {
    if (hit1[pointI].hit()) {
      nearest[pointI] = hit1[pointI].hitPoint();
    } else {
      // Disable testing by setting to end.
      nearest[pointI] = end[pointI];
    }
  }
  FOR_ALL(surfacesToTest, testI) {
    label surfI = surfacesToTest[testI];
    const searchableSurface& geom = allGeometry_[surfaces_[surfI]];
    // See if any intersection between end and current nearest
    geom.findLine(end, nearest, nearestInfo);
    geom.getRegion(nearestInfo, region);
    geom.getNormal(nearestInfo, normal);
    FOR_ALL(nearestInfo, pointI) {
      if (nearestInfo[pointI].hit()) {
        hit2[pointI] = nearestInfo[pointI];
        surface2[pointI] = surfI;
        region2[pointI] = region[pointI];
        normal2[pointI] = normal[pointI];
        nearest[pointI] = hit2[pointI].hitPoint();
      }
    }
  }
  // Make sure that if hit1 has hit something, hit2 will have at least the
  // same point (due to tolerances it might miss its end point)
  FOR_ALL(hit1, pointI) {
    if (hit1[pointI].hit() && !hit2[pointI].hit()) {
      hit2[pointI] = hit1[pointI];
      surface2[pointI] = surface1[pointI];
      region2[pointI] = region1[pointI];
      normal2[pointI] = normal1[pointI];
    }
  }
}


void mousse::refinementSurfaces::findAnyIntersection
(
  const pointField& start,
  const pointField& end,
  labelList& hitSurface,
  List<pointIndexHit>& hitInfo
) const
{
  searchableSurfacesQueries::findAnyIntersection
    (
      allGeometry_,
      surfaces_,
      start,
      end,
      hitSurface,
      hitInfo
    );
}


void mousse::refinementSurfaces::findNearest
(
  const labelList& surfacesToTest,
  const pointField& samples,
  const  scalarField& nearestDistSqr,
  labelList& hitSurface,
  List<pointIndexHit>& hitInfo
) const
{
  labelList geometries{UIndirectList<label>{surfaces_, surfacesToTest}};
  // Do the tests. Note that findNearest returns index in geometries.
  searchableSurfacesQueries::findNearest
    (
      allGeometry_,
      geometries,
      samples,
      nearestDistSqr,
      hitSurface,
      hitInfo
    );
  // Rework the hitSurface to be surface (i.e. index into surfaces_)
  FOR_ALL(hitSurface, pointI) {
    if (hitSurface[pointI] != -1) {
      hitSurface[pointI] = surfacesToTest[hitSurface[pointI]];
    }
  }
}


void mousse::refinementSurfaces::findNearestRegion
(
  const labelList& surfacesToTest,
  const pointField& samples,
  const scalarField& nearestDistSqr,
  labelList& hitSurface,
  labelList& hitRegion
) const
{
  labelList geometries{UIndirectList<label>{surfaces_, surfacesToTest}};
  // Do the tests. Note that findNearest returns index in geometries.
  List<pointIndexHit> hitInfo;
  searchableSurfacesQueries::findNearest
    (
      allGeometry_,
      geometries,
      samples,
      nearestDistSqr,
      hitSurface,
      hitInfo
    );
  // Rework the hitSurface to be surface (i.e. index into surfaces_)
  FOR_ALL(hitSurface, pointI) {
    if (hitSurface[pointI] != -1) {
      hitSurface[pointI] = surfacesToTest[hitSurface[pointI]];
    }
  }
  // Collect the region
  hitRegion.setSize(hitSurface.size());
  hitRegion = -1;
  FOR_ALL(surfacesToTest, i) {
    label surfI = surfacesToTest[i];
    // Collect hits for surfI
    const labelList localIndices{findIndices(hitSurface, surfI)};
    List<pointIndexHit> localHits
    {
      UIndirectList<pointIndexHit>{hitInfo, localIndices}
    };
    labelList localRegion;
    allGeometry_[surfaces_[surfI]].getRegion(localHits, localRegion);
    FOR_ALL(localIndices, i) {
      hitRegion[localIndices[i]] = localRegion[i];
    }
  }
}


void mousse::refinementSurfaces::findNearestRegion
(
  const labelList& surfacesToTest,
  const pointField& samples,
  const scalarField& nearestDistSqr,
  labelList& hitSurface,
  List<pointIndexHit>& hitInfo,
  labelList& hitRegion,
  vectorField& hitNormal
) const
{
  labelList geometries{UIndirectList<label>{surfaces_, surfacesToTest}};
  // Do the tests. Note that findNearest returns index in geometries.
  searchableSurfacesQueries::findNearest
    (
      allGeometry_,
      geometries,
      samples,
      nearestDistSqr,
      hitSurface,
      hitInfo
    );
  // Rework the hitSurface to be surface (i.e. index into surfaces_)
  FOR_ALL(hitSurface, pointI) {
    if (hitSurface[pointI] != -1) {
      hitSurface[pointI] = surfacesToTest[hitSurface[pointI]];
    }
  }
  // Collect the region
  hitRegion.setSize(hitSurface.size());
  hitRegion = -1;
  hitNormal.setSize(hitSurface.size());
  hitNormal = vector::zero;
  FOR_ALL(surfacesToTest, i) {
    label surfI = surfacesToTest[i];
    // Collect hits for surfI
    const labelList localIndices{findIndices(hitSurface, surfI)};
    List<pointIndexHit> localHits
    {
      UIndirectList<pointIndexHit>{hitInfo, localIndices}
    };
    // Region
    labelList localRegion;
    allGeometry_[surfaces_[surfI]].getRegion(localHits, localRegion);
    FOR_ALL(localIndices, i) {
      hitRegion[localIndices[i]] = localRegion[i];
    }
    // Normal
    vectorField localNormal;
    allGeometry_[surfaces_[surfI]].getNormal(localHits, localNormal);
    FOR_ALL(localIndices, i) {
      hitNormal[localIndices[i]] = localNormal[i];
    }
  }
}


void mousse::refinementSurfaces::findInside
(
  const labelList& testSurfaces,
  const pointField& pt,
  labelList& insideSurfaces
) const
{
  insideSurfaces.setSize(pt.size());
  insideSurfaces = -1;
  FOR_ALL(testSurfaces, i) {
    label surfI = testSurfaces[i];
    const searchableSurface& surface = allGeometry_[surfaces_[surfI]];
    const surfaceZonesInfo::areaSelectionAlgo selectionMethod =
      surfZones_[surfI].zoneInside();
    if (selectionMethod != surfaceZonesInfo::INSIDE
        && selectionMethod != surfaceZonesInfo::OUTSIDE)
    {
      FATAL_ERROR_IN("refinementSurfaces::findInside(..)")
        << "Trying to use surface "
        << surface.name()
        << " which has non-geometric inside selection method "
        << surfaceZonesInfo::areaSelectionAlgoNames[selectionMethod]
        << exit(FatalError);
    }
    if (surface.hasVolumeType()) {
      List<volumeType> volType;
      surface.getVolumeType(pt, volType);
      FOR_ALL(volType, pointI) {
        if (insideSurfaces[pointI] == -1) {
          if ((volType[pointI] == volumeType::INSIDE
               && selectionMethod == surfaceZonesInfo::INSIDE)
              || (volType[pointI] == volumeType::OUTSIDE
                  && selectionMethod == surfaceZonesInfo::OUTSIDE)) {
            insideSurfaces[pointI] = surfI;
          }
        }
      }
    }
  }
}

