// mousse: CFD toolbox
// Copyright (C) 2011-2015 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include "kinematic_single_layer.hpp"
#include "fvm.hpp"
#include "fvc_div.hpp"
#include "fvc_laplacian.hpp"
#include "fvc_sn_grad.hpp"
#include "fvc_reconstruct.hpp"
#include "fvc_volume_integrate.hpp"
#include "add_to_run_time_selection_table.hpp"
#include "mapped_wall_poly_patch.hpp"
#include "map_distribute.hpp"
#include "film_thermo_model.hpp"


namespace mousse {
namespace regionModels {
namespace surfaceFilmModels {

// Static Data Members
DEFINE_TYPE_NAME_AND_DEBUG(kinematicSingleLayer, 0);
ADD_TO_RUN_TIME_SELECTION_TABLE(surfaceFilmModel, kinematicSingleLayer, mesh);


// Protected Member Functions 
bool kinematicSingleLayer::read()
{
  if (surfaceFilmModel::read()) {
    const dictionary& solution = this->solution().subDict("PISO");
    solution.lookup("momentumPredictor") >> momentumPredictor_;
    solution.readIfPresent("nOuterCorr", nOuterCorr_);
    solution.lookup("nCorr") >> nCorr_;
    solution.lookup("nNonOrthCorr") >> nNonOrthCorr_;
    return true;
  }
  return false;
}


void kinematicSingleLayer::correctThermoFields()
{
  rho_ == filmThermo_->rho();
  mu_ == filmThermo_->mu();
  sigma_ == filmThermo_->sigma();
}


void kinematicSingleLayer::resetPrimaryRegionSourceTerms()
{
  if (debug) {
    Info << "kinematicSingleLayer::resetPrimaryRegionSourceTerms()" << endl;
  }
  rhoSpPrimary_ == dimensionedScalar("zero", rhoSp_.dimensions(), 0.0);
  USpPrimary_ == dimensionedVector("zero", USp_.dimensions(), vector::zero);
  pSpPrimary_ == dimensionedScalar("zero", pSp_.dimensions(), 0.0);
}


void kinematicSingleLayer::transferPrimaryRegionThermoFields()
{
  if (debug) {
    Info << "kinematicSingleLayer::"
      << "transferPrimaryRegionThermoFields()" << endl;
  }
  // Update fields from primary region via direct mapped
  // (coupled) boundary conditions
  UPrimary_.correctBoundaryConditions();
  pPrimary_.correctBoundaryConditions();
  rhoPrimary_.correctBoundaryConditions();
  muPrimary_.correctBoundaryConditions();
}


void kinematicSingleLayer::transferPrimaryRegionSourceFields()
{
  if (debug) {
    Info << "kinematicSingleLayer::"
      << "transferPrimaryRegionSourceFields()" << endl;
  }
  // Convert accummulated source terms into per unit area per unit time
  const scalar deltaT = time_.deltaTValue();
  FOR_ALL(rhoSpPrimary_.boundaryField(), patchI) {
    const scalarField& priMagSf =
      primaryMesh().magSf().boundaryField()[patchI];
    rhoSpPrimary_.boundaryField()[patchI] /= priMagSf*deltaT;
    USpPrimary_.boundaryField()[patchI] /= priMagSf*deltaT;
    pSpPrimary_.boundaryField()[patchI] /= priMagSf*deltaT;
  }
  // Retrieve the source fields from the primary region via direct mapped
  // (coupled) boundary conditions
  // - fields require transfer of values for both patch AND to push the
  //   values into the first layer of internal cells
  rhoSp_.correctBoundaryConditions();
  USp_.correctBoundaryConditions();
  pSp_.correctBoundaryConditions();
  // update addedMassTotal counter
  if (time().outputTime()) {
    scalar addedMassTotal = 0.0;
    outputProperties().readIfPresent("addedMassTotal", addedMassTotal);
    addedMassTotal += returnReduce(addedMassTotal_, sumOp<scalar>());
    outputProperties().add("addedMassTotal", addedMassTotal, true);
    addedMassTotal_ = 0.0;
  }
}


tmp<volScalarField> kinematicSingleLayer::pu()
{
  return
    tmp<volScalarField>
    {
      new volScalarField
      {
        {
          typeName + ":pu",
          time_.timeName(),
          regionMesh(),
          IOobject::NO_READ,
          IOobject::NO_WRITE
        },
        pPrimary_                        // pressure (mapped from primary region)
        - pSp_                           // accumulated particle impingement
        - fvc::laplacian(sigma_, delta_) // surface tension
      }
    };
}


tmp<volScalarField> kinematicSingleLayer::pp()
{
  return
    tmp<volScalarField>
    {
      new volScalarField
      {
        {
          typeName + ":pp",
          time_.timeName(),
          regionMesh(),
          IOobject::NO_READ,
          IOobject::NO_WRITE
        },
        -rho_*gNormClipped() // hydrostatic effect only
      }
    };
}


void kinematicSingleLayer::correctAlpha()
{
  alpha_ == pos(delta_ - deltaSmall_);
}


void kinematicSingleLayer::updateSubmodels()
{
  if (debug) {
    Info << "kinematicSingleLayer::updateSubmodels()" << endl;
  }
  // Update injection model - mass returned is mass available for injection
  injection_.correct(availableMass_, cloudMassTrans_, cloudDiameterTrans_);
  // Update source fields
  const dimensionedScalar deltaT = time().deltaT();
  rhoSp_ += cloudMassTrans_/magSf()/deltaT;
  turbulence_->correct();
}


void kinematicSingleLayer::continuityCheck()
{
  const volScalarField deltaRho0(deltaRho_);
  solveContinuity();
  if (debug) {
    const volScalarField mass{deltaRho_*magSf()};
    const dimensionedScalar totalMass =
      fvc::domainIntegrate(mass)
      + dimensionedScalar("SMALL", dimMass*dimVolume, ROOTVSMALL);
    const scalar sumLocalContErr =
      (
        fvc::domainIntegrate(mag(mass - magSf()*deltaRho0))/totalMass
      ).value();
   const scalar globalContErr =
      (
        fvc::domainIntegrate(mass - magSf()*deltaRho0)/totalMass
      ).value();
    cumulativeContErr_ += globalContErr;
    Info << "Surface film: " << type() << nl
      << "    time step continuity errors: sum local = "
      << sumLocalContErr << ", global = " << globalContErr
      << ", cumulative = " << cumulativeContErr_ << endl;
  }
}


void kinematicSingleLayer::solveContinuity()
{
  if (debug) {
    Info << "kinematicSingleLayer::solveContinuity()" << endl;
  }
  solve
  (
    fvm::ddt(deltaRho_)
  + fvc::div(phi_)
  ==
  - rhoSp_
  );
}


void kinematicSingleLayer::updateSurfaceVelocities()
{
  // Push boundary film velocity values into internal field
  for (label i=0; i<intCoupledPatchIDs_.size(); i++) {
    label patchI = intCoupledPatchIDs_[i];
    const polyPatch& pp = regionMesh().boundaryMesh()[patchI];
    UIndirectList<vector>{Uw_, pp.faceCells()} = U_.boundaryField()[patchI];
  }
  Uw_ -= nHat()*(Uw_ & nHat());
  Uw_.correctBoundaryConditions();
  Us_ = turbulence_->Us();
}


tmp<mousse::fvVectorMatrix> kinematicSingleLayer::solveMomentum
(
  const volScalarField& pu,
  const volScalarField& pp
)
{
  if (debug) {
    Info << "kinematicSingleLayer::solveMomentum()" << endl;
  }
  // Momentum
  tmp<fvVectorMatrix> tUEqn {
    fvm::ddt(deltaRho_, U_)
  + fvm::div(phi_, U_)
  ==
  - USp_
  - rhoSp_*U_
  + forces_.correct(U_)
  + turbulence_->Su(U_)
  };
  fvVectorMatrix& UEqn = tUEqn();
  UEqn.relax();
  if (momentumPredictor_) {
    solve
    (
      UEqn
    ==
      fvc::reconstruct
      (
       - fvc::interpolate(delta_)
        *(
          regionMesh().magSf()
         *(
            fvc::snGrad(pu, "snGrad(p)")
          + fvc::snGrad(pp, "snGrad(p)")*fvc::interpolate(delta_)
          + fvc::snGrad(delta_)*fvc::interpolate(pp)
          )
        - (fvc::interpolate(rho_*gTan()) & regionMesh().Sf())
        )
      )
    );
    // Remove any patch-normal components of velocity
    U_ -= nHat()*(nHat() & U_);
    U_.correctBoundaryConditions();
  }
  return tUEqn;
}


void kinematicSingleLayer::solveThickness
(
  const volScalarField& pu,
  const volScalarField& pp,
  const fvVectorMatrix& UEqn
)
{
  if (debug) {
    Info << "kinematicSingleLayer::solveThickness()" << endl;
  }
  volScalarField rUA{1.0/UEqn.A()};
  U_ = rUA*UEqn.H();
  surfaceScalarField deltarUAf{fvc::interpolate(delta_*rUA)};
  surfaceScalarField rhof{fvc::interpolate(rho_)};
  surfaceScalarField phiAdd
  {
    "phiAdd",
    regionMesh().magSf()
    *(fvc::snGrad(pu, "snGrad(p)")
    + fvc::snGrad(pp, "snGrad(p)")*fvc::interpolate(delta_))
    - (fvc::interpolate(rho_*gTan()) & regionMesh().Sf())
  };
  constrainFilmField(phiAdd, 0.0);
  surfaceScalarField phid
  {
    "phid",
    (fvc::interpolate(U_*rho_) & regionMesh().Sf()) - deltarUAf*phiAdd*rhof
  };
  constrainFilmField(phid, 0.0);
  surfaceScalarField ddrhorUAppf
  {
    "deltaCoeff",
    fvc::interpolate(delta_)*deltarUAf*rhof*fvc::interpolate(pp)
  };
  regionMesh().setFluxRequired(delta_.name());
  for (int nonOrth=0; nonOrth<=nNonOrthCorr_; nonOrth++) {
    // Film thickness equation
    fvScalarMatrix deltaEqn {
      fvm::ddt(rho_, delta_)
    + fvm::div(phid, delta_)
    - fvm::laplacian(ddrhorUAppf, delta_)
    ==
    - rhoSp_
    };
    deltaEqn.solve();
    if (nonOrth == nNonOrthCorr_) {
      phiAdd +=
        fvc::interpolate(pp)*fvc::snGrad(delta_)*regionMesh().magSf();
      phi_ == deltaEqn.flux();
    }
  }
  // Bound film thickness by a minimum of zero
  delta_.max(0.0);
  // Update U field
  U_ -= fvc::reconstruct(deltarUAf*phiAdd);
  // Remove any patch-normal components of velocity
  U_ -= nHat()*(nHat() & U_);
  U_.correctBoundaryConditions();
  // Continuity check
  continuityCheck();
}


// Constructors 
kinematicSingleLayer::kinematicSingleLayer
(
  const word& modelType,
  const fvMesh& mesh,
  const dimensionedVector& g,
  const word& regionType,
  const bool readFields
)
:
  surfaceFilmModel{modelType, mesh, g, regionType},
  momentumPredictor_{solution().subDict("PISO").lookup("momentumPredictor")},
  nOuterCorr_{solution().subDict("PISO").lookupOrDefault("nOuterCorr", 1)},
  nCorr_{readLabel(solution().subDict("PISO").lookup("nCorr"))},
  nNonOrthCorr_{readLabel(solution().subDict("PISO").lookup("nNonOrthCorr"))},
  cumulativeContErr_{0.0},
  deltaSmall_{"deltaSmall", dimLength, SMALL},
  deltaCoLimit_{solution().lookupOrDefault("deltaCoLimit", 1e-4)},
  rho_
  {
    {
      "rhof",
      time().timeName(),
      regionMesh(),
      IOobject::NO_READ,
      IOobject::AUTO_WRITE
    },
    regionMesh(),
    {"zero", dimDensity, 0.0},
    zeroGradientFvPatchScalarField::typeName
  },
  mu_
  {
    {
      "muf",
      time().timeName(),
      regionMesh(),
      IOobject::NO_READ,
      IOobject::AUTO_WRITE
    },
    regionMesh(),
    {"zero", dimPressure*dimTime, 0.0},
    zeroGradientFvPatchScalarField::typeName
  },
  sigma_
  {
    {
      "sigmaf",
      time().timeName(),
      regionMesh(),
      IOobject::NO_READ,
      IOobject::AUTO_WRITE
    },
    regionMesh(),
    {"zero", dimMass/sqr(dimTime), 0.0},
    zeroGradientFvPatchScalarField::typeName
  },
  delta_
  {
    {
      "deltaf",
      time().timeName(),
      regionMesh(),
      IOobject::MUST_READ,
      IOobject::AUTO_WRITE
    },
    regionMesh()
  },
  alpha_
  {
    {
      "alpha",
      time().timeName(),
      regionMesh(),
      IOobject::READ_IF_PRESENT,
      IOobject::AUTO_WRITE
    },
    regionMesh(),
    {"zero", dimless, 0.0},
    zeroGradientFvPatchScalarField::typeName
  },
  U_
  {
    {
      "Uf",
      time().timeName(),
      regionMesh(),
      IOobject::MUST_READ,
      IOobject::AUTO_WRITE
    },
    regionMesh()
  },
  Us_
  {
    {
      "Usf",
      time().timeName(),
      regionMesh(),
      IOobject::NO_READ,
      IOobject::NO_WRITE
    },
    U_,
    zeroGradientFvPatchScalarField::typeName
  },
  Uw_
  {
    {
      "Uwf",
      time().timeName(),
      regionMesh(),
      IOobject::NO_READ,
      IOobject::NO_WRITE
    },
    U_,
    zeroGradientFvPatchScalarField::typeName
  },
  deltaRho_
  {
    {
      delta_.name() + "*" + rho_.name(),
      time().timeName(),
      regionMesh(),
      IOobject::NO_READ,
      IOobject::NO_WRITE
    },
    regionMesh(),
    {"zero", delta_.dimensions()*rho_.dimensions(), 0.0},
    zeroGradientFvPatchScalarField::typeName
  },
  phi_
  {
    {
      "phi",
      time().timeName(),
      regionMesh(),
      IOobject::NO_READ,
      IOobject::AUTO_WRITE
    },
    regionMesh(),
    {"0", dimLength*dimMass/dimTime, 0.0}
  },
  primaryMassTrans_
  {
    {
      "primaryMassTrans",
      time().timeName(),
      regionMesh(),
      IOobject::NO_READ,
      IOobject::NO_WRITE
    },
    regionMesh(),
    {"zero", dimMass, 0.0},
    zeroGradientFvPatchScalarField::typeName
  },
  cloudMassTrans_
  {
    {
      "cloudMassTrans",
      time().timeName(),
      regionMesh(),
      IOobject::NO_READ,
      IOobject::NO_WRITE
    },
    regionMesh(),
    {"zero", dimMass, 0.0},
    zeroGradientFvPatchScalarField::typeName
  },
  cloudDiameterTrans_
  {
    {
      "cloudDiameterTrans",
      time().timeName(),
      regionMesh(),
      IOobject::NO_READ,
      IOobject::NO_WRITE
    },
    regionMesh(),
    {"zero", dimLength, -1.0},
    zeroGradientFvPatchScalarField::typeName
  },
  USp_
  {
    {
      "USpf",
      time().timeName(),
      regionMesh(),
      IOobject::NO_READ,
      IOobject::NO_WRITE
    },
    regionMesh(),
    {"zero", dimMass*dimVelocity/dimArea/dimTime, vector::zero},
    this->mappedPushedFieldPatchTypes<vector>()
  },
  pSp_
  {
    {
      "pSpf",
      time_.timeName(),
      regionMesh(),
      IOobject::NO_READ,
      IOobject::NO_WRITE
    },
    regionMesh(),
    {"zero", dimPressure, 0.0},
    this->mappedPushedFieldPatchTypes<scalar>()
  },
  rhoSp_
  {
    {
      "rhoSpf",
      time_.timeName(),
      regionMesh(),
      IOobject::NO_READ,
      IOobject::NO_WRITE
    },
    regionMesh(),
    {"zero", dimMass/dimTime/dimArea, 0.0},
    this->mappedPushedFieldPatchTypes<scalar>()
  },
  USpPrimary_
  {
    {
      USp_.name(), // must have same name as USp_ to enable mapping
      time().timeName(),
      primaryMesh(),
      IOobject::NO_READ,
      IOobject::NO_WRITE
    },
    primaryMesh(),
    {"zero", USp_.dimensions(), vector::zero}
  },
  pSpPrimary_
  {
    {
      pSp_.name(), // must have same name as pSp_ to enable mapping
      time().timeName(),
      primaryMesh(),
      IOobject::NO_READ,
      IOobject::NO_WRITE
    },
    primaryMesh(),
    {"zero", pSp_.dimensions(), 0.0}
  },
  rhoSpPrimary_
  {
    {
      rhoSp_.name(), // must have same name as rhoSp_ to enable mapping
      time().timeName(),
      primaryMesh(),
      IOobject::NO_READ,
      IOobject::NO_WRITE
    },
    primaryMesh(),
    {"zero", rhoSp_.dimensions(), 0.0}
  },
  UPrimary_
  {
    {
      "U", // must have same name as U to enable mapping
      time().timeName(),
      regionMesh(),
      IOobject::NO_READ,
      IOobject::NO_WRITE
    },
    regionMesh(),
    {"zero", dimVelocity, vector::zero},
    this->mappedFieldAndInternalPatchTypes<vector>()
  },
  pPrimary_
  {
    {
      "p", // must have same name as p to enable mapping
      time().timeName(),
      regionMesh(),
      IOobject::NO_READ,
      IOobject::NO_WRITE
    },
    regionMesh(),
    {"zero", dimPressure, 0.0},
    this->mappedFieldAndInternalPatchTypes<scalar>()
  },
  rhoPrimary_
  {
    {
      "rho", // must have same name as rho to enable mapping
      time().timeName(),
      regionMesh(),
      IOobject::NO_READ,
      IOobject::NO_WRITE
    },
    regionMesh(),
    {"zero", dimDensity, 0.0},
    this->mappedFieldAndInternalPatchTypes<scalar>()
  },
  muPrimary_
  {
    {
      "thermo:mu", // must have same name as mu to enable mapping
      time().timeName(),
      regionMesh(),
      IOobject::NO_READ,
      IOobject::NO_WRITE
    },
    regionMesh(),
    {"zero", dimPressure*dimTime, 0.0},
    this->mappedFieldAndInternalPatchTypes<scalar>()
  },
  filmThermo_{filmThermoModel::New(*this, coeffs_)},
  availableMass_{regionMesh().nCells(), 0.0},
  injection_{*this, coeffs_},
  turbulence_{filmTurbulenceModel::New(*this, coeffs_)},
  forces_{*this, coeffs_},
  addedMassTotal_{0.0}
{
  if (readFields) {
    transferPrimaryRegionThermoFields();
    correctAlpha();
    correctThermoFields();
    deltaRho_ == delta_*rho_;
    surfaceScalarField phi0
    {
      {
        "phi",
        time().timeName(),
        regionMesh(),
        IOobject::READ_IF_PRESENT,
        IOobject::AUTO_WRITE,
        false
      },
      fvc::interpolate(deltaRho_*U_) & regionMesh().Sf()
    };
    phi_ == phi0;
  }
}


// Destructor 
kinematicSingleLayer::~kinematicSingleLayer()
{}


// Member Functions 
void kinematicSingleLayer::addSources
(
  const label patchI,
  const label faceI,
  const scalar massSource,
  const vector& momentumSource,
  const scalar pressureSource,
  const scalar /*energySource*/
)
{
  if (debug) {
    Info << "\nSurface film: " << type() << ": adding to film source:" << nl
      << "    mass     = " << massSource << nl
      << "    momentum = " << momentumSource << nl
      << "    pressure = " << pressureSource << endl;
  }
  rhoSpPrimary_.boundaryField()[patchI][faceI] -= massSource;
  USpPrimary_.boundaryField()[patchI][faceI] -= momentumSource;
  pSpPrimary_.boundaryField()[patchI][faceI] -= pressureSource;
  addedMassTotal_ += massSource;
}


void kinematicSingleLayer::preEvolveRegion()
{
  if (debug) {
    Info << "kinematicSingleLayer::preEvolveRegion()" << endl;
  }
  surfaceFilmModel::preEvolveRegion();
  transferPrimaryRegionThermoFields();
  correctThermoFields();
  transferPrimaryRegionSourceFields();
  // Reset transfer fields
//    availableMass_ = mass();
  availableMass_ = netMass();
  cloudMassTrans_ == dimensionedScalar{"zero", dimMass, 0.0};
  cloudDiameterTrans_ == dimensionedScalar{"zero", dimLength, 0.0};
}


void kinematicSingleLayer::evolveRegion()
{
  if (debug) {
    Info << "kinematicSingleLayer::evolveRegion()" << endl;
  }
  // Update film coverage indicator
  correctAlpha();
  // Update film wall and surface velocities
  updateSurfaceVelocities();
  // Update sub-models to provide updated source contributions
  updateSubmodels();
  // Solve continuity for deltaRho_
  solveContinuity();
  // Implicit pressure source coefficient - constant
  tmp<volScalarField> tpp{this->pp()};
  for (int oCorr=1; oCorr<=nOuterCorr_; oCorr++) {
    // Explicit pressure source contribution - varies with delta_
    tmp<volScalarField> tpu{this->pu()};
    // Solve for momentum for U_
    tmp<fvVectorMatrix> UEqn = solveMomentum(tpu(), tpp());
    // Film thickness correction loop
    for (int corr=1; corr<=nCorr_; corr++) {
      // Solve thickness for delta_
      solveThickness(tpu(), tpp(), UEqn());
    }
  }
  // Update deltaRho_ with new delta_
  deltaRho_ == delta_*rho_;
  // Reset source terms for next time integration
  resetPrimaryRegionSourceTerms();
}


scalar kinematicSingleLayer::CourantNumber() const
{
  scalar CoNum = 0.0;
  if (regionMesh().nInternalFaces() > 0) {
    const scalarField sumPhi
    {
      fvc::surfaceSum(mag(phi_))().internalField()
      /(deltaRho_.internalField() + ROOTVSMALL)
    };
    FOR_ALL(delta_, i) {
      if (delta_[i] > deltaCoLimit_) {
        CoNum = max(CoNum, sumPhi[i]/(delta_[i]*magSf()[i]));
      }
    }
    CoNum *= 0.5*time_.deltaTValue();
  }
  reduce(CoNum, maxOp<scalar>());
  Info << "Film max Courant number: " << CoNum << endl;
  return CoNum;
}


const volVectorField& kinematicSingleLayer::U() const
{
  return U_;
}


const volVectorField& kinematicSingleLayer::Us() const
{
  return Us_;
}


const volVectorField& kinematicSingleLayer::Uw() const
{
  return Uw_;
}


const volScalarField& kinematicSingleLayer::deltaRho() const
{
  return deltaRho_;
}


const surfaceScalarField& kinematicSingleLayer::phi() const
{
  return phi_;
}


const volScalarField& kinematicSingleLayer::rho() const
{
  return rho_;
}


const volScalarField& kinematicSingleLayer::T() const
{
  FATAL_ERROR_IN
  (
    "const volScalarField& kinematicSingleLayer::T() const"
  )
  << "T field not available for " << type() << abort(FatalError);
  return volScalarField::null();
}


const volScalarField& kinematicSingleLayer::Ts() const
{
  FATAL_ERROR_IN
  (
    "const volScalarField& kinematicSingleLayer::Ts() const"
  )
  << "Ts field not available for " << type() << abort(FatalError);
  return volScalarField::null();
}


const volScalarField& kinematicSingleLayer::Tw() const
{
  FATAL_ERROR_IN
  (
    "const volScalarField& kinematicSingleLayer::Tw() const"
  )
  << "Tw field not available for " << type() << abort(FatalError);
  return volScalarField::null();
}


const volScalarField& kinematicSingleLayer::Cp() const
{
  FATAL_ERROR_IN
  (
    "const volScalarField& kinematicSingleLayer::Cp() const"
  )
  << "Cp field not available for " << type() << abort(FatalError);
  return volScalarField::null();
}


const volScalarField& kinematicSingleLayer::kappa() const
{
  FATAL_ERROR_IN
  (
    "const volScalarField& kinematicSingleLayer::kappa() const"
  )
  << "kappa field not available for " << type() << abort(FatalError);
  return volScalarField::null();
}


tmp<volScalarField> kinematicSingleLayer::primaryMassTrans() const
{
  return
    tmp<volScalarField>
    {
      new volScalarField
      {
        {
          typeName + ":primaryMassTrans",
          time().timeName(),
          primaryMesh(),
          IOobject::NO_READ,
          IOobject::NO_WRITE,
          false
        },
        primaryMesh(),
        {"zero", dimMass/dimVolume/dimTime, 0.0}
      }
    };
}


const volScalarField& kinematicSingleLayer::cloudMassTrans() const
{
  return cloudMassTrans_;
}


const volScalarField& kinematicSingleLayer::cloudDiameterTrans() const
{
  return cloudDiameterTrans_;
}


void kinematicSingleLayer::info()
{
  Info << "\nSurface film: " << type() << endl;
  const scalarField& deltaInternal = delta_.internalField();
  const vectorField& Uinternal = U_.internalField();
  scalar addedMassTotal = 0.0;
  outputProperties().readIfPresent("addedMassTotal", addedMassTotal);
  addedMassTotal += returnReduce(addedMassTotal_, sumOp<scalar>());
  Info << indent << "added mass         = " << addedMassTotal << nl
    << indent << "current mass       = "
    << gSum((deltaRho_*magSf())()) << nl
    << indent << "min/max(mag(U))    = " << gMin(mag(Uinternal)) << ", "
    << gMax(mag(Uinternal)) << nl
    << indent << "min/max(delta)     = " << gMin(deltaInternal) << ", "
    << gMax(deltaInternal) << nl
    << indent << "coverage           = "
    << gSum(alpha_.internalField()*magSf())/gSum(magSf()) <<  nl;
  injection_.info(Info);
}


tmp<DimensionedField<scalar, volMesh>> kinematicSingleLayer::Srho() const
{
  return
    tmp<DimensionedField<scalar, volMesh>>
    {
      new DimensionedField<scalar, volMesh>
      {
        {
          typeName + ":Srho",
          time().timeName(),
          primaryMesh(),
          IOobject::NO_READ,
          IOobject::NO_WRITE,
          false
        },
        primaryMesh(),
        {"zero", dimMass/dimVolume/dimTime, 0.0}
      }
    };
}


tmp<DimensionedField<scalar, volMesh>> kinematicSingleLayer::Srho
(
  const label i
) const
{
  return
    tmp<DimensionedField<scalar, volMesh>>
    {
      new DimensionedField<scalar, volMesh>
      {
        IOobject
        {
          typeName + ":Srho(" + mousse::name(i) + ")",
          time().timeName(),
          primaryMesh(),
          IOobject::NO_READ,
          IOobject::NO_WRITE,
          false
        },
        primaryMesh(),
        {"zero", dimMass/dimVolume/dimTime, 0.0}
      }
    };
}


tmp<DimensionedField<scalar, volMesh>> kinematicSingleLayer::Sh() const
{
  return
    tmp<DimensionedField<scalar, volMesh>>
    {
      new DimensionedField<scalar, volMesh>
      {
        {
          typeName + ":Sh",
          time().timeName(),
          primaryMesh(),
          IOobject::NO_READ,
          IOobject::NO_WRITE,
          false
        },
        primaryMesh(),
        {"zero", dimEnergy/dimVolume/dimTime, 0.0}
      }
    };
}

}  // namespace surfaceFilmModels
}  // namespace regionModels
}  // namespace mousse
