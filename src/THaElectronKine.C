//*-- Author :    Ole Hansen   25-Feb-03

//////////////////////////////////////////////////////////////////////////
//
// THaElectronKine
//
// Calculate standard electron kinematics for a single spectrometer.
//
//////////////////////////////////////////////////////////////////////////

#include "THaElectronKine.h"
#include "THaSpectrometer.h"
#include "THaTrack.h"
#include "THaRun.h"
#include "VarDef.h"
#include "TLorentzVector.h"
#include "TVector3.h"
#include "TMath.h"

ClassImp(THaElectronKine)

//_____________________________________________________________________________
THaElectronKine::THaElectronKine( const char* name, const char* description,
				  const char* spectro, Double_t mass ) :
  THaPhysicsModule(name,description), fMA(mass), 
  fSpectroName(spectro), fSpectro(NULL)
{
  // Normal constructor.

}

//_____________________________________________________________________________
THaElectronKine::~THaElectronKine()
{
  // Destructor

  DefineVariables( kDelete );
}

//_____________________________________________________________________________
void THaElectronKine::Clear( Option_t* opt )
{
  // Clear all internal variables.

  fQ2 = fOmega = fW2 = fXbj = fScatAngle = fEpsilon = fQ3mag
    = fThetaQ = fPhiQ = 0.0;
}

//_____________________________________________________________________________
Int_t THaElectronKine::DefineVariables( EMode mode )
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
THaAnalysisObject::EStatus THaElectronKine::Init( const TDatime& run_time )
{
  // Initialize the module.
  // Locate the spectrometer apparatus named in fSpectroName and save
  // pointer to it.

  // Standard initialization. Calls this object's DefineVariables().
  if( THaPhysicsModule::Init( run_time ) != kOK )
    return fStatus;

  fSpectro = static_cast<THaSpectrometer*>
    ( FindModule( fSpectroName.Data(), "THaSpectrometer"));
  return fStatus;
}

//_____________________________________________________________________________
Int_t THaElectronKine::Process()
{
  // Calculate electron kinematics for the Golden Track of the spectrometer

  if( !IsOK() || !gHaRun ) return -1;

  THaTrack* theTrack = fSpectro->GetGoldenTrack();
  if( !theTrack ) return 1;

  Double_t p_in  = gHaRun->GetBeamP();

  TLorentzVector p0, p1, pA, pA1, q;
  const Double_t me = 0.511e-3; // electron mass FIXME: variable?
  const Double_t Mp = 0.938;    // proton mass (for x_bj)

  p0.SetXYZM( 0.0, 0.0, p_in, me );        // FIXME: beam slopes?
  p1.SetVectM( theTrack->GetPvect(), me );
  pA.SetXYZM( 0.0, 0.0, 0.0, fMA );        // Assume target at rest

  // Standard electron kinematics
  q          = p0 - p1;
  fQ2        = -q.M2();
  fQ3mag     = q.Vect().Mag();
  fOmega     = q.E();
  pA1        = pA + q;
  fW2        = pA1.M2();
  fScatAngle = p0.Angle( p1.Vect() );
  fEpsilon   = 1.0 / ( 1.0 + 2.0*q.Vect().Mag2()/fQ2*
		       TMath::Power( TMath::Tan(fScatAngle/2.0), 2.0 ));
  fThetaQ    = q.Theta();
  fPhiQ      = q.Phi();
  fXbj       = fQ2/(2.0*Mp*fOmega);

  return 0;
}

//_____________________________________________________________________________
Int_t THaElectronKine::ReadRunDatabase( const TDatime& date )
{
  // Qeury the run database. Currently queries for the target mass.
  // First searches for "<prefix>.MA", then, if not found, for "MA".
  // If still not found, use proton mass.

  Int_t err = THaPhysicsModule::ReadRunDatabase( date );
  if( err ) return err;

  FILE* f = OpenRunDBFile( date );
  if( !f ) return kFileError;

  fMA = 0.938;

  TString name(fPrefix), tag("MA"); name += tag;
  Int_t st = LoadDBvalue( f, date, name.Data(), fMA );
  if( st )
    LoadDBvalue( f, date, tag.Data(), fMA );

  fclose(f);
  return 0;
}
  
