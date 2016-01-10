// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

// Constructors 
inline mousse::septernion::septernion()
{}
inline mousse::septernion::septernion(const vector& t, const quaternion& r)
:
  t_(t),
  r_(r)
{}
inline mousse::septernion::septernion(const vector& t)
:
  t_(t),
  r_(quaternion::I)
{}
inline mousse::septernion::septernion(const quaternion& r)
:
  t_(vector::zero),
  r_(r)
{}
// Member Functions 
inline const mousse::vector& mousse::septernion::t() const
{
  return t_;
}
inline const mousse::quaternion& mousse::septernion::r() const
{
  return r_;
}
inline mousse::vector& mousse::septernion::t()
{
  return t_;
}
inline mousse::quaternion& mousse::septernion::r()
{
  return r_;
}
inline mousse::vector mousse::septernion::transform(const vector& v) const
{
  return t() + r().transform(v);
}
inline mousse::vector mousse::septernion::invTransform(const vector& v) const
{
  return r().invTransform(v - t());
}
// Member Operators 
inline void mousse::septernion::operator=(const septernion& tr)
{
  t_ = tr.t_;
  r_ = tr.r_;
}
inline void mousse::septernion::operator*=(const septernion& tr)
{
  t_ += r().transform(tr.t());
  r_ *= tr.r();
}
inline void mousse::septernion::operator=(const vector& t)
{
  t_ = t;
}
inline void mousse::septernion::operator+=(const vector& t)
{
  t_ += t;
}
inline void mousse::septernion::operator-=(const vector& t)
{
  t_ -= t;
}
inline void mousse::septernion::operator=(const quaternion& r)
{
  r_ = r;
}
inline void mousse::septernion::operator*=(const quaternion& r)
{
  r_ *= r;
}
inline void mousse::septernion::operator/=(const quaternion& r)
{
  r_ /= r;
}
inline void mousse::septernion::operator*=(const scalar s)
{
  t_ *= s;
  r_ *= s;
}
inline void mousse::septernion::operator/=(const scalar s)
{
  t_ /= s;
  r_ /= s;
}
// Global Functions 
inline mousse::septernion mousse::inv(const septernion& tr)
{
  return septernion(-tr.r().invTransform(tr.t()), conjugate(tr.r()));
}
// Global Operators 
inline bool mousse::operator==(const septernion& tr1, const septernion& tr2)
{
  return (tr1.t() == tr2.t() && tr1.r() == tr2.r());
}
inline bool mousse::operator!=(const septernion& tr1, const septernion& tr2)
{
  return !operator==(tr1, tr2);
}
inline mousse::septernion mousse::operator+
(
  const septernion& tr,
  const vector& t
)
{
  return septernion(tr.t() + t, tr.r());
}
inline mousse::septernion mousse::operator+
(
  const vector& t,
  const septernion& tr
)
{
  return septernion(t + tr.t(), tr.r());
}
inline mousse::septernion mousse::operator-
(
  const septernion& tr,
  const vector& t
)
{
  return septernion(tr.t() - t, tr.r());
}
inline mousse::septernion mousse::operator*
(
  const quaternion& r,
  const septernion& tr
)
{
  return septernion(tr.t(), r*tr.r());
}
inline mousse::septernion mousse::operator*
(
  const septernion& tr,
  const quaternion& r
)
{
  return septernion(tr.t(), tr.r()*r);
}
inline mousse::septernion mousse::operator/
(
  const septernion& tr,
  const quaternion& r
)
{
  return septernion(tr.t(), tr.r()/r);
}
inline mousse::septernion mousse::operator*
(
  const septernion& tr1,
  const septernion& tr2
)
{
  return septernion
  (
    tr1.t() + tr1.r().transform(tr2.t()),
    tr1.r().transform(tr2.r())
  );
}
inline mousse::septernion mousse::operator/
(
  const septernion& tr1,
  const septernion& tr2
)
{
  return tr1*inv(tr2);
}
inline mousse::septernion mousse::operator*(const scalar s, const septernion& tr)
{
  return septernion(s*tr.t(), s*tr.r());
}
inline mousse::septernion mousse::operator*(const septernion& tr, const scalar s)
{
  return septernion(s*tr.t(), s*tr.r());
}
inline mousse::septernion mousse::operator/(const septernion& tr, const scalar s)
{
  return septernion(tr.t()/s, tr.r()/s);
}