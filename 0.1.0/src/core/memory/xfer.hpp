#ifndef CORE_MEMORY_XFER_HPP_
#define CORE_MEMORY_XFER_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::Xfer
// Description
//   A simple container for copying or transferring objects of type \<T\>.
//   The wrapped object of type \<T\> must implement a transfer() method and
//   an operator=() copy method.
//   Since it is decided upon construction of the Xfer object whether the
//   parameter is to be copied or transferred, the contents of the resulting
//   Xfer object can be transferred unconditionally. This greatly simplifies
//   defining constructors or methods in other classes with mixed
//   transfer/copy semantics without requiring 2^N different versions.
//   When transferring between dissimilar types, the xferCopyTo() and
//   xferMoveTo() functions can prove useful. An example is transferring
//   from a DynamicList to a List.  Since the
//   List\<T\>::transfer(List\<T\>&) method could result in some allocated
//   memory becoming inaccessible, the xferMoveTo() function should be used to
//   invoke the correct List\<T\>::transfer(DynamicList\<T\>&) method.
//   \code
//     DynamicList<label> dynLst;
//     ...
//     labelList plainLst( xferMoveTo<labelList>(dynLst) );
//   \endcode
//   Of course, since this example is a very common operation, the
//   DynamicList::xfer() method transfers to a plain List anyhow.
//   It would thus be simpler (and clearer) just to use the following code:
//   \code
//     DynamicList<label> dynLst;
//     ...
//     labelList plainLst(dynLst.xfer());
//   \endcode
// SeeAlso
//   xferCopy, xferCopyTo, xferMove, xferMoveTo, xferTmp

#include "null_object.hpp"


namespace mousse {

// Forward declaration of classes
template<class T> class tmp;


template<class T>
class Xfer
{
  // Private data
    //- Pointer to underlying datatype
    mutable T* ptr_;
public:
  // Constructors
    //- Store object pointer and manage its deletion
    //  Can also be used later to transfer by assignment
    inline explicit Xfer(T* = nullptr);
    //- Construct by copying or by transferring the parameter contents
    inline explicit Xfer(T&, bool allowTransfer=false);
    //- Construct by copying the parameter contents
    inline explicit Xfer(const T&);
    //- Construct by transferring the contents
    inline Xfer(const Xfer<T>&);
  //- Destructor
  inline ~Xfer();
  // Member Functions
    //- Return a null object reference
    inline static const Xfer<T>& null();
  // Member Operators
    //- Transfer the contents into the object
    inline void operator=(T&);
    //- Transfer the contents into the object
    inline void operator=(const Xfer<T>&);
    //- Reference to the underlying datatype
    inline T& operator()() const;
    //- Pointer to the underlying datatype
    inline T* operator->() const;
};


/**
* Construct by copying the contents of the \a arg
*
* \sa xferCopyTo, xferMove, xferMoveTo, xferTmp and mousse::Xfer
*/
template<class T>
inline Xfer<T> xferCopy(const T&);


/**
* Construct by transferring the contents of the \a arg
*
* \sa xferCopy, xferCopyTo, xferMoveTo, xferTmp and mousse::Xfer
*/
template<class T>
inline Xfer<T> xferMove(T&);


/**
* Construct by transferring the contents of the \a arg
*
* \sa xferCopy, xferCopyTo, xferMove, xferMoveTo and mousse::Xfer
*/
template<class T>
inline Xfer<T> xferTmp(mousse::tmp<T>&);


/**
* Construct by copying the contents of the \a arg
* between dissimilar types
*
* \sa xferCopy, xferMove, xferMoveTo, xferTmp and mousse::Xfer
*/
template<class To, class From>
inline Xfer<To> xferCopyTo(const From&);


/**
* Construct by transferring the contents of the \a arg
* between dissimilar types
*
* \par Example Use
* \code
*     DynamicList<label> dynLst;
*     ...
*     labelList plainLst( xferMoveTo<labelList>(dynLst) );
* \endcode
*
* \sa xferCopy, xferCopyTo, xferMove, xferTmp and mousse::Xfer
*/
template<class To, class From>
inline Xfer<To> xferMoveTo(From&);

}  // namespace mousse


// Static Member Functions
template<class T>
inline const mousse::Xfer<T>& mousse::Xfer<T>::null()
{
  return NullObjectRef<Xfer<T>>();
}


// Constructors 
template<class T>
inline mousse::Xfer<T>::Xfer(T* p)
:
  ptr_{p ? p : new T}
{}


template<class T>
inline mousse::Xfer<T>::Xfer(T& t, bool allowTransfer)
:
  ptr_{new T}
{
  if (allowTransfer) {
    ptr_->transfer(t);
  } else {
    ptr_->operator=(t);
  }
}


template<class T>
inline mousse::Xfer<T>::Xfer(const T& t)
:
  ptr_{new T}
{
  ptr_->operator=(t);
}


template<class T>
inline mousse::Xfer<T>::Xfer(const Xfer<T>& t)
:
  ptr_{new T}
{
  ptr_->transfer(*(t.ptr_));
}


// Destructor 
template<class T>
inline mousse::Xfer<T>::~Xfer()
{
  delete ptr_;
  ptr_ = nullptr;
}


//  Member Functions
// Member Operators 
template<class T>
inline void mousse::Xfer<T>::operator=(T& t)
{
  ptr_->transfer(t);
}


template<class T>
inline void mousse::Xfer<T>::operator=(const Xfer<T>& t)
{
  // silently ignore attempted copy to self
  if (this != &t) {
    ptr_->transfer(*(t.ptr_));
  }
}


template<class T>
inline T& mousse::Xfer<T>::operator()() const
{
  return *ptr_;
}


template<class T>
inline T* mousse::Xfer<T>::operator->() const
{
  return ptr_;
}


//  Helper Functions
template<class T>
inline mousse::Xfer<T> mousse::xferCopy(const T& t)
{
  return mousse::Xfer<T>(t);
}


template<class T>
inline mousse::Xfer<T> mousse::xferMove(T& t)
{
  return mousse::Xfer<T>(t, true);
}


template<class T>
inline mousse::Xfer<T> mousse::xferTmp(mousse::tmp<T>& tt)
{
  return mousse::Xfer<T>(tt(), tt.isTmp());
}


template<class To, class From>
inline mousse::Xfer<To> mousse::xferCopyTo(const From& t)
{
  mousse::Xfer<To> xf;
  xf() = t;
  return xf;
}


template<class To, class From>
inline mousse::Xfer<To> mousse::xferMoveTo(From& t)
{
  mousse::Xfer<To> xf;
  xf().transfer(t);
  return xf;
}

#endif
