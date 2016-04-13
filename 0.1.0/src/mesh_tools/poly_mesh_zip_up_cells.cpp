// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "poly_mesh_zip_up_cells.hpp"
#include "poly_mesh.hpp"
#include "time.hpp"


// #define DEBUG_ZIPUP 1
// #define DEBUG_CHAIN 1
// #define DEBUG_ORDER 1

bool mousse::polyMeshZipUpCells(polyMesh& mesh)
{
  if (polyMesh::debug) {
    Info << "bool polyMeshZipUpCells(polyMesh& mesh) const: "
      << "zipping up topologically open cells" << endl;
  }
  // Algorithm:
  // Take the original mesh and visit all cells.  For every cell
  // calculate the edges of all faces on the cells.  A cell is
  // correctly topologically closed when all the edges are referenced
  // by exactly two faces.  If the edges are referenced only by a
  // single face, additional vertices need to be inserted into some
  // of the faces (topological closedness).  If an edge is
  // referenced by more that two faces, there is an error in
  // topological closedness.
  // Point insertion into the faces is done by attempting to create
  // closed loops and inserting the intermediate points into the
  // defining edge
  // Note:
  // The algorithm is recursive and changes the mesh faces in each
  // pass.  It is therefore essential to discard the addressing
  // after every pass.  The algorithm is completed when the mesh
  // stops changing.
  label nChangedFacesInMesh = 0;
  label nCycles = 0;
  labelHashSet problemCells;
  do {
    nChangedFacesInMesh = 0;
    const cellList& Cells = mesh.cells();
    const pointField& Points = mesh.points();
    faceList newFaces = mesh.faces();
    const faceList& oldFaces = mesh.faces();
    const labelListList& pFaces = mesh.pointFaces();
    FOR_ALL(Cells, cellI) {
      const labelList& curFaces = Cells[cellI];
      const edgeList cellEdges = Cells[cellI].edges(oldFaces);
      const labelList cellPoints = Cells[cellI].labels(oldFaces);
      // Find the edges used only once in the cell
      labelList edgeUsage{cellEdges.size(), 0};
      FOR_ALL(curFaces, faceI) {
        edgeList curFaceEdges = oldFaces[curFaces[faceI]].edges();
        FOR_ALL(curFaceEdges, faceEdgeI) {
          const edge& curEdge = curFaceEdges[faceEdgeI];
          FOR_ALL(cellEdges, cellEdgeI) {
            if (cellEdges[cellEdgeI] == curEdge) {
              edgeUsage[cellEdgeI]++;
              break;
            }
          }
        }
      }
      edgeList singleEdges{cellEdges.size()};
      label nSingleEdges = 0;
      FOR_ALL(edgeUsage, edgeI) {
        if (edgeUsage[edgeI] == 1) {
          singleEdges[nSingleEdges] = cellEdges[edgeI];
          nSingleEdges++;
        } else if (edgeUsage[edgeI] != 2) {
          WARNING_IN("void polyMeshZipUpCells(polyMesh& mesh)")
            << "edge " << cellEdges[edgeI] << " in cell " << cellI
            << " used " << edgeUsage[edgeI] << " times. " << nl
            << "Should be 1 or 2 - serious error "
            << "in mesh structure. " << endl;
#ifdef DEBUG_ZIPUP
          FOR_ALL(curFaces, faceI) {
            Info << "face: " << oldFaces[curFaces[faceI]] << endl;
          }
          Info << "Cell edges: " << cellEdges << nl
            << "Edge usage: " << edgeUsage << nl
            << "Cell points: " << cellPoints << endl;
          FOR_ALL(cellPoints, cpI) {
            Info << "vertex create \"" << cellPoints[cpI]
              << "\" coordinates "
              << Points[cellPoints[cpI]] << endl;
          }
#endif
          // Gather the problem cell
          problemCells.insert(cellI);
        }
      }
      // Check if the cell is already zipped up
      if (nSingleEdges == 0)
        continue;
      singleEdges.setSize(nSingleEdges);
#ifdef DEBUG_ZIPUP
      Info << "Cell " << cellI << endl;
      FOR_ALL(curFaces, faceI) {
        Info << "face: " << oldFaces[curFaces[faceI]] << endl;
      }
      Info
        << "Cell edges: " << cellEdges << nl
        << "Edge usage: " << edgeUsage << nl
        << "Single edges: " << singleEdges << nl
        << "Cell points: " << cellPoints << endl;
      FOR_ALL(cellPoints, cpI) {
        Info << "vertex create \"" << cellPoints[cpI]
          << "\" coordinates "
          << points()[cellPoints[cpI]] << endl;
      }
#endif
      // Loop through all single edges and mark the points they use
      // points marked twice are internal to edge; those marked more than
      // twice are corners
      labelList pointUsage{cellPoints.size(), 0};
      FOR_ALL(singleEdges, edgeI) {
        const edge& curEdge = singleEdges[edgeI];
        FOR_ALL(cellPoints, pointI) {
          if (cellPoints[pointI] == curEdge.start()
              || cellPoints[pointI] == curEdge.end()) {
            pointUsage[pointI]++;
          }
        }
      }
      boolList singleEdgeUsage{singleEdges.size(), false};
      // loop through all edges and eliminate the ones that are
      // blocked out
      FOR_ALL(singleEdges, edgeI) {
        bool blockedHead = false;
        bool blockedTail = false;
        label newEdgeStart = singleEdges[edgeI].start();
        label newEdgeEnd = singleEdges[edgeI].end();
        // check that the edge has not got all ends blocked
        FOR_ALL(cellPoints, pointI) {
          if (cellPoints[pointI] == newEdgeStart) {
            if (pointUsage[pointI] > 2) {
              blockedHead = true;
            }
          } else if (cellPoints[pointI] == newEdgeEnd) {
            if (pointUsage[pointI] > 2) {
              blockedTail = true;
            }
          }
        }
        if (blockedHead && blockedTail) {
          // Eliminating edge singleEdges[edgeI] as blocked
          singleEdgeUsage[edgeI] = true;
        }
      }
      // Go through the points and start from the point used twice
      // check all the edges to find the edges starting from this point
      // add the
      labelListList edgesToInsert{singleEdges.size()};
      label nEdgesToInsert = 0;
      // Find a good edge
      FOR_ALL(singleEdges, edgeI) {
        SLList<label> pointChain;
        bool blockHead = false;
        bool blockTail = false;
        if (!singleEdgeUsage[edgeI]) {
          // found a new edge
          singleEdgeUsage[edgeI] = true;
          label newEdgeStart = singleEdges[edgeI].start();
          label newEdgeEnd = singleEdges[edgeI].end();
          pointChain.insert(newEdgeStart);
          pointChain.append(newEdgeEnd);
#ifdef DEBUG_CHAIN
          Info << "found edge to start with: " << singleEdges[edgeI] << endl;
#endif
          // Check if head or tail are blocked
          FOR_ALL(cellPoints, pointI) {
            if (cellPoints[pointI] == newEdgeStart) {
              if (pointUsage[pointI] > 2) {
#ifdef DEBUG_CHAIN
                Info << "start head blocked" << endl;
#endif
                blockHead = true;
              }
            } else if (cellPoints[pointI] == newEdgeEnd) {
              if (pointUsage[pointI] > 2) {
#ifdef DEBUG_CHAIN
                Info << "start tail blocked" << endl;
#endif
                blockTail = true;
              }
            }
          }
          bool stopSearching = false;
          // Go through the unused edges and try to chain them up
          do {
            stopSearching = false;
            FOR_ALL(singleEdges, addEdgeI) {
              if (!singleEdgeUsage[addEdgeI]) {
                // Grab start and end of the candidate
                label addStart = singleEdges[addEdgeI].start();
                label addEnd = singleEdges[addEdgeI].end();
#ifdef DEBUG_CHAIN
                Info << "Trying candidate " << singleEdges[addEdgeI] << endl;
#endif
                // Try to add the edge onto the head
                if (!blockHead) {
                  if (pointChain.first() == addStart) {
                    // Added at start mark as used
                    pointChain.insert(addEnd);
                    singleEdgeUsage[addEdgeI] = true;
                  } else if (pointChain.first() == addEnd) {
                    pointChain.insert(addStart);
                    singleEdgeUsage[addEdgeI] = true;
                  }
                }
                // Try the other end only if the first end
                // did not add it
                if (!blockTail && !singleEdgeUsage[addEdgeI]) {
                  if (pointChain.last() == addStart) {
                    // Added at start mark as used
                    pointChain.append(addEnd);
                    singleEdgeUsage[addEdgeI] = true;
                  } else if (pointChain.last() == addEnd) {
                    pointChain.append(addStart);
                    singleEdgeUsage[addEdgeI] = true;
                  }
                }
                // check if the new head or tail are blocked
                label curEdgeStart = pointChain.first();
                label curEdgeEnd = pointChain.last();
#ifdef DEBUG_CHAIN
                Info << "curEdgeStart: " << curEdgeStart
                  << " curEdgeEnd: " << curEdgeEnd << endl;
#endif
                FOR_ALL(cellPoints, pointI) {
                  if (cellPoints[pointI] == curEdgeStart) {
                    if (pointUsage[pointI] > 2) {
#ifdef DEBUG_CHAIN
                      Info << "head blocked" << endl;
#endif
                      blockHead = true;
                    }
                  } else if (cellPoints[pointI] == curEdgeEnd) {
                    if (pointUsage[pointI] > 2) {
#ifdef DEBUG_CHAIN
                      Info << "tail blocked" << endl;
#endif
                      blockTail = true;
                    }
                  }
                }
                // Check if the loop is closed
                if (curEdgeStart == curEdgeEnd) {
#ifdef DEBUG_CHAIN
                  Info << "closed loop" << endl;
#endif
                  pointChain.removeHead();
                  blockHead = true;
                  blockTail = true;
                  stopSearching = true;
                }
#ifdef DEBUG_CHAIN
                Info << "current pointChain: " << pointChain << endl;
#endif
                if (stopSearching)
                  break;
              }
            }
          } while (stopSearching);
        }
#ifdef DEBUG_CHAIN
        Info << "completed patch chain: " << pointChain << endl;
#endif
        if (pointChain.size() > 2) {
          edgesToInsert[nEdgesToInsert] = pointChain;
          nEdgesToInsert++;
        }
      }
      edgesToInsert.setSize(nEdgesToInsert);
#ifdef DEBUG_ZIPUP
      Info << "edgesToInsert: " << edgesToInsert << endl;
#endif
      // Insert the edges into a list of faces
      FOR_ALL(edgesToInsert, edgeToInsertI) {
        // Order the points of the edge
        // Warning: the ordering must be parametric, because in
        // the case of multiple point insertion onto the same edge
        // it is possible to get non-cyclic loops
        //
        const labelList& unorderedEdge = edgesToInsert[edgeToInsertI];
        scalarField dist{unorderedEdge.size()};
        // Calculate distance
        point startPoint = Points[unorderedEdge[0]];
        dist[0] = 0;
        vector dir = Points[unorderedEdge.last()] - startPoint;
        for (label i = 1; i < dist.size(); i++) {
          dist[i] = (Points[unorderedEdge[i]] - startPoint) & dir;
        }
        // Sort points
        labelList orderedEdge{unorderedEdge.size(), -1};
        boolList used{unorderedEdge.size(), false};
        FOR_ALL(orderedEdge, epI) {
          label nextPoint = -1;
          scalar minDist = GREAT;
          FOR_ALL(dist, i) {
            if (!used[i] && dist[i] < minDist) {
              minDist = dist[i];
              nextPoint = i;
            }
          }
          // Insert the next point
          orderedEdge[epI] = unorderedEdge[nextPoint];
          used[nextPoint] = true;
        }
#ifdef DEBUG_ORDER
        Info << "unorderedEdge: " << unorderedEdge << nl
          << "orderedEdge: " << orderedEdge << endl;
#endif
        // check for duplicate points in the ordered edge
        FOR_ALL(orderedEdge, checkI) {
          for
          (
            label checkJ = checkI + 1;
            checkJ < orderedEdge.size();
            checkJ++
          ) {
            if (orderedEdge[checkI] == orderedEdge[checkJ]) {
              WARNING_IN("void polyMeshZipUpCells(polyMesh& mesh)")
                << "Duplicate point found in edge to insert. "
                << nl << "Point: " << orderedEdge[checkI]
                << " edge: " << orderedEdge << endl;
              problemCells.insert(cellI);
            }
          }
        }
        edge testEdge
        {
          orderedEdge[0],
          orderedEdge.last()
        };
        // In order to avoid edge-to-edge comparison, get faces using
        // point-face addressing in two goes.
        const labelList& startPF = pFaces[testEdge.start()];
        const labelList& endPF = pFaces[testEdge.start()];
        labelList facesSharingEdge{startPF.size() + endPF.size()};
        label nfse = 0;
        FOR_ALL(startPF, pfI) {
          facesSharingEdge[nfse++] = startPF[pfI];
        }
        FOR_ALL(endPF, pfI) {
          facesSharingEdge[nfse++] = endPF[pfI];
        }
        FOR_ALL(facesSharingEdge, faceI) {
          bool faceChanges = false;
          // Label of the face being analysed
          const label currentFaceIndex = facesSharingEdge[faceI];
          const edgeList curFaceEdges =
            oldFaces[currentFaceIndex].edges();
          FOR_ALL(curFaceEdges, cfeI) {
            if (curFaceEdges[cfeI] == testEdge) {
              faceChanges = true;
              break;
            }
          }
          if (faceChanges) {
            nChangedFacesInMesh++;
            // In order to avoid loosing point from multiple
            // insertions into the same face, the new face
            // will be change incrementally.
            // 1) Check if all the internal points of the edge
            // to add already exist in the face. If so, the
            // edge has already been included 2) Check if the
            // point insertion occurs on an edge which is
            // still untouched. If so, simply insert
            // additional points into the face.  3) If not,
            // the edge insertion occurs on an already
            // modified edge. ???
            face& newFace = newFaces[currentFaceIndex];
            bool allPointsPresent = true;
            FOR_ALL(orderedEdge, oeI) {
              bool curPointFound = false;
              FOR_ALL(newFace, nfI) {
                if (newFace[nfI] == orderedEdge[oeI]) {
                  curPointFound = true;
                  break;
                }
              }
              allPointsPresent = allPointsPresent && curPointFound;
            }
#ifdef DEBUG_ZIPUP
            if (allPointsPresent) {
              Info << "All points present" << endl;
            }
#endif
            if (!allPointsPresent) {
              // Not all points are already present.  The
              // new edge will need to be inserted into the
              // face.
              // Check to see if a new edge fits onto an
              // untouched edge of the face.  Make sure the
              // edges are grabbed before the face is
              // resized.
              edgeList newFaceEdges = newFace.edges();
#ifdef DEBUG_ZIPUP
              Info << "Not all points present." << endl;
#endif
              label nNewFacePoints = 0;
              bool edgeAdded = false;
              FOR_ALL(newFaceEdges, curFacEdgI) {
                // Does the current edge change?
                if (newFaceEdges[curFacEdgI] == testEdge) {
                  // Found an edge match
                  edgeAdded = true;
                  // Resize the face to accept additional
                  // points
                  newFace.setSize
                  (
                    newFace.size() + orderedEdge.size() - 2
                  );
                  if (newFaceEdges[curFacEdgI].start() == testEdge.start()) {
                    // insertion in ascending order
                    for
                    (
                      label i = 0;
                      i < orderedEdge.size() - 1;
                      i++
                    ) {
                      newFace[nNewFacePoints] = orderedEdge[i];
                      nNewFacePoints++;
                    }
                  } else {
                    // insertion in reverse order
                    for
                    (
                      label i = orderedEdge.size() - 1;
                      i > 0;
                      i--
                    ) {
                      newFace[nNewFacePoints] = orderedEdge[i];
                      nNewFacePoints++;
                    }
                  }
                } else {
                  // Does not fit onto this edge.
                  // Copy the next point into the face
                  newFace[nNewFacePoints] =
                    newFaceEdges[curFacEdgI].start();
                  nNewFacePoints++;
                }
              }
#ifdef DEBUG_ZIPUP
              Info
                << "oldFace: " << oldFaces[currentFaceIndex] << nl
                << "newFace: " << newFace << endl;
#endif
              // Check for duplicate points in the new face
              FOR_ALL(newFace, checkI) {
                for
                (
                  label checkJ = checkI + 1;
                  checkJ < newFace.size();
                  checkJ++
                ) {
                  if (newFace[checkI] == newFace[checkJ]) {
                    WARNING_IN
                    (
                      "void polyMeshZipUpCells"
                      "("
                        "polyMesh& mesh"
                      ")"
                    )
                    << "Duplicate point found "
                    << "in the new face. " << nl
                    << "Point: "
                    << orderedEdge[checkI]
                    << " face: "
                    << newFace << endl;
                    problemCells.insert(cellI);
                  }
                }
              }
              // Check if the edge is added.
              // If not, then it comes on top of an already
              // modified edge and they need to be
              // merged in together.
              if (!edgeAdded) {
                Info << "This edge modifies an already modified "
                  << "edge.  Point insertions skipped."
                  << endl;
              }
            }
          }
        }
      }
    }
    if (problemCells.size()) {
      // This cycle has failed.  Print out the problem cells
      labelList toc{problemCells.toc()};
      sort(toc);
      FATAL_ERROR_IN("void polyMeshZipUpCells(polyMesh& mesh)")
        << "Found " << problemCells.size() << " problem cells." << nl
        << "Cells: " << toc
        << abort(FatalError);
    }
    Info << "Cycle " << ++nCycles
      << " changed " << nChangedFacesInMesh << " faces." << endl;
    const polyBoundaryMesh& bMesh = mesh.boundaryMesh();
    // Reset the polyMesh. Number of points/faces/cells/patches stays the
    // same, only the faces themselves have changed so clear all derived
    // (edge, point) addressing.
    // Collect the patch sizes
    labelList patchSizes{bMesh.size(), 0};
    labelList patchStarts{bMesh.size(), 0};
    FOR_ALL(bMesh, patchI) {
      patchSizes[patchI] = bMesh[patchI].size();
      patchStarts[patchI] = bMesh[patchI].start();
    }
    // Reset the mesh. Number of active faces is one beyond the last patch
    // (patches guaranteed to be in increasing order)
    mesh.resetPrimitives
    (
      Xfer<pointField>::null(),
      xferMove(newFaces),
      Xfer<labelList>::null(),
      Xfer<labelList>::null(),
      patchSizes,
      patchStarts,
      true                // boundary forms valid boundary mesh.
    );
    // Reset any addressing on face zones.
    mesh.faceZones().clearAddressing();
    // Clear the addressing
    mesh.clearOut();
  } while (nChangedFacesInMesh > 0 || nCycles > 100);
  // Flags the mesh files as being changed
  mesh.setInstance(mesh.time().timeName());
  if (nChangedFacesInMesh > 0) {
    FATAL_ERROR_IN("void polyMeshZipUpCells(polyMesh& mesh)")
      << "cell zip-up failed after 100 cycles.  Probable problem "
      << "with the original mesh"
      << abort(FatalError);
  }
  return nCycles != 1;
}
