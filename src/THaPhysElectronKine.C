//*-- Author :    Ole Hansen   25-Feb-03

//////////////////////////////////////////////////////////////////////////
//
// THaPhysElectronKine
//
// Calculate electron kinematics
//
//////////////////////////////////////////////////////////////////////////

#include "THaPhysElectronKine.h"
#include "THaGlobals.h"
#include "THaSpectrometer.h"
#include "THaTrack.h"
#include "THaRun.h"
#include "VarDef.h"
#include "TClass.h"
#include "TLorentzVector.h"
#include "TVector3.h"
#include "TMath.h"

ClassImp(THaPhysElectronKine)

const Double_t THaPhysElectronKine::kMp = 0.938;

//_____________________________________________________________________________
THaPhysElectronKine::THaPhysElectronKine( const char* name, 
					  const char* description ) :
  THaPhysicsModule(name,description), fMA(kMp), fSpectro(NULL)
{
  // Normal constructor.

}

//_____________________________________________________________________________
THaPhysElectronKine::~THaPhysElectronKine()
{
  // Destructor

  DefineVariables( kDelete );
}

//_____________________________________________________________________________
THaAnalysisObject::EStatus THaPhysElectronKine::Init( const TDatime& run_time )
{
  // Initialize the kinematics module. 
  // Locate the spectrometer apparatus named in fSpectroName and save
  // pointer to it.
  // Initialize our own global variables.

  static const char* const here = "Init()";

  fSpectro = NULL;

  //FIXME: read run database to get beam energy
  //FIXME: also get pointers to class that provides energy corrections

  if( THaPhysicsModule::Init( run_time ) )
    return fStatus;

  TObject* obj = gHaApps->FindObject( fSpectroName );
  if( !obj ) {
    Error( Here(here), "Object %s does not exist. "
	   "%s initialization failed.", 
	   fSpectroName.Data(), ClassName() );
    return fStatus = kInitError;
  }
  if( !obj->IsA()->InheritsFrom( "THaSpectrometer" )) {
    Error( Here(here), "Object %s (%s) is not a THaSpectrometer. "
	   "%s initialization failed.", 
	   obj->GetName(), obj->GetTitle(), ClassName() );
    return fStatus = kInitError;
  }
  fSpectro = static_cast<THaSpectrometer*>( obj );
  if( !fSpectro->IsOK() ) {
    Error( Here(here), "Spectrometer %s (%s) not properly initialized. "
	   "%s initialization failed.", 
	   obj->GetName(), obj->GetTitle(), ClassName() );
    fSpectro = NULL;
    return fStatus = kInitError;
  }
    
  return fStatus = kOK;
}

//_____________________________________________________________________________
Int_t THaPhysElectronKine::DefineVariables( EMode mode )
{
  // Define/delete standard variables for a spectrometer (tracks etc.)
  // Can be overridden or extended by derived (actual) apparatuses

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
Int_t THaPhysElectronKine::Process()
{
  // Calculate electron kinematics for the Golden Track of the spectrometer

  //FIXME: very preliminary test code
  // Input: should come from tracks, hardcode for test
  // Units are GeV.

//    Double_t p_in   = 1.0;       // projectile momentum
//    Double_t p_out  = 0.7334754797;   // scattered electron momentum
//    Double_t th_out = 0.8510991511;     // Theta (rad) of scattered electron
//    Double_t ph_out = 0.0;       // Phi (rad) of scattered electron
//    Double_t me     = 0.511e-3;  // particle mass (electron)
//    Double_t MA     = 0.938;     // target mass

  if( !fSpectro || !gHaRun ) return -1;

  THaTrack* theTrack = fSpectro->GetGoldenTrack();
  if( !theTrack ) return 1;

  Double_t p_in  = gHaRun->GetBeamP();

  TLorentzVector p0, p1, pA, pA1, q;
  const Double_t me = 0.511e-3; // electron mass FIXME: variable?
  const Double_t Mp = 0.938;    // proton mass (for x_bj)

  p0.SetXYZM( 0.0, 0.0, p_in, me ); // FIXME: beam slopes?
  p1.SetVectM( theTrack->GetPvect(), me );
  // Assume target at rest
  pA.SetXYZM( 0.0, 0.0, 0.0, fMA );

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

//    cout << "p0\n"; p0.Dump();
//    cout << "p1\n"; p1.Dump();
//    cout << "pA\n"; pA.Dump();
//    cout << "pA1\n"; pA1.Dump();
//    cout << "q\n"; q.Dump();

//    cout << "Q2, q3m, w, W2, xbj, ang, eps, th_q, ph_q ="
//         << "th_A, ph_A, p3A, q3m_cm = "
//         << fQ2 << " " << fQ3mag << " " << fOmega << " " << fW2 << " "
//         << fXbj << " " << fScatAngle << " " << fEpsilon << " " 
//         << fThetaQ << " "<< fPhiQ
//         << endl;

  return 0;
}
