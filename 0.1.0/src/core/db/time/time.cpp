// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "time.hpp"
#include "pstream_reduce_ops.hpp"
#include "arg_list.hpp"
#include <sstream>

// Static Member Data 
namespace mousse
{

DEFINE_TYPE_NAME_AND_DEBUG(Time, 0);
template<>
const char* mousse::NamedEnum
<
  mousse::Time::stopAtControls,
  4
>::names[] =
{
  "endTime",
  "noWriteNow",
  "writeNow",
  "nextWrite"
};
template<>
const char* mousse::NamedEnum
<
  mousse::Time::writeControls,
  5
>::names[] =
{
  "timeStep",
  "runTime",
  "adjustableRunTime",
  "clockTime",
  "cpuTime"
};

}

const mousse::NamedEnum<mousse::Time::stopAtControls, 4>
  mousse::Time::stopAtControlNames_;

const mousse::NamedEnum<mousse::Time::writeControls, 5>
  mousse::Time::writeControlNames_;

mousse::Time::fmtflags mousse::Time::format_(mousse::Time::general);

int mousse::Time::precision_(6);

const int mousse::Time::maxPrecision_(3 - log10(SMALL));

mousse::word mousse::Time::controlDictName("controlDict");

// Private Member Functions 
void mousse::Time::adjustDeltaT()
{
  bool adjustTime = false;
  scalar timeToNextWrite = VGREAT;
  if (writeControl_ == wcAdjustableRunTime)
  {
    adjustTime = true;
    timeToNextWrite = max
    (
      0.0,
      (outputTimeIndex_ + 1)*writeInterval_ - (value() - startTime_)
    );
  }
  if (secondaryWriteControl_ == wcAdjustableRunTime)
  {
    adjustTime = true;
    timeToNextWrite = max
    (
      0.0,
      min
      (
        timeToNextWrite,
        (secondaryOutputTimeIndex_ + 1)*secondaryWriteInterval_
       - (value() - startTime_)
      )
    );
  }
  if (adjustTime)
  {
    scalar nSteps = timeToNextWrite/deltaT_ - SMALL;
    // For tiny deltaT the label can overflow!
    if (nSteps < labelMax)
    {
      label nStepsToNextWrite = label(nSteps) + 1;
      scalar newDeltaT = timeToNextWrite/nStepsToNextWrite;
      // Control the increase of the time step to within a factor of 2
      // and the decrease within a factor of 5.
      if (newDeltaT >= deltaT_)
      {
        deltaT_ = min(newDeltaT, 2.0*deltaT_);
      }
      else
      {
        deltaT_ = max(newDeltaT, 0.2*deltaT_);
      }
    }
  }
  functionObjects_.adjustTimeStep();
}


void mousse::Time::setControls()
{
  // default is to resume calculation from "latestTime"
  const word startFrom = controlDict_.lookupOrDefault<word>
  (
    "startFrom",
    "latestTime"
  );
  if (startFrom == "startTime")
  {
    controlDict_.lookup("startTime") >> startTime_;
  }
  else
  {
    // Search directory for valid time directories
    instantList timeDirs = findTimes(path(), constant());
    if (startFrom == "firstTime")
    {
      if (timeDirs.size())
      {
        if (timeDirs[0].name() == constant() && timeDirs.size() >= 2)
        {
          startTime_ = timeDirs[1].value();
        }
        else
        {
          startTime_ = timeDirs[0].value();
        }
      }
    }
    else if (startFrom == "latestTime")
    {
      if (timeDirs.size())
      {
        startTime_ = timeDirs.last().value();
      }
    }
    else
    {
      FATAL_IO_ERROR_IN("Time::setControls()", controlDict_)
        << "expected startTime, firstTime or latestTime"
        << " found '" << startFrom << "'"
        << exit(FatalIOError);
    }
  }
  setTime(startTime_, 0);
  readDict();
  deltaTSave_ = deltaT_;
  deltaT0_ = deltaT_;
  // Check if time directory exists
  // If not increase time precision to see if it is formatted differently.
  if (!exists(timePath(), false))
  {
    int oldPrecision = precision_;
    int requiredPrecision = -1;
    bool found = false;
    for
    (
      precision_ = maxPrecision_;
      precision_ > oldPrecision;
      precision_--
    )
    {
      // Update the time formatting
      setTime(startTime_, 0);
      // Check the existence of the time directory with the new format
      found = exists(timePath(), false);
      if (found)
      {
        requiredPrecision = precision_;
      }
    }
    if (requiredPrecision > 0)
    {
      // Update the time precision
      precision_ = requiredPrecision;
      // Update the time formatting
      setTime(startTime_, 0);
      WARNING_IN("Time::setControls()")
        << "Increasing the timePrecision from " << oldPrecision
        << " to " << precision_
        << " to support the formatting of the current time directory "
        << timeName() << nl << endl;
    }
    else
    {
      // Could not find time directory so assume it is not present
      precision_ = oldPrecision;
      // Revert the time formatting
      setTime(startTime_, 0);
    }
  }
  if (Pstream::parRun())
  {
    scalar sumStartTime = startTime_;
    reduce(sumStartTime, sumOp<scalar>());
    if
    (
      mag(Pstream::nProcs()*startTime_ - sumStartTime)
     > Pstream::nProcs()*deltaT_/10.0
    )
    {
      FATAL_IO_ERROR_IN("Time::setControls()", controlDict_)
        << "Start time is not the same for all processors" << nl
        << "processor " << Pstream::myProcNo() << " has startTime "
        << startTime_ << exit(FatalIOError);
    }
  }
  IOdictionary timeDict
  (
    IOobject
    (
      "time",
      timeName(),
      "uniform",
      *this,
      IOobject::READ_IF_PRESENT,
      IOobject::NO_WRITE,
      false
    )
  );
  // Read and set the deltaT only if time-step adjustment is active
  // otherwise use the deltaT from the controlDict
  if (controlDict_.lookupOrDefault<Switch>("adjustTimeStep", false))
  {
    if (timeDict.readIfPresent("deltaT", deltaT_))
    {
      deltaTSave_ = deltaT_;
      deltaT0_ = deltaT_;
    }
  }
  timeDict.readIfPresent("deltaT0", deltaT0_);
  if (timeDict.readIfPresent("index", startTimeIndex_))
  {
    timeIndex_ = startTimeIndex_;
  }
  // Check if values stored in time dictionary are consistent
  // 1. Based on time name
  bool checkValue = true;
  string storedTimeName;
  if (timeDict.readIfPresent("name", storedTimeName))
  {
    if (storedTimeName == timeName())
    {
      // Same time. No need to check stored value
      checkValue = false;
    }
  }
  // 2. Based on time value
  //    (consistent up to the current time writing precision so it won't
  //     trigger if we just change the write precision)
  if (checkValue)
  {
    scalar storedTimeValue;
    if (timeDict.readIfPresent("value", storedTimeValue))
    {
      word storedTimeName(timeName(storedTimeValue));
      if (storedTimeName != timeName())
      {
        IO_WARNING_IN("Time::setControls()", timeDict)
          << "Time read from time dictionary " << storedTimeName
          << " differs from actual time " << timeName() << '.' << nl
          << "    This may cause unexpected database behaviour."
          << " If you are not interested" << nl
          << "    in preserving time state delete"
          << " the time dictionary."
          << endl;
      }
    }
  }
}


// Constructors 
mousse::Time::Time
(
  const word& controlDictName,
  const fileName& rootPath,
  const fileName& caseName,
  const word& systemName,
  const word& constantName,
  const bool enableFunctionObjects
)
:
  TimePaths
  {
    rootPath,
    caseName,
    systemName,
    constantName
  },
  objectRegistry{*this},
  libs_(),
  controlDict_
  {
    IOobject
    {
      controlDictName,
      system(),
      *this,
      IOobject::MUST_READ_IF_MODIFIED,
      IOobject::NO_WRITE,
      false
    }
  },
  startTimeIndex_{0},
  startTime_{0},
  endTime_{0},
  stopAt_{saEndTime},
  writeControl_{wcTimeStep},
  writeInterval_{GREAT},
  secondaryWriteControl_{wcTimeStep},
  secondaryWriteInterval_{labelMax/10.0}, // bit less to allow calculations
  purgeWrite_{0},
  secondaryPurgeWrite_{0},
  writeOnce_{false},
  subCycling_{false},
  sigWriteNow_{true, *this},
  sigStopAtWriteNow_{true, *this},
  writeFormat_{IOstream::ASCII},
  writeVersion_{IOstream::currentVersion},
  writeCompression_{IOstream::UNCOMPRESSED},
  graphFormat_{"raw"},
  runTimeModifiable_{false},
  functionObjects_{*this, enableFunctionObjects}
{
  libs_.open(controlDict_, "libs");
  // Explicitly set read flags on objectRegistry so anything constructed
  // from it reads as well (e.g. fvSolution).
  readOpt() = IOobject::MUST_READ_IF_MODIFIED;
  setControls();
  // Time objects not registered so do like objectRegistry::checkIn ourselves.
  if (runTimeModifiable_)
  {
    monitorPtr_.reset
    (
      new fileMonitor
      (
        regIOobject::fileModificationChecking == inotify
      || regIOobject::fileModificationChecking == inotifyMaster
      )
    );
    // File might not exist yet.
    fileName f(controlDict_.filePath());
    if (!f.size())
    {
      // We don't have this file but would like to re-read it.
      // Possibly if in master-only reading mode. Use a non-existing
      // file to keep fileMonitor synced.
      f = controlDict_.objectPath();
    }
    controlDict_.watchIndex() = addWatch(f);
  }
}


mousse::Time::Time
(
  const word& controlDictName,
  const argList& args,
  const word& systemName,
  const word& constantName
)
:
  TimePaths
  {
    args.parRunControl().parRun(),
    args.rootPath(),
    args.globalCaseName(),
    args.caseName(),
    systemName,
    constantName
  },
  objectRegistry{*this},
  libs_(),
  controlDict_
  {
    IOobject
    {
      controlDictName,
      system(),
      *this,
      IOobject::MUST_READ_IF_MODIFIED,
      IOobject::NO_WRITE,
      false
    }
  },
  startTimeIndex_{0},
  startTime_{0},
  endTime_{0},
  stopAt_{saEndTime},
  writeControl_{wcTimeStep},
  writeInterval_{GREAT},
  secondaryWriteControl_{wcTimeStep},
  secondaryWriteInterval_{labelMax/10.0},
  purgeWrite_{0},
  secondaryPurgeWrite_{0},
  writeOnce_{false},
  subCycling_{false},
  sigWriteNow_{true, *this},
  sigStopAtWriteNow_{true, *this},
  writeFormat_{IOstream::ASCII},
  writeVersion_{IOstream::currentVersion},
  writeCompression_{IOstream::UNCOMPRESSED},
  graphFormat_{"raw"},
  runTimeModifiable_{false},
  functionObjects_
  {
    *this,
    argList::validOptions.found("withFunctionObjects")
   ? args.optionFound("withFunctionObjects")
   : !args.optionFound("noFunctionObjects")
  }
{
  libs_.open(controlDict_, "libs");
  // Explicitly set read flags on objectRegistry so anything constructed
  // from it reads as well (e.g. fvSolution).
  readOpt() = IOobject::MUST_READ_IF_MODIFIED;
  setControls();
  // Time objects not registered so do like objectRegistry::checkIn ourselves.
  if (runTimeModifiable_)
  {
    monitorPtr_.reset
    (
      new fileMonitor
      (
        regIOobject::fileModificationChecking == inotify
      || regIOobject::fileModificationChecking == inotifyMaster
      )
    );
    // File might not exist yet.
    fileName f(controlDict_.filePath());
    if (!f.size())
    {
      // We don't have this file but would like to re-read it.
      // Possibly if in master-only reading mode. Use a non-existing
      // file to keep fileMonitor synced.
      f = controlDict_.objectPath();
    }
    controlDict_.watchIndex() = addWatch(f);
  }
}


mousse::Time::Time
(
  const dictionary& dict,
  const fileName& rootPath,
  const fileName& caseName,
  const word& systemName,
  const word& constantName,
  const bool enableFunctionObjects
)
:
  TimePaths
  {
    rootPath,
    caseName,
    systemName,
    constantName
  },
  objectRegistry{*this},
  libs_(),
  controlDict_
  {
    IOobject
    {
      controlDictName,
      system(),
      *this,
      IOobject::NO_READ,
      IOobject::NO_WRITE,
      false
    },
    dict
  },
  startTimeIndex_{0},
  startTime_{0},
  endTime_{0},
  stopAt_{saEndTime},
  writeControl_{wcTimeStep},
  writeInterval_{GREAT},
  secondaryWriteControl_{wcTimeStep},
  secondaryWriteInterval_{labelMax/10.0},
  purgeWrite_{0},
  secondaryPurgeWrite_{0},
  writeOnce_{false},
  subCycling_{false},
  sigWriteNow_{true, *this},
  sigStopAtWriteNow_{true, *this},
  writeFormat_{IOstream::ASCII},
  writeVersion_{IOstream::currentVersion},
  writeCompression_{IOstream::UNCOMPRESSED},
  graphFormat_{"raw"},
  runTimeModifiable_{false},
  functionObjects_{*this, enableFunctionObjects}
{
  libs_.open(controlDict_, "libs");
  // Explicitly set read flags on objectRegistry so anything constructed
  // from it reads as well (e.g. fvSolution).
  readOpt() = IOobject::MUST_READ_IF_MODIFIED;
  // Since could not construct regIOobject with setting:
  controlDict_.readOpt() = IOobject::MUST_READ_IF_MODIFIED;
  setControls();
  // Time objects not registered so do like objectRegistry::checkIn ourselves.
  if (runTimeModifiable_)
  {
    monitorPtr_.reset
    (
      new fileMonitor
      (
        regIOobject::fileModificationChecking == inotify
      || regIOobject::fileModificationChecking == inotifyMaster
      )
    );
    // File might not exist yet.
    fileName f(controlDict_.filePath());
    if (!f.size())
    {
      // We don't have this file but would like to re-read it.
      // Possibly if in master-only reading mode. Use a non-existing
      // file to keep fileMonitor synced.
      f = controlDict_.objectPath();
    }
    controlDict_.watchIndex() = addWatch(f);
  }
}


mousse::Time::Time
(
  const fileName& rootPath,
  const fileName& caseName,
  const word& systemName,
  const word& constantName,
  const bool enableFunctionObjects
)
:
  TimePaths
  {
    rootPath,
    caseName,
    systemName,
    constantName
  },
  objectRegistry{*this},
  libs_(),
  controlDict_
  {
    IOobject
    {
      controlDictName,
      system(),
      *this,
      IOobject::NO_READ,
      IOobject::NO_WRITE,
      false
    }
  },
  startTimeIndex_{0},
  startTime_{0},
  endTime_{0},
  stopAt_{saEndTime},
  writeControl_{wcTimeStep},
  writeInterval_{GREAT},
  secondaryWriteControl_{wcTimeStep},
  secondaryWriteInterval_{labelMax/10.0},
  purgeWrite_{0},
  secondaryPurgeWrite_{0},
  writeOnce_{false},
  subCycling_{false},
  writeFormat_{IOstream::ASCII},
  writeVersion_{IOstream::currentVersion},
  writeCompression_{IOstream::UNCOMPRESSED},
  graphFormat_{"raw"},
  runTimeModifiable_{false},
  functionObjects_{*this, enableFunctionObjects}
{
  libs_.open(controlDict_, "libs");
}


// Destructor 
mousse::Time::~Time()
{
  if (controlDict_.watchIndex() != -1)
  {
    removeWatch(controlDict_.watchIndex());
  }
  // destroy function objects first
  functionObjects_.clear();
}


// Member Functions 
mousse::label mousse::Time::addWatch(const fileName& fName) const
{
  return monitorPtr_().addWatch(fName);
}


bool mousse::Time::removeWatch(const label watchIndex) const
{
  return monitorPtr_().removeWatch(watchIndex);
}


const mousse::fileName& mousse::Time::getFile(const label watchIndex) const
{
  return monitorPtr_().getFile(watchIndex);
}


mousse::fileMonitor::fileState mousse::Time::getState
(
  const label watchFd
) const
{
  return monitorPtr_().getState(watchFd);
}


void mousse::Time::setUnmodified(const label watchFd) const
{
  monitorPtr_().setUnmodified(watchFd);
}


mousse::word mousse::Time::timeName(const scalar t, const int precision)
{
  std::ostringstream buf;
  buf.setf(ios_base::fmtflags(format_), ios_base::floatfield);
  buf.precision(precision);
  buf << t;
  return buf.str();
}


mousse::word mousse::Time::timeName() const
{
  return dimensionedScalar::name();
}


// Search the construction path for times
mousse::instantList mousse::Time::times() const
{
  return findTimes(path(), constant());
}


mousse::word mousse::Time::findInstancePath(const instant& t) const
{
  const fileName directory = path();
  const word& constantName = constant();
  // Read directory entries into a list
  fileNameList dirEntries{readDir(directory, fileName::DIRECTORY)};
  FOR_ALL(dirEntries, i)
  {
    scalar timeValue;
    if (readScalar(dirEntries[i].c_str(), timeValue) && t.equal(timeValue))
    {
      return dirEntries[i];
    }
  }
  if (t.equal(0.0))
  {
    // Looking for 0 or constant. 0 already checked above.
    if (isDir(directory/constantName))
    {
      return constantName;
    }
  }
  return word::null;
}


mousse::instant mousse::Time::findClosestTime(const scalar t) const
{
  instantList timeDirs = findTimes(path(), constant());
  // there is only one time (likely "constant") so return it
  if (timeDirs.size() == 1)
  {
    return timeDirs[0];
  }
  if (t < timeDirs[1].value())
  {
    return timeDirs[1];
  }
  else if (t > timeDirs.last().value())
  {
    return timeDirs.last();
  }
  label nearestIndex = -1;
  scalar deltaT = GREAT;
  for (label timei=1; timei < timeDirs.size(); ++timei)
  {
    scalar diff = mag(timeDirs[timei].value() - t);
    if (diff < deltaT)
    {
      deltaT = diff;
      nearestIndex = timei;
    }
  }
  return timeDirs[nearestIndex];
}


// This should work too,
// if we don't worry about checking "constant" explicitly
//
// mousse::instant mousse::Time::findClosestTime(const scalar t) const
// {
//     instantList timeDirs = findTimes(path(), constant());
//     label timeIndex = min(findClosestTimeIndex(timeDirs, t), 0, constant());
//     return timeDirs[timeIndex];
// }
mousse::label mousse::Time::findClosestTimeIndex
(
  const instantList& timeDirs,
  const scalar t,
  const word& constantName
)
{
  label nearestIndex = -1;
  scalar deltaT = GREAT;
  FOR_ALL(timeDirs, timei)
  {
    if (timeDirs[timei].name() == constantName) continue;
    scalar diff = mag(timeDirs[timei].value() - t);
    if (diff < deltaT)
    {
      deltaT = diff;
      nearestIndex = timei;
    }
  }
  return nearestIndex;
}


mousse::label mousse::Time::startTimeIndex() const
{
  return startTimeIndex_;
}


mousse::dimensionedScalar mousse::Time::startTime() const
{
  return dimensionedScalar("startTime", dimTime, startTime_);
}


mousse::dimensionedScalar mousse::Time::endTime() const
{
  return dimensionedScalar("endTime", dimTime, endTime_);
}


bool mousse::Time::run() const
{
  bool running = value() < (endTime_ - 0.5*deltaT_);
  if (!subCycling_)
  {
    // only execute when the condition is no longer true
    // ie, when exiting the control loop
    if (!running && timeIndex_ != startTimeIndex_)
    {
      // Note, end() also calls an indirect start() as required
      functionObjects_.end();
    }
  }
  if (running)
  {
    if (!subCycling_)
    {
      const_cast<Time&>(*this).readModifiedObjects();
      if (timeIndex_ == startTimeIndex_)
      {
        functionObjects_.start();
      }
      else
      {
        functionObjects_.execute();
      }
    }
    // Update the "running" status following the
    // possible side-effects from functionObjects
    running = value() < (endTime_ - 0.5*deltaT_);
  }
  return running;
}


bool mousse::Time::loop()
{
  bool running = run();
  if (running)
  {
    operator++();
  }
  return running;
}


bool mousse::Time::end() const
{
  return value() > (endTime_ + 0.5*deltaT_);
}


bool mousse::Time::stopAt(const stopAtControls sa) const
{
  const bool changed = (stopAt_ != sa);
  stopAt_ = sa;
  // adjust endTime
  if (sa == saEndTime)
  {
    controlDict_.lookup("endTime") >> endTime_;
  }
  else
  {
    endTime_ = GREAT;
  }
  return changed;
}


void mousse::Time::setTime(const Time& t)
{
  value() = t.value();
  dimensionedScalar::name() = t.dimensionedScalar::name();
  timeIndex_ = t.timeIndex_;
}


void mousse::Time::setTime(const instant& inst, const label newIndex)
{
  value() = inst.value();
  dimensionedScalar::name() = inst.name();
  timeIndex_ = newIndex;
  IOdictionary timeDict
  {
    IOobject
    {
      "time",
      timeName(),
      "uniform",
      *this,
      IOobject::READ_IF_PRESENT,
      IOobject::NO_WRITE,
      false
    }
  };
  timeDict.readIfPresent("deltaT", deltaT_);
  timeDict.readIfPresent("deltaT0", deltaT0_);
  timeDict.readIfPresent("index", timeIndex_);
}


void mousse::Time::setTime(const dimensionedScalar& newTime, const label newIndex)
{
  setTime(newTime.value(), newIndex);
}


void mousse::Time::setTime(const scalar newTime, const label newIndex)
{
  value() = newTime;
  dimensionedScalar::name() = timeName(timeToUserTime(newTime));
  timeIndex_ = newIndex;
}


void mousse::Time::setEndTime(const dimensionedScalar& endTime)
{
  setEndTime(endTime.value());
}


void mousse::Time::setEndTime(const scalar endTime)
{
  endTime_ = endTime;
}


void mousse::Time::setDeltaT
(
  const dimensionedScalar& deltaT,
  const bool bAdjustDeltaT
)
{
  setDeltaT(deltaT.value(), bAdjustDeltaT);
}


void mousse::Time::setDeltaT(const scalar deltaT, const bool bAdjustDeltaT)
{
  deltaT_ = deltaT;
  deltaTchanged_ = true;
  if (bAdjustDeltaT)
  {
    adjustDeltaT();
  }
}


mousse::TimeState mousse::Time::subCycle(const label nSubCycles)
{
  subCycling_ = true;
  prevTimeState_.set(new TimeState(*this));
  setTime(*this - deltaT(), (timeIndex() - 1)*nSubCycles);
  deltaT_ /= nSubCycles;
  deltaT0_ /= nSubCycles;
  deltaTSave_ = deltaT0_;
  return prevTimeState();
}


void mousse::Time::endSubCycle()
{
  if (subCycling_)
  {
    subCycling_ = false;
    TimeState::operator=(prevTimeState());
    prevTimeState_.clear();
  }
}


// Member Operators 
mousse::Time& mousse::Time::operator+=(const dimensionedScalar& deltaT)
{
  return operator+=(deltaT.value());
}


mousse::Time& mousse::Time::operator+=(const scalar deltaT)
{
  setDeltaT(deltaT);
  return operator++();
}


mousse::Time& mousse::Time::operator++()
{
  deltaT0_ = deltaTSave_;
  deltaTSave_ = deltaT_;
  // Save old time value and name
  const scalar oldTimeValue = timeToUserTime(value());
  const word oldTimeName = dimensionedScalar::name();
  // Increment time
  setTime(value() + deltaT_, timeIndex_ + 1);
  if (!subCycling_)
  {
    // If the time is very close to zero reset to zero
    if (mag(value()) < 10*SMALL*deltaT_)
    {
      setTime(0.0, timeIndex_);
    }
    if (sigStopAtWriteNow_.active() || sigWriteNow_.active())
    {
      // A signal might have been sent on one processor only
      // Reduce so all decide the same.
      label flag = 0;
      if (sigStopAtWriteNow_.active() && stopAt_ == saWriteNow)
      {
        flag += 1;
      }
      if (sigWriteNow_.active() && writeOnce_)
      {
        flag += 2;
      }
      reduce(flag, maxOp<label>());
      if (flag & 1)
      {
        stopAt_ = saWriteNow;
      }
      if (flag & 2)
      {
        writeOnce_ = true;
      }
    }
    outputTime_ = false;
    primaryOutputTime_ = false;
    secondaryOutputTime_ = false;
    switch (writeControl_)
    {
      case wcTimeStep:
        primaryOutputTime_ = !(timeIndex_ % label(writeInterval_));
      break;
      case wcRunTime:
      case wcAdjustableRunTime:
      {
        label outputIndex = label
        (
          ((value() - startTime_) + 0.5*deltaT_)
         / writeInterval_
        );
        if (outputIndex > outputTimeIndex_)
        {
          primaryOutputTime_ = true;
          outputTimeIndex_ = outputIndex;
        }
      }
      break;
      case wcCpuTime:
      {
        label outputIndex = label
        (
          returnReduce(elapsedCpuTime(), maxOp<double>())
         / writeInterval_
        );
        if (outputIndex > outputTimeIndex_)
        {
          primaryOutputTime_ = true;
          outputTimeIndex_ = outputIndex;
        }
      }
      break;
      case wcClockTime:
      {
        label outputIndex = label
        (
          returnReduce(label(elapsedClockTime()), maxOp<label>())
         / writeInterval_
        );
        if (outputIndex > outputTimeIndex_)
        {
          primaryOutputTime_ = true;
          outputTimeIndex_ = outputIndex;
        }
      }
      break;
    }
    // Adapt for secondaryWrite controls
    switch (secondaryWriteControl_)
    {
      case wcTimeStep:
        secondaryOutputTime_ =
          !(timeIndex_ % label(secondaryWriteInterval_));
      break;
      case wcRunTime:
      case wcAdjustableRunTime:
      {
        label outputIndex = label
        (
          ((value() - startTime_) + 0.5*deltaT_)
         / secondaryWriteInterval_
        );
        if (outputIndex > secondaryOutputTimeIndex_)
        {
          secondaryOutputTime_ = true;
          secondaryOutputTimeIndex_ = outputIndex;
        }
      }
      break;
      case wcCpuTime:
      {
        label outputIndex = label
        (
          returnReduce(elapsedCpuTime(), maxOp<double>())
         / secondaryWriteInterval_
        );
        if (outputIndex > secondaryOutputTimeIndex_)
        {
          secondaryOutputTime_ = true;
          secondaryOutputTimeIndex_ = outputIndex;
        }
      }
      break;
      case wcClockTime:
      {
        label outputIndex = label
        (
          returnReduce(label(elapsedClockTime()), maxOp<label>())
         / secondaryWriteInterval_
        );
        if (outputIndex > secondaryOutputTimeIndex_)
        {
          secondaryOutputTime_ = true;
          secondaryOutputTimeIndex_ = outputIndex;
        }
      }
      break;
    }
    outputTime_ = primaryOutputTime_ || secondaryOutputTime_;
    // Check if endTime needs adjustment to stop at the next run()/end()
    if (!end())
    {
      if (stopAt_ == saNoWriteNow)
      {
        endTime_ = value();
      }
      else if (stopAt_ == saWriteNow)
      {
        endTime_ = value();
        outputTime_ = true;
        primaryOutputTime_ = true;
      }
      else if (stopAt_ == saNextWrite && outputTime_ == true)
      {
        endTime_ = value();
      }
    }
    // Override outputTime if one-shot writing
    if (writeOnce_)
    {
      primaryOutputTime_ = true;
      outputTime_ = true;
      writeOnce_ = false;
    }
    // Adjust the precision of the time directory name if necessary
    if (outputTime_)
    {
      // Tolerance used when testing time equivalence
      const scalar timeTol =
        max(min(pow(10.0, -precision_), 0.1*deltaT_), SMALL);
      // User-time equivalent of deltaT
      const scalar userDeltaT = timeToUserTime(deltaT_);
      // Time value obtained by reading timeName
      scalar timeNameValue = -VGREAT;
      // Check that new time representation differs from old one
      // reinterpretation of the word
      if
      (
        readScalar(dimensionedScalar::name().c_str(), timeNameValue)
      && (mag(timeNameValue - oldTimeValue - userDeltaT) > timeTol)
      )
      {
        int oldPrecision = precision_;
        while
        (
          precision_ < maxPrecision_
        && readScalar(dimensionedScalar::name().c_str(), timeNameValue)
        && (mag(timeNameValue - oldTimeValue - userDeltaT) > timeTol)
        )
        {
          precision_++;
          setTime(value(), timeIndex());
        }
        if (precision_ != oldPrecision)
        {
          WARNING_IN("Time::operator++()")
            << "Increased the timePrecision from " << oldPrecision
            << " to " << precision_
            << " to distinguish between timeNames at time "
            << dimensionedScalar::name()
            << endl;
          if (precision_ == maxPrecision_)
          {
            // Reached maxPrecision limit
            WARNING_IN("Time::operator++()")
              << "Current time name " << dimensionedScalar::name()
              << nl
              << "    The maximum time precision has been reached"
               " which might result in overwriting previous"
               " results."
              << endl;
          }
          // Check if round-off error caused time-reversal
          scalar oldTimeNameValue = -VGREAT;
          if
          (
            readScalar(oldTimeName.c_str(), oldTimeNameValue)
            && (sign(timeNameValue - oldTimeNameValue) != sign(deltaT_))
          )
          {
            WARNING_IN("Time::operator++()")
              << "Current time name " << dimensionedScalar::name()
              << " is set to an instance prior to the "
               "previous one "
              << oldTimeName << nl
              << "    This might result in temporal "
               "discontinuities."
              << endl;
          }
        }
      }
    }
    functionObjects_.timeSet();
  }
  return *this;
}


mousse::Time& mousse::Time::operator++(int)
{
  return operator++();
}
