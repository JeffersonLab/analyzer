//*-- Author :    Ole Hansen   17-Mar-03

//////////////////////////////////////////////////////////////////////////
//
// THaExtTarCor
//
// Calculate corrections to the reconstructed momentum and out-of-plane
// angle that are due to a finite x position of the reaction point
// at the target.  Finite x-positions occur as the result of
//
// - beam position deviations and raster
// - spectrometer misalignment
// - targets extended along the beam (finite y_target positions)
//
// This module implements a simple first-order correction devised by
// Mark Jones. It is sufficient for most applications of the Hall A HRS 
// spectrometers.  
//
// Currently, only the Golden Track of the given spectrometer is corrected.
// (This restriction will be dropped once all vertex modules support 
// multiple tracks.)
// The corrected quantities are (in THaTrack):
//
//   fTX:      Effective x_tg found by this routine
//   fTTheta:  TRANSPORT theta at target
//   fDp:      delta
//   fP:       absolute momentum
//   fPvect:   lab momentum vector
//
// The following global variables are avalable:
//
//   delta_p:  Size of momentum correction
//   delta_dp: Size of delta correction
//   delta_th: Size of theta correction
//
//////////////////////////////////////////////////////////////////////////

#include "THaExtTarCor.h"
#include "THaVertexModule.h"
#include "THaSpectrometer.h"
#include "THaTrack.h"
#include "TClonesArray.h"
#include "TMath.h"
#include "TVector3.h"
#include "VarDef.h"

#include <iostream>

ClassImp(THaExtTarCor)

//_____________________________________________________________________________
THaExtTarCor::THaExtTarCor( const char* name, const char* description,
			    const char* spectro, const char* vertex ) :
  THaPhysicsModule(name,description), fThetaCorr(0.0), fDeltaCorr(0.0),
  fSpectroName(spectro), fSpectro(0), fVertexName(vertex), fVertexModule(0)
{
  // Normal constructor.

  Clear();
}

//_____________________________________________________________________________
THaExtTarCor::~THaExtTarCor()
{
  // Destructor

  DefineVariables( kDelete );
}

//_____________________________________________________________________________
void THaExtTarCor::Clear( Option_t* opt )
{
  // Clear all event-by-event variables variables.
  
  fDeltaTh = fDeltaDp = fDeltaP = 0.0;
}

//_____________________________________________________________________________
THaAnalysisObject::EStatus THaExtTarCor::Init( const TDatime& run_time )
{
  // Initialize the module.
  // Locate the spectrometer apparatus named in fSpectroName and save
  // pointer to it.
  // Also, if given, locate the vertex module given in fVertexName
  // and save pointer to it.

  // Standard initialization. Calls this object's DefineVariables().
  if( THaPhysicsModule::Init( run_time ) != kOK )
    return fStatus;

  fSpectro = static_cast<THaSpectrometer*>
    ( FindModule( fSpectroName.Data(), "THaSpectrometer"));
  if( !fSpectro )
    return fStatus;

  if( fVertexName.Length() > 0 ) 
    fVertexModule = static_cast<THaVertexModule*>
      ( FindModule( fVertexName.Data(), "THaVertexModule"));

  return fStatus;
}

//_____________________________________________________________________________
Int_t THaExtTarCor::DefineVariables( EMode mode )
{
  // Define/delete global variables.

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  RVarDef vars[] = {
    { "delta_p",  "Size of momentum correction",    "fDeltaP" },
    { "delta_dp", "Size of delta correction",       "fDeltaDp" },
    { "delta_th", "Size of theta correction (rad)", "fDeltaTh" },
    { 0 }
  };
  return DefineVarsFromList( vars, mode );
}

//_____________________________________________________________________________
Int_t THaExtTarCor::Process()
{
  // Calculate corrections and adjust the track parameters.

  if( !IsOK() ) return -1;

  THaTrack* theTrack = fSpectro->GetGoldenTrack();
  if( !theTrack ) return 1;

  // Get the reaction point vector - either from the track
  // or from an explicitly-specified vertex module
  const TVector3* vertex;
  if( fVertexModule )
    vertex = &(fVertexModule->GetVertex());
  else
    vertex = &(theTrack->GetVertex());

  Double_t ray[6];
  fSpectro->LabToTransport( *vertex, theTrack->GetPvect(), ray );
  // Ignore junk
  if( TMath::Abs(ray[0]) > 0.1 || TMath::Abs(ray[1]) > 1.0 ||
      TMath::Abs(ray[2]) > 0.1 || TMath::Abs(ray[3]) > 1.0 ||
      TMath::Abs(ray[5]) > 1.0 )
    return 2;

  // Calculate corrections & recalculate track parameters
  Double_t x_tg = ray[0];
  fDeltaTh = fThetaCorr * x_tg;
  fDeltaDp = x_tg / fDeltaCorr;
  Double_t theta = theTrack->GetTTheta() + fDeltaTh;
  theTrack->SetTarget( x_tg, theTrack->GetTY(), 
		       theta, theTrack->GetTPhi() );
  Double_t dp = theTrack->GetDp() + fDeltaDp;
  Double_t p  = fSpectro->GetPcentral() * ( 1.0+dp );
  fDeltaP = p - theTrack->GetP();
  theTrack->SetDp( dp );
  theTrack->SetMomentum( p );
  fSpectro->TransportToLab( p, theta, theTrack->GetTPhi(), 
			    theTrack->GetPvect() );
  // Get a second-iteration value for x_tg based on the 
  // corrected momentum vector
  fSpectro->LabToTransport( *vertex, theTrack->GetPvect(), ray );
  theTrack->SetTarget( ray[0], theTrack->GetTY(), 
		       theta, theTrack->GetTPhi() );

  return 0;
}

//_____________________________________________________________________________
Int_t THaExtTarCor::ReadRunDatabase( FILE* f, const TDatime& date )
{
  // Qeury the run database for the correction coefficients.
  // Use reasonable defaults if not found.
  // First try tags "<prefix>.theta_corr" and "<prefix>.delta_corr", then
  // global names "theta_corr" and "delta_corr".

  const Double_t DEL_COR = 5.18;
  fThetaCorr = 0.61;
  fDeltaCorr = DEL_COR;

  TString name(fPrefix), tag("theta_corr"); name += tag;
  Int_t st = LoadDBvalue( f, date, name.Data(), fThetaCorr );
  if( st )
    LoadDBvalue( f, date, tag.Data(), fThetaCorr );

  name = fPrefix; tag = "delta_corr"; name += tag;
  st = LoadDBvalue( f, date, name.Data(), fDeltaCorr );
  if( st )
    LoadDBvalue( f, date, tag.Data(), fDeltaCorr );

  if( TMath::Abs(fDeltaCorr) < 1e-6 ) {
    Warning(Here("ReadRunDatabase()"), 
	    "delta_corr parameter from database too small (%e).\n"
	    "Using default.", fDeltaCorr );
    fDeltaCorr = DEL_COR;
  }	    
  return 0;
}
  
