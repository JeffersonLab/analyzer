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
  THaPhysicsModule(name,description), fM(particle_mass), 
  fMA(target_mass), fSpectroName(spectro), fSpectro(NULL), fBeam(NULL)
{
  // Standard constructor.

}

//_____________________________________________________________________________
THaPrimaryKine::THaPrimaryKine( const char* name, const char* description,
				const char* spectro, const char* beam, 
				Double_t particle_mass,	Double_t target_mass ) 
  : THaPhysicsModule(name,description), fM(particle_mass), 
    fMA(target_mass), fSpectroName(spectro), fBeamName(beam), 
    fSpectro(NULL), fBeam(NULL)
{
  // Constructor with specification of optional beam apparatus

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
  // Clear all internal variables.

  fQ2 = fOmega = fW2 = fXbj = fScatAngle = fEpsilon = fQ3mag
    = fThetaQ = fPhiQ = kBig;
  fDataValid = false;
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
    { "nu",      "Energy transfer (GeV)",                   "fOmega" },
    { "W2",      "Invariant mass of recoil system (GeV^2)", "fW2" }, 
    { "x_bj",    "Bjorken x",                               "fXbj" },
    { "angle",   "Scattering angle (rad)",                  "fScatAngle" },
    { "epsilon", "Virtual photon polarization factor",      "fEpsilon" },
    { "q3m",     "Magnitude of 3-momentum transfer",        "fQ3mag" },
    { "th_q",    "Theta of 3-momentum vector (rad)",        "fThetaQ" },
    { "ph_q",    "Phi of 3-momentum vector (rad)",          "fPhiQ" },
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

  // Standard initialization. Calls this object's DefineVariables().
  if( THaPhysicsModule::Init( run_time ) != kOK )
    return fStatus;

  fSpectro = dynamic_cast<THaTrackingModule*>
    ( FindModule( fSpectroName.Data(), "THaTrackingModule"));

  // Optional beam apparatus
  if( fBeamName.Length() > 0 )
    fBeam = dynamic_cast<THaBeam*>( FindModule( fBeamName.Data(), "THaBeam") );
  
  return fStatus;
}

//_____________________________________________________________________________
Int_t THaPrimaryKine::Process( const THaEvData& evdata )
{
  // Calculate electron kinematics for the Golden Track of the spectrometer

  if( !IsOK() || !gHaRun ) return -1;

  THaTrackInfo* trkifo = fSpectro->GetTrackInfo();
  if( !trkifo || !trkifo->IsOK() ) return 1;

  // FIXME: allow for beam energy loss
  Double_t p_in  = gHaRun->GetParameters()->GetBeamP();

  // Determine 4-momentum of incident particle. 
  // If a beam apparatus given, use it to set the beam direction.
  if( fBeam ) {
    if( fBeam->GetDirection().Mag2() > 0.0 ) {
      TVector3 p_beam = fBeam->GetDirection();
      p_beam.SetMag(p_in);
      fP0.SetVectM( p_beam, fM );
    } else
      // Oops, beam direction vector is zero?
      return 2;
  } else
    // If no beam given, assume beam along z_lab
    fP0.SetXYZM( 0.0, 0.0, p_in, fM );

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
void THaPrimaryKine::PrintInitError( const char* here )
{
  Error( Here(here), "Cannot set. Module already initialized." );
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
