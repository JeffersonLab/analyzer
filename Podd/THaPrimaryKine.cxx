//*-- Author :    Ole Hansen   25-Feb-03

//////////////////////////////////////////////////////////////////////////
//
// THaPrimaryKine
//
// Calculate kinematics of scattering of the primary (beam) particle.
// These are usually the electron kinematics.
// For backwards-compatibility, primary electron kinematics are also
// available through the derived class THaElectronKine.
//
//////////////////////////////////////////////////////////////////////////

#include "THaPrimaryKine.h"
#include "THaTrackingModule.h"
#include "THaRunBase.h"
#include "THaRunParameters.h"
#include "THaBeam.h"
#include "VarDef.h"
#include "TMath.h"

using namespace std;

ClassImp(THaPrimaryKine)

//_____________________________________________________________________________
THaPrimaryKine::THaPrimaryKine( const char* name, const char* description,
				const char* spectro, Double_t particle_mass,
				Double_t target_mass ) :
  THaPhysicsModule(name,description),
  fQ2(kBig), fOmega(kBig), fW2(kBig), fXbj(kBig), fScatAngle(kBig),
  fEpsilon(kBig), fQ3mag(kBig), fThetaQ(kBig), fPhiQ(kBig),
  fM(particle_mass), fMA(target_mass),
  fSpectroName(spectro), fSpectro(0), fBeam(0)
{
  // Standard constructor. Must specify particle mass. Incident particles
  // are assumed to be along z_lab.

}

//_____________________________________________________________________________
THaPrimaryKine::THaPrimaryKine( const char* name, const char* description,
				const char* spectro, const char* beam, 
				Double_t target_mass ) 
  : THaPhysicsModule(name,description),
    fQ2(kBig), fOmega(kBig), fW2(kBig), fXbj(kBig), fScatAngle(kBig),
    fEpsilon(kBig), fQ3mag(kBig), fThetaQ(kBig), fPhiQ(kBig),
    fM(-1.0), fMA(target_mass),
    fSpectroName(spectro), fBeamName(beam), fSpectro(0), fBeam(0)
{
  // Constructor with specification of optional beam module.
  // Particle mass will normally come from the beam module.

}

//_____________________________________________________________________________
THaPrimaryKine::~THaPrimaryKine()
{
  // Destructor

  DefineVariables( kDelete );
}

//_____________________________________________________________________________
void THaPrimaryKine::Clear( Option_t* opt )
{
  // Clear event-by-event variables.

  THaPhysicsModule::Clear(opt);
  fQ2 = fOmega = fW2 = fXbj = fScatAngle = fEpsilon = fQ3mag
    = fThetaQ = fPhiQ = kBig;
  // Clear the 4-vectors  
  fP0.SetXYZT(kBig,kBig,kBig,kBig);
  fP1 = fA = fA1 = fQ = fP0;

}

//_____________________________________________________________________________
Int_t THaPrimaryKine::DefineVariables( EMode mode )
{
  // Define/delete global variables.

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  RVarDef vars[] = {
    { "Q2",      "4-momentum transfer squared (GeV^2)",     "fQ2" },
    { "omega",   "Energy transfer (GeV)",                   "fOmega" },
    { "W2",      "Invariant mass of recoil system (GeV^2)", "fW2" }, 
    { "x_bj",    "Bjorken x",                               "fXbj" },
    { "angle",   "Scattering angle (rad)",                  "fScatAngle" },
    { "epsilon", "Virtual photon polarization factor",      "fEpsilon" },
    { "q3m",     "Magnitude of 3-momentum transfer",        "fQ3mag" },
    { "th_q",    "Theta of 3-momentum vector (rad)",        "fThetaQ" },
    { "ph_q",    "Phi of 3-momentum vector (rad)",          "fPhiQ" },
    { "nu",      "Energy transfer (GeV)",                   "fOmega" },
    { "q_x",     "x-cmp of Photon vector in the lab",       "fQ.X()" },
    { "q_y",     "y-cmp of Photon vector in the lab",       "fQ.Y()" },
    { "q_z",     "z-cmp of Photon vector in the lab",       "fQ.Z()" },
    { 0 }
  };
  return DefineVarsFromList( vars, mode );
}

//_____________________________________________________________________________
THaAnalysisObject::EStatus THaPrimaryKine::Init( const TDatime& run_time )
{
  // Initialize the module.
  // Locate the spectrometer apparatus named in fSpectroName and save
  // pointer to it.

  fSpectro = dynamic_cast<THaTrackingModule*>
    ( FindModule( fSpectroName.Data(), "THaTrackingModule"));
  if( !fSpectro )
    return fStatus;

  // Optional beam apparatus
  if( fBeamName.Length() > 0 ) {
    fBeam = dynamic_cast<THaBeamModule*>
      ( FindModule( fBeamName.Data(), "THaBeamModule") );
    if( !fBeam )
      return fStatus;
    if( fM <= 0.0 )
      fM = fBeam->GetBeamInfo()->GetM();
  }

  // Standard initialization. Calls this object's DefineVariables().
  if( THaPhysicsModule::Init( run_time ) != kOK )
    return fStatus;

  if( fM <= 0.0 ) {
    Error( Here("Init"), "Particle mass not defined. Module "
	   "initialization failed" );
    fStatus = kInitError;
  }
  return fStatus;
}

//_____________________________________________________________________________
Int_t THaPrimaryKine::Process( const THaEvData& )
{
  // Calculate electron kinematics for the Golden Track of the spectrometer

  if( !IsOK() || !gHaRun ) return -1;

  THaTrackInfo* trkifo = fSpectro->GetTrackInfo();
  if( !trkifo || !trkifo->IsOK() ) return 1;

  // Determine 4-momentum of incident particle. 
  // If a beam module given, use it to get the beam momentum. This 
  // module may apply corrections for beam energy loss, variations, etc.
  if( fBeam ) {
    fP0.SetVectM( fBeam->GetBeamInfo()->GetPvect(), fM );
  } else {
    // If no beam given, assume beam along z_lab
    Double_t p_in  = gHaRun->GetParameters()->GetBeamP();
    fP0.SetXYZM( 0.0, 0.0, p_in, fM );
  }

  fP1.SetVectM( trkifo->GetPvect(), fM );
  fA.SetXYZM( 0.0, 0.0, 0.0, fMA );         // Assume target at rest

  // proton mass (for x_bj)
  const Double_t Mp = 0.938;

  // Standard electron kinematics
  fQ         = fP0 - fP1;
  fQ2        = -fQ.M2();
  fQ3mag     = fQ.P();
  fOmega     = fQ.E();
  fA1        = fA + fQ;
  fW2        = fA1.M2();
  fScatAngle = fP0.Angle( fP1.Vect() );
  fEpsilon   = 1.0 / ( 1.0 + 2.0*fQ3mag*fQ3mag/fQ2*
		       TMath::Power( TMath::Tan(fScatAngle/2.0), 2.0 ));
  fThetaQ    = fQ.Theta();
  fPhiQ      = fQ.Phi();
  fXbj       = fQ2/(2.0*Mp*fOmega);

  fDataValid = true;
  return 0;
}

//_____________________________________________________________________________
Int_t THaPrimaryKine::ReadRunDatabase( const TDatime& date )
{
  // Query the run database for particle and target masses.
  //
  // If the particle or target masses weren't given to the constructor, 
  // then search for them in the run database.
  //
  // For the particle mass, first search for "<prefix>.M", then, 
  // if not found, for "M". If still not found, use electron mass mass.
  //
  // For the target mass, first search for "<prefix>.MA", then, if not found, 
  // for "MA". If still not found, use proton mass.

  Int_t err = THaPhysicsModule::ReadRunDatabase( date );
  if( err ) return err;

  if ( fM > 0.0 && fMA > 0.0 ) 
    return 0;

  FILE* f = OpenRunDBFile( date );
  if( !f ) return kFileError;

  if( fM <= 0.0 ) {
    TString name(fPrefix), tag("M"); name += tag;
    Int_t st = LoadDBvalue( f, date, name.Data(), fM );
    if( st )
      LoadDBvalue( f, date, tag.Data(), fM );
    if( fM <= 0.0 ) fM = 0.511e-3;
  }
  if( fMA <= 0.0 ) {
    TString name(fPrefix), tag("MA"); name += tag;
    Int_t st = LoadDBvalue( f, date, name.Data(), fMA );
    if( st )
      LoadDBvalue( f, date, tag.Data(), fMA );
    if( fMA <= 0.0 ) fMA = 0.938;
  }
  
  fclose(f);
  return 0;
}
  
//_____________________________________________________________________________
void THaPrimaryKine::SetMass( Double_t m ) 
{
  if( !IsInit())
    fM = m; 
  else
    PrintInitError("SetMass()");
}

//_____________________________________________________________________________
void THaPrimaryKine::SetTargetMass( Double_t m ) 
{
  if( !IsInit())
    fMA = m; 
  else
    PrintInitError("SetTargetMass()");
}

//_____________________________________________________________________________
void THaPrimaryKine::SetSpectrometer( const char* name ) 
{
  if( !IsInit())
    fSpectroName = name; 
  else
    PrintInitError("SetSpectrometer()");
}

//_____________________________________________________________________________
void THaPrimaryKine::SetBeam( const char* name ) 
{
  if( !IsInit())
    fBeamName = name; 
  else
    PrintInitError("SetBeam()");
}
