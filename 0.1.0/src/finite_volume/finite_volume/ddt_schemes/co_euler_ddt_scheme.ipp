// mousse: CFD toolbox
// Copyright (C) 2011-2014 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "co_euler_ddt_scheme.hpp"
#include "surface_interpolate.hpp"
#include "fvc_div.hpp"
#include "fv_matrices.hpp"


namespace mousse {
namespace fv {

template<class Type>
tmp<volScalarField> CoEulerDdtScheme<Type>::CorDeltaT() const
{
  const surfaceScalarField cofrDeltaT{CofrDeltaT()};
  tmp<volScalarField> tcorDeltaT
  {
    new volScalarField
    {
      {
        "CorDeltaT",
        cofrDeltaT.instance(),
        mesh()
      },
      mesh(),
      {"CorDeltaT", cofrDeltaT.dimensions(), 0.0},
      zeroGradientFvPatchScalarField::typeName
    }
  };
  volScalarField& corDeltaT = tcorDeltaT();
  const labelUList& owner = mesh().owner();
  const labelUList& neighbour = mesh().neighbour();
  FOR_ALL(owner, faceI) {
    corDeltaT[owner[faceI]] =
      max(corDeltaT[owner[faceI]], cofrDeltaT[faceI]);
    corDeltaT[neighbour[faceI]] =
      max(corDeltaT[neighbour[faceI]], cofrDeltaT[faceI]);
  }
  volScalarField::GeometricBoundaryField& bcorDeltaT =
    corDeltaT.boundaryField();
  FOR_ALL(bcorDeltaT, patchi) {
    const fvsPatchScalarField& pcofrDeltaT =
      cofrDeltaT.boundaryField()[patchi];
    const fvPatch& p = pcofrDeltaT.patch();
    const labelUList& faceCells = p.patch().faceCells();
    FOR_ALL(pcofrDeltaT, patchFacei) {
      corDeltaT[faceCells[patchFacei]] = max
      (
        corDeltaT[faceCells[patchFacei]],
        pcofrDeltaT[patchFacei]
      );
    }
  }
  corDeltaT.correctBoundaryConditions();
  //corDeltaT = max(corDeltaT, max(corDeltaT)/100.0);
  return tcorDeltaT;
}


template<class Type>
tmp<surfaceScalarField> CoEulerDdtScheme<Type>::CofrDeltaT() const
{
  const dimensionedScalar& deltaT = mesh().time().deltaT();
  const surfaceScalarField& phi =
    static_cast<const objectRegistry&>(mesh())
    .lookupObject<surfaceScalarField>(phiName_);
  if (phi.dimensions() == dimensionSet(0, 3, -1, 0, 0)) {
    surfaceScalarField Co
    {
      mesh().surfaceInterpolation::deltaCoeffs()*(mag(phi)/mesh().magSf())
      *deltaT
    };
    return max(Co/maxCo_, scalar(1))/deltaT;
  } else if (phi.dimensions() == dimensionSet(1, 0, -1, 0, 0)) {
    const volScalarField& rho =
      static_cast<const objectRegistry&>(mesh())
      .lookupObject<volScalarField>(rhoName_).oldTime();
    surfaceScalarField Co
    {
      mesh().surfaceInterpolation::deltaCoeffs()
      *(mag(phi)/(fvc::interpolate(rho)*mesh().magSf()))
      *deltaT
    };
    return max(Co/maxCo_, scalar(1))/deltaT;
  } else {
    FATAL_ERROR_IN("CoEulerDdtScheme<Type>::CofrDeltaT() const")
      << "Incorrect dimensions of phi: " << phi.dimensions()
      << abort(FatalError);
    return tmp<surfaceScalarField>{NULL};
  }
}


template<class Type>
tmp<GeometricField<Type, fvPatchField, volMesh>>
CoEulerDdtScheme<Type>::fvcDdt
(
  const dimensioned<Type>& dt
)
{
  const volScalarField rDeltaT(CorDeltaT());
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
        dimensioned<Type>{"0", dt.dimensions()/dimTime, pTraits<Type>::zero}
      }
    };
    tdtdt().internalField() = rDeltaT.internalField()*dt.value()
      *(1.0 - mesh().Vsc0()/mesh().Vsc());
    return tdtdt;
  } else {
    return tmp<GeometricField<Type, fvPatchField, volMesh>>
    {
      new GeometricField<Type, fvPatchField, volMesh>
      {
        ddtIOobject,
        mesh(),
        dimensioned<Type>{"0", dt.dimensions()/dimTime, pTraits<Type>::zero},
        calculatedFvPatchField<Type>::typeName
      }
    };
  }
}


template<class Type>
tmp<GeometricField<Type, fvPatchField, volMesh>>
CoEulerDdtScheme<Type>::fvcDdt
(
  const GeometricField<Type, fvPatchField, volMesh>& vf
)
{
  const volScalarField rDeltaT{CorDeltaT()};
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
        rDeltaT.internalField()*
        (
          vf.internalField()
          - vf.oldTime().internalField()*mesh().Vsc0()/mesh().Vsc()
        ),
        rDeltaT.boundaryField()*(vf.boundaryField()
                                 - vf.oldTime().boundaryField())
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
CoEulerDdtScheme<Type>::fvcDdt
(
  const dimensionedScalar& rho,
  const GeometricField<Type, fvPatchField, volMesh>& vf
)
{
  const volScalarField rDeltaT{CorDeltaT()};
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
        rDeltaT.internalField()*rho.value()*
        (
          vf.internalField()
          - vf.oldTime().internalField()*mesh().Vsc0()/mesh().Vsc()
        ),
        rDeltaT.boundaryField()*rho.value()*(vf.boundaryField()
                                             - vf.oldTime().boundaryField())
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
CoEulerDdtScheme<Type>::fvcDdt
(
  const volScalarField& rho,
  const GeometricField<Type, fvPatchField, volMesh>& vf
)
{
  const volScalarField rDeltaT{CorDeltaT()};
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
        rDeltaT.internalField()*
        (
          rho.internalField()*vf.internalField()
          - rho.oldTime().internalField()*vf.oldTime().internalField()*mesh().Vsc0()/mesh().Vsc()
        ),
        rDeltaT.boundaryField()*
        (
          rho.boundaryField()*vf.boundaryField()
          - rho.oldTime().boundaryField()*vf.oldTime().boundaryField()
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
CoEulerDdtScheme<Type>::fvcDdt
(
  const volScalarField& alpha,
  const volScalarField& rho,
  const GeometricField<Type, fvPatchField, volMesh>& vf
)
{
  const volScalarField rDeltaT{CorDeltaT()};
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
        rDeltaT.dimensions()*alpha.dimensions()*rho.dimensions()*vf.dimensions(),
        rDeltaT.internalField()
        *(alpha.internalField()
          *rho.internalField()
          *vf.internalField()
          - alpha.oldTime().internalField()
          *rho.oldTime().internalField()
          *vf.oldTime().internalField()
          *mesh().Vsc0()/mesh().Vsc()),
        rDeltaT.boundaryField()*
        (
          alpha.boundaryField()*rho.boundaryField()*vf.boundaryField()
          - alpha.oldTime().boundaryField()*rho.oldTime().boundaryField()
          *vf.oldTime().boundaryField()
        )
      }
    };
  }
  else
  {
    return tmp<GeometricField<Type, fvPatchField, volMesh>>
    {
      new GeometricField<Type, fvPatchField, volMesh>
      {
        ddtIOobject,
        rDeltaT*(alpha*rho*vf - alpha.oldTime()*rho.oldTime()*vf.oldTime())
      }
    };
  }
}


template<class Type>
tmp<fvMatrix<Type>>
CoEulerDdtScheme<Type>::fvmDdt
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
  scalarField rDeltaT{CorDeltaT()().internalField()};
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
CoEulerDdtScheme<Type>::fvmDdt
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
  scalarField rDeltaT{CorDeltaT()().internalField()};
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
CoEulerDdtScheme<Type>::fvmDdt
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
  scalarField rDeltaT{CorDeltaT()().internalField()};
  fvm.diag() = rDeltaT*rho.internalField()*mesh().Vsc();
  if (mesh().moving()) {
    fvm.source() =
      rDeltaT*rho.oldTime().internalField()*vf.oldTime().internalField()*mesh().Vsc0();
  } else {
    fvm.source() =
      rDeltaT*rho.oldTime().internalField()*vf.oldTime().internalField()*mesh().Vsc();
  }
  return tfvm;
}


template<class Type>
tmp<fvMatrix<Type>>
CoEulerDdtScheme<Type>::fvmDdt
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
  scalarField rDeltaT(CorDeltaT()().internalField());
  fvm.diag() = rDeltaT*alpha.internalField()*rho.internalField()*mesh().Vsc();
  if (mesh().moving())
  {
    fvm.source() = rDeltaT
      *alpha.oldTime().internalField()
      *rho.oldTime().internalField()
      *vf.oldTime().internalField()*mesh().Vsc0();
  }
  else
  {
    fvm.source() = rDeltaT
      *alpha.oldTime().internalField()
      *rho.oldTime().internalField()
      *vf.oldTime().internalField()*mesh().Vsc();
  }
  return tfvm;
}


template<class Type>
tmp<typename CoEulerDdtScheme<Type>::fluxFieldType>
CoEulerDdtScheme<Type>::fvcDdtUfCorr
(
  const GeometricField<Type, fvPatchField, volMesh>& U,
  const GeometricField<Type, fvsPatchField, surfaceMesh>& Uf
)
{
  const surfaceScalarField rDeltaT{fvc::interpolate(CorDeltaT())};
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
      this->fvcDdtPhiCoeff(U.oldTime(),
                           (mesh().Sf() & Uf.oldTime()),
                           phiCorr)*rDeltaT*phiCorr
    }
  };
}


template<class Type>
tmp<typename CoEulerDdtScheme<Type>::fluxFieldType>
CoEulerDdtScheme<Type>::fvcDdtPhiCorr
(
  const GeometricField<Type, fvPatchField, volMesh>& U,
  const fluxFieldType& phi
)
{
  const surfaceScalarField rDeltaT{fvc::interpolate(CorDeltaT())};
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
      this->fvcDdtPhiCoeff(U.oldTime(), phi.oldTime(), phiCorr)*rDeltaT*phiCorr
    }
  };
}


template<class Type>
tmp<typename CoEulerDdtScheme<Type>::fluxFieldType>
CoEulerDdtScheme<Type>::fvcDdtUfCorr
(
  const volScalarField& rho,
  const GeometricField<Type, fvPatchField, volMesh>& U,
  const GeometricField<Type, fvsPatchField, surfaceMesh>& Uf
)
{
  if (U.dimensions() == dimVelocity
      && Uf.dimensions() == dimDensity*dimVelocity) {
    const surfaceScalarField rDeltaT{fvc::interpolate(CorDeltaT())};
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
        this->fvcDdtPhiCoeff(rhoU0,
                             mesh().Sf() & Uf.oldTime(),
                             phiCorr)*rDeltaT*phiCorr
      }
    };
  } else if (U.dimensions() == dimDensity*dimVelocity
             && Uf.dimensions() == dimDensity*dimVelocity) {
    return fvcDdtUfCorr(U, Uf);
  } else {
    FATAL_ERROR_IN
    (
      "CoEulerDdtScheme<Type>::fvcDdtPhiCorr"
    )
    << "dimensions of Uf are not correct"
    << abort(FatalError);
    return fluxFieldType::null();
  }
}


template<class Type>
tmp<typename CoEulerDdtScheme<Type>::fluxFieldType>
CoEulerDdtScheme<Type>::fvcDdtPhiCorr
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
          "ddtCorr(" + rho.name() + ',' + U.name() + ',' + phi.name() + ')',
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
      "CoEulerDdtScheme<Type>::fvcDdtPhiCorr"
    )
    << "dimensions of phi are not correct"
    << abort(FatalError);
    return fluxFieldType::null();
  }
}


template<class Type>
tmp<surfaceScalarField> CoEulerDdtScheme<Type>::meshPhi
(
  const GeometricField<Type, fvPatchField, volMesh>&
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
        IOobject::NO_WRITE
      },
      mesh(),
      {"0", dimVolume/dimTime, 0.0}
    }
  };
}

}  // namespace fv
}  // namespace mousse
