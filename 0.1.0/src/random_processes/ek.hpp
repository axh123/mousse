#ifndef ek_hpp_
#define ek_hpp_
namespace mousse
{
inline tmp<scalarField> Ek
(
  const scalar Ea,
  const scalar k0,
  const scalarField& k
)
{
  tmp<scalarField> tEk = Ea*pow(k/k0, 4.0)*exp(-2.0*sqr(k/k0));
  /*
  scalarField& Ekf = tEk();
  label i;
  forAll(Ekf, i)
  {
    if (k[i] < 2 || k[i] > 10)
    {
      Ekf[i] = 0.0;
    }
  }
  */
  return tEk;
}
}  // namespace mousse
#endif
