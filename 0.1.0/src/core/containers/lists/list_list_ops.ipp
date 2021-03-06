// mousse: CFD toolbox
// Copyright (C) 2011-2013 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "list_list_ops.hpp"


namespace mousse {

// Member Functions 
template<class AccessType, class T, class AccessOp>
AccessType ListListOps::combine(const List<T>& lst, AccessOp aop)
{
  label sum = 0;
  FOR_ALL(lst, lstI) {
    sum += aop(lst[lstI]).size();
  }
  AccessType result(sum);
  label globalElemI = 0;
  FOR_ALL(lst, lstI) {
    const T& sub = lst[lstI];
    FOR_ALL(aop(sub), elemI) {
      result[globalElemI++] = aop(sub)[elemI];
    }
  }
  return result;
}


template<class T, class AccessOp>
labelList ListListOps::subSizes(const List<T>& lst, AccessOp aop)
{
  labelList sizes{lst.size()};
  FOR_ALL(lst, lstI) {
    sizes[lstI] = aop(lst[lstI]).size();
  }
  return sizes;
}


template<class AccessType, class T, class AccessOp, class OffsetOp>
AccessType ListListOps::combineOffset
(
  const List<T>& lst,
  const labelList& sizes,
  AccessOp aop,
  OffsetOp oop
)
{
  label sum = 0;
  FOR_ALL(lst, lstI) {
    sum += aop(lst[lstI]).size();
  }
  AccessType result(sum);
  label globalElemI = 0;
  label offset = 0;
  FOR_ALL(lst, lstI) {
    const T& sub = lst[lstI];
    FOR_ALL(aop(sub), elemI) {
      result[globalElemI++] = oop(aop(sub)[elemI], offset);
    }
    offset += sizes[lstI];
  }
  return result;
}

}  // namespace mousse
