//*-- Author :    Ole Hansen   04-May-04

//////////////////////////////////////////////////////////////////////////
//
// THaTrackEloss
//
// Correct energy loss of a track by a constant. This is basically trivial,
// but useful to have to provide input for kinematics modules. 
// More advanced corrections can be implemented by inheritance from
// this module.
//
//////////////////////////////////////////////////////////////////////////

#include "THaTrackEloss.h"
#include "THaSpectrometer.h"
#include "THaTrack.h"
#include "THaTrackInfo.h"
#include "TMath.h"
#include "TVector3.h"
#include "VarDef.h"
#include <iostream>

using namespace std;

//_____________________________________________________________________________
THaTrackEloss::THaTrackEloss( const char* name, 
			      const char* description,
			      const char* input_tracks,
			      Double_t particle_mass,
			      Int_t hadron_charge ) :
  THaElossCorrection(name,description,input_tracks,particle_mass,
		     hadron_charge), fTrackModule(NULL)
{
  // Normal constructor.

  Clear();
}

//_____________________________________________________________________________
THaTrackEloss::~THaTrackEloss()
{
  // Destructor

  DefineVariables( kDelete );
}

//_____________________________________________________________________________
void THaTrackEloss::CalcEloss( THaTrackInfo* trkifo )
{
  // Compute the expected energy loss for the track given in trkifo.
  //
  // Currently, we use a very simple algorithm that computes
  // the energy loss based on a fixed material thickness.
  // Only the beta dependence of the energy loss is used,
  // but there are no corrections for angle etc.
  //
  // May be overridden by derived classes as necessary.

  Double_t p0 = trkifo->GetP();
  Double_t beta = p0 / TMath::Sqrt(p0*p0 + fM*fM);
  if( fElectronMode ) {
    fEloss = ElossElectron( beta, fZmed, fAmed, 
			    fDensity, fPathlength );
  } else {
    fEloss = ElossHadron( fZ, beta, fZmed, fAmed, 
			  fDensity, fPathlength );
  }
}

//_____________________________________________________________________________
void THaTrackEloss::Clear( Option_t* opt )
{
  // Clear all event-by-event variables.
  
  THaElossCorrection::Clear(opt);
  TrkIfoClear();
}

//_____________________________________________________________________________
THaAnalysisObject::EStatus THaTrackEloss::Init( const TDatime& run_time )
{
  // Initialize the module.
  // Locate the input tracking module named in fInputName and save
  // pointer to it.

  // Standard initialization. Calls this object's DefineVariables().
  if( THaElossCorrection::Init( run_time ) != kOK )
    return fStatus;

  fTrackModule = dynamic_cast<THaTrackingModule*>
    ( FindModule( fInputName.Data(), "THaTrackingModule"));

  return fStatus;
}

//_____________________________________________________________________________
Int_t THaTrackEloss::DefineVariables( EMode mode )
{
  // Define/delete global variables.

  if( mode == kDefine && fIsSetup ) return kOK;
  THaElossCorrection::DefineVariables( mode );

  const char* var_prefix = "fTrkIfo.";

  // Corrected track parameters
  const RVarDef var1[] = {
    { "x",        "Target x coordinate",            "fX"},
    { "y",        "Target y coordinate",            "fY"},
    { "th",       "Tangent of target theta angle",  "fTheta"},
    { "ph",       "Tangent of target phi angle",    "fPhi"},    
    { "dp",       "Target delta",                   "fDp"},
    { "p",        "Lab momentum (GeV)",             "fP"},
    { "px",       "Lab momentum x (GeV)",           "GetPx()"},
    { "py",       "Lab momentum y (GeV)",           "GetPy()"},
    { "pz",       "Lab momentum z (GeV)",           "GetPz()"},
    { "ok",       "Data valid status flag (1=ok)",  "fOK"},
    { 0 }
  };
  DefineVarsFromList( var1, mode, var_prefix );

  return 0;
}

//_____________________________________________________________________________
Int_t THaTrackEloss::Process( const THaEvData& evdata )
{
  // Calculate corrections and adjust the track parameters.

  if( !IsOK() ) return -1;

  THaTrackInfo* trkifo = fTrackModule->GetTrackInfo();
  if( !trkifo->IsOK() ) return 2;

  // Copy the input track info
  fTrkIfo = *trkifo;
  
  // Compute the correction
  Double_t p_out = fTrkIfo.GetP(); 
  if( p_out <= 0.0 ) return 4; //oops
  Double_t E_out = TMath::Sqrt(p_out*p_out + fM*fM);
  if( !fTestMode )
    CalcEloss(trkifo);  //update fEloss
  Double_t p_in = TMath::Sqrt(p_out*p_out + fEloss*fEloss + 2.0*E_out*fEloss);
  
  // Apply the correction
  fTrkIfo.SetP(p_in);


  fDataValid = true;
  return 0;
}

//_____________________________________________________________________________
ClassImp(THaTrackEloss)

