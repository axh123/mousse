// mousse: CFD toolbox
// Copyright (C) 2011-2013 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "motion_smoother_algo.hpp"
#include "poly_mesh_geometry.hpp"
#include "iomanip.hpp"


// Member Functions 
bool mousse::motionSmootherAlgo::checkMesh
(
  const bool report,
  const polyMesh& mesh,
  const dictionary& dict,
  const labelList& checkFaces,
  labelHashSet& wrongFaces
)
{
  List<labelPair> emptyBaffles;
  return checkMesh(report, mesh, dict, checkFaces, emptyBaffles, wrongFaces);
}


bool mousse::motionSmootherAlgo::checkMesh
(
  const bool report,
  const polyMesh& mesh,
  const dictionary& dict,
  const labelList& checkFaces,
  const List<labelPair>& baffles,
  labelHashSet& wrongFaces
)
{
  const scalar maxNonOrtho{readScalar(dict.lookup("maxNonOrtho", true))};
  const scalar minVol{readScalar(dict.lookup("minVol", true))};
  const scalar minTetQuality{readScalar(dict.lookup("minTetQuality", true))};
  const scalar maxConcave{readScalar(dict.lookup("maxConcave", true))};
  const scalar minArea{readScalar(dict.lookup("minArea", true))};
  const scalar maxIntSkew{readScalar(dict.lookup("maxInternalSkewness", true))};
  const scalar
    maxBounSkew{readScalar(dict.lookup("maxBoundarySkewness", true))};
  const scalar minWeight{readScalar(dict.lookup("minFaceWeight", true))};
  const scalar minVolRatio{readScalar(dict.lookup("minVolRatio", true))};
  const scalar minTwist{readScalar(dict.lookup("minTwist", true))};
  const scalar
    minTriangleTwist{readScalar(dict.lookup("minTriangleTwist", true))};
  scalar minFaceFlatness = -1.0;
  dict.readIfPresent("minFaceFlatness", minFaceFlatness, true);
  const scalar minDet{readScalar(dict.lookup("minDeterminant", true))};
  label nWrongFaces = 0;
  Info << "Checking faces in error :" << endl;
  if (maxNonOrtho < 180.0-SMALL) {
    polyMeshGeometry::checkFaceDotProduct(report, maxNonOrtho, mesh,
                                          mesh.cellCentres(), mesh.faceAreas(),
                                          checkFaces, baffles, &wrongFaces);
    label nNewWrongFaces = returnReduce(wrongFaces.size(), sumOp<label>());
    Info << "    non-orthogonality > "
      << setw(3) << maxNonOrtho
      << " degrees                        : "
      << nNewWrongFaces-nWrongFaces << endl;
    nWrongFaces = nNewWrongFaces;
  }
  if (minVol > -GREAT) {
    polyMeshGeometry::checkFacePyramids
      (
        report,
        minVol,
        mesh,
        mesh.cellCentres(),
        mesh.points(),
        checkFaces,
        baffles,
        &wrongFaces
      );
    label nNewWrongFaces = returnReduce(wrongFaces.size(), sumOp<label>());
    Info << "    faces with face pyramid volume < "
      << setw(5) << minVol << "                 : "
      << nNewWrongFaces-nWrongFaces << endl;
    nWrongFaces = nNewWrongFaces;
  }
  if (minTetQuality > -GREAT) {
    polyMeshGeometry::checkFaceTets
      (
        report,
        minTetQuality,
        mesh,
        mesh.cellCentres(),
        mesh.faceCentres(),
        mesh.points(),
        checkFaces,
        baffles,
        &wrongFaces
      );
    label nNewWrongFaces = returnReduce(wrongFaces.size(), sumOp<label>());
    Info << "    faces with face-decomposition tet quality < "
      << setw(5) << minTetQuality << "      : "
      << nNewWrongFaces-nWrongFaces << endl;
    nWrongFaces = nNewWrongFaces;
  }
  if (maxConcave < 180.0-SMALL) {
    polyMeshGeometry::checkFaceAngles
      (
        report,
        maxConcave,
        mesh,
        mesh.faceAreas(),
        mesh.points(),
        checkFaces,
        &wrongFaces
      );
    label nNewWrongFaces = returnReduce(wrongFaces.size(), sumOp<label>());
    Info << "    faces with concavity > "
      << setw(3) << maxConcave
      << " degrees                     : "
      << nNewWrongFaces-nWrongFaces << endl;
    nWrongFaces = nNewWrongFaces;
  }
  if (minArea > -SMALL) {
    polyMeshGeometry::checkFaceArea
      (
        report,
        minArea,
        mesh,
        mesh.faceAreas(),
        checkFaces,
        &wrongFaces
      );
    label nNewWrongFaces = returnReduce(wrongFaces.size(), sumOp<label>());
    Info << "    faces with area < "
      << setw(5) << minArea
      << " m^2                            : "
      << nNewWrongFaces-nWrongFaces << endl;
    nWrongFaces = nNewWrongFaces;
  }
  if (maxIntSkew > 0 || maxBounSkew > 0) {
    polyMeshGeometry::checkFaceSkewness
      (
        report,
        maxIntSkew,
        maxBounSkew,
        mesh,
        mesh.points(),
        mesh.cellCentres(),
        mesh.faceCentres(),
        mesh.faceAreas(),
        checkFaces,
        baffles,
        &wrongFaces
      );
    label nNewWrongFaces = returnReduce(wrongFaces.size(), sumOp<label>());
    Info << "    faces with skewness > "
      << setw(3) << maxIntSkew
      << " (internal) or " << setw(3) << maxBounSkew
      << " (boundary) : " << nNewWrongFaces-nWrongFaces << endl;
    nWrongFaces = nNewWrongFaces;
  }
  if (minWeight >= 0 && minWeight < 1) {
    polyMeshGeometry::checkFaceWeights
      (
        report,
        minWeight,
        mesh,
        mesh.cellCentres(),
        mesh.faceCentres(),
        mesh.faceAreas(),
        checkFaces,
        baffles,
        &wrongFaces
      );
    label nNewWrongFaces = returnReduce(wrongFaces.size(), sumOp<label>());
    Info << "    faces with interpolation weights (0..1)  < "
      << setw(5) << minWeight
      << "       : "
      << nNewWrongFaces-nWrongFaces << endl;
    nWrongFaces = nNewWrongFaces;
  }
  if (minVolRatio >= 0) {
    polyMeshGeometry::checkVolRatio
      (
        report,
        minVolRatio,
        mesh,
        mesh.cellVolumes(),
        checkFaces,
        baffles,
        &wrongFaces
      );
    label nNewWrongFaces = returnReduce(wrongFaces.size(), sumOp<label>());
    Info << "    faces with volume ratio of neighbour cells < "
      << setw(5) << minVolRatio
      << "     : "
      << nNewWrongFaces-nWrongFaces << endl;
    nWrongFaces = nNewWrongFaces;
  }
  if (minTwist > -1) {
    polyMeshGeometry::checkFaceTwist
      (
        report,
        minTwist,
        mesh,
        mesh.cellCentres(),
        mesh.faceAreas(),
        mesh.faceCentres(),
        mesh.points(),
        checkFaces,
        &wrongFaces
      );
    label nNewWrongFaces = returnReduce(wrongFaces.size(), sumOp<label>());
    Info << "    faces with face twist < "
      << setw(5) << minTwist
      << "                          : "
      << nNewWrongFaces-nWrongFaces << endl;
    nWrongFaces = nNewWrongFaces;
  }
  if (minTriangleTwist > -1) {
    polyMeshGeometry::checkTriangleTwist
      (
        report,
        minTriangleTwist,
        mesh,
        mesh.faceAreas(),
        mesh.faceCentres(),
        mesh.points(),
        checkFaces,
        &wrongFaces
      );
    label nNewWrongFaces = returnReduce(wrongFaces.size(), sumOp<label>());
    Info << "    faces with triangle twist < "
      << setw(5) << minTriangleTwist
      << "                      : "
      << nNewWrongFaces-nWrongFaces << endl;
    nWrongFaces = nNewWrongFaces;
  }
  if (minFaceFlatness > -SMALL) {
    polyMeshGeometry::checkFaceFlatness
      (
        report,
        minFaceFlatness,
        mesh,
        mesh.faceAreas(),
        mesh.faceCentres(),
        mesh.points(),
        checkFaces,
        &wrongFaces
      );
    label nNewWrongFaces = returnReduce(wrongFaces.size(), sumOp<label>());
    Info << "    faces with flatness < "
      << setw(5) << minFaceFlatness
      << "                      : "
      << nNewWrongFaces-nWrongFaces << endl;
    nWrongFaces = nNewWrongFaces;
  }
  if (minDet > -1) {
    polyMeshGeometry::checkCellDeterminant
      (
        report,
        minDet,
        mesh,
        mesh.faceAreas(),
        checkFaces,
        polyMeshGeometry::affectedCells(mesh, checkFaces),
        &wrongFaces
      );
    label nNewWrongFaces = returnReduce(wrongFaces.size(), sumOp<label>());
    Info << "    faces on cells with determinant < "
      << setw(5) << minDet << "                : "
      << nNewWrongFaces-nWrongFaces << endl;
    nWrongFaces = nNewWrongFaces;
  }
  //Pout.setf(ios_base::right);
  return nWrongFaces > 0;
}


bool mousse::motionSmootherAlgo::checkMesh
(
  const bool report,
  const polyMesh& mesh,
  const dictionary& dict,
  labelHashSet& wrongFaces
)
{
  return checkMesh(report, mesh, dict, identity(mesh.nFaces()), wrongFaces);
}


bool mousse::motionSmootherAlgo::checkMesh
(
  const bool report,
  const dictionary& dict,
  const polyMeshGeometry& meshGeom,
  const labelList& checkFaces,
  labelHashSet& wrongFaces
)
{
  List<labelPair> emptyBaffles;
  return
    checkMesh(report, dict, meshGeom, checkFaces, emptyBaffles, wrongFaces);
}


bool mousse::motionSmootherAlgo::checkMesh
(
  const bool report,
  const dictionary& dict,
  const polyMeshGeometry& meshGeom,
  const labelList& checkFaces,
  const List<labelPair>& baffles,
  labelHashSet& wrongFaces
)
{
  const scalar maxNonOrtho{readScalar(dict.lookup("maxNonOrtho", true))};
  const scalar minVol{readScalar(dict.lookup("minVol", true))};
  const scalar minTetQuality{readScalar(dict.lookup("minTetQuality", true))};
  const scalar maxConcave{readScalar(dict.lookup("maxConcave", true))};
  const scalar minArea{readScalar(dict.lookup("minArea", true))};
  const scalar minWeight{readScalar(dict.lookup("minFaceWeight", true))};
  const scalar minVolRatio{readScalar(dict.lookup("minVolRatio", true))};
  const scalar minTwist{readScalar(dict.lookup("minTwist", true))};
  const scalar
    minTriangleTwist{readScalar(dict.lookup("minTriangleTwist", true))};
  scalar minFaceFlatness = -1.0;
  dict.readIfPresent("minFaceFlatness", minFaceFlatness, true);
  const scalar minDet{readScalar(dict.lookup("minDeterminant", true))};
  label nWrongFaces = 0;
  Info << "Checking faces in error :" << endl;
  if (maxNonOrtho < 180.0-SMALL) {
    meshGeom.checkFaceDotProduct(report, maxNonOrtho, checkFaces, baffles,
                                 &wrongFaces);
    label nNewWrongFaces = returnReduce(wrongFaces.size(), sumOp<label>());
    Info << "    non-orthogonality > "
      << setw(3) << maxNonOrtho
      << " degrees                        : "
      << nNewWrongFaces-nWrongFaces << endl;
    nWrongFaces = nNewWrongFaces;
  }
  if (minVol > -GREAT) {
    meshGeom.checkFacePyramids
      (
        report,
        minVol,
        meshGeom.mesh().points(),
        checkFaces,
        baffles,
        &wrongFaces
      );
    label nNewWrongFaces = returnReduce(wrongFaces.size(), sumOp<label>());
    Info << "    faces with face pyramid volume < "
      << setw(5) << minVol << "                 : "
      << nNewWrongFaces-nWrongFaces << endl;
    nWrongFaces = nNewWrongFaces;
  }
  if (minTetQuality > -GREAT) {
    meshGeom.checkFaceTets
      (
        report,
        minTetQuality,
        meshGeom.mesh().points(),
        checkFaces,
        baffles,
        &wrongFaces
      );
    label nNewWrongFaces = returnReduce(wrongFaces.size(), sumOp<label>());
    Info << "    faces with face-decomposition tet quality < "
      << setw(5) << minTetQuality << "                : "
      << nNewWrongFaces-nWrongFaces << endl;
    nWrongFaces = nNewWrongFaces;
  }
  if (maxConcave < 180.0-SMALL) {
    meshGeom.checkFaceAngles
      (
        report,
        maxConcave,
        meshGeom.mesh().points(),
        checkFaces,
        &wrongFaces
      );
    label nNewWrongFaces = returnReduce(wrongFaces.size(), sumOp<label>());
    Info << "    faces with concavity > "
      << setw(3) << maxConcave
      << " degrees                     : "
      << nNewWrongFaces-nWrongFaces << endl;
    nWrongFaces = nNewWrongFaces;
  }
  if (minArea > -SMALL) {
    meshGeom.checkFaceArea(report, minArea, checkFaces, &wrongFaces);
    label nNewWrongFaces = returnReduce(wrongFaces.size(), sumOp<label>());
    Info << "    faces with area < "
      << setw(5) << minArea
      << " m^2                            : "
      << nNewWrongFaces-nWrongFaces << endl;
    nWrongFaces = nNewWrongFaces;
  }
  if (minWeight >= 0 && minWeight < 1) {
    meshGeom.checkFaceWeights
      (
        report,
        minWeight,
        checkFaces,
        baffles,
        &wrongFaces
      );
    label nNewWrongFaces = returnReduce(wrongFaces.size(), sumOp<label>());
    Info << "    faces with interpolation weights (0..1)  < "
      << setw(5) << minWeight
      << "       : "
      << nNewWrongFaces-nWrongFaces << endl;
    nWrongFaces = nNewWrongFaces;
  }
  if (minVolRatio >= 0) {
    meshGeom.checkVolRatio
      (
        report,
        minVolRatio,
        checkFaces,
        baffles,
        &wrongFaces
      );
    label nNewWrongFaces = returnReduce(wrongFaces.size(), sumOp<label>());
    Info << "    faces with volume ratio of neighbour cells < "
      << setw(5) << minVolRatio
      << "     : "
      << nNewWrongFaces-nWrongFaces << endl;
    nWrongFaces = nNewWrongFaces;
  }
  if (minTwist > -1) {
    meshGeom.checkFaceTwist
      (
        report,
        minTwist,
        meshGeom.mesh().points(),
        checkFaces,
        &wrongFaces
      );
    label nNewWrongFaces = returnReduce(wrongFaces.size(), sumOp<label>());
    Info << "    faces with face twist < "
      << setw(5) << minTwist
      << "                          : "
      << nNewWrongFaces-nWrongFaces << endl;
    nWrongFaces = nNewWrongFaces;
  }
  if (minTriangleTwist > -1) {
    meshGeom.checkTriangleTwist
      (
        report,
        minTriangleTwist,
        meshGeom.mesh().points(),
        checkFaces,
        &wrongFaces
      );
    label nNewWrongFaces = returnReduce(wrongFaces.size(), sumOp<label>());
    Info << "    faces with triangle twist < "
      << setw(5) << minTriangleTwist
      << "                      : "
      << nNewWrongFaces-nWrongFaces << endl;
    nWrongFaces = nNewWrongFaces;
  }
  if (minFaceFlatness > -1) {
    meshGeom.checkFaceFlatness
      (
        report,
        minFaceFlatness,
        meshGeom.mesh().points(),
        checkFaces,
        &wrongFaces
      );
    label nNewWrongFaces = returnReduce(wrongFaces.size(), sumOp<label>());
    Info << "    faces with flatness < "
      << setw(5) << minFaceFlatness
      << "                      : "
      << nNewWrongFaces-nWrongFaces << endl;
    nWrongFaces = nNewWrongFaces;
  }
  if (minDet > -1) {
    meshGeom.checkCellDeterminant
      (
        report,
        minDet,
        checkFaces,
        meshGeom.affectedCells(meshGeom.mesh(), checkFaces),
        &wrongFaces
      );
    label nNewWrongFaces = returnReduce(wrongFaces.size(), sumOp<label>());
    Info << "    faces on cells with determinant < "
      << setw(5) << minDet << "                : "
      << nNewWrongFaces-nWrongFaces << endl;
    nWrongFaces = nNewWrongFaces;
  }
  return nWrongFaces > 0;
}

