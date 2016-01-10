// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "matrix.hpp"

// Private Member Functions 
template<class Form, class Type>
void mousse::Matrix<Form, Type>::allocate()
{
  if (n_ && m_)
  {
    v_ = new Type*[n_];
    v_[0] = new Type[n_*m_];
    for (label i=1; i<n_; i++)
    {
      v_[i] = v_[i-1] + m_;
    }
  }
}


// Destructor 
template<class Form, class Type>
mousse::Matrix<Form, Type>::~Matrix()
{
  if (v_)
  {
    delete[] (v_[0]);
    delete[] v_;
  }
}


// Member Functions 
template<class Form, class Type>
mousse::Matrix<Form, Type>::Matrix(const label n, const label m)
:
  n_{n},
  m_{m},
  v_{NULL}
{
  if (n_ < 0 || m_ < 0)
  {
    FATAL_ERROR_IN("Matrix<Form, Type>::Matrix(const label n, const label m)")
      << "bad n, m " << n_ << ", " << m_
      << abort(FatalError);
  }
  allocate();
}


template<class Form, class Type>
mousse::Matrix<Form, Type>::Matrix(const label n, const label m, const Type& a)
:
  n_{n},
  m_{m},
  v_{NULL}
{
  if (n_ < 0 || m_ < 0)
  {
    FATAL_ERROR_IN
    (
      "Matrix<Form, Type>::Matrix(const label n, const label m, const T&)"
    )   << "bad n, m " << n_ << ", " << m_
      << abort(FatalError);
  }
  allocate();
  if (v_)
  {
    Type* v = v_[0];
    label nm = n_*m_;
    for (label i=0; i<nm; i++)
    {
      v[i] = a;
    }
  }
}


template<class Form, class Type>
mousse::Matrix<Form, Type>::Matrix(const Matrix<Form, Type>& a)
:
  n_{a.n_},
  m_{a.m_},
  v_{NULL}
{
  if (a.v_)
  {
    allocate();
    Type* v = v_[0];
    const Type* av = a.v_[0];
    label nm = n_*m_;
    for (label i=0; i<nm; i++)
    {
      v[i] = av[i];
    }
  }
}


template<class Form, class Type>
void mousse::Matrix<Form, Type>::clear()
{
  if (v_)
  {
    delete[] (v_[0]);
    delete[] v_;
  }
  n_ = 0;
  m_ = 0;
  v_ = NULL;
}


template<class Form, class Type>
void mousse::Matrix<Form, Type>::transfer(Matrix<Form, Type>& a)
{
  clear();
  n_ = a.n_;
  a.n_ = 0;
  m_ = a.m_;
  a.m_ = 0;
  v_ = a.v_;
  a.v_ = NULL;
}


template<class Form, class Type>
Form mousse::Matrix<Form, Type>::T() const
{
  const Matrix<Form, Type>& A = *this;
  Form At(m(), n());
  for (label i=0; i<n(); i++)
  {
    for (label j=0; j<m(); j++)
    {
      At[j][i] = A[i][j];
    }
  }
  return At;
}


// Member Operators 
template<class Form, class Type>
void mousse::Matrix<Form, Type>::operator=(const Type& t)
{
  if (v_)
  {
    Type* v = v_[0];
    label nm = n_*m_;
    for (label i=0; i<nm; i++)
    {
      v[i] = t;
    }
  }
}


// Assignment operator. Takes linear time.
template<class Form, class Type>
void mousse::Matrix<Form, Type>::operator=(const Matrix<Form, Type>& a)
{
  if (this == &a)
  {
    FATAL_ERROR_IN("Matrix<Form, Type>::operator=(const Matrix<Form, Type>&)")
      << "attempted assignment to self"
      << abort(FatalError);
  }
  if (n_ != a.n_ || m_ != a.m_)
  {
    clear();
    n_ = a.n_;
    m_ = a.m_;
    allocate();
  }
  if (v_)
  {
    Type* v = v_[0];
    const Type* av = a.v_[0];
    label nm = n_*m_;
    for (label i=0; i<nm; i++)
    {
      v[i] = av[i];
    }
  }
}


// Global Functions 
template<class Form, class Type>
const Type& mousse::max(const Matrix<Form, Type>& a)
{
  label nm = a.n()*a.m();
  if (nm)
  {
    label curMaxI = 0;
    const Type* v = a[0];
    for (label i=1; i<nm; i++)
    {
      if (v[i] > v[curMaxI])
      {
        curMaxI = i;
      }
    }
    return v[curMaxI];
  }
  else
  {
    FATAL_ERROR_IN("max(const Matrix<Form, Type>&)")
      << "matrix is empty"
      << abort(FatalError);
    // Return in error to keep compiler happy
    return a[0][0];
  }
}


template<class Form, class Type>
const Type& mousse::min(const Matrix<Form, Type>& a)
{
  label nm = a.n()*a.m();
  if (nm)
  {
    label curMinI = 0;
    const Type* v = a[0];
    for (label i=1; i<nm; i++)
    {
      if (v[i] < v[curMinI])
      {
        curMinI = i;
      }
    }
    return v[curMinI];
  }
  else
  {
    FATAL_ERROR_IN("min(const Matrix<Form, Type>&)")
      << "matrix is empty"
      << abort(FatalError);
    // Return in error to keep compiler happy
    return a[0][0];
  }
}


// Global Operators 
template<class Form, class Type>
Form mousse::operator-(const Matrix<Form, Type>& a)
{
  Form na(a.n(), a.m());
  if (a.n() && a.m())
  {
    Type* nav = na[0];
    const Type* av = a[0];
    label nm = a.n()*a.m();
    for (label i=0; i<nm; i++)
    {
      nav[i] = -av[i];
    }
  }
  return na;
}


template<class Form, class Type>
Form mousse::operator+(const Matrix<Form, Type>& a, const Matrix<Form, Type>& b)
{
  if (a.n() != b.n())
  {
    FATAL_ERROR_IN
    (
      "Matrix<Form, Type>::operator+"
      "(const Matrix<Form, Type>&, const Matrix<Form, Type>&)"
    )
    << "attempted add matrices with different number of rows: "
    << a.n() << ", " << b.n()
    << abort(FatalError);
  }
  if (a.m() != b.m())
  {
    FATAL_ERROR_IN
    (
      "Matrix<Form, Type>::operator+"
      "(const Matrix<Form, Type>&, const Matrix<Form, Type>&)"
    )
    << "attempted add matrices with different number of columns: "
    << a.m() << ", " << b.m()
    << abort(FatalError);
  }
  Form ab{a.n(), a.m()};
  Type* abv = ab[0];
  const Type* av = a[0];
  const Type* bv = b[0];
  label nm = a.n()*a.m();
  for (label i=0; i<nm; i++)
  {
    abv[i] = av[i] + bv[i];
  }

  return ab;
}


template<class Form, class Type>
Form mousse::operator-(const Matrix<Form, Type>& a, const Matrix<Form, Type>& b)
{
  if (a.n() != b.n())
  {
    FATAL_ERROR_IN
    (
      "Matrix<Form, Type>::operator-"
      "(const Matrix<Form, Type>&, const Matrix<Form, Type>&)"
    )
    << "attempted add matrices with different number of rows: "
    << a.n() << ", " << b.n()
    << abort(FatalError);
  }
  if (a.m() != b.m())
  {
    FATAL_ERROR_IN
    (
      "Matrix<Form, Type>::operator-"
      "(const Matrix<Form, Type>&, const Matrix<Form, Type>&)"
    )
    << "attempted add matrices with different number of columns: "
    << a.m() << ", " << b.m()
    << abort(FatalError);
  }
  Form ab{a.n(), a.m()};
  Type* abv = ab[0];
  const Type* av = a[0];
  const Type* bv = b[0];
  label nm = a.n()*a.m();
  for (label i=0; i<nm; i++)
  {
    abv[i] = av[i] - bv[i];
  }

  return ab;
}


template<class Form, class Type>
Form mousse::operator*(const scalar s, const Matrix<Form, Type>& a)
{
  Form sa{a.n(), a.m()};
  if (a.n() && a.m())
  {
    Type* sav = sa[0];
    const Type* av = a[0];
    label nm = a.n()*a.m();
    for (label i=0; i<nm; i++)
    {
      sav[i] = s*av[i];
    }
  }
  return sa;
}


template<class Form, class Type>
Form mousse::operator*(const Matrix<Form, Type>& a, const Matrix<Form, Type>& b)
{
  if (a.m() != b.n())
  {
    FATAL_ERROR_IN
    (
      "Matrix<Form, Type>::operator*"
      "(const Matrix<Form, Type>&, const Matrix<Form, Type>&)"
    )
    << "attempted to multiply incompatible matrices:" << nl
    << "Matrix A : " << a.n() << " rows, " << a.m() << " columns" << nl
    << "Matrix B : " << b.n() << " rows, " << b.m() << " columns" << nl
    << "In order to multiply matrices, columns of A must equal "
    << "rows of B"
    << abort(FatalError);
  }
  Form ab{a.n(), b.m(), scalar(0)};
  for (label i = 0; i < ab.n(); i++)
  {
    for (label j = 0; j < ab.m(); j++)
    {
      for (label l = 0; l < b.n(); l++)
      {
        ab[i][j] += a[i][l]*b[l][j];
      }
    }
  }

  return ab;
}

//  IOStream operators
#include "matrix_io.cpp"
