// mousse: CFD toolbox
// Copyright (C) 2011-2014 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "euler_ddt_scheme.hpp"
#include "surface_interpolate.hpp"
#include "fvc_div.hpp"
#include "fv_matrices.hpp"

namespace mousse {
namespace fv {

template<class Type>
tmp<GeometricField<Type, fvPatchField, volMesh>>
EulerDdtScheme<Type>::fvcDdt
(
  const dimensioned<Type>& dt
)
{
  dimensionedScalar rDeltaT = 1.0/mesh().time().deltaT();
  IOobject ddtIOobject
  {
    "ddt("+dt.name()+')',
    mesh().time().timeName(),
    mesh()
  };
  if (mesh().moving()) {
    tmp<GeometricField<Type, fvPatchField, volMesh>> tdtdt
    {
      new GeometricField<Type, fvPatchField, volMesh>
      {
        ddtIOobject,
        mesh(),
        dimensioned<Type>
        {
          "0",
          dt.dimensions()/dimTime,
          pTraits<Type>::zero
        }
      }
    };
    tdtdt().internalField() =
      rDeltaT.value()*dt.value()*(1.0 - mesh().Vsc0()/mesh().Vsc());
    return tdtdt;
  } else {
    return tmp<GeometricField<Type, fvPatchField, volMesh>>
    {
      new GeometricField<Type, fvPatchField, volMesh>
      {
        ddtIOobject,
        mesh(),
        dimensioned<Type>
        {
          "0",
          dt.dimensions()/dimTime,
          pTraits<Type>::zero
        },
        calculatedFvPatchField<Type>::typeName
      }
    };
  }
}


template<class Type>
tmp<GeometricField<Type, fvPatchField, volMesh>>
EulerDdtScheme<Type>::fvcDdt
(
  const GeometricField<Type, fvPatchField, volMesh>& vf
)
{
  dimensionedScalar rDeltaT = 1.0/mesh().time().deltaT();
  IOobject ddtIOobject
  {
    "ddt("+vf.name()+')',
    mesh().time().timeName(),
    mesh()
  };
  if (mesh().moving()) {
    return tmp<GeometricField<Type, fvPatchField, volMesh>>
    {
      new GeometricField<Type, fvPatchField, volMesh>
      {
        ddtIOobject,
        mesh(),
        rDeltaT.dimensions()*vf.dimensions(),
        rDeltaT.value()*
        (
          vf.internalField()
         - vf.oldTime().internalField()*mesh().Vsc0()/mesh().Vsc()
        ),
        rDeltaT.value()*
        (
          vf.boundaryField() - vf.oldTime().boundaryField()
        )
      }
    };
  } else {
    return tmp<GeometricField<Type, fvPatchField, volMesh>>
    {
      new GeometricField<Type, fvPatchField, volMesh>
      {
        ddtIOobject,
        rDeltaT*(vf - vf.oldTime())
      }
    };
  }
}


template<class Type>
tmp<GeometricField<Type, fvPatchField, volMesh>>
EulerDdtScheme<Type>::fvcDdt
(
  const dimensionedScalar& rho,
  const GeometricField<Type, fvPatchField, volMesh>& vf
)
{
  dimensionedScalar rDeltaT = 1.0/mesh().time().deltaT();
  IOobject ddtIOobject
  {
    "ddt("+rho.name()+','+vf.name()+')',
    mesh().time().timeName(),
    mesh()
  };
  if (mesh().moving()) {
    return tmp<GeometricField<Type, fvPatchField, volMesh>>
    {
      new GeometricField<Type, fvPatchField, volMesh>
      {
        ddtIOobject,
        mesh(),
        rDeltaT.dimensions()*rho.dimensions()*vf.dimensions(),
        rDeltaT.value()*rho.value()*
        (
          vf.internalField()
          - vf.oldTime().internalField()*mesh().Vsc0()/mesh().Vsc()
        ),
        rDeltaT.value()*rho.value()*
        (
          vf.boundaryField() - vf.oldTime().boundaryField()
        )
      }
    };
  } else {
    return tmp<GeometricField<Type, fvPatchField, volMesh>>
    {
      new GeometricField<Type, fvPatchField, volMesh>
      {
        ddtIOobject,
        rDeltaT*rho*(vf - vf.oldTime())
      }
    };
  }
}


template<class Type>
tmp<GeometricField<Type, fvPatchField, volMesh>>
EulerDdtScheme<Type>::fvcDdt
(
  const volScalarField& rho,
  const GeometricField<Type, fvPatchField, volMesh>& vf
)
{
  dimensionedScalar rDeltaT = 1.0/mesh().time().deltaT();
  IOobject ddtIOobject
  {
    "ddt("+rho.name()+','+vf.name()+')',
    mesh().time().timeName(),
    mesh()
  };
  if (mesh().moving()) {
    return tmp<GeometricField<Type, fvPatchField, volMesh>>
    {
      new GeometricField<Type, fvPatchField, volMesh>
      {
        ddtIOobject,
        mesh(),
        rDeltaT.dimensions()*rho.dimensions()*vf.dimensions(),
        rDeltaT.value()*
        (
          rho.internalField()*vf.internalField()
         - rho.oldTime().internalField()
         *vf.oldTime().internalField()*mesh().Vsc0()/mesh().Vsc()
        ),
        rDeltaT.value()*
        (
          rho.boundaryField()*vf.boundaryField()
         - rho.oldTime().boundaryField()
         *vf.oldTime().boundaryField()
        )
      }
    };
  } else {
    return tmp<GeometricField<Type, fvPatchField, volMesh>>
    {
      new GeometricField<Type, fvPatchField, volMesh>
      {
        ddtIOobject,
        rDeltaT*(rho*vf - rho.oldTime()*vf.oldTime())
      }
    };
  }
}


template<class Type>
tmp<GeometricField<Type, fvPatchField, volMesh>>
EulerDdtScheme<Type>::fvcDdt
(
  const volScalarField& alpha,
  const volScalarField& rho,
  const GeometricField<Type, fvPatchField, volMesh>& vf
)
{
  dimensionedScalar rDeltaT = 1.0/mesh().time().deltaT();
  IOobject ddtIOobject
  {
    "ddt("+alpha.name()+','+rho.name()+','+vf.name()+')',
    mesh().time().timeName(),
    mesh()
  };
  if (mesh().moving()) {
    return tmp<GeometricField<Type, fvPatchField, volMesh>>
    {
      new GeometricField<Type, fvPatchField, volMesh>
      {
        ddtIOobject,
        mesh(),
        rDeltaT.dimensions()
       *alpha.dimensions()*rho.dimensions()*vf.dimensions(),
        rDeltaT.value()*
        (
          alpha.internalField()
         *rho.internalField()
         *vf.internalField()
         - alpha.oldTime().internalField()
         *rho.oldTime().internalField()
         *vf.oldTime().internalField()*mesh().Vsc0()/mesh().Vsc()
        ),
        rDeltaT.value()*
        (
          alpha.boundaryField()
         *rho.boundaryField()
         *vf.boundaryField()
         - alpha.oldTime().boundaryField()
         *rho.oldTime().boundaryField()
         *vf.oldTime().boundaryField()
        )
      }
    };
  } else {
    return tmp<GeometricField<Type, fvPatchField, volMesh>>
    {
      new GeometricField<Type, fvPatchField, volMesh>
      {
        ddtIOobject,
        rDeltaT
          *(alpha*rho*vf
            - alpha.oldTime()*rho.oldTime()*vf.oldTime())
      }
    };
  }
}


template<class Type>
tmp<fvMatrix<Type>>
EulerDdtScheme<Type>::fvmDdt
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
  fvMatrix<Type>& fvm = tfvm();
  scalar rDeltaT = 1.0/mesh().time().deltaTValue();
  fvm.diag() = rDeltaT*mesh().Vsc();
  if (mesh().moving()) {
    fvm.source() = rDeltaT*vf.oldTime().internalField()*mesh().Vsc0();
  } else {
    fvm.source() = rDeltaT*vf.oldTime().internalField()*mesh().Vsc();
  }
  return tfvm;
}


template<class Type>
tmp<fvMatrix<Type>>
EulerDdtScheme<Type>::fvmDdt
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
  fvMatrix<Type>& fvm = tfvm();
  scalar rDeltaT = 1.0/mesh().time().deltaTValue();
  fvm.diag() = rDeltaT*rho.value()*mesh().Vsc();
  if (mesh().moving()) {
    fvm.source() =
      rDeltaT*rho.value()*vf.oldTime().internalField()*mesh().Vsc0();
  } else {
    fvm.source() =
      rDeltaT*rho.value()*vf.oldTime().internalField()*mesh().Vsc();
  }
  return tfvm;
}


template<class Type>
tmp<fvMatrix<Type>>
EulerDdtScheme<Type>::fvmDdt
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
  fvMatrix<Type>& fvm = tfvm();
  scalar rDeltaT = 1.0/mesh().time().deltaTValue();
  fvm.diag() = rDeltaT*rho.internalField()*mesh().Vsc();
  if (mesh().moving()) {
    fvm.source() = rDeltaT
      *rho.oldTime().internalField()
      *vf.oldTime().internalField()*mesh().Vsc0();
  } else {
    fvm.source() = rDeltaT
      *rho.oldTime().internalField()
      *vf.oldTime().internalField()*mesh().Vsc();
  }
  return tfvm;
}


template<class Type>
tmp<fvMatrix<Type>>
EulerDdtScheme<Type>::fvmDdt
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
  fvMatrix<Type>& fvm = tfvm();
  scalar rDeltaT = 1.0/mesh().time().deltaTValue();
  fvm.diag() = rDeltaT*alpha.internalField()*rho.internalField()*mesh().Vsc();
  if (mesh().moving()) {
    fvm.source() = rDeltaT
      *alpha.oldTime().internalField()
      *rho.oldTime().internalField()
      *vf.oldTime().internalField()*mesh().Vsc0();
  } else {
    fvm.source() = rDeltaT
      *alpha.oldTime().internalField()
      *rho.oldTime().internalField()
      *vf.oldTime().internalField()*mesh().Vsc();
  }
  return tfvm;
}


template<class Type>
tmp<typename EulerDdtScheme<Type>::fluxFieldType>
EulerDdtScheme<Type>::fvcDdtUfCorr
(
  const GeometricField<Type, fvPatchField, volMesh>& U,
  const GeometricField<Type, fvsPatchField, surfaceMesh>& Uf
)
{
  dimensionedScalar rDeltaT = 1.0/mesh().time().deltaT();
  fluxFieldType phiCorr
  {
    mesh().Sf() & (Uf.oldTime() - fvc::interpolate(U.oldTime()))
  };
  return tmp<fluxFieldType>
  {
    new fluxFieldType
    {
      {
        "ddtCorr(" + U.name() + ',' + Uf.name() + ')',
        mesh().time().timeName(),
        mesh()
      },
      this->fvcDdtPhiCoeff
      (
        U.oldTime(),
        mesh().Sf() & Uf.oldTime(),
        phiCorr
      )
     *rDeltaT*phiCorr
    }
  };
}


template<class Type>
tmp<typename EulerDdtScheme<Type>::fluxFieldType>
EulerDdtScheme<Type>::fvcDdtPhiCorr
(
  const GeometricField<Type, fvPatchField, volMesh>& U,
  const fluxFieldType& phi
)
{
  dimensionedScalar rDeltaT = 1.0/mesh().time().deltaT();
  fluxFieldType phiCorr
  {
    phi.oldTime() - (mesh().Sf() & fvc::interpolate(U.oldTime()))
  };
  return tmp<fluxFieldType>
  {
    new fluxFieldType
    {
      {
        "ddtCorr(" + U.name() + ',' + phi.name() + ')',
        mesh().time().timeName(),
        mesh()
      },
      this->fvcDdtPhiCoeff(U.oldTime(), phi.oldTime(), phiCorr)
     *rDeltaT*phiCorr
    }
  };
}


template<class Type>
tmp<typename EulerDdtScheme<Type>::fluxFieldType>
EulerDdtScheme<Type>::fvcDdtUfCorr
(
  const volScalarField& rho,
  const GeometricField<Type, fvPatchField, volMesh>& U,
  const GeometricField<Type, fvsPatchField, surfaceMesh>& Uf
)
{
  if (U.dimensions() == dimVelocity
      && Uf.dimensions() == rho.dimensions()*dimVelocity) {
    dimensionedScalar rDeltaT = 1.0/mesh().time().deltaT();
    GeometricField<Type, fvPatchField, volMesh> rhoU0
    {
      rho.oldTime()*U.oldTime()
    };
    fluxFieldType phiCorr
    {
      mesh().Sf() & (Uf.oldTime() - fvc::interpolate(rhoU0))
    };
    return tmp<fluxFieldType>
    {
      new fluxFieldType
      {
        {
          "ddtCorr("
         + rho.name() + ',' + U.name() + ',' + Uf.name() + ')',
          mesh().time().timeName(),
          mesh()
        },
        this->fvcDdtPhiCoeff
        (
          rhoU0,
          mesh().Sf() & Uf.oldTime(),
          phiCorr
        )
       *rDeltaT*phiCorr
      }
    };
  } else if (U.dimensions() == rho.dimensions()*dimVelocity
             && Uf.dimensions() == rho.dimensions()*dimVelocity)
  {
    return fvcDdtUfCorr(U, Uf);
  } else {
    FATAL_ERROR_IN
    (
      "EulerDdtScheme<Type>::fvcDdtPhiCorr"
    )
    << "dimensions of Uf are not correct"
    << abort(FatalError);
    return fluxFieldType::null();
  }
}


template<class Type>
tmp<typename EulerDdtScheme<Type>::fluxFieldType>
EulerDdtScheme<Type>::fvcDdtPhiCorr
(
  const volScalarField& rho,
  const GeometricField<Type, fvPatchField, volMesh>& U,
  const fluxFieldType& phi
)
{
  if (U.dimensions() == dimVelocity
      && phi.dimensions() == rho.dimensions()*dimVelocity*dimArea) {
    dimensionedScalar rDeltaT = 1.0/mesh().time().deltaT();
    GeometricField<Type, fvPatchField, volMesh> rhoU0
    {
      rho.oldTime()*U.oldTime()
    };
    fluxFieldType phiCorr
    {
      phi.oldTime() - (mesh().Sf() & fvc::interpolate(rhoU0))
    };
    return tmp<fluxFieldType>
    {
      new fluxFieldType
      {
        {
          "ddtCorr("
         + rho.name() + ',' + U.name() + ',' + phi.name() + ')',
          mesh().time().timeName(),
          mesh()
        },
        this->fvcDdtPhiCoeff(rhoU0, phi.oldTime(), phiCorr)
       *rDeltaT*phiCorr
      }
    };
  } else if (U.dimensions() == rho.dimensions()*dimVelocity
             && phi.dimensions() == rho.dimensions()*dimVelocity*dimArea) {
    return fvcDdtPhiCorr(U, phi);
  } else {
    FATAL_ERROR_IN
    (
      "EulerDdtScheme<Type>::fvcDdtPhiCorr"
    )
    << "dimensions of phi are not correct"
    << abort(FatalError);
    return fluxFieldType::null();
  }
}


template<class Type>
tmp<surfaceScalarField> EulerDdtScheme<Type>::meshPhi
(
  const GeometricField<Type, fvPatchField, volMesh>&
)
{
  return mesh().phi();
}

}  // namespace fv
}  // namespace mousse
