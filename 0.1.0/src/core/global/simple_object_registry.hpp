// mousse: CFD toolbox
// Copyright (C) 2012-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::simpleObjectRegistry
// Description
//   Object registry for simpleRegIOobject. Maintains ordering.
// SourceFiles
#ifndef simple_object_registry_hpp_
#define simple_object_registry_hpp_
#include "_dictionary.hpp"
#include "simple_reg_ioobject.hpp"
namespace mousse
{
class simpleObjectRegistryEntry
:
  public Dictionary<simpleObjectRegistryEntry>::link,
  public List<simpleRegIOobject*>
{
public:
  simpleObjectRegistryEntry(const List<simpleRegIOobject*>& data)
  :
    List<simpleRegIOobject*>(data)
  {}
};
class simpleObjectRegistry
:
  public Dictionary<simpleObjectRegistryEntry>
{
public:
  // Constructors
    //- Construct given initial table size
    simpleObjectRegistry(const label nIoObjects = 128)
    :
      Dictionary<simpleObjectRegistryEntry>(nIoObjects)
    {}
};
}  // namespace mousse
#endif
