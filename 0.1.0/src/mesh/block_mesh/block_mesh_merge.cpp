// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "block_mesh.hpp"


// Private Member Functions 
void mousse::blockMesh::calcMergeInfo()
{
  const blockList& blocks = *this;
  if (verboseOutput) {
    Info << "Creating block offsets" << endl;
  }
  blockOffsets_.setSize(blocks.size());
  nPoints_ = 0;
  nCells_  = 0;
  FOR_ALL(blocks, blockI) {
    blockOffsets_[blockI] = nPoints_;
    nPoints_ += blocks[blockI].nPoints();
    nCells_  += blocks[blockI].nCells();
  }
  if (verboseOutput) {
    Info << "Creating merge list " << flush;
  }
  // set unused to -1
  mergeList_.setSize(nPoints_);
  mergeList_ = -1;
  const pointField& blockPoints = topology().points();
  const cellList& blockCells = topology().cells();
  const faceList& blockFaces = topology().faces();
  const labelList& faceOwnerBlocks = topology().faceOwner();
  // For efficiency, create merge pairs in the first pass
  labelListListList glueMergePairs{blockFaces.size()};
  const labelList& faceNeighbourBlocks = topology().faceNeighbour();
  FOR_ALL(blockFaces, blockFaceLabel) {
    label blockPlabel = faceOwnerBlocks[blockFaceLabel];
    const pointField& blockPpoints = blocks[blockPlabel].points();
    const labelList& blockPfaces = blockCells[blockPlabel];
    bool foundFace = false;
    label blockPfaceLabel;
    for (blockPfaceLabel = 0;
         blockPfaceLabel < blockPfaces.size();
         blockPfaceLabel++) {
      if (blockPfaces[blockPfaceLabel] == blockFaceLabel) {
        foundFace = true;
        break;
      }
    }
    if (!foundFace) {
      FATAL_ERROR_IN("blockMesh::calcMergeInfo()")
        << "Cannot find merge face for block " << blockPlabel
        << exit(FatalError);
    }
    const labelListList& blockPfaceFaces =
      blocks[blockPlabel].boundaryPatches()[blockPfaceLabel];
    labelListList& curPairs = glueMergePairs[blockFaceLabel];
    curPairs.setSize(blockPfaceFaces.size());
    // Calculate sqr of the merge tolerance as 1/10th of the min sqr
    // point to point distance on the block face.
    // At the same time merge collated points on the block's faces
    // (removes boundary poles etc.)
    // Collated points detected by initally taking a constant factor of
    // the size of the block.
    boundBox bb{blockCells[blockPlabel].points(blockFaces, blockPoints)};
    const scalar mergeSqrDist = magSqr(10*SMALL*bb.span());
    // This is an N^2 algorithm
    scalar sqrMergeTol = GREAT;
    FOR_ALL(blockPfaceFaces, blockPfaceFaceLabel) {
      const labelList& blockPfaceFacePoints
        = blockPfaceFaces[blockPfaceFaceLabel];
      FOR_ALL(blockPfaceFacePoints, blockPfaceFacePointLabel) {
        FOR_ALL(blockPfaceFacePoints, blockPfaceFacePointLabel2) {
          if (blockPfaceFacePointLabel != blockPfaceFacePointLabel2) {
            scalar magSqrDist =
              magSqr
              (
                blockPpoints[blockPfaceFacePoints[blockPfaceFacePointLabel]]
                - blockPpoints[blockPfaceFacePoints[blockPfaceFacePointLabel2]]
              );
            if (magSqrDist < mergeSqrDist)
            {
              label PpointLabel =
                blockPfaceFacePoints[blockPfaceFacePointLabel]
                + blockOffsets_[blockPlabel];
              label PpointLabel2 =
                blockPfaceFacePoints[blockPfaceFacePointLabel2]
                + blockOffsets_[blockPlabel];
              label minPP2 = min(PpointLabel, PpointLabel2);
              if (mergeList_[PpointLabel] != -1) {
                minPP2 = min(minPP2, mergeList_[PpointLabel]);
              }
              if (mergeList_[PpointLabel2] != -1) {
                minPP2 = min(minPP2, mergeList_[PpointLabel2]);
              }
              mergeList_[PpointLabel] = mergeList_[PpointLabel2]
                = minPP2;
            } else {
              sqrMergeTol = min(sqrMergeTol, magSqrDist);
            }
          }
        }
      }
    }
    sqrMergeTol /= 10.0;
    if (topology().isInternalFace(blockFaceLabel)) {
      label blockNlabel = faceNeighbourBlocks[blockFaceLabel];
      const pointField& blockNpoints = blocks[blockNlabel].points();
      const labelList& blockNfaces = blockCells[blockNlabel];
      foundFace = false;
      label blockNfaceLabel;
      for (blockNfaceLabel = 0;
           blockNfaceLabel < blockNfaces.size();
           blockNfaceLabel++) {
          if (blockFaces[blockNfaces[blockNfaceLabel]]
              == blockFaces[blockFaceLabel]) {
              foundFace = true;
              break;
            }
        }
      if (!foundFace) {
        FATAL_ERROR_IN("blockMesh::calcMergeInfo()")
          << "Cannot find merge face for block " << blockNlabel
          << exit(FatalError);
      }
      const labelListList& blockNfaceFaces =
        blocks[blockNlabel].boundaryPatches()[blockNfaceLabel];
      if (blockPfaceFaces.size() != blockNfaceFaces.size()) {
        FATAL_ERROR_IN("blockMesh::calcMergeInfo()")
          << "Inconsistent number of faces between block pair "
          << blockPlabel << " and " << blockNlabel
          << exit(FatalError);
      }
      // N-squared point search over all points of all faces of
      // master block over all point of all faces of slave block
      FOR_ALL(blockPfaceFaces, blockPfaceFaceLabel) {
        const labelList& blockPfaceFacePoints
          = blockPfaceFaces[blockPfaceFaceLabel];
        labelList& cp = curPairs[blockPfaceFaceLabel];
        cp.setSize(blockPfaceFacePoints.size());
        cp = -1;
        FOR_ALL(blockPfaceFacePoints, blockPfaceFacePointLabel) {
          FOR_ALL(blockNfaceFaces, blockNfaceFaceLabel) {
            const labelList& blockNfaceFacePoints
              = blockNfaceFaces[blockNfaceFaceLabel];
            FOR_ALL(blockNfaceFacePoints, blockNfaceFacePointLabel) {
              if (magSqr
                  (
                    blockPpoints[blockPfaceFacePoints[blockPfaceFacePointLabel]]
                    - blockNpoints[blockNfaceFacePoints[blockNfaceFacePointLabel]]
                  ) < sqrMergeTol) {
                  // Found a new pair
                  cp[blockPfaceFacePointLabel] =
                    blockNfaceFacePoints[blockNfaceFacePointLabel];
                  label PpointLabel =
                    blockPfaceFacePoints[blockPfaceFacePointLabel]
                    + blockOffsets_[blockPlabel];
                  label NpointLabel =
                    blockNfaceFacePoints[blockNfaceFacePointLabel]
                    + blockOffsets_[blockNlabel];
                  label minPN = min(PpointLabel, NpointLabel);
                  if (mergeList_[PpointLabel] != -1) {
                    minPN = min(minPN, mergeList_[PpointLabel]);
                  }
                  if (mergeList_[NpointLabel] != -1) {
                    minPN = min(minPN, mergeList_[NpointLabel]);
                  }
                  mergeList_[PpointLabel] = mergeList_[NpointLabel]
                    = minPN;
                }
            }
          }
        }
        FOR_ALL(blockPfaceFacePoints, blockPfaceFacePointLabel) {
          if (cp[blockPfaceFacePointLabel] == -1) {
            FATAL_ERROR_IN("blockMesh::calcMergeInfo()")
              << "Inconsistent point locations between block pair "
              << blockPlabel << " and " << blockNlabel << nl
              << "    probably due to inconsistent grading."
              << exit(FatalError);
          }
        }
      }
    }
  }
  const faceList::subList blockInternalFaces
  {
    blockFaces,
    topology().nInternalFaces()
  };
  bool changedPointMerge = false;
  label nPasses = 0;
  do {
    changedPointMerge = false;
    nPasses++;
    FOR_ALL(blockInternalFaces, blockFaceLabel) {
      label blockPlabel = faceOwnerBlocks[blockFaceLabel];
      label blockNlabel = faceNeighbourBlocks[blockFaceLabel];
      const labelList& blockPfaces = blockCells[blockPlabel];
      const labelList& blockNfaces = blockCells[blockNlabel];
      const labelListList& curPairs = glueMergePairs[blockFaceLabel];
      label blockPfaceLabel;
      for (blockPfaceLabel = 0;
           blockPfaceLabel < blockPfaces.size();
           blockPfaceLabel++) {
        if (blockFaces[blockPfaces[blockPfaceLabel]]
            == blockInternalFaces[blockFaceLabel]) {
          break;
        }
      }
      label blockNfaceLabel;
      for (blockNfaceLabel = 0;
           blockNfaceLabel < blockNfaces.size();
           blockNfaceLabel++) {
        if (blockFaces[blockNfaces[blockNfaceLabel]]
            == blockInternalFaces[blockFaceLabel]) {
          break;
        }
      }
      const labelListList& blockPfaceFaces =
        blocks[blockPlabel].boundaryPatches()[blockPfaceLabel];
      FOR_ALL(blockPfaceFaces, blockPfaceFaceLabel) {
        const labelList& blockPfaceFacePoints
          = blockPfaceFaces[blockPfaceFaceLabel];
        const labelList& cp = curPairs[blockPfaceFaceLabel];
        FOR_ALL(blockPfaceFacePoints, blockPfaceFacePointLabel) {
          label PpointLabel =
            blockPfaceFacePoints[blockPfaceFacePointLabel]
            + blockOffsets_[blockPlabel];
          label NpointLabel =
            cp[blockPfaceFacePointLabel]
            + blockOffsets_[blockNlabel];
          if (mergeList_[PpointLabel] != mergeList_[NpointLabel]) {
            changedPointMerge = true;
            mergeList_[PpointLabel]
              = mergeList_[NpointLabel]
              = min(mergeList_[PpointLabel], mergeList_[NpointLabel]);
          }
        }
      }
    }
    if (verboseOutput) {
      Info << "." << flush;
    }
    if (nPasses > 100) {
      FATAL_ERROR_IN("blockMesh::calcMergeInfo()")
        << "Point merging failed after max number of passes."
        << exit(FatalError);
    }
  } while (changedPointMerge);
  if (verboseOutput) {
    Info << endl;
  }
  FOR_ALL(blockInternalFaces, blockFaceLabel) {
    label blockPlabel = faceOwnerBlocks[blockFaceLabel];
    label blockNlabel = faceNeighbourBlocks[blockFaceLabel];
    const labelList& blockPfaces = blockCells[blockPlabel];
    const labelList& blockNfaces = blockCells[blockNlabel];
    const pointField& blockPpoints = blocks[blockPlabel].points();
    const pointField& blockNpoints = blocks[blockNlabel].points();
    bool foundFace = false;
    label blockPfaceLabel;
    for (blockPfaceLabel = 0;
         blockPfaceLabel < blockPfaces.size();
         blockPfaceLabel++) {
      if (blockFaces[blockPfaces[blockPfaceLabel]]
          == blockInternalFaces[blockFaceLabel]) {
        foundFace = true;
        break;
      }
    }
    if (!foundFace) {
      FATAL_ERROR_IN("blockMesh::calcMergeInfo()")
        << "Cannot find merge face for block " << blockPlabel
        << exit(FatalError);
    }
    foundFace = false;
    label blockNfaceLabel;
    for (blockNfaceLabel = 0;
         blockNfaceLabel < blockNfaces.size();
         blockNfaceLabel++) {
      if (blockFaces[blockNfaces[blockNfaceLabel]]
          == blockInternalFaces[blockFaceLabel]) {
        foundFace = true;
        break;
      }
    }
    if (!foundFace) {
      FATAL_ERROR_IN("blockMesh::calcMergeInfo()")
        << "Cannot find merge face for block " << blockNlabel
        << exit(FatalError);
    }
    const labelListList& blockPfaceFaces =
      blocks[blockPlabel].boundaryPatches()[blockPfaceLabel];
    const labelListList& blockNfaceFaces =
      blocks[blockNlabel].boundaryPatches()[blockNfaceLabel];
    FOR_ALL(blockPfaceFaces, blockPfaceFaceLabel) {
      const labelList& blockPfaceFacePoints
        = blockPfaceFaces[blockPfaceFaceLabel];
      FOR_ALL(blockPfaceFacePoints, blockPfaceFacePointLabel) {
        label PpointLabel =
          blockPfaceFacePoints[blockPfaceFacePointLabel]
          + blockOffsets_[blockPlabel];
        if (mergeList_[PpointLabel] == -1) {
          FATAL_ERROR_IN("blockMesh::calcMergeInfo()")
            << "Unable to merge point "
            << blockPfaceFacePointLabel
            << ' ' << blockPpoints[blockPfaceFacePointLabel]
            << " of face "
            << blockPfaceLabel
            << " of block "
            << blockPlabel
            << exit(FatalError);
        }
      }
    }
    FOR_ALL(blockNfaceFaces, blockNfaceFaceLabel) {
      const labelList& blockNfaceFacePoints
        = blockNfaceFaces[blockNfaceFaceLabel];
      FOR_ALL(blockNfaceFacePoints, blockNfaceFacePointLabel) {
        label NpointLabel =
          blockNfaceFacePoints[blockNfaceFacePointLabel]
          + blockOffsets_[blockNlabel];
        if (mergeList_[NpointLabel] == -1) {
          FATAL_ERROR_IN("blockMesh::calcMergeInfo()")
            << "unable to merge point "
            << blockNfaceFacePointLabel
            << ' ' << blockNpoints[blockNfaceFacePointLabel]
            << " of face "
            << blockNfaceLabel
            << " of block "
            << blockNlabel
            << exit(FatalError);
        }
      }
    }
  }
  // Sort merge list to return new point label (in new shorter list)
  // given old point label
  label newPointLabel = 0;
  FOR_ALL(mergeList_, pointLabel) {
    if (mergeList_[pointLabel] > pointLabel) {
      FATAL_ERROR_IN("blockMesh::calcMergeInfo()")
        << "Merge list contains point index out of range"
        << exit(FatalError);
    }
    if (mergeList_[pointLabel] == -1
        || mergeList_[pointLabel] == pointLabel) {
      mergeList_[pointLabel] = newPointLabel;
      newPointLabel++;
    } else {
      mergeList_[pointLabel] = mergeList_[mergeList_[pointLabel]];
    }
  }
  nPoints_ = newPointLabel;
}

