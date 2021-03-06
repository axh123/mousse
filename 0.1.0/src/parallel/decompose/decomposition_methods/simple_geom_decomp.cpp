// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "simple_geom_decomp.hpp"
#include "add_to_run_time_selection_table.hpp"
#include "sortable_list.hpp"
#include "global_index.hpp"
#include "sub_field.hpp"


namespace mousse {

DEFINE_TYPE_NAME_AND_DEBUG(simpleGeomDecomp, 0);
ADD_TO_RUN_TIME_SELECTION_TABLE
(
  decompositionMethod,
  simpleGeomDecomp,
  dictionary
);

}


// assignToProcessorGroup : given nCells cells and nProcGroup processor
// groups to share them, how do we share them out? Answer : each group
// gets nCells/nProcGroup cells, and the first few get one
// extra to make up the numbers. This should produce almost
// perfect load balancing
void mousse::simpleGeomDecomp::assignToProcessorGroup
(
  labelList& processorGroup,
  const label nProcGroup
) const
{
  label jump = processorGroup.size()/nProcGroup;
  label jumpb = jump + 1;
  label fstProcessorGroup = processorGroup.size() - jump*nProcGroup;
  label ind = 0;
  label j = 0;
  // assign cells to the first few processor groups (those with
  // one extra cell each
  for (j=0; j<fstProcessorGroup; j++) {
    for (label k=0; k<jumpb; k++) {
      processorGroup[ind++] = j;
    }
  }
  // and now to the `normal' processor groups
  for (; j<nProcGroup; j++) {
    for (label k=0; k<jump; k++) {
      processorGroup[ind++] = j;
    }
  }
}


void mousse::simpleGeomDecomp::assignToProcessorGroup
(
  labelList& processorGroup,
  const label nProcGroup,
  const labelList& indices,
  const scalarField& weights,
  const scalar summedWeights
) const
{
  // This routine gets the sorted points.
  // Easiest to explain with an example.
  // E.g. 400 points, sum of weights : 513.
  // Now with number of divisions in this direction (nProcGroup) : 4
  // gives the split at 513/4 = 128
  // So summed weight from 0..128 goes into bin 0,
  //     ,,              128..256 goes into bin 1
  //   etc.
  // Finally any remaining ones go into the last bin (3).
  const scalar jump = summedWeights/nProcGroup;
  const label nProcGroupM1 = nProcGroup - 1;
  scalar sumWeights = 0;
  label ind = 0;
  label j = 0;
  // assign cells to all except last group.
  for (j=0; j<nProcGroupM1; j++) {
    const scalar limit = jump*scalar(j + 1);
    while (sumWeights < limit) {
      sumWeights += weights[indices[ind]];
      processorGroup[ind++] = j;
    }
  }
  // Ensure last included.
  while (ind < processorGroup.size()) {
    processorGroup[ind++] = nProcGroupM1;
  }
}


mousse::labelList mousse::simpleGeomDecomp::decomposeOneProc
(
  const pointField& points
) const
{
  // construct a list for the final result
  labelList finalDecomp{points.size()};
  labelList processorGroups{points.size()};
  labelList pointIndices{points.size()};
  FOR_ALL(pointIndices, i) {
    pointIndices[i] = i;
  }
  const pointField rotatedPoints{rotDelta_ & points};
  // and one to take the processor group id's. For each direction.
  // we assign the processors to groups of processors labelled
  // 0..nX to give a banded structure on the mesh. Then we
  // construct the actual processor number by treating this as
  // the units part of the processor number.
  sort
    (
      pointIndices,
      UList<scalar>::less(rotatedPoints.component(vector::X))
    );
  assignToProcessorGroup(processorGroups, n_.x());
  FOR_ALL(points, i) {
    finalDecomp[pointIndices[i]] = processorGroups[i];
  }
  // now do the same thing in the Y direction. These processor group
  // numbers add multiples of nX to the proc. number (columns)
  sort
    (
      pointIndices,
      UList<scalar>::less(rotatedPoints.component(vector::Y))
    );
  assignToProcessorGroup(processorGroups, n_.y());
  FOR_ALL(points, i) {
    finalDecomp[pointIndices[i]] += n_.x()*processorGroups[i];
  }
  // finally in the Z direction. Now we add multiples of nX*nY to give
  // layers
  sort
    (
      pointIndices,
      UList<scalar>::less(rotatedPoints.component(vector::Z))
    );
  assignToProcessorGroup(processorGroups, n_.z());
  FOR_ALL(points, i) {
    finalDecomp[pointIndices[i]] += n_.x()*n_.y()*processorGroups[i];
  }
  return finalDecomp;
}


mousse::labelList mousse::simpleGeomDecomp::decomposeOneProc
(
  const pointField& points,
  const scalarField& weights
) const
{
  // construct a list for the final result
  labelList finalDecomp{points.size()};
  labelList processorGroups{points.size()};
  labelList pointIndices{points.size()};
  FOR_ALL(pointIndices, i) {
    pointIndices[i] = i;
  }
  typedef UList<scalar>::less less;
  const pointField rotatedPoints(rotDelta_ & points);
  // and one to take the processor group id's. For each direction.
  // we assign the processors to groups of processors labelled
  // 0..nX to give a banded structure on the mesh. Then we
  // construct the actual processor number by treating this as
  // the units part of the processor number.
  sort(pointIndices, less(rotatedPoints.component(vector::X)));
  const scalar summedWeights = sum(weights);
  assignToProcessorGroup
    (
      processorGroups,
      n_.x(),
      pointIndices,
      weights,
      summedWeights
    );
  FOR_ALL(points, i) {
    finalDecomp[pointIndices[i]] = processorGroups[i];
  }
  // now do the same thing in the Y direction. These processor group
  // numbers add multiples of nX to the proc. number (columns)
  sort(pointIndices, less(rotatedPoints.component(vector::Y)));
  assignToProcessorGroup
    (
      processorGroups,
      n_.y(),
      pointIndices,
      weights,
      summedWeights
    );
  FOR_ALL(points, i) {
    finalDecomp[pointIndices[i]] += n_.x()*processorGroups[i];
  }
  // finally in the Z direction. Now we add multiples of nX*nY to give
  // layers
  sort(pointIndices, less(rotatedPoints.component(vector::Z)));
  assignToProcessorGroup
    (
      processorGroups,
      n_.z(),
      pointIndices,
      weights,
      summedWeights
    );
  FOR_ALL(points, i) {
    finalDecomp[pointIndices[i]] += n_.x()*n_.y()*processorGroups[i];
  }
  return finalDecomp;
}


// Constructors 
mousse::simpleGeomDecomp::simpleGeomDecomp(const dictionary& decompositionDict)
:
  geomDecomp(decompositionDict, typeName)
{}


// Member Functions 
mousse::labelList mousse::simpleGeomDecomp::decompose
(
  const pointField& points
)
{
  if (!Pstream::parRun()) {
    return decomposeOneProc(points);
  } else {
    globalIndex globalNumbers{points.size()};
    // Collect all points on master
    if (Pstream::master()) {
      pointField allPoints{globalNumbers.size()};
      label nTotalPoints = 0;
      // Master first
      SubField<point>{allPoints, points.size()}.assign(points);
      nTotalPoints += points.size();
      // Add slaves
      for (int slave=1; slave<Pstream::nProcs(); slave++) {
        IPstream fromSlave{Pstream::scheduled, slave};
        pointField nbrPoints{fromSlave};
        SubField<point>{allPoints, nbrPoints.size(), nTotalPoints}
          .assign(nbrPoints);
        nTotalPoints += nbrPoints.size();
      }
      // Decompose
      labelList finalDecomp{decomposeOneProc(allPoints)};
      // Send back
      for (int slave=1; slave<Pstream::nProcs(); slave++) {
        OPstream toSlave{Pstream::scheduled, slave};
        toSlave <<
          SubField<label>
          {
            finalDecomp,
            globalNumbers.localSize(slave),
            globalNumbers.offset(slave)
          };
      }
      // Get my own part
      finalDecomp.setSize(points.size());
      return finalDecomp;
    } else {
      // Send my points
      {
        OPstream toMaster{Pstream::scheduled, Pstream::masterNo()};
        toMaster<< points;
      }
      // Receive back decomposition
      IPstream fromMaster{Pstream::scheduled, Pstream::masterNo()};
      labelList finalDecomp{fromMaster};
      return finalDecomp;
    }
  }
}


mousse::labelList mousse::simpleGeomDecomp::decompose
(
  const pointField& points,
  const scalarField& weights
)
{
  if (!Pstream::parRun()) {
    return decomposeOneProc(points, weights);
  } else {
    globalIndex globalNumbers{points.size()};
    // Collect all points on master
    if (Pstream::master()) {
      pointField allPoints{globalNumbers.size()};
      scalarField allWeights{allPoints.size()};
      label nTotalPoints = 0;
      // Master first
      SubField<point>{allPoints, points.size()}.assign(points);
      SubField<scalar>{allWeights, points.size()}.assign(weights);
      nTotalPoints += points.size();
      // Add slaves
      for (int slave=1; slave<Pstream::nProcs(); slave++) {
        IPstream fromSlave{Pstream::scheduled, slave};
        pointField nbrPoints{fromSlave};
        scalarField nbrWeights{fromSlave};
        SubField<point>(allPoints, nbrPoints.size(), nTotalPoints)
          .assign(nbrPoints);
        SubField<scalar>(allWeights, nbrWeights.size(), nTotalPoints)
          .assign(nbrWeights);
        nTotalPoints += nbrPoints.size();
      }
      // Decompose
      labelList finalDecomp{decomposeOneProc(allPoints, allWeights)};
      // Send back
      for (int slave=1; slave<Pstream::nProcs(); slave++) {
        OPstream toSlave{Pstream::scheduled, slave};
        toSlave <<
          SubField<label>
          {
            finalDecomp,
            globalNumbers.localSize(slave),
            globalNumbers.offset(slave)
          };
      }
      // Get my own part
      finalDecomp.setSize(points.size());
      return finalDecomp;
    } else {
      // Send my points
      {
        OPstream toMaster{Pstream::scheduled, Pstream::masterNo()};
        toMaster<< points << weights;
      }
      // Receive back decomposition
      IPstream fromMaster{Pstream::scheduled, Pstream::masterNo()};
      labelList finalDecomp{fromMaster};
      return finalDecomp;
    }
  }
}

