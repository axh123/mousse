// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "steady_state_ddt_scheme.hpp"
#include "fvc_div.hpp"
#include "fv_matrices.hpp"


namespace mousse {
namespace fv {

template<class Type>
tmp<GeometricField<Type, fvPatchField, volMesh>>
steadyStateDdtScheme<Type>::fvcDdt
(
  const dimensioned<Type>& dt
)
{
  return tmp<GeometricField<Type, fvPatchField, volMesh>>
  {
    new GeometricField<Type, fvPatchField, volMesh>
    {
      {
        "ddt("+dt.name()+')',
        mesh().time().timeName(),
        mesh()
      },
      mesh(),
      dimensioned<Type>
      {
        "0",
        dt.dimensions()/dimTime,
        pTraits<Type>::zero
      }
    }
  };
}


template<class Type>
tmp<GeometricField<Type, fvPatchField, volMesh>>
steadyStateDdtScheme<Type>::fvcDdt
(
  const GeometricField<Type, fvPatchField, volMesh>& vf
)
{
  return tmp<GeometricField<Type, fvPatchField, volMesh>>
  {
    new GeometricField<Type, fvPatchField, volMesh>
    {
      {
        "ddt("+vf.name()+')',
        mesh().time().timeName(),
        mesh()
      },
      mesh(),
      dimensioned<Type>
      {
        "0",
        vf.dimensions()/dimTime,
        pTraits<Type>::zero
      }
    }
  };
}


template<class Type>
tmp<GeometricField<Type, fvPatchField, volMesh>>
steadyStateDdtScheme<Type>::fvcDdt
(
  const dimensionedScalar& rho,
  const GeometricField<Type, fvPatchField, volMesh>& vf
)
{
  return tmp<GeometricField<Type, fvPatchField, volMesh>>
  {
    new GeometricField<Type, fvPatchField, volMesh>
    {
      {
        "ddt("+rho.name()+','+vf.name()+')',
        mesh().time().timeName(),
        mesh()
      },
      mesh(),
      dimensioned<Type>
      {
        "0",
        rho.dimensions()*vf.dimensions()/dimTime,
        pTraits<Type>::zero
      }
    }
  };
}


template<class Type>
tmp<GeometricField<Type, fvPatchField, volMesh>>
steadyStateDdtScheme<Type>::fvcDdt
(
  const volScalarField& rho,
  const GeometricField<Type, fvPatchField, volMesh>& vf
)
{
  return tmp<GeometricField<Type, fvPatchField, volMesh>>
  {
    new GeometricField<Type, fvPatchField, volMesh>
    {
      {
        "ddt("+rho.name()+','+vf.name()+')',
        mesh().time().timeName(),
        mesh()
      },
      mesh(),
      dimensioned<Type>
      {
        "0",
        rho.dimensions()*vf.dimensions()/dimTime,
        pTraits<Type>::zero
      }
    }
  };
}


template<class Type>
tmp<GeometricField<Type, fvPatchField, volMesh>>
steadyStateDdtScheme<Type>::fvcDdt
(
  const volScalarField& alpha,
  const volScalarField& rho,
  const GeometricField<Type, fvPatchField, volMesh>& vf
)
{
  return tmp<GeometricField<Type, fvPatchField, volMesh>>
  {
    new GeometricField<Type, fvPatchField, volMesh>
    {
      {
        "ddt("+alpha.name()+','+rho.name()+','+vf.name()+')',
        mesh().time().timeName(),
        mesh()
      },
      mesh(),
      dimensioned<Type>
      {
        "0",
        rho.dimensions()*vf.dimensions()/dimTime,
        pTraits<Type>::zero
      }
    }
  };
}


template<class Type>
tmp<fvMatrix<Type>>
steadyStateDdtScheme<Type>::fvmDdt
(
  const GeometricField<Type, fvPatchField, volMesh>& vf
)
{
  tmp<fvMatrix<Type>> tfvm
  {
    new fvMatrix<Type>
    {
      vf,
      vf.dimensions()*dimVol/dimTime
    }
  };
  return tfvm;
}


template<class Type>
tmp<fvMatrix<Type>>
steadyStateDdtScheme<Type>::fvmDdt
(
  const dimensionedScalar& rho,
  const GeometricField<Type, fvPatchField, volMesh>& vf
)
{
  tmp<fvMatrix<Type>> tfvm
  {
    new fvMatrix<Type>
    {
      vf,
      rho.dimensions()*vf.dimensions()*dimVol/dimTime
    }
  };
  return tfvm;
}


template<class Type>
tmp<fvMatrix<Type>>
steadyStateDdtScheme<Type>::fvmDdt
(
  const volScalarField& rho,
  const GeometricField<Type, fvPatchField, volMesh>& vf
)
{
  tmp<fvMatrix<Type>> tfvm
  {
    new fvMatrix<Type>
    {
      vf,
      rho.dimensions()*vf.dimensions()*dimVol/dimTime
    }
  };
  return tfvm;
}


template<class Type>
tmp<fvMatrix<Type>>
steadyStateDdtScheme<Type>::fvmDdt
(
  const volScalarField& alpha,
  const volScalarField& rho,
  const GeometricField<Type, fvPatchField, volMesh>& vf
)
{
  tmp<fvMatrix<Type>> tfvm
  {
    new fvMatrix<Type>
    {
      vf,
      alpha.dimensions()*rho.dimensions()*vf.dimensions()*dimVol/dimTime
    }
  };
  return tfvm;
}


template<class Type>
tmp<typename steadyStateDdtScheme<Type>::fluxFieldType>
steadyStateDdtScheme<Type>::fvcDdtUfCorr
(
  const GeometricField<Type, fvPatchField, volMesh>& U,
  const GeometricField<Type, fvsPatchField, surfaceMesh>& Uf
)
{
  return tmp<fluxFieldType>
  {
    new fluxFieldType
    {
      {
        "ddtCorr(" + U.name() + ',' + Uf.name() + ')',
        mesh().time().timeName(),
        mesh()
      },
      mesh(),
      dimensioned<typename flux<Type>::type>
      {
        "0",
        Uf.dimensions()*dimArea/dimTime,
        pTraits<typename flux<Type>::type>::zero
      }
    }
  };
}


template<class Type>
tmp<typename steadyStateDdtScheme<Type>::fluxFieldType>
steadyStateDdtScheme<Type>::fvcDdtPhiCorr
(
  const GeometricField<Type, fvPatchField, volMesh>& U,
  const fluxFieldType& phi
)
{
  return tmp<fluxFieldType>
  {
    new fluxFieldType
    {
      {
        "ddtCorr(" + U.name() + ',' + phi.name() + ')',
        mesh().time().timeName(),
        mesh()
      },
      mesh(),
      dimensioned<typename flux<Type>::type>
      {
        "0",
        phi.dimensions()/dimTime,
        pTraits<typename flux<Type>::type>::zero
      }
    }
  };
}


template<class Type>
tmp<typename steadyStateDdtScheme<Type>::fluxFieldType>
steadyStateDdtScheme<Type>::fvcDdtUfCorr
(
  const volScalarField& rho,
  const GeometricField<Type, fvPatchField, volMesh>& U,
  const GeometricField<Type, fvsPatchField, surfaceMesh>& Uf
)
{
  return tmp<fluxFieldType>
  {
    new fluxFieldType
    {
      {
        "ddtCorr(" + rho.name() + ',' + U.name() + ',' + Uf.name() + ')',
        mesh().time().timeName(),
        mesh()
      },
      mesh(),
      dimensioned<typename flux<Type>::type>
      {
        "0",
        Uf.dimensions()*dimArea/dimTime,
        pTraits<typename flux<Type>::type>::zero
      }
    }
  };
}


template<class Type>
tmp<typename steadyStateDdtScheme<Type>::fluxFieldType>
steadyStateDdtScheme<Type>::fvcDdtPhiCorr
(
  const volScalarField& rho,
  const GeometricField<Type, fvPatchField, volMesh>& U,
  const fluxFieldType& phi
)
{
  return tmp<fluxFieldType>
  {
    new fluxFieldType
    {
      {
        "ddtCorr(" + rho.name() + ',' + U.name() + ',' + phi.name() + ')',
        mesh().time().timeName(),
        mesh()
      },
      mesh(),
      dimensioned<typename flux<Type>::type>
      {
        "0",
        phi.dimensions()/dimTime,
        pTraits<typename flux<Type>::type>::zero
      }
    }
  };
}


template<class Type>
tmp<surfaceScalarField> steadyStateDdtScheme<Type>::meshPhi
(
  const GeometricField<Type, fvPatchField, volMesh>& /*vf*/
)
{
  return tmp<surfaceScalarField>
  {
    new surfaceScalarField
    {
      {
        "meshPhi",
        mesh().time().timeName(),
        mesh(),
        IOobject::NO_READ,
        IOobject::NO_WRITE,
        false
      },
      mesh(),
      {"0", dimVolume/dimTime, 0.0}
    }
  };
}

}  // namespace fv
}  // namespace mousse
