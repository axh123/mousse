// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "uil_list.hpp"


// Constructors 
template<class LListBase, class T>
mousse::UILList<LListBase, T>::UILList(const UILList<LListBase, T>& lst)
{
  for (const_iterator iter = lst.begin(); iter != lst.end(); ++iter) {
    this->append(&iter());
  }
}


// Member Operators 
template<class LListBase, class T>
void mousse::UILList<LListBase, T>::operator=(const UILList<LListBase, T>& rhs)
{
  LListBase::clear();
  for (const_iterator iter = rhs.begin(); iter != rhs.end(); ++iter) {
    this->append(&iter());
  }
}


template<class LListBase, class T>
bool mousse::UILList<LListBase, T>::operator==
(
  const UILList<LListBase, T>& rhs
) const
{
  if (this->size() != rhs.size()) {
    return false;
  }
  bool equal = true;
  const_iterator iter1 = this->begin();
  const_iterator iter2 = rhs.begin();
  for (; iter1 != this->end(); ++iter1, ++iter2) {
    equal = equal && iter1() == iter2();
  }
  return equal;
}


// Comparison for inequality
template<class LListBase, class T>
bool mousse::UILList<LListBase, T>::operator!=
(
  const UILList<LListBase, T>& rhs
) const
{
  return !operator==(rhs);
}

// Friend Operators 
#include "uil_list_io.ipp"
