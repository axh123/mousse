// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "cell_matcher.hpp"
#include "primitive_mesh.hpp"
#include "map.hpp"
#include "face_list.hpp"
#include "label_list.hpp"
#include "list_ops.hpp"


// Constructors 
mousse::cellMatcher::cellMatcher
(
  const label vertPerCell,
  const label facePerCell,
  const label maxVertPerFace,
  const word& cellModelName
)
:
  localPoint_{100},
  localFaces_{facePerCell},
  faceSize_{facePerCell, -1},
  pointMap_{vertPerCell},
  faceMap_{facePerCell},
  edgeFaces_{2*vertPerCell*vertPerCell},
  pointFaceIndex_{vertPerCell},
  vertLabels_{vertPerCell},
  faceLabels_{facePerCell},
  cellModelName_{cellModelName},
  cellModelPtr_{NULL}
{
  FOR_ALL(localFaces_, faceI) {
    face& f = localFaces_[faceI];
    f.setSize(maxVertPerFace);
  }
  FOR_ALL(pointFaceIndex_, vertI) {
    pointFaceIndex_[vertI].setSize(facePerCell);
  }
}


// Member Functions 
// Create localFaces_ , pointMap_ , faceMap_
mousse::label mousse::cellMatcher::calcLocalFaces
(
  const faceList& faces,
  const labelList& myFaces
)
{
  // Clear map from global to cell numbering
  localPoint_.clear();
  // Renumber face vertices and insert directly into localFaces_
  label newVertI = 0;
  FOR_ALL(myFaces, myFaceI) {
    label faceI = myFaces[myFaceI];
    const face& f = faces[faceI];
    face& localFace = localFaces_[myFaceI];
    // Size of localFace
    faceSize_[myFaceI] = f.size();
    FOR_ALL(f, localVertI) {
      label vertI = f[localVertI];
      Map<label>::iterator iter = localPoint_.find(vertI);
      if (iter == localPoint_.end()) {
        // Not found. Assign local vertex number.
        if (newVertI >= pointMap_.size()) {
          // Illegal face: more unique vertices than vertPerCell
          return -1;
        }
        localFace[localVertI] = newVertI;
        localPoint_.insert(vertI, newVertI);
        newVertI++;
      } else {
        // Reuse local vertex number.
        localFace[localVertI] = *iter;
      }
    }
    // Create face from localvertex labels
    faceMap_[myFaceI] = faceI;
  }
  // Create local to global vertex mapping
  FOR_ALL_CONST_ITER(Map<label>, localPoint_, iter) {
    const label fp = iter();
    pointMap_[fp] = iter.key();
  }
  ////debug
  //write(Info);
  return newVertI;
}


// Create edgeFaces_ : map from edge to two localFaces for single cell.
void mousse::cellMatcher::calcEdgeAddressing(const label numVert)
{
  edgeFaces_ = -1;
  FOR_ALL(localFaces_, localFaceI) {
    const face& f = localFaces_[localFaceI];
    label prevVertI = faceSize_[localFaceI] - 1;
    //FOR_ALL(f, fp)
    for
    (
      label fp = 0;
      fp < faceSize_[localFaceI];
      fp++
    ) {
      label start = f[prevVertI];
      label end = f[fp];
      label key1 = edgeKey(numVert, start, end);
      label key2 = edgeKey(numVert, end, start);
      if (edgeFaces_[key1] == -1) {
        // Entry key1 unoccupied. Store both permutations.
        edgeFaces_[key1] = localFaceI;
        edgeFaces_[key2] = localFaceI;
      } else if (edgeFaces_[key1+1] == -1) {
        // Entry key1+1 unoccupied
        edgeFaces_[key1+1] = localFaceI;
        edgeFaces_[key2+1] = localFaceI;
      } else {
        FATAL_ERROR_IN
        (
          "calcEdgeAddressing"
          "(const faceList&, const label)"
        )
        << "edgeFaces_ full at entry:" << key1
        << " for edge " << start << " " << end
        << abort(FatalError);
      }
      prevVertI = fp;
    }
  }
}


// Create pointFaceIndex_ : map from vertI, faceI to index of vertI on faceI.
void mousse::cellMatcher::calcPointFaceIndex()
{
  // Fill pointFaceIndex_ with -1
  FOR_ALL(pointFaceIndex_, i) {
    labelList& faceIndices = pointFaceIndex_[i];
    faceIndices = -1;
  }
  FOR_ALL(localFaces_, localFaceI) {
    const face& f = localFaces_[localFaceI];
    for
    (
      label fp = 0;
      fp < faceSize_[localFaceI];
      fp++
    ) {
      label vert = f[fp];
      pointFaceIndex_[vert][localFaceI] = fp;
    }
  }
}


// Given edge(v0,v1) and (local)faceI return the other face
mousse::label mousse::cellMatcher::otherFace
(
  const label numVert,
  const label v0,
  const label v1,
  const label localFaceI
) const
{
  label key = edgeKey(numVert, v0, v1);
  if (edgeFaces_[key] == localFaceI) {
    return edgeFaces_[key+1];
  } else if (edgeFaces_[key+1] == localFaceI) {
    return edgeFaces_[key];
  } else {
    FATAL_ERROR_IN
    (
      "otherFace"
      "(const label, const labelList&, const label, const label, "
      "const label)"
    )
    << "edgeFaces_ does not contain:" << localFaceI
    << " for edge " << v0 << " " << v1 << " at key " << key
    << " edgeFaces_[key, key+1]:" <<  edgeFaces_[key]
    << " , " << edgeFaces_[key+1]
    << abort(FatalError);
    return -1;
  }
}


void mousse::cellMatcher::write(mousse::Ostream& os) const
{
  os << "Faces:" << endl;
  FOR_ALL(localFaces_, faceI) {
    os << "    ";
    for (label fp = 0; fp < faceSize_[faceI]; fp++) {
      os << ' ' << localFaces_[faceI][fp];
    }
    os << endl;
  }
  os <<  "Face map  : " << faceMap_ << endl;
  os <<  "Point map : " << pointMap_ << endl;
}
