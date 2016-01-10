// mousse: CFD toolbox
// Copyright (C) 2011-2012 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "comm_schedule.hpp"
#include "sortable_list.hpp"
#include "bool_list.hpp"
#include "iostreams.hpp"
#include "iomanip.hpp"
#include "ostring_stream.hpp"
#include "pstream.hpp"

// Static Data Members
namespace mousse {

DEFINE_TYPE_NAME_AND_DEBUG(commSchedule, 0);

}

// Private Member Functions 
mousse::label mousse::commSchedule::outstandingComms
(
  const labelList& commToSchedule,
  DynamicList<label>& procComms
) const
{
  label nOutstanding = 0;
  FOR_ALL(procComms, i)
  {
    if (commToSchedule[procComms[i]] == -1)
    {
      nOutstanding++;
    }
  }
  return nOutstanding;
}
// Constructors 
// Construct from separate addressing
mousse::commSchedule::commSchedule
(
  const label nProcs,
  const List<labelPair>& comms
)
:
  schedule_(comms.size()),
  procSchedule_(nProcs)
{
  // Determine comms per processor.
  List<DynamicList<label> > procToComms(nProcs);
  FOR_ALL(comms, commI)
  {
    label proc0 = comms[commI][0];
    label proc1 = comms[commI][1];
    if (proc0 < 0 || proc0 >= nProcs || proc1 < 0 || proc1 >= nProcs)
    {
      FATAL_ERROR_IN
      (
        "commSchedule::commSchedule"
        "(const label, const List<labelPair>&)"
      )   << "Illegal processor " << comms[commI] << abort(FatalError);
    }
    procToComms[proc0].append(commI);
    procToComms[proc1].append(commI);
  }
  // Note: no need to shrink procToComms. Are small.
  if (debug && Pstream::master())
  {
    Pout<< "commSchedule::commSchedule : Wanted communication:" << endl;
    FOR_ALL(comms, i)
    {
      const labelPair& twoProcs = comms[i];
      Pout<< i << ": "
        << twoProcs[0] << " with " << twoProcs[1] << endl;
    }
    Pout<< endl;
    Pout<< "commSchedule::commSchedule : Schedule:" << endl;
    // Print header. Use buffered output to prevent parallel output messing
    // up.
    {
      OStringStream os;
      os  << "iter|";
      for (int i = 0; i < nProcs; i++)
      {
        os  << setw(3) << i;
      }
      Pout<< os.str().c_str() << endl;
    }
    {
      OStringStream os;
      os  << "----+";
      for (int i = 0; i < nProcs; i++)
      {
        os  << "---";
      }
      Pout<< os.str().c_str() << endl;
    }
  }
  // Schedule all. Note: crap scheduler. Assumes all communication takes
  // equally long.
  label nScheduled = 0;
  label iter = 0;
  // Per index into comms the time when it was scheduled
  labelList commToSchedule(comms.size(), -1);
  while (nScheduled < comms.size())
  {
    label oldNScheduled = nScheduled;
    // Find unscheduled comms. This is the comms where the two processors
    // still have the most unscheduled comms.
    boolList busy(nProcs, false);
    while (true)
    {
      label maxCommI = -1;
      label maxNeed = labelMin;
      FOR_ALL(comms, commI)
      {
        label proc0 = comms[commI][0];
        label proc1 = comms[commI][1];
        if
        (
          commToSchedule[commI] == -1             // unscheduled
        && !busy[proc0]
        && !busy[proc1]
        )
        {
          label need =
            outstandingComms(commToSchedule, procToComms[proc0])
           + outstandingComms(commToSchedule, procToComms[proc1]);
          if (need > maxNeed)
          {
            maxNeed = need;
            maxCommI = commI;
          }
        }
      }
      if (maxCommI == -1)
      {
        // Found no unscheduled procs.
        break;
      }
      // Schedule commI in this iteration
      commToSchedule[maxCommI] = nScheduled++;
      busy[comms[maxCommI][0]] = true;
      busy[comms[maxCommI][1]] = true;
    }
    if (debug && Pstream::master())
    {
      label nIterComms = nScheduled-oldNScheduled;
      if (nIterComms > 0)
      {
        labelList procToComm(nProcs, -1);
        FOR_ALL(commToSchedule, commI)
        {
          label sched = commToSchedule[commI];
          if (sched >= oldNScheduled && sched < nScheduled)
          {
            label proc0 = comms[commI][0];
            procToComm[proc0] = commI;
            label proc1 = comms[commI][1];
            procToComm[proc1] = commI;
          }
        }
        // Print it
        OStringStream os;
        os  << setw(3) << iter << " |";
        FOR_ALL(procToComm, procI)
        {
          if (procToComm[procI] == -1)
          {
            os  << "   ";
          }
          else
          {
            os  << setw(3) << procToComm[procI];
          }
        }
        Pout<< os.str().c_str() << endl;
      }
    }
    iter++;
  }
  if (debug && Pstream::master())
  {
    Pout<< endl;
  }
  // Sort commToSchedule and obtain order in comms
  schedule_ = SortableList<label>(commToSchedule).indices();
  // Sort schedule_ by processor
  labelList nProcScheduled(nProcs, 0);
  // Count
  FOR_ALL(schedule_, i)
  {
    label commI = schedule_[i];
    const labelPair& twoProcs = comms[commI];
    nProcScheduled[twoProcs[0]]++;
    nProcScheduled[twoProcs[1]]++;
  }
  // Allocate
  FOR_ALL(procSchedule_, procI)
  {
    procSchedule_[procI].setSize(nProcScheduled[procI]);
  }
  nProcScheduled = 0;
  // Fill
  FOR_ALL(schedule_, i)
  {
    label commI = schedule_[i];
    const labelPair& twoProcs = comms[commI];
    label proc0 = twoProcs[0];
    procSchedule_[proc0][nProcScheduled[proc0]++] = commI;
    label proc1 = twoProcs[1];
    procSchedule_[proc1][nProcScheduled[proc1]++] = commI;
  }
  if (debug && Pstream::master())
  {
    Pout<< "commSchedule::commSchedule : Per processor:" << endl;
    FOR_ALL(procSchedule_, procI)
    {
      const labelList& procComms = procSchedule_[procI];
      Pout<< "Processor " << procI << " talks to processors:" << endl;
      FOR_ALL(procComms, i)
      {
        const labelPair& twoProcs = comms[procComms[i]];
        label nbr = (twoProcs[1] == procI ? twoProcs[0] : twoProcs[1]);
        Pout<< "    " << nbr << endl;
      }
    }
    Pout<< endl;
  }
}
