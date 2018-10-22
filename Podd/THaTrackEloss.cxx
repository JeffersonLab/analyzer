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
#include "THaVertexModule.h"
#include "TMath.h"
#include "TVector3.h"

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

  static const char* const here = "Init()";

  // Find the input tracking module
  fTrackModule = dynamic_cast<THaTrackingModule*>
    ( FindModule( fInputName.Data(), "THaTrackingModule"));
  if( !fTrackModule )
    return fStatus;

  // Get the parent spectrometer apparatus from the input module  
  THaSpectrometer* spectro = fTrackModule->GetTrackInfo()->GetSpectrometer();
  if( !spectro ) {
    Error( Here(here), "Oops. Input tracking module has no pointer "
	   "to a spectrometer?!?" );
    return fStatus = kInitError;
  }
  // Needed for initialization of dependent modules in a chain
  fTrkIfo.SetSpectrometer( spectro );

  // Standard initialization. Calls this object's DefineVariables() and
  // reads meterial properties from the run database.
  THaElossCorrection::Init( run_time );

  return fStatus;
}

//_____________________________________________________________________________
Int_t THaTrackEloss::DefineVariables( EMode mode )
{
  // Define/delete global variables.

  if( mode == kDefine && fIsSetup ) return kOK;
  THaElossCorrection::DefineVariables( mode );

  return DefineVarsFromList( THaTrackingModule::GetRVarDef(), mode );
}

//_____________________________________________________________________________
Int_t THaTrackEloss::Process( const THaEvData& )
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
  if( !fTestMode ) {
    // calculate pathlength for this event if we have a vertex module
    if( fExtPathMode ) {
      if( !fVertexModule->HasVertex() )
	return 1;
      fPathlength = 
	TMath::Abs(fVertexModule->GetVertex().Z() - fZref) * fScale;
    }
    //update fEloss
    CalcEloss(trkifo); 
  }
  Double_t p_in = TMath::Sqrt(p_out*p_out + fEloss*fEloss + 2.0*E_out*fEloss);
  
  // Apply the correction
  fTrkIfo.SetP(p_in);

  fDataValid = true;
  return 0;
}

//_____________________________________________________________________________
ClassImp(THaTrackEloss)

