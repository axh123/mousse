// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

// Constructors 
template<class T, unsigned SizeInc, unsigned SizeMult, unsigned SizeDiv>
inline mousse::DynamicList<T, SizeInc, SizeMult, SizeDiv>::DynamicList()
:
  List<T>(0),
  capacity_(0)
{
  List<T>::size(0);
}
template<class T, unsigned SizeInc, unsigned SizeMult, unsigned SizeDiv>
inline mousse::DynamicList<T, SizeInc, SizeMult, SizeDiv>::DynamicList
(
  const label nElem
)
:
  List<T>(nElem),
  capacity_(nElem)
{
  // we could also enforce SizeInc granularity when (!SizeMult || !SizeDiv)
  List<T>::size(0);
}
template<class T, unsigned SizeInc, unsigned SizeMult, unsigned SizeDiv>
inline mousse::DynamicList<T, SizeInc, SizeMult, SizeDiv>::DynamicList
(
  const DynamicList<T, SizeInc, SizeMult, SizeDiv>& lst
)
:
  List<T>(lst),
  capacity_(lst.size())
{}
template<class T, unsigned SizeInc, unsigned SizeMult, unsigned SizeDiv>
inline mousse::DynamicList<T, SizeInc, SizeMult, SizeDiv>::DynamicList
(
  const UList<T>& lst
)
:
  List<T>(lst),
  capacity_(lst.size())
{}
template<class T, unsigned SizeInc, unsigned SizeMult, unsigned SizeDiv>
inline mousse::DynamicList<T, SizeInc, SizeMult, SizeDiv>::DynamicList
(
  const UIndirectList<T>& lst
)
:
  List<T>(lst),
  capacity_(lst.size())
{}
template<class T, unsigned SizeInc, unsigned SizeMult, unsigned SizeDiv>
inline mousse::DynamicList<T, SizeInc, SizeMult, SizeDiv>::DynamicList
(
  const Xfer<List<T> >& lst
)
:
  List<T>(lst),
  capacity_(List<T>::size())
{}
// Member Functions 
template<class T, unsigned SizeInc, unsigned SizeMult, unsigned SizeDiv>
inline mousse::label mousse::DynamicList<T, SizeInc, SizeMult, SizeDiv>::capacity()
const
{
  return capacity_;
}
template<class T, unsigned SizeInc, unsigned SizeMult, unsigned SizeDiv>
inline void mousse::DynamicList<T, SizeInc, SizeMult, SizeDiv>::setCapacity
(
  const label nElem
)
{
  label nextFree = List<T>::size();
  capacity_ = nElem;
  if (nextFree > capacity_)
  {
    // truncate addressed sizes too
    nextFree = capacity_;
  }
  // we could also enforce SizeInc granularity when (!SizeMult || !SizeDiv)
  List<T>::setSize(capacity_);
  List<T>::size(nextFree);
}
template<class T, unsigned SizeInc, unsigned SizeMult, unsigned SizeDiv>
inline void mousse::DynamicList<T, SizeInc, SizeMult, SizeDiv>::reserve
(
  const label nElem
)
{
  // allocate more capacity?
  if (nElem > capacity_)
  {
// TODO: convince the compiler that division by zero does not occur
//        if (SizeInc && (!SizeMult || !SizeDiv))
//        {
//            // resize with SizeInc as the granularity
//            capacity_ = nElem;
//            unsigned pad = SizeInc - (capacity_ % SizeInc);
//            if (pad != SizeInc)
//            {
//                capacity_ += pad;
//            }
//        }
//        else
    {
      capacity_ = max
      (
        nElem,
        label(SizeInc + capacity_ * SizeMult / SizeDiv)
      );
    }
    // adjust allocated size, leave addressed size untouched
    label nextFree = List<T>::size();
    List<T>::setSize(capacity_);
    List<T>::size(nextFree);
  }
}
template<class T, unsigned SizeInc, unsigned SizeMult, unsigned SizeDiv>
inline void mousse::DynamicList<T, SizeInc, SizeMult, SizeDiv>::setSize
(
  const label nElem
)
{
  // allocate more capacity?
  if (nElem > capacity_)
  {
// TODO: convince the compiler that division by zero does not occur
//        if (SizeInc && (!SizeMult || !SizeDiv))
//        {
//            // resize with SizeInc as the granularity
//            capacity_ = nElem;
//            unsigned pad = SizeInc - (capacity_ % SizeInc);
//            if (pad != SizeInc)
//            {
//                capacity_ += pad;
//            }
//        }
//        else
    {
      capacity_ = max
      (
        nElem,
        label(SizeInc + capacity_ * SizeMult / SizeDiv)
      );
    }
    List<T>::setSize(capacity_);
  }
  // adjust addressed size
  List<T>::size(nElem);
}
template<class T, unsigned SizeInc, unsigned SizeMult, unsigned SizeDiv>
inline void mousse::DynamicList<T, SizeInc, SizeMult, SizeDiv>::setSize
(
  const label nElem,
  const T& t
)
{
  label nextFree = List<T>::size();
  setSize(nElem);
  // set new elements to constant value
  while (nextFree < nElem)
  {
    this->operator[](nextFree++) = t;
  }
}
template<class T, unsigned SizeInc, unsigned SizeMult, unsigned SizeDiv>
inline void mousse::DynamicList<T, SizeInc, SizeMult, SizeDiv>::resize
(
  const label nElem
)
{
  this->setSize(nElem);
}
template<class T, unsigned SizeInc, unsigned SizeMult, unsigned SizeDiv>
inline void mousse::DynamicList<T, SizeInc, SizeMult, SizeDiv>::resize
(
  const label nElem,
  const T& t
)
{
  this->setSize(nElem, t);
}
template<class T, unsigned SizeInc, unsigned SizeMult, unsigned SizeDiv>
inline void mousse::DynamicList<T, SizeInc, SizeMult, SizeDiv>::clear()
{
  List<T>::size(0);
}
template<class T, unsigned SizeInc, unsigned SizeMult, unsigned SizeDiv>
inline void mousse::DynamicList<T, SizeInc, SizeMult, SizeDiv>::clearStorage()
{
  List<T>::clear();
  capacity_ = 0;
}
template<class T, unsigned SizeInc, unsigned SizeMult, unsigned SizeDiv>
inline mousse::DynamicList<T, SizeInc, SizeMult, SizeDiv>&
mousse::DynamicList<T, SizeInc, SizeMult, SizeDiv>::shrink()
{
  label nextFree = List<T>::size();
  if (capacity_ > nextFree)
  {
    // use the full list when resizing
    List<T>::size(capacity_);
    // the new size
    capacity_ = nextFree;
    List<T>::setSize(capacity_);
    List<T>::size(nextFree);
  }
  return *this;
}
template<class T, unsigned SizeInc, unsigned SizeMult, unsigned SizeDiv>
inline void
mousse::DynamicList<T, SizeInc, SizeMult, SizeDiv>::transfer(List<T>& lst)
{
  capacity_ = lst.size();
  List<T>::transfer(lst);   // take over storage, clear addressing for lst.
}
template<class T, unsigned SizeInc, unsigned SizeMult, unsigned SizeDiv>
inline void
mousse::DynamicList<T, SizeInc, SizeMult, SizeDiv>::transfer
(
  DynamicList<T, SizeInc, SizeMult, SizeDiv>& lst
)
{
  // take over storage as-is (without shrink), clear addressing for lst.
  capacity_ = lst.capacity_;
  lst.capacity_ = 0;
  List<T>::transfer(static_cast<List<T>&>(lst));
}
template<class T, unsigned SizeInc, unsigned SizeMult, unsigned SizeDiv>
inline mousse::Xfer<mousse::List<T> >
mousse::DynamicList<T, SizeInc, SizeMult, SizeDiv>::xfer()
{
  return xferMoveTo< List<T> >(*this);
}
template<class T, unsigned SizeInc, unsigned SizeMult, unsigned SizeDiv>
inline mousse::DynamicList<T, SizeInc, SizeMult, SizeDiv>&
mousse::DynamicList<T, SizeInc, SizeMult, SizeDiv>::append
(
  const T& t
)
{
  const label elemI = List<T>::size();
  setSize(elemI + 1);
  this->operator[](elemI) = t;
  return *this;
}
template<class T, unsigned SizeInc, unsigned SizeMult, unsigned SizeDiv>
inline mousse::DynamicList<T, SizeInc, SizeMult, SizeDiv>&
mousse::DynamicList<T, SizeInc, SizeMult, SizeDiv>::append
(
  const UList<T>& lst
)
{
  if (this == &lst)
  {
    FATAL_ERROR_IN
    (
      "DynamicList<T, SizeInc, SizeMult, SizeDiv>::append"
      "(const UList<T>&)"
    )   << "attempted appending to self" << abort(FatalError);
  }
  label nextFree = List<T>::size();
  setSize(nextFree + lst.size());
  FOR_ALL(lst, elemI)
  {
    this->operator[](nextFree++) = lst[elemI];
  }
  return *this;
}
template<class T, unsigned SizeInc, unsigned SizeMult, unsigned SizeDiv>
inline mousse::DynamicList<T, SizeInc, SizeMult, SizeDiv>&
mousse::DynamicList<T, SizeInc, SizeMult, SizeDiv>::append
(
  const UIndirectList<T>& lst
)
{
  label nextFree = List<T>::size();
  setSize(nextFree + lst.size());
  FOR_ALL(lst, elemI)
  {
    this->operator[](nextFree++) = lst[elemI];
  }
  return *this;
}
template<class T, unsigned SizeInc, unsigned SizeMult, unsigned SizeDiv>
inline T mousse::DynamicList<T, SizeInc, SizeMult, SizeDiv>::remove()
{
  const label elemI = List<T>::size() - 1;
  if (elemI < 0)
  {
    FATAL_ERROR_IN
    (
      "mousse::DynamicList<T, SizeInc, SizeMult, SizeDiv>::remove()"
    )   << "List is empty" << abort(FatalError);
  }
  const T& val = List<T>::operator[](elemI);
  List<T>::size(elemI);
  return val;
}
// Member Operators 
template<class T, unsigned SizeInc, unsigned SizeMult, unsigned SizeDiv>
inline T& mousse::DynamicList<T, SizeInc, SizeMult, SizeDiv>::operator()
(
  const label elemI
)
{
  if (elemI >= List<T>::size())
  {
    setSize(elemI + 1);
  }
  return this->operator[](elemI);
}
template<class T, unsigned SizeInc, unsigned SizeMult, unsigned SizeDiv>
inline void mousse::DynamicList<T, SizeInc, SizeMult, SizeDiv>::operator=
(
  const T& t
)
{
  UList<T>::operator=(t);
}
template<class T, unsigned SizeInc, unsigned SizeMult, unsigned SizeDiv>
inline void mousse::DynamicList<T, SizeInc, SizeMult, SizeDiv>::operator=
(
  const DynamicList<T, SizeInc, SizeMult, SizeDiv>& lst
)
{
  if (this == &lst)
  {
    FATAL_ERROR_IN
    (
      "DynamicList<T, SizeInc, SizeMult, SizeDiv>::operator="
      "(const DynamicList<T, SizeInc, SizeMult, SizeDiv>&)"
    )   << "attempted assignment to self" << abort(FatalError);
  }
  if (capacity_ >= lst.size())
  {
    // can copy w/o reallocating, match initial size to avoid reallocation
    List<T>::size(lst.size());
    List<T>::operator=(lst);
  }
  else
  {
    // make everything available for the copy operation
    List<T>::size(capacity_);
    List<T>::operator=(lst);
    capacity_ = List<T>::size();
  }
}
template<class T, unsigned SizeInc, unsigned SizeMult, unsigned SizeDiv>
inline void mousse::DynamicList<T, SizeInc, SizeMult, SizeDiv>::operator=
(
  const UList<T>& lst
)
{
  if (capacity_ >= lst.size())
  {
    // can copy w/o reallocating, match initial size to avoid reallocation
    List<T>::size(lst.size());
    List<T>::operator=(lst);
  }
  else
  {
    // make everything available for the copy operation
    List<T>::size(capacity_);
    List<T>::operator=(lst);
    capacity_ = List<T>::size();
  }
}
template<class T, unsigned SizeInc, unsigned SizeMult, unsigned SizeDiv>
inline void mousse::DynamicList<T, SizeInc, SizeMult, SizeDiv>::operator=
(
  const UIndirectList<T>& lst
)
{
  if (capacity_ >= lst.size())
  {
    // can copy w/o reallocating, match initial size to avoid reallocation
    List<T>::size(lst.size());
    List<T>::operator=(lst);
  }
  else
  {
    // make everything available for the copy operation
    List<T>::size(capacity_);
    List<T>::operator=(lst);
    capacity_ = List<T>::size();
  }
}