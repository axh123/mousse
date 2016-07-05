// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "cell_shape_recognition.hpp"
#include "label_list.hpp"


namespace mousse {

cellShape create3DCellShape
(
  const label cellIndex,
  const labelList& faceLabels,
  const faceList& faces,
  const labelList& owner,
  const labelList& neighbour,
  const label fluentCellModelID
)
{
  // List of pointers to shape models for 3-D shape recognition
  static List<const cellModel*> fluentCellModelLookup
  {
    7,
    reinterpret_cast<const cellModel*>(0)
  };
  fluentCellModelLookup[2] = cellModeller::lookup("tet");
  fluentCellModelLookup[4] = cellModeller::lookup("hex");
  fluentCellModelLookup[5] = cellModeller::lookup("pyr");
  fluentCellModelLookup[6] = cellModeller::lookup("prism");
  static label faceMatchingOrder[7][6] =
  {
    {-1, -1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1, -1},
    { 0,  1,  2,  3, -1, -1},    // tet
    {-1, -1, -1, -1, -1, -1},
    { 0,  2,  4,  3,  5,  1},    // hex
    { 0,  1,  2,  3,  4, -1},    // pyr
    { 0,  2,  3,  4,  1, -1},    // prism
  };
  const cellModel& curModel = *fluentCellModelLookup[fluentCellModelID];
  // Checking
  if (faceLabels.size() != curModel.nFaces()) {
    FATAL_ERROR_IN
    (
      "create3DCellShape(const label cellIndex, "
      "const labelList& faceLabels, const labelListList& faces, "
      "const labelList& owner, const labelList& neighbour, "
      "const label fluentCellModelID)"
    )
    << "Number of face labels not equal to"
    << "number of face in the model. "
    << "Number of face labels: " << faceLabels.size()
    << " number of faces in model: " << curModel.nFaces()
    << abort(FatalError);
  }
  // make a list of outward-pointing faces
  labelListList localFaces{faceLabels.size()};
  FOR_ALL(faceLabels, faceI) {
    const label curFaceLabel = faceLabels[faceI];
    const labelList& curFace = faces[curFaceLabel];
    if (owner[curFaceLabel] == cellIndex) {
      localFaces[faceI] = curFace;
    } else if (neighbour[curFaceLabel] == cellIndex) {
      // Reverse the face
      localFaces[faceI].setSize(curFace.size());
      FOR_ALL_REVERSE(curFace, i) {
        localFaces[faceI][curFace.size() - i - 1] = curFace[i];
      }
    } else {
      FATAL_ERROR_IN
      (
        "create3DCellShape(const label cellIndex, "
        "const labelList& faceLabels, const labelListList& faces, "
        "const labelList& owner, const labelList& neighbour, "
        "const label fluentCellModelID)"
      )
      << "face " << curFaceLabel
      << " does not belong to cell " << cellIndex
      << ". Face owner: " << owner[curFaceLabel] << " neighbour: "
      << neighbour[curFaceLabel]
      << abort(FatalError);
    }
  }
  // Algorithm:
  // Make an empty list of pointLabels and initialise it with -1. Pick the
  // first face from modelFaces and look through the faces to find one with
  // the same number of labels. Insert face by copying its labels into
  // pointLabels. Mark the face as used. Loop through all model faces.
  // For each model face loop through faces. If the face is unused and the
  // numbers of labels fit, try to match the face onto the point labels. If
  // at least one edge is matched, insert the face into pointLabels. If at
  // any stage the matching algorithm reaches the end of faces, the matching
  // algorithm has failed. Once all the faces are matched, the list of
  // pointLabels defines the model.
  // Make a list of empty pointLabels
  labelList pointLabels{curModel.nPoints(), -1};
  // Follow the used mesh faces
  List<bool> meshFaceUsed{localFaces.size(), false};
  // Get the raw model faces
  const faceList& modelFaces = curModel.modelFaces();
  // Insert the first face into the list
  const labelList& firstModelFace =
    modelFaces[faceMatchingOrder[fluentCellModelID][0]];
  bool found = false;
  FOR_ALL(localFaces, meshFaceI) {
    if (localFaces[meshFaceI].size() == firstModelFace.size()) {
      // Match. Insert points into the pointLabels
      found = true;
      const labelList& curMeshFace = localFaces[meshFaceI];
      meshFaceUsed[meshFaceI] = true;
      FOR_ALL(curMeshFace, pointI) {
        pointLabels[firstModelFace[pointI]] = curMeshFace[pointI];
      }
      break;
    }
  }
  if (!found) {
    FATAL_ERROR_IN
    (
      "create3DCellShape(const label cellIndex, "
      "const labelList& faceLabels, const labelListList& faces, "
      "const labelList& owner, const labelList& neighbour, "
      "const label fluentCellModelID)"
    )
    << "Cannot find match for first face. "
    << "cell model: " << curModel.name() << " first model face: "
    << firstModelFace << " Mesh faces: " << localFaces
    << abort(FatalError);
  }
  for (label modelFaceI = 1; modelFaceI < modelFaces.size(); modelFaceI++) {
    // get the next model face
    const labelList& curModelFace =
      modelFaces[faceMatchingOrder[fluentCellModelID][modelFaceI]];
    found = false;
    // Loop through mesh faces until a match is found
    FOR_ALL(localFaces, meshFaceI) {
      if (!meshFaceUsed[meshFaceI]
          && localFaces[meshFaceI].size() == curModelFace.size()) {
        // A possible match. A mesh face will be rotated, so make a copy
        labelList meshFaceLabels = localFaces[meshFaceI];
        for (label rotation = 0;
             rotation < meshFaceLabels.size();
             rotation++) {
          // try matching the face
          label nMatchedLabels = 0;
          FOR_ALL(meshFaceLabels, pointI) {
            if (pointLabels[curModelFace[pointI]] == meshFaceLabels[pointI]) {
              nMatchedLabels++;
            }
          }
          if (nMatchedLabels >= 2) {
            // match!
            found = true;
          }
          if (found) {
            // match found. Insert mesh face
            FOR_ALL(meshFaceLabels, pointI) {
              pointLabels[curModelFace[pointI]] =
                meshFaceLabels[pointI];
            }
            meshFaceUsed[meshFaceI] = true;
            break;
          } else {
            // No match found. Rotate face
            label firstLabel = meshFaceLabels[0];
            for (label i = 1; i < meshFaceLabels.size(); i++) {
              meshFaceLabels[i - 1] = meshFaceLabels[i];
            }
            meshFaceLabels.last() = firstLabel;
          }
        }
        if (found)
          break;
      }
    }
    if (!found) {
      // A model face is not matched. Shape detection failed
      FATAL_ERROR_IN
      (
        "create3DCellShape(const label cellIndex, "
        "const labelList& faceLabels, const labelListList& faces, "
        "const labelList& owner, const labelList& neighbour, "
        "const label fluentCellModelID)"
      )
      << "Cannot find match for face "
      << modelFaceI
      << ".\nModel: " << curModel.name() << " model face: "
      << curModelFace << " Mesh faces: " << localFaces
      << "Matched points: " << pointLabels
      << abort(FatalError);
    }
  }
  return cellShape{curModel, pointLabels};
}

}  // namespace mousse
