// mousse: CFD toolbox
// Copyright (C) 2011-2014 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "motion_smoother_algo.hpp"
#include "twod_point_corrector.hpp"
#include "face_set.hpp"
#include "point_set.hpp"
#include "fixed_value_point_patch_fields.hpp"
#include "point_constraints.hpp"
#include "sync_tools.hpp"
#include "mesh_tools.hpp"
#include "ofstream.hpp"


// Static Data Members
namespace mousse {

DEFINE_TYPE_NAME_AND_DEBUG(motionSmootherAlgo, 0);

}


// Private Member Functions 
void mousse::motionSmootherAlgo::testSyncPositions
(
  const pointField& fld,
  const scalar maxMag
) const
{
  pointField syncedFld(fld);
  syncTools::syncPointPositions(mesh_, syncedFld, minEqOp<point>(),
                                {GREAT,GREAT,GREAT});
  FOR_ALL(syncedFld, i) {
    if (mag(syncedFld[i] - fld[i]) > maxMag) {
      FATAL_ERROR_IN
      (
        "motionSmootherAlgo::testSyncPositions"
        "("
          "const pointField&, "
          "const scalar"
        ")"
      )
      << "On point " << i << " point:" << fld[i]
      << " synchronised point:" << syncedFld[i]
      << abort(FatalError);
    }
  }
}


void mousse::motionSmootherAlgo::checkFld(const pointScalarField& fld)
{
  FOR_ALL(fld, pointI) {
    const scalar val = fld[pointI];
    if ((val > -GREAT) && (val < GREAT)) {
    } else {
      FATAL_ERROR_IN
      (
        "motionSmootherAlgo::checkFld"
        "(const pointScalarField&)"
      )
      << "Problem : point:" << pointI << " value:" << val
      << abort(FatalError);
    }
  }
}


mousse::labelHashSet mousse::motionSmootherAlgo::getPoints
(
  const labelHashSet& faceLabels
) const
{
  labelHashSet usedPoints{mesh_.nPoints()/100};
  FOR_ALL_CONST_ITER(labelHashSet, faceLabels, iter) {
    const face& f = mesh_.faces()[iter.key()];
    FOR_ALL(f, fp) {
      usedPoints.insert(f[fp]);
    }
  }
  return usedPoints;
}


mousse::tmp<mousse::scalarField> mousse::motionSmootherAlgo::calcEdgeWeights
(
  const pointField& points
) const
{
  const edgeList& edges = mesh_.edges();
  tmp<scalarField> twght{new scalarField(edges.size())};
  scalarField& wght = twght();
  FOR_ALL(edges, edgeI) {
    wght[edgeI] = 1.0/(edges[edgeI].mag(points)+SMALL);
  }
  return twght;
}


// Smooth on selected points (usually patch points)
void mousse::motionSmootherAlgo::minSmooth
(
  const scalarField& edgeWeights,
  const PackedBoolList& isAffectedPoint,
  const labelList& meshPoints,
  const pointScalarField& fld,
  pointScalarField& newFld
) const
{
  tmp<pointScalarField> tavgFld = avg(fld, edgeWeights);
  const pointScalarField& avgFld = tavgFld();
  FOR_ALL(meshPoints, i) {
    label pointI = meshPoints[i];
    if (isAffectedPoint.get(pointI) == 1) {
      newFld[pointI] = min(fld[pointI], 0.5*(fld[pointI] + avgFld[pointI]));
    }
  }
  // Single and multi-patch constraints
  pointConstraints::New(pMesh()).constrain(newFld, false);
}


// Smooth on all internal points
void mousse::motionSmootherAlgo::minSmooth
(
  const scalarField& edgeWeights,
  const PackedBoolList& isAffectedPoint,
  const pointScalarField& fld,
  pointScalarField& newFld
) const
{
  tmp<pointScalarField> tavgFld = avg(fld, edgeWeights);
  const pointScalarField& avgFld = tavgFld();
  FOR_ALL(fld, pointI) {
    if (isAffectedPoint.get(pointI) == 1 && isInternalPoint(pointI)) {
      newFld[pointI] = min(fld[pointI], 0.5*(fld[pointI] + avgFld[pointI]));
    }
  }
 // Single and multi-patch constraints
  pointConstraints::New(pMesh()).constrain(newFld, false);
}


// Scale on all internal points
void mousse::motionSmootherAlgo::scaleField
(
  const labelHashSet& pointLabels,
  const scalar scale,
  pointScalarField& fld
) const
{
  FOR_ALL_CONST_ITER(labelHashSet, pointLabels, iter) {
    if (isInternalPoint(iter.key())) {
      fld[iter.key()] *= scale;
    }
  }
  // Single and multi-patch constraints
  pointConstraints::New(pMesh()).constrain(fld, false);
}


// Scale on selected points (usually patch points)
void mousse::motionSmootherAlgo::scaleField
(
  const labelList& meshPoints,
  const labelHashSet& pointLabels,
  const scalar scale,
  pointScalarField& fld
) const
{
  FOR_ALL(meshPoints, i) {
    label pointI = meshPoints[i];
    if (pointLabels.found(pointI)) {
      fld[pointI] *= scale;
    }
  }
}


// Lower on internal points
void mousse::motionSmootherAlgo::subtractField
(
  const labelHashSet& pointLabels,
  const scalar f,
  pointScalarField& fld
) const
{
  FOR_ALL_CONST_ITER(labelHashSet, pointLabels, iter) {
    if (isInternalPoint(iter.key())) {
      fld[iter.key()] = max(0.0, fld[iter.key()]-f);
    }
  }
  // Single and multi-patch constraints
  pointConstraints::New(pMesh()).constrain(fld);
}


// Scale on selected points (usually patch points)
void mousse::motionSmootherAlgo::subtractField
(
  const labelList& meshPoints,
  const labelHashSet& pointLabels,
  const scalar f,
  pointScalarField& fld
) const
{
  FOR_ALL(meshPoints, i) {
    label pointI = meshPoints[i];
    if (pointLabels.found(pointI)) {
      fld[pointI] = max(0.0, fld[pointI]-f);
    }
  }
}


bool mousse::motionSmootherAlgo::isInternalPoint(const label pointI) const
{
  return isInternalPoint_.get(pointI) == 1;
}


void mousse::motionSmootherAlgo::getAffectedFacesAndPoints
(
  const label nPointIter,
  const faceSet& wrongFaces,
  labelList& affectedFaces,
  PackedBoolList& isAffectedPoint
) const
{
  isAffectedPoint.setSize(mesh_.nPoints());
  isAffectedPoint = 0;
  faceSet nbrFaces{mesh_, "checkFaces", wrongFaces};
  // Find possible points influenced by nPointIter iterations of
  // scaling and smoothing by doing pointCellpoint walk.
  // Also update faces-to-be-checked to extend one layer beyond the points
  // that will get updated.
  for (label i = 0; i < nPointIter; i++) {
    pointSet nbrPoints{mesh_, "grownPoints", getPoints(nbrFaces.toc())};
    FOR_ALL_CONST_ITER(pointSet, nbrPoints, iter) {
      const labelList& pCells = mesh_.pointCells(iter.key());
      FOR_ALL(pCells, pCellI) {
        const cell& cFaces = mesh_.cells()[pCells[pCellI]];
        FOR_ALL(cFaces, cFaceI) {
          nbrFaces.insert(cFaces[cFaceI]);
        }
      }
    }
    nbrFaces.sync(mesh_);
    if (i == nPointIter - 2) {
      FOR_ALL_CONST_ITER(faceSet, nbrFaces, iter) {
        const face& f = mesh_.faces()[iter.key()];
        FOR_ALL(f, fp) {
          isAffectedPoint.set(f[fp], 1);
        }
      }
    }
  }
  affectedFaces = nbrFaces.toc();
}


// Constructors 
mousse::motionSmootherAlgo::motionSmootherAlgo
(
  polyMesh& mesh,
  pointMesh& pMesh,
  indirectPrimitivePatch& pp,
  pointVectorField& displacement,
  pointScalarField& scale,
  pointField& oldPoints,
  const labelList& adaptPatchIDs,
  const dictionary& paramDict
)
:
  mesh_{mesh},
  pMesh_{pMesh},
  pp_{pp},
  displacement_{displacement},
  scale_{scale},
  oldPoints_{oldPoints},
  adaptPatchIDs_{adaptPatchIDs},
  paramDict_{paramDict},
  isInternalPoint_{mesh_.nPoints(), 1}
{
  updateMesh();
}


// Destructor 
mousse::motionSmootherAlgo::~motionSmootherAlgo()
{}


// Member Functions 
const mousse::polyMesh& mousse::motionSmootherAlgo::mesh() const
{
  return mesh_;
}


const mousse::pointMesh& mousse::motionSmootherAlgo::pMesh() const
{
  return pMesh_;
}


const mousse::indirectPrimitivePatch& mousse::motionSmootherAlgo::patch() const
{
  return pp_;
}


const mousse::labelList& mousse::motionSmootherAlgo::adaptPatchIDs() const
{
  return adaptPatchIDs_;
}


const mousse::dictionary& mousse::motionSmootherAlgo::paramDict() const
{
  return paramDict_;
}


void mousse::motionSmootherAlgo::correct()
{
  oldPoints_ = mesh_.points();
  scale_ = 1.0;
  // No need to update twoDmotion corrector since only holds edge labels
  // which will remain the same as before. So unless the mesh was distorted
  // severely outside of motionSmootherAlgo there will be no need.
}


void mousse::motionSmootherAlgo::setDisplacementPatchFields
(
  const labelList& patchIDs,
  pointVectorField& displacement
)
{
  // Adapt the fixedValue bc's (i.e. copy internal point data to
  // boundaryField for all affected patches)
  FOR_ALL(patchIDs, i) {
    label patchI = patchIDs[i];
    displacement.boundaryField()[patchI] ==
      displacement.boundaryField()[patchI].patchInternalField();
  }
  // Make consistent with non-adapted bc's by evaluating those now and
  // resetting the displacement from the values.
  // Note that we're just doing a correctBoundaryConditions with
  // fixedValue bc's first.
  labelHashSet adaptPatchSet{patchIDs};
  const lduSchedule& patchSchedule =
    displacement.mesh().globalData().patchSchedule();
  FOR_ALL(patchSchedule, patchEvalI) {
    label patchI = patchSchedule[patchEvalI].patch;
    if (!adaptPatchSet.found(patchI)) {
      if (patchSchedule[patchEvalI].init) {
        displacement.boundaryField()[patchI].initEvaluate(Pstream::scheduled);
      } else {
        displacement.boundaryField()[patchI].evaluate(Pstream::scheduled);
      }
    }
  }
  // Multi-patch constraints
  pointConstraints::New(displacement.mesh()).constrainCorners(displacement);
  // Adapt the fixedValue bc's (i.e. copy internal point data to
  // boundaryField for all affected patches) to take the changes caused
  // by multi-corner constraints into account.
  FOR_ALL(patchIDs, i) {
    label patchI = patchIDs[i];
    displacement.boundaryField()[patchI] ==
      displacement.boundaryField()[patchI].patchInternalField();
  }
}


void mousse::motionSmootherAlgo::setDisplacementPatchFields()
{
  setDisplacementPatchFields(adaptPatchIDs_, displacement_);
}


void mousse::motionSmootherAlgo::setDisplacement
(
  const labelList& patchIDs,
  const indirectPrimitivePatch& pp,
  pointField& patchDisp,
  pointVectorField& displacement
)
{
  const polyMesh& mesh = displacement.mesh()();
  // See comment in .H file about shared points.
  // We want to disallow effect of loose coupled points - we only
  // want to see effect of proper fixedValue boundary conditions
  const labelList& cppMeshPoints =
    mesh.globalData().coupledPatch().meshPoints();
  FOR_ALL(cppMeshPoints, i) {
    displacement[cppMeshPoints[i]] = vector::zero;
  }
  const labelList& ppMeshPoints = pp.meshPoints();
  // Set internal point data from displacement on combined patch points.
  FOR_ALL(ppMeshPoints, patchPointI) {
    displacement[ppMeshPoints[patchPointI]] = patchDisp[patchPointI];
  }
  // Combine any coupled points
  syncTools::syncPointList(mesh, displacement, maxMagEqOp(), vector::zero);
  // Adapt the fixedValue bc's (i.e. copy internal point data to
  // boundaryField for all affected patches)
  setDisplacementPatchFields(patchIDs, displacement);
  if (debug) {
    OFstream str{mesh.db().path()/"changedPoints.obj"};
    label nVerts = 0;
    FOR_ALL(ppMeshPoints, patchPointI) {
      const vector& newDisp = displacement[ppMeshPoints[patchPointI]];
      if (mag(newDisp-patchDisp[patchPointI]) > SMALL) {
        const point& pt = mesh.points()[ppMeshPoints[patchPointI]];
        meshTools::writeOBJ(str, pt);
        nVerts++;
      }
    }
    Pout << "Written " << nVerts << " points that are changed to file "
      << str.name() << endl;
  }
  // Now reset input displacement
  FOR_ALL(ppMeshPoints, patchPointI) {
    patchDisp[patchPointI] = displacement[ppMeshPoints[patchPointI]];
  }
}


void mousse::motionSmootherAlgo::setDisplacement(pointField& patchDisp)
{
  setDisplacement(adaptPatchIDs_, pp_, patchDisp, displacement_);
}


// correctBoundaryConditions with fixedValue bc's first.
void mousse::motionSmootherAlgo::correctBoundaryConditions
(
  pointVectorField& displacement
) const
{
  labelHashSet adaptPatchSet(adaptPatchIDs_);
  const lduSchedule& patchSchedule = mesh_.globalData().patchSchedule();
  // 1. evaluate on adaptPatches
  FOR_ALL(patchSchedule, patchEvalI) {
    label patchI = patchSchedule[patchEvalI].patch;
    if (adaptPatchSet.found(patchI)) {
      if (patchSchedule[patchEvalI].init) {
        displacement.boundaryField()[patchI].initEvaluate(Pstream::blocking);
      } else {
        displacement.boundaryField()[patchI].evaluate(Pstream::blocking);
      }
    }
  }
  // 2. evaluate on non-AdaptPatches
  FOR_ALL(patchSchedule, patchEvalI) {
    label patchI = patchSchedule[patchEvalI].patch;
    if (!adaptPatchSet.found(patchI)) {
      if (patchSchedule[patchEvalI].init) {
        displacement.boundaryField()[patchI].initEvaluate(Pstream::blocking);
      } else {
        displacement.boundaryField()[patchI].evaluate(Pstream::blocking);
      }
    }
  }
  // Multi-patch constraints
  pointConstraints::New(displacement.mesh()).constrainCorners(displacement);
  // Correct for problems introduced by corner constraints
  syncTools::syncPointList(mesh_, displacement, maxMagEqOp(), vector::zero);
}


void mousse::motionSmootherAlgo::modifyMotionPoints(pointField& newPoints) const
{
  // Correct for 2-D motion
  const twoDPointCorrector& twoDCorrector = twoDPointCorrector::New(mesh_);
  if (twoDCorrector.required()) {
    Info << "Correcting 2-D mesh motion";
    if (mesh_.globalData().parallel()) {
      WARNING_IN("motionSmootherAlgo::modifyMotionPoints(pointField&)")
        << "2D mesh-motion probably not correct in parallel" << endl;
    }
    // We do not want to move 3D planes so project all points onto those
    const pointField& oldPoints = mesh_.points();
    const edgeList& edges = mesh_.edges();
    const labelList& neIndices = twoDCorrector.normalEdgeIndices();
    const vector& pn = twoDCorrector.planeNormal();
    FOR_ALL(neIndices, i) {
      const edge& e = edges[neIndices[i]];
      point& pStart = newPoints[e.start()];
      pStart += pn*(pn & (oldPoints[e.start()] - pStart));
      point& pEnd = newPoints[e.end()];
      pEnd += pn*(pn & (oldPoints[e.end()] - pEnd));
    }
    // Correct tangentially
    twoDCorrector.correctPoints(newPoints);
    Info << " ...done" << endl;
  }
  if (debug) {
    Pout << "motionSmootherAlgo::modifyMotionPoints :"
      << " testing sync of newPoints."
      << endl;
    testSyncPositions(newPoints, 1e-6*mesh_.bounds().mag());
  }
}


void mousse::motionSmootherAlgo::movePoints()
{
  // Make sure to clear out tetPtIs since used in checks (sometimes, should
  // really check)
  mesh_.clearAdditionalGeom();
  pp_.movePoints(mesh_.points());
}


mousse::scalar mousse::motionSmootherAlgo::setErrorReduction
(
  const scalar errorReduction
)
{
  scalar oldErrorReduction = readScalar(paramDict_.lookup("errorReduction"));
  paramDict_.remove("errorReduction");
  paramDict_.add("errorReduction", errorReduction);
  return oldErrorReduction;
}


bool mousse::motionSmootherAlgo::scaleMesh
(
  labelList& checkFaces,
  const bool smoothMesh,
  const label nAllowableErrors
)
{
  List<labelPair> emptyBaffles;
  return scaleMesh(checkFaces, emptyBaffles, smoothMesh, nAllowableErrors);
}


bool mousse::motionSmootherAlgo::scaleMesh
(
  labelList& checkFaces,
  const List<labelPair>& baffles,
  const bool smoothMesh,
  const label nAllowableErrors
)
{
  return scaleMesh(checkFaces, baffles, paramDict_, paramDict_, smoothMesh,
                   nAllowableErrors);
}


mousse::tmp<mousse::pointField> mousse::motionSmootherAlgo::curPoints() const
{
  // Set newPoints as old + scale*displacement
  // Create overall displacement with same b.c.s as displacement_
  wordList actualPatchTypes;

  {
    const pointBoundaryMesh& pbm = displacement_.mesh().boundary();
    actualPatchTypes.setSize(pbm.size());
    FOR_ALL(pbm, patchI) {
      actualPatchTypes[patchI] = pbm[patchI].type();
    }
  }

  wordList actualPatchFieldTypes;

  {
    const pointVectorField::GeometricBoundaryField& pfld =
      displacement_.boundaryField();
    actualPatchFieldTypes.setSize(pfld.size());
    FOR_ALL(pfld, patchI) {
      if (isA<fixedValuePointPatchField<vector> >(pfld[patchI])) {
        // Get rid of funny
        actualPatchFieldTypes[patchI] =
          fixedValuePointPatchField<vector>::typeName;
      } else {
        actualPatchFieldTypes[patchI] = pfld[patchI].type();
      }
    }
  }
  pointVectorField totalDisplacement
  {
    {
      "totalDisplacement",
      mesh_.time().timeName(),
      mesh_,
      IOobject::NO_READ,
      IOobject::NO_WRITE,
      false
    },
    scale_*displacement_,
    actualPatchFieldTypes,
    actualPatchTypes
  };
  correctBoundaryConditions(totalDisplacement);
  if (debug) {
    Pout << "scaleMesh : testing sync of totalDisplacement" << endl;
    testSyncField(totalDisplacement, maxMagEqOp(), vector::zero,
                  1e-6*mesh_.bounds().mag());
  }
  tmp<pointField> tnewPoints{oldPoints_ + totalDisplacement.internalField()};
  // Correct for 2-D motion
  modifyMotionPoints(tnewPoints());
  return tnewPoints;
}


bool mousse::motionSmootherAlgo::scaleMesh
(
  labelList& checkFaces,
  const List<labelPair>& baffles,
  const dictionary& paramDict,
  const dictionary& meshQualityDict,
  const bool smoothMesh,
  const label nAllowableErrors
)
{
  if (!smoothMesh && adaptPatchIDs_.empty()) {
    FATAL_ERROR_IN
    (
      "motionSmootherAlgo::scaleMesh"
      "("
        "labelList&, "
        "const List<labelPair>&, "
        "const dictionary&, "
        "const dictionary&, "
        "const bool, "
        "const label"
      ")"
    )
    << "You specified both no movement on the internal mesh points"
    << " (smoothMesh = false)" << nl
    << "and no movement on the patch (adaptPatchIDs is empty)" << nl
    << "Hence nothing to adapt."
    << exit(FatalError);
  }
  if (debug) {
    // Had a problem with patches moved non-synced. Check transformations.
    const polyBoundaryMesh& patches = mesh_.boundaryMesh();
    Pout << "Entering scaleMesh : coupled patches:" << endl;
    FOR_ALL(patches, patchI) {
      if (patches[patchI].coupled()) {
        const coupledPolyPatch& pp =
          refCast<const coupledPolyPatch>(patches[patchI]);
        Pout << '\t' << patchI << '\t' << pp.name()
          << " parallel:" << pp.parallel()
          << " separated:" << pp.separated()
          << " forwardT:" << pp.forwardT().size()
          << endl;
      }
    }
  }
  const scalar errorReduction = readScalar(paramDict.lookup("errorReduction"));
  const label nSmoothScale = readLabel(paramDict.lookup("nSmoothScale"));
  // Note: displacement_ should already be synced already from setDisplacement
  // but just to make sure.
  syncTools::syncPointList(mesh_, displacement_, maxMagEqOp(), vector::zero);
  Info << "Moving mesh using displacement scaling :"
    << " min:" << gMin(scale_.internalField())
    << "  max:" << gMax(scale_.internalField())
    << endl;
  // Get points using current displacement and scale. Optionally 2D corrected.
  pointField newPoints{curPoints()};
  // Move. No need to do 2D correction since curPoints already done that.
  mesh_.movePoints(newPoints);
  movePoints();
  // Check. Returns parallel number of incorrect faces.
  faceSet wrongFaces{mesh_, "wrongFaces", mesh_.nFaces()/100+100};
  checkMesh(false, mesh_, meshQualityDict, checkFaces, baffles, wrongFaces);
  if (returnReduce(wrongFaces.size(), sumOp<label>()) <= nAllowableErrors) {
    return true;
  } else {
    // Sync across coupled faces by extending the set.
    wrongFaces.sync(mesh_);
    // Special case:
    // if errorReduction is set to zero, extend wrongFaces
    // to face-Cell-faces to ensure quick return to previously valid mesh
    if (mag(errorReduction) < SMALL) {
      labelHashSet newWrongFaces{wrongFaces};
      FOR_ALL_CONST_ITER(labelHashSet, wrongFaces, iter) {
        label own = mesh_.faceOwner()[iter.key()];
        const cell& ownFaces = mesh_.cells()[own];
        FOR_ALL(ownFaces, cfI) {
          newWrongFaces.insert(ownFaces[cfI]);
        }
        if (iter.key() < mesh_.nInternalFaces()) {
          label nei = mesh_.faceNeighbour()[iter.key()];
          const cell& neiFaces = mesh_.cells()[nei];
          FOR_ALL(neiFaces, cfI) {
            newWrongFaces.insert(neiFaces[cfI]);
          }
        }
      }
      wrongFaces.transfer(newWrongFaces);
      wrongFaces.sync(mesh_);
    }
    // Find out points used by wrong faces and scale displacement.
    pointSet usedPoints{mesh_, "usedPoints", getPoints(wrongFaces)};
    usedPoints.sync(mesh_);
    // Grow a few layers to determine
    // - points to be smoothed
    // - faces to be checked in next iteration
    PackedBoolList isAffectedPoint{mesh_.nPoints()};
    getAffectedFacesAndPoints(nSmoothScale, wrongFaces, checkFaces,
                              isAffectedPoint);
    if (debug) {
      Pout << "Faces in error:" << wrongFaces.size()
        << "  with points:" << usedPoints.size()
        << endl;
    }
    if (adaptPatchIDs_.size()) {
      // Scale conflicting patch points
      scaleField(pp_.meshPoints(), usedPoints, errorReduction, scale_);
      //subtractField(pp_.meshPoints(), usedPoints, 0.1, scale_);
    }
    if (smoothMesh) {
      // Scale conflicting internal points
      scaleField(usedPoints, errorReduction, scale_);
      //subtractField(usedPoints, 0.1, scale_);
    }
    scalarField eWeights{calcEdgeWeights(oldPoints_)};
    for (label i = 0; i < nSmoothScale; i++) {
      if (adaptPatchIDs_.size()) {
        // Smooth patch values
        pointScalarField oldScale{scale_};
        minSmooth(eWeights, isAffectedPoint, pp_.meshPoints(), oldScale,
                  scale_);
        checkFld(scale_);
      }
      if (smoothMesh) {
        // Smooth internal values
        pointScalarField oldScale(scale_);
        minSmooth(eWeights, isAffectedPoint, oldScale, scale_);
        checkFld(scale_);
      }
    }
    syncTools::syncPointList(mesh_, scale_, maxEqOp<scalar>(), -GREAT);
    if (debug) {
      Pout << "scale_ after smoothing :"
        << " min:" << mousse::gMin(scale_)
        << " max:" << mousse::gMax(scale_)
        << endl;
    }
    return false;
  }
}


void mousse::motionSmootherAlgo::updateMesh()
{
  const pointBoundaryMesh& patches = pMesh_.boundary();
  // Check whether displacement has fixed value b.c. on adaptPatchID
  FOR_ALL(adaptPatchIDs_, i) {
    label patchI = adaptPatchIDs_[i];
    if (!isA<fixedValuePointPatchVectorField>
        (
          displacement_.boundaryField()[patchI]
        )) {
      FATAL_ERROR_IN
      (
        "motionSmootherAlgo::updateMesh"
      )
      << "Patch " << patches[patchI].name()
      << " has wrong boundary condition "
      << displacement_.boundaryField()[patchI].type()
      << " on field " << displacement_.name() << nl
      << "Only type allowed is "
      << fixedValuePointPatchVectorField::typeName
      << exit(FatalError);
    }
  }
  // Determine internal points. Note that for twoD there are no internal
  // points so we use the points of adaptPatchIDs instead
  const labelList& meshPoints = pp_.meshPoints();
  FOR_ALL(meshPoints, i) {
    isInternalPoint_.unset(meshPoints[i]);
  }
  // Calculate master edge addressing
  isMasterEdge_ = syncTools::getMasterEdges(mesh_);
}

