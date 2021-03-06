// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "hex_cell_looper.hpp"
#include "cell_features.hpp"
#include "poly_mesh.hpp"
#include "cell_modeller.hpp"
#include "plane.hpp"
#include "list_ops.hpp"
#include "mesh_tools.hpp"
#include "ofstream.hpp"
#include "add_to_run_time_selection_table.hpp"


// Static Data Members
namespace mousse {

DEFINE_TYPE_NAME_AND_DEBUG(hexCellLooper, 0);
ADD_TO_RUN_TIME_SELECTION_TABLE(cellLooper, hexCellLooper, word);

}


// Private Member Functions 

// Starting from cut edge start walking.
bool mousse::hexCellLooper::walkHex
(
  const label cellI,
  const label startFaceI,
  const label startEdgeI,
  labelList& loop,
  scalarField& loopWeights
) const
{
  label faceI = startFaceI;
  label edgeI = startEdgeI;
  label cutI = 0;
  do {
    if (debug & 2) {
      Pout << "    walkHex : inserting cut onto edge:" << edgeI
        << " vertices:" << mesh().edges()[edgeI] << endl;
    }
    // Store cut through edge. For now cut edges halfway.
    loop[cutI] = edgeToEVert(edgeI);
    loopWeights[cutI] = 0.5;
    cutI++;
    faceI = meshTools::otherFace(mesh(), cellI, faceI, edgeI);
    const edge& e = mesh().edges()[edgeI];
    // Walk two edges further
    edgeI = meshTools::walkFace(mesh(), faceI, edgeI, e.end(), 2);
    if (edgeI == startEdgeI) {
      break;
    }
  } while (true);
  // Checks.
  if (cutI > 4) {
    Pout << "hexCellLooper::walkHex" << "Problem : cell:" << cellI
      << " collected loop:";
    writeCuts(Pout, loop, loopWeights);
    Pout << "loopWeights:" << loopWeights << endl;
    return false;
  } else {
    return true;
  }
}


void mousse::hexCellLooper::makeFace
(
  const labelList& loop,
  const scalarField& loopWeights,
  labelList& faceVerts,
  pointField& facePoints
) const
{
  facePoints.setSize(loop.size());
  faceVerts.setSize(loop.size());
  FOR_ALL(loop, cutI) {
    label cut = loop[cutI];
    if (isEdge(cut)) {
      label edgeI = getEdge(cut);
      const edge& e = mesh().edges()[edgeI];
      const point& v0 = mesh().points()[e.start()];
      const point& v1 = mesh().points()[e.end()];
      facePoints[cutI] = loopWeights[cutI]*v1 + (1.0 - loopWeights[cutI])*v0;
    } else {
      label vertI = getVertex(cut);
      facePoints[cutI] = mesh().points()[vertI];
    }
    faceVerts[cutI] = cutI;
  }
}


// Constructors 
// Construct from components
mousse::hexCellLooper::hexCellLooper(const polyMesh& mesh)
:
  geomCellLooper{mesh},
  hex_{*(cellModeller::lookup("hex"))}
{}


// Destructor 
mousse::hexCellLooper::~hexCellLooper()
{}


// Member Functions 
bool mousse::hexCellLooper::cut
(
  const vector& refDir,
  const label cellI,
  const boolList& vertIsCut,
  const boolList& edgeIsCut,
  const scalarField& edgeWeight,
  labelList& loop,
  scalarField& loopWeights
) const
{
  bool success = false;
  if (mesh().cellShapes()[cellI].model() == hex_) {
    // Get starting edge. Note: should be compatible with way refDir is
    // determined.
    label edgeI = meshTools::cutDirToEdge(mesh(), cellI, refDir);
    // Get any face using edge
    label face0;
    label face1;
    meshTools::getEdgeFaces(mesh(), cellI, edgeI, face0, face1);
    // Walk circumference of hex, cutting edges only
    loop.setSize(4);
    loopWeights.setSize(4);
    success = walkHex(cellI, face0, edgeI, loop, loopWeights);
  } else {
    success = geomCellLooper::cut(refDir, cellI, vertIsCut, edgeIsCut,
                                  edgeWeight, loop, loopWeights);
  }
  if (debug) {
    if (loop.empty()) {
      WARNING_IN("hexCellLooper")
        << "could not cut cell " << cellI << endl;
      fileName cutsFile{"hexCellLooper_" + name(cellI) + ".obj"};
      Pout << "hexCellLooper : writing cell to " << cutsFile << endl;
      OFstream cutsStream{cutsFile};
      meshTools::writeOBJ(cutsStream, mesh().cells(), mesh().faces(),
                          mesh().points(), labelList{1, cellI});
      return false;
    }
    // Check for duplicate cuts.
    labelHashSet loopSet{loop.size()};
    FOR_ALL(loop, elemI) {
      label elem = loop[elemI];
      if (loopSet.found(elem)) {
        FATAL_ERROR_IN("hexCellLooper::walkHex") << " duplicate cut"
          << abort(FatalError);
      }
      loopSet.insert(elem);
    }
    face faceVerts{loop.size()};
    pointField facePoints{loop.size()};
    makeFace(loop, loopWeights, faceVerts, facePoints);
    if ((faceVerts.mag(facePoints) < SMALL) || (loop.size() < 3)) {
      FATAL_ERROR_IN("hexCellLooper::walkHex") << "Face:" << faceVerts
        << " on points:" << facePoints << endl
        << UIndirectList<point>(facePoints, faceVerts)()
        << abort(FatalError);
    }
  }
  return success;
}


// No shortcuts for cutting with arbitrary plane.
bool mousse::hexCellLooper::cut
(
  const plane& cutPlane,
  const label cellI,
  const boolList& vertIsCut,
  const boolList& edgeIsCut,
  const scalarField& edgeWeight,
  labelList& loop,
  scalarField& loopWeights
) const
{
  return geomCellLooper::cut(cutPlane, cellI, vertIsCut, edgeIsCut, edgeWeight,
                             loop, loopWeights);
}

