#ifndef CORE_PRIMITIVES_SYMM_TENSOR_2D_TSYMM_TENSOR_2D_HPP_
#define CORE_PRIMITIVES_SYMM_TENSOR_2D_TSYMM_TENSOR_2D_HPP_

// mousse: CFD toolbox
// Copyright (C) 2011-2013 OpenFOAM Foundation
// Copyright (C) 2016 mousse project
// Class
//   mousse::SymmTensor2D
// Description
//   Templated 2D symmetric tensor derived from VectorSpace adding construction
//   from 4 components, element access using xx(), xy() etc. member functions
//   and the inner-product (dot-product) and outer-product of two Vectors
//   (tensor-product) operators.

#include "vector_space.hpp"
#include "_spherical_tensor_2d.hpp"
#include "_tensor_2d.hpp"


namespace mousse {

template<class Cmpt>
class SymmTensor2D
:
  public VectorSpace<SymmTensor2D<Cmpt>, Cmpt, 3>
{
public:
  //- Equivalent type of labels used for valid component indexing
  typedef SymmTensor2D<label> labelType;
  // Member constants
    enum
    {
      rank = 2 // Rank of SymmTensor2D is 2
    };
  // Static data members
    static const char* const typeName;
    static const char* componentNames[];
    static const SymmTensor2D zero;
    static const SymmTensor2D one;
    static const SymmTensor2D max;
    static const SymmTensor2D min;
    static const SymmTensor2D I;
  //- Component labeling enumeration
  enum components { XX, XY, YY };
  // Constructors
    //- Construct null
    inline SymmTensor2D();
    //- Construct given VectorSpace
    inline SymmTensor2D(const VectorSpace<SymmTensor2D<Cmpt>, Cmpt, 3>&);
    //- Construct given SphericalTensor
    inline SymmTensor2D(const SphericalTensor2D<Cmpt>&);
    //- Construct given the three components
    inline SymmTensor2D
    (
      const Cmpt txx, const Cmpt txy,
              const Cmpt tyy
    );
    //- Construct from Istream
    SymmTensor2D(Istream&);
  // Member Functions
    // Access
      inline const Cmpt& xx() const;
      inline const Cmpt& xy() const;
      inline const Cmpt& yy() const;
      inline Cmpt& xx();
      inline Cmpt& xy();
      inline Cmpt& yy();
    //- Transpose
    inline const SymmTensor2D<Cmpt>& T() const;
  // Member Operators
    //- Construct given SphericalTensor2D
    inline void operator=(const SphericalTensor2D<Cmpt>&);
};

}  // namespace mousse


namespace mousse {

// Constructors
template<class Cmpt>
inline SymmTensor2D<Cmpt>::SymmTensor2D()
{}


template<class Cmpt>
inline SymmTensor2D<Cmpt>::SymmTensor2D
(
  const VectorSpace<SymmTensor2D<Cmpt>, Cmpt, 3>& vs
)
:
  VectorSpace<SymmTensor2D<Cmpt>, Cmpt, 3>{vs}
{}


template<class Cmpt>
inline SymmTensor2D<Cmpt>::SymmTensor2D(const SphericalTensor2D<Cmpt>& st)
{
  this->v_[XX] = st.ii(); this->v_[XY] = 0;
                          this->v_[YY] = st.ii();
}


template<class Cmpt>
inline SymmTensor2D<Cmpt>::SymmTensor2D
(
  const Cmpt txx, const Cmpt txy,
          const Cmpt tyy
)
{
  this->v_[XX] = txx; this->v_[XY] = txy;
                      this->v_[YY] = tyy;
}


template<class Cmpt>
inline SymmTensor2D<Cmpt>::SymmTensor2D(Istream& is)
:
  VectorSpace<SymmTensor2D<Cmpt>, Cmpt, 3>{is}
{}


// Member Functions
template<class Cmpt>
inline const Cmpt& SymmTensor2D<Cmpt>::xx() const
{
  return this->v_[XX];
}


template<class Cmpt>
inline const Cmpt& SymmTensor2D<Cmpt>::xy() const
{
  return this->v_[XY];
}


template<class Cmpt>
inline const Cmpt& SymmTensor2D<Cmpt>::yy() const
{
  return this->v_[YY];
}


template<class Cmpt>
inline Cmpt& SymmTensor2D<Cmpt>::xx()
{
  return this->v_[XX];
}


template<class Cmpt>
inline Cmpt& SymmTensor2D<Cmpt>::xy()
{
  return this->v_[XY];
}


template<class Cmpt>
inline Cmpt& SymmTensor2D<Cmpt>::yy()
{
  return this->v_[YY];
}


template<class Cmpt>
inline const SymmTensor2D<Cmpt>& SymmTensor2D<Cmpt>::T() const
{
  return *this;
}


// Member Functions
template<class Cmpt>
inline void SymmTensor2D<Cmpt>::operator=(const SphericalTensor2D<Cmpt>& st)
{
  this->v_[XX] = st.ii(); this->v_[XY] = 0;
                          this->v_[YY] = st.ii();
}


// Global Operators
//- Inner-product between two symmetric tensors
template<class Cmpt>
inline Tensor2D<Cmpt>
operator&(const SymmTensor2D<Cmpt>& st1, const SymmTensor2D<Cmpt>& st2)
{
  return {st1.xx()*st2.xx() + st1.xy()*st2.xy(),
          st1.xx()*st2.xy() + st1.xy()*st2.yy(),
          st1.xy()*st2.xx() + st1.yy()*st2.xy(),
          st1.xy()*st2.xy() + st1.yy()*st2.yy()};
}


//- Double-dot-product between a symmetric tensor and a symmetric tensor
template<class Cmpt>
inline Cmpt
operator&&(const SymmTensor2D<Cmpt>& st1, const SymmTensor2D<Cmpt>& st2)
{
  return st1.xx()*st2.xx() + 2*st1.xy()*st2.xy() + st1.yy()*st2.yy();
}


//- Inner-product between a symmetric tensor and a vector
template<class Cmpt>
inline Vector2D<Cmpt>
operator&(const SymmTensor2D<Cmpt>& st, const Vector2D<Cmpt>& v)
{
  return { st.xx()*v.x() + st.xy()*v.y(), st.xy()*v.x() + st.yy()*v.y() };
}


//- Inner-product between a vector and a symmetric tensor
template<class Cmpt>
inline Vector2D<Cmpt>
operator&(const Vector2D<Cmpt>& v, const SymmTensor2D<Cmpt>& st)
{
  return {v.x()*st.xx() + v.y()*st.xy(),
          v.x()*st.xy() + v.y()*st.yy()};
}


//- Inner-sqr of a symmetric tensor
template<class Cmpt>
inline SymmTensor2D<Cmpt>
innerSqr(const SymmTensor2D<Cmpt>& st)
{
  return {st.xx()*st.xx() + st.xy()*st.xy(),
          st.xx()*st.xy() + st.xy()*st.yy(),
          st.xy()*st.xy() + st.yy()*st.yy()};
}


template<class Cmpt>
inline Cmpt magSqr(const SymmTensor2D<Cmpt>& st)
{
  return magSqr(st.xx()) + 2*magSqr(st.xy()) + magSqr(st.yy());
}


//- Return the trace of a symmetric tensor
template<class Cmpt>
inline Cmpt tr(const SymmTensor2D<Cmpt>& st)
{
  return st.xx() + st.yy();
}


//- Return the spherical part of a symmetric tensor
template<class Cmpt>
inline SphericalTensor2D<Cmpt> sph(const SymmTensor2D<Cmpt>& st)
{
  return (1.0/2.0)*tr(st);
}


//- Return the symmetric part of a symmetric tensor, i.e. itself
template<class Cmpt>
inline const SymmTensor2D<Cmpt>& symm(const SymmTensor2D<Cmpt>& st)
{
  return st;
}


//- Return twice the symmetric part of a symmetric tensor
template<class Cmpt>
inline SymmTensor2D<Cmpt> twoSymm(const SymmTensor2D<Cmpt>& st)
{
  return 2*st;
}


//- Return the deviatoric part of a symmetric tensor
template<class Cmpt>
inline SymmTensor2D<Cmpt> dev(const SymmTensor2D<Cmpt>& st)
{
  return st - SphericalTensor2D<Cmpt>::oneThirdI*tr(st);
}


//- Return the deviatoric part of a symmetric tensor
template<class Cmpt>
inline SymmTensor2D<Cmpt> dev2(const SymmTensor2D<Cmpt>& st)
{
  return st - SphericalTensor2D<Cmpt>::twoThirdsI*tr(st);
}


//- Return the determinant of a symmetric tensor
template<class Cmpt>
inline Cmpt det(const SymmTensor2D<Cmpt>& st)
{
  return st.xx()*st.yy() - st.xy()*st.xy();
}


//- Return the cofactor symmetric tensor of a symmetric tensor
template<class Cmpt>
inline SymmTensor2D<Cmpt> cof(const SymmTensor2D<Cmpt>& st)
{
  return {st.yy(), -st.xy(), st.xx()};
}


//- Return the inverse of a symmetric tensor give the determinant
template<class Cmpt>
inline SymmTensor2D<Cmpt> inv(const SymmTensor2D<Cmpt>& st, const Cmpt detst)
{
  return cof(st)/detst;
}


//- Return the inverse of a symmetric tensor
template<class Cmpt>
inline SymmTensor2D<Cmpt> inv(const SymmTensor2D<Cmpt>& st)
{
  return inv(st, det(st));
}


//- Return the 1st invariant of a symmetric tensor
template<class Cmpt>
inline Cmpt invariantI(const SymmTensor2D<Cmpt>& st)
{
  return tr(st);
}


//- Return the 2nd invariant of a symmetric tensor
template<class Cmpt>
inline Cmpt invariantII(const SymmTensor2D<Cmpt>& st)
{
  return 0.5*sqr(tr(st)) - 0.5*(st.xx()*st.xx() + st.xy()*st.xy()
                                + st.xy()*st.xy() + st.yy()*st.yy());
}


//- Return the 3rd invariant of a symmetric tensor
template<class Cmpt>
inline Cmpt invariantIII(const SymmTensor2D<Cmpt>& st)
{
  return det(st);
}


template<class Cmpt>
inline SymmTensor2D<Cmpt>
operator+(const SphericalTensor2D<Cmpt>& spt1, const SymmTensor2D<Cmpt>& st2)
{
  return {spt1.ii() + st2.xx(), st2.xy(), spt1.ii() + st2.yy()};
}


template<class Cmpt>
inline SymmTensor2D<Cmpt>
operator+(const SymmTensor2D<Cmpt>& st1, const SphericalTensor2D<Cmpt>& spt2)
{
  return {st1.xx() + spt2.ii(), st1.xy(), st1.yy() + spt2.ii()};
}


template<class Cmpt>
inline SymmTensor2D<Cmpt>
operator-(const SphericalTensor2D<Cmpt>& spt1, const SymmTensor2D<Cmpt>& st2)
{
  return {spt1.ii() - st2.xx(), -st2.xy(), spt1.ii() - st2.yy()};
}


template<class Cmpt>
inline SymmTensor2D<Cmpt>
operator-(const SymmTensor2D<Cmpt>& st1, const SphericalTensor2D<Cmpt>& spt2)
{
  return {st1.xx() - spt2.ii(), st1.xy(), st1.yy() - spt2.ii()};
}


//- Inner-product between a spherical symmetric tensor and a symmetric tensor
template<class Cmpt>
inline SymmTensor2D<Cmpt>
operator&(const SphericalTensor2D<Cmpt>& spt1, const SymmTensor2D<Cmpt>& st2)
{
  return {spt1.ii()*st2.xx(), spt1.ii()*st2.xy(), spt1.ii()*st2.yy()};
}


//- Inner-product between a tensor and a spherical tensor
template<class Cmpt>
inline SymmTensor2D<Cmpt>
operator&(const SymmTensor2D<Cmpt>& st1, const SphericalTensor2D<Cmpt>& spt2)
{
  return {st1.xx()*spt2.ii(), st1.xy()*spt2.ii(), st1.yy()*spt2.ii()};
}


//- Double-dot-product between a spherical tensor and a symmetric tensor
template<class Cmpt>
inline Cmpt
operator&&(const SphericalTensor2D<Cmpt>& spt1, const SymmTensor2D<Cmpt>& st2)
{
  return spt1.ii()*st2.xx() + spt1.ii()*st2.yy();
}


//- Double-dot-product between a tensor and a spherical tensor
template<class Cmpt>
inline Cmpt
operator&&(const SymmTensor2D<Cmpt>& st1, const SphericalTensor2D<Cmpt>& spt2)
{
  return st1.xx()*spt2.ii() + st1.yy()*spt2.ii();
}


template<class Cmpt>
inline SymmTensor2D<Cmpt> sqr(const Vector2D<Cmpt>& v)
{
  return {v.x()*v.x(), v.x()*v.y(), v.y()*v.y()};
}


template<class Cmpt>
class outerProduct<SymmTensor2D<Cmpt>, Cmpt>
{
public:
  typedef SymmTensor2D<Cmpt> type;
};


template<class Cmpt>
class outerProduct<Cmpt, SymmTensor2D<Cmpt> >
{
public:
  typedef SymmTensor2D<Cmpt> type;
};


template<class Cmpt>
class innerProduct<SymmTensor2D<Cmpt>, SymmTensor2D<Cmpt> >
{
public:
  typedef Tensor2D<Cmpt> type;
};


template<class Cmpt>
class innerProduct<SymmTensor2D<Cmpt>, Vector2D<Cmpt> >
{
public:
  typedef Vector2D<Cmpt> type;
};


template<class Cmpt>
class innerProduct<Vector2D<Cmpt>, SymmTensor2D<Cmpt> >
{
public:
  typedef Vector2D<Cmpt> type;
};


template<class Cmpt>
class typeOfSum<SphericalTensor2D<Cmpt>, SymmTensor2D<Cmpt> >
{
public:
  typedef SymmTensor2D<Cmpt> type;
};


template<class Cmpt>
class typeOfSum<SymmTensor2D<Cmpt>, SphericalTensor2D<Cmpt> >
{
public:
  typedef SymmTensor2D<Cmpt> type;
};


template<class Cmpt>
class innerProduct<SphericalTensor2D<Cmpt>, SymmTensor2D<Cmpt> >
{
public:
  typedef SymmTensor2D<Cmpt> type;
};


template<class Cmpt>
class innerProduct<SymmTensor2D<Cmpt>, SphericalTensor2D<Cmpt> >
{
public:
  typedef SymmTensor2D<Cmpt> type;
};


}  // namespace mousse
#endif