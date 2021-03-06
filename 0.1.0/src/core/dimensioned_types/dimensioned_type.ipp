// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "dimensioned_type.hpp"
#include "ptraits.hpp"
#include "dictionary.hpp"


// Member Functions
template<class Type>
void mousse::dimensioned<Type>::initialize(Istream& is)
{
  token nextToken{is};
  is.putBack(nextToken);
  // Check if the original format is used in which the name is provided
  // and reset the name to that read
  if (nextToken.isWord()) {
    is >> name_;
    is >> nextToken;
    is.putBack(nextToken);
  }
  // If the dimensions are provided compare with the argument
  scalar multiplier = 1.0;
  if (nextToken == token::BEGIN_SQR) {
    dimensionSet dims{dimless};
    dims.read(is, multiplier);
    if (dims != dimensions_) {
      FATAL_IO_ERROR_IN
      (
        "dimensioned<Type>::dimensioned"
        "(const word&, const dimensionSet&, Istream&)",
        is
      )
      << "The dimensions " << dims
      << " provided do not match the required dimensions "
      << dimensions_
      << abort(FatalIOError);
    }
  }
  is >> value_;
  value_ *= multiplier;
}


// Constructors
template<class Type>
mousse::dimensioned<Type>::dimensioned
(
  const word& name,
  const dimensionSet& dimSet,
  const Type t
)
:
  name_{name},
  dimensions_{dimSet},
  value_{t}
{}


template<class Type>
mousse::dimensioned<Type>::dimensioned
(
  const word& name,
  const dimensioned<Type>& dt
)
:
  name_{name},
  dimensions_{dt.dimensions_},
  value_{dt.value_}
{}


template<class Type>
mousse::dimensioned<Type>::dimensioned
(
  Istream& is
)
:
  dimensions_{dimless}
{
  read(is);
}


template<class Type>
mousse::dimensioned<Type>::dimensioned
(
  const word& name,
  Istream& is
)
:
  name_{name},
  dimensions_{dimless}
{
  scalar multiplier;
  dimensions_.read(is, multiplier);
  is >> value_;
  value_ *= multiplier;
}


template<class Type>
mousse::dimensioned<Type>::dimensioned
(
  const word& name,
  const dimensionSet& dimSet,
  Istream& is
)
:
  name_{name},
  dimensions_{dimSet},
  value_{pTraits<Type>::zero}
{
  initialize(is);
}


template<class Type>
mousse::dimensioned<Type>::dimensioned
(
  const word& name,
  const dimensionSet& dimSet,
  const dictionary& dict
)
:
  name_{name},
  dimensions_{dimSet},
  value_{pTraits<Type>::zero}
{
  initialize(dict.lookup(name));
}


template<class Type>
mousse::dimensioned<Type>::dimensioned
()
:
  name_{"undefined"},
  dimensions_{dimless},
  value_{pTraits<Type>::zero}
{}


// Static Member Functions
template<class Type>
mousse::dimensioned<Type> mousse::dimensioned<Type>::lookupOrDefault
(
  const word& name,
  const dictionary& dict,
  const dimensionSet& dims,
  const Type& defaultValue
)
{
  if (dict.found(name)) {
    return dimensioned<Type>(name, dims, dict.lookup(name));
  } else {
    return dimensioned<Type>(name, dims, defaultValue);
  }
}


template<class Type>
mousse::dimensioned<Type> mousse::dimensioned<Type>::lookupOrDefault
(
  const word& name,
  const dictionary& dict,
  const Type& defaultValue
)
{
  return lookupOrDefault(name, dict, dimless, defaultValue);
}


template<class Type>
mousse::dimensioned<Type> mousse::dimensioned<Type>::lookupOrAddToDict
(
  const word& name,
  dictionary& dict,
  const dimensionSet& dims,
  const Type& defaultValue
)
{
  Type value = dict.lookupOrAddDefault<Type>(name, defaultValue);
  return dimensioned<Type>(name, dims, value);
}


template<class Type>
mousse::dimensioned<Type> mousse::dimensioned<Type>::lookupOrAddToDict
(
  const word& name,
  dictionary& dict,
  const Type& defaultValue
)
{
  return lookupOrAddToDict(name, dict, dimless, defaultValue);
}


// Member Functions
template<class Type>
const mousse::word& mousse::dimensioned<Type>::name() const
{
  return name_;
}


template<class Type>
mousse::word& mousse::dimensioned<Type>::name()
{
  return name_;
}


template<class Type>
const mousse::dimensionSet& mousse::dimensioned<Type>::dimensions() const
{
  return dimensions_;
}


template<class Type>
mousse::dimensionSet& mousse::dimensioned<Type>::dimensions()
{
  return dimensions_;
}


template<class Type>
const Type& mousse::dimensioned<Type>::value() const
{
  return value_;
}


template<class Type>
Type& mousse::dimensioned<Type>::value()
{
  return value_;
}


template<class Type>
mousse::dimensioned<typename mousse::dimensioned<Type>::cmptType>
mousse::dimensioned<Type>::component
(
  const direction d
) const
{
  return // dimensioned<cmptType>
  {
    name_ + ".component(" + mousse::name(d) + ')',
    dimensions_,
    value_.component(d)
  };
}


template<class Type>
void mousse::dimensioned<Type>::replace
(
  const direction d,
  const dimensioned<typename dimensioned<Type>::cmptType>& dc
)
{
  dimensions_ = dc.dimensions();
  value_.replace(d, dc.value());
}


template<class Type>
void mousse::dimensioned<Type>::read(const dictionary& dict)
{
  dict.lookup(name_) >> value_;
}


template<class Type>
bool mousse::dimensioned<Type>::readIfPresent(const dictionary& dict)
{
  return dict.readIfPresent(name_, value_);
}


template<class Type>
mousse::Istream&
mousse::dimensioned<Type>::read(Istream& is, const dictionary& readSet)
{
  // Read name
  is >> name_;
  // Read dimensionSet + multiplier
  scalar mult;
  dimensions_.read(is, mult, readSet);
  // Read value
  is >> value_;
  value_ *= mult;
  // Check state of Istream
  is.check
  (
    "Istream& dimensioned<Type>::read(Istream& is, const dictionary&)"
  );
  return is;
}


template<class Type>
mousse::Istream& mousse::dimensioned<Type>::read
(
  Istream& is,
  const HashTable<dimensionedScalar>& readSet
)
{
  // Read name
  is >> name_;
  // Read dimensionSet + multiplier
  scalar mult;
  dimensions_.read(is, mult, readSet);
  // Read value
  is >> value_;
  value_ *= mult;
  // Check state of Istream
  is.check
  (
    "Istream& dimensioned<Type>::read"
    "(Istream& is, const HashTable<dimensionedScalar>&)"
  );
  return is;
}


template<class Type>
mousse::Istream& mousse::dimensioned<Type>::read(Istream& is)
{
  // Read name
  is >> name_;
  // Read dimensionSet + multiplier
  scalar mult;
  dimensions_.read(is, mult);
  // Read value
  is >> value_;
  value_ *= mult;
  // Check state of Istream
  is.check
  (
    "Istream& dimensioned<Type>::read(Istream& is)"
  );
  return is;
}


// Member Operators
template<class Type>
mousse::dimensioned<typename mousse::dimensioned<Type>::cmptType>
mousse::dimensioned<Type>::operator[]
(
  const direction d
) const
{
  return component(d);
}


template<class Type>
void mousse::dimensioned<Type>::operator+=
(
  const dimensioned<Type>& dt
)
{
  dimensions_ += dt.dimensions_;
  value_ += dt.value_;
}


template<class Type>
void mousse::dimensioned<Type>::operator-=
(
  const dimensioned<Type>& dt
)
{
  dimensions_ -= dt.dimensions_;
  value_ -= dt.value_;
}


template<class Type>
void mousse::dimensioned<Type>::operator*=
(
  const scalar s
)
{
  value_ *= s;
}


template<class Type>
void mousse::dimensioned<Type>::operator/=
(
  const scalar s
)
{
  value_ /= s;
}


// Friend Functions
template<class Type, int r>
mousse::dimensioned<typename mousse::powProduct<Type, r>::type>
mousse::pow(const dimensioned<Type>& dt, typename powProduct<Type, r>::type)
{
  return // dimensioned<typename powProduct<Type, r>::type>
  {
    "pow(" + dt.name() + ',' + name(r) + ')',
    pow(dt.dimensions(), r),
    pow(dt.value(), 2)
  };
}


template<class Type>
mousse::dimensioned<typename mousse::outerProduct<Type, Type>::type>
mousse::sqr(const dimensioned<Type>& dt)
{
  return // dimensioned<typename outerProduct<Type, Type>::type>
  {
    "sqr(" + dt.name() + ')',
    sqr(dt.dimensions()),
    sqr(dt.value())
  };
}


template<class Type>
mousse::dimensioned<mousse::scalar> mousse::magSqr(const dimensioned<Type>& dt)
{
  return // dimensioned<scalar>
  {
    "magSqr(" + dt.name() + ')',
    magSqr(dt.dimensions()),
    magSqr(dt.value())
  };
}


template<class Type>
mousse::dimensioned<mousse::scalar> mousse::mag(const dimensioned<Type>& dt)
{
  return // dimensioned<scalar>
  {
    "mag(" + dt.name() + ')',
    dt.dimensions(),
    mag(dt.value())
  };
}


template<class Type>
mousse::dimensioned<Type> mousse::cmptMultiply
(
  const dimensioned<Type>& dt1,
  const dimensioned<Type>& dt2
)
{
  return // dimensioned<Type>
  {
    "cmptMultiply(" + dt1.name() + ',' + dt2.name() + ')',
    cmptMultiply(dt1.dimensions(), dt2.dimensions()),
    cmptMultiply(dt1.value(), dt2.value())
  };
}


template<class Type>
mousse::dimensioned<Type> mousse::cmptDivide
(
  const dimensioned<Type>& dt1,
  const dimensioned<Type>& dt2
)
{
  return // dimensioned<Type>
  {
    "cmptDivide(" + dt1.name() + ',' + dt2.name() + ')',
    cmptDivide(dt1.dimensions(), dt2.dimensions()),
    cmptDivide(dt1.value(), dt2.value())
  };
}


template<class Type>
mousse::dimensioned<Type> mousse::max
(
  const dimensioned<Type>& dt1,
  const dimensioned<Type>& dt2
)
{
  if (dt1.dimensions() != dt2.dimensions()) {
    FATAL_ERROR_IN("max(const dimensioned<Type>&, const dimensioned<Type>&)")
      << "dimensions of arguments are not equal"
      << abort(FatalError);
  }
  return // dimensioned<Type>
  {
    "max(" + dt1.name() + ',' + dt2.name() + ')',
    dt1.dimensions(),
    max(dt1.value(), dt2.value())
  };
}


template<class Type>
mousse::dimensioned<Type> mousse::min
(
  const dimensioned<Type>& dt1,
  const dimensioned<Type>& dt2
)
{
  if (dt1.dimensions() != dt2.dimensions()) {
    FATAL_ERROR_IN("min(const dimensioned<Type>&, const dimensioned<Type>&)")
      << "dimensions of arguments are not equal"
      << abort(FatalError);
  }
  return // dimensioned<Type>
  {
    "min(" + dt1.name() + ',' + dt2.name() + ')',
    dt1.dimensions(),
    min(dt1.value(), dt2.value())
  };
}


// IOstream Operators
template<class Type>
mousse::Istream& mousse::operator>>(Istream& is, dimensioned<Type>& dt)
{
  token nextToken{is};
  is.putBack(nextToken);
  // Check if the original format is used in which the name is provided
  // and reset the name to that read
  if (nextToken.isWord()) {
    is >> dt.name_;
    is >> nextToken;
    is.putBack(nextToken);
  }
  // If the dimensions are provided reset the dimensions to those read
  scalar multiplier = 1.0;
  if (nextToken == token::BEGIN_SQR) {
    dt.dimensions_.read(is, multiplier);
  }
  // Read the value
  is >> dt.value_;
  dt.value_ *= multiplier;
  // Check state of Istream
  is.check("Istream& operator>>(Istream&, dimensioned<Type>&)");
  return is;
}


template<class Type>
mousse::Ostream& mousse::operator<<(Ostream& os, const dimensioned<Type>& dt)
{
  // Write the name
  os << dt.name() << token::SPACE;
  // Write the dimensions
  scalar mult;
  dt.dimensions().write(os, mult);
  os << token::SPACE;
  // Write the value
  os << dt.value()/mult;
  // Check state of Ostream
  os.check("Ostream& operator<<(Ostream&, const dimensioned<Type>&)");
  return os;
}


// Global Operators
template<class Type>
bool mousse::operator>
(
  const dimensioned<Type>& dt1,
  const dimensioned<Type>& dt2
)
{
  return dt1.value() > dt2.value();
}


template<class Type>
bool mousse::operator<
(
  const dimensioned<Type>& dt1,
  const dimensioned<Type>& dt2
)
{
  return dt1.value() < dt2.value();
}


template<class Type>
mousse::dimensioned<Type> mousse::operator+
(
  const dimensioned<Type>& dt1,
  const dimensioned<Type>& dt2
)
{
  return // dimensioned<Type>
  {
    '(' + dt1.name() + '+' + dt2.name() + ')',
    dt1.dimensions() + dt2.dimensions(),
    dt1.value() + dt2.value()
  };
}


template<class Type>
mousse::dimensioned<Type> mousse::operator-(const dimensioned<Type>& dt)
{
  return // dimensioned<Type>
  {
    '-' + dt.name(),
    dt.dimensions(),
    -dt.value()
  };
}


template<class Type>
mousse::dimensioned<Type> mousse::operator-
(
  const dimensioned<Type>& dt1,
  const dimensioned<Type>& dt2
)
{
  return // dimensioned<Type>
  {
    '(' + dt1.name() + '-' + dt2.name() + ')',
    dt1.dimensions() - dt2.dimensions(),
    dt1.value() - dt2.value()
  };
}


template<class Type>
mousse::dimensioned<Type> mousse::operator*
(
  const dimensioned<scalar>& ds,
  const dimensioned<Type>& dt
)
{
  return // dimensioned<Type>
  {
    '(' + ds.name() + '*' + dt.name() + ')',
    ds.dimensions() * dt.dimensions(),
    ds.value() * dt.value()
  };
}


template<class Type>
mousse::dimensioned<Type> mousse::operator/
(
  const dimensioned<Type>& dt,
  const dimensioned<scalar>& ds
)
{
  return // dimensioned<Type>
  {
    '(' + dt.name() + '|' + ds.name() + ')',
    dt.dimensions()/ds.dimensions(),
    dt.value()/ds.value()
  };
}


// Products
#define PRODUCT_OPERATOR(product, op, opFunc)                                 \
                                                                              \
template<class Type1, class Type2>                                            \
mousse::dimensioned<typename mousse::product<Type1, Type2>::type>             \
mousse::operator op                                                           \
(                                                                             \
  const dimensioned<Type1>& dt1,                                              \
  const dimensioned<Type2>& dt2                                               \
)                                                                             \
{                                                                             \
  return dimensioned<typename product<Type1, Type2>::type>                    \
  (                                                                           \
    '(' + dt1.name() + #op + dt2.name() + ')',                                \
    dt1.dimensions() op dt2.dimensions(),                                     \
    dt1.value() op dt2.value()                                                \
  );                                                                          \
}                                                                             \
                                                                              \
template<class Type, class Form, class Cmpt, int nCmpt>                       \
mousse::dimensioned<typename mousse::product<Type, Form>::type>               \
mousse::operator op                                                           \
(                                                                             \
  const dimensioned<Type>& dt1,                                               \
  const VectorSpace<Form,Cmpt,nCmpt>& t2                                      \
)                                                                             \
{                                                                             \
  return dimensioned<typename product<Type, Form>::type>                      \
  (                                                                           \
    '(' + dt1.name() + #op + name(t2) + ')',                                  \
    dt1.dimensions(),                                                         \
    dt1.value() op static_cast<const Form&>(t2)                               \
  );                                                                          \
}                                                                             \
                                                                              \
template<class Type, class Form, class Cmpt, int nCmpt>                       \
mousse::dimensioned<typename mousse::product<Form, Type>::type>               \
mousse::operator op                                                           \
(                                                                             \
  const VectorSpace<Form,Cmpt,nCmpt>& t1,                                     \
  const dimensioned<Type>& dt2                                                \
)                                                                             \
{                                                                             \
  return dimensioned<typename product<Form, Type>::type>                      \
  (                                                                           \
    '(' + name(t1) + #op + dt2.name() + ')',                                  \
    dt2.dimensions(),                                                         \
    static_cast<const Form&>(t1) op dt2.value()                               \
  );                                                                          \
}

PRODUCT_OPERATOR(outerProduct, *, outer)
PRODUCT_OPERATOR(crossProduct, ^, cross)
PRODUCT_OPERATOR(innerProduct, &, dot)
PRODUCT_OPERATOR(scalarProduct, &&, dotdot)

#undef PRODUCT_OPERATOR
