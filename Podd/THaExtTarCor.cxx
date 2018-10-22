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
// The corrected quantities are available in global variables 
// of this module:
//
//   x:        Effective x_tg found by this routine (m)
//   y:        y_tg (unchanged) (m)
//   th:       TRANSPORT theta at target (rad)
//   ph:       TRANSPORT phi (unchanged) (rad)
//   dp:       delta
//   p:        absolute momentum (GeV)
//   px,py,pz: lab momentum vector components (GeV)
//   ok:       data valid status flag (0/1)
//
// The following global variables are avalable:
//
//   delta_p:  Size of momentum correction (GeV)
//   delta_dp: Size of delta correction
//   delta_th: Size of theta correction (rad)
//
//////////////////////////////////////////////////////////////////////////

#include "THaExtTarCor.h"
#include "THaVertexModule.h"
#include "THaSpectrometer.h"
#include "THaTrack.h"
#include "THaTrackInfo.h"
#include "TMath.h"
#include "TVector3.h"
#include "VarDef.h"

//_____________________________________________________________________________
THaExtTarCor::THaExtTarCor( const char* name, const char* description,
			    const char* spectro, const char* vertex ) :
  THaPhysicsModule(name,description), fThetaCorr(0.0), fDeltaCorr(0.0),
  fSpectroName(spectro), fVertexName(vertex), 
  fTrackModule(NULL), fVertexModule(NULL)
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
  // Clear all event-by-event variables.
  
  THaPhysicsModule::Clear(opt);
  TrkIfoClear();
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

  fTrackModule = dynamic_cast<THaTrackingModule*>
    ( FindModule( fSpectroName.Data(), "THaTrackingModule"));
  if( !fTrackModule )
    return fStatus;

  fTrkIfo.SetSpectrometer( fTrackModule->GetTrackInfo()->GetSpectrometer() );

  // If no vertex module given, try to get the vertex info from the
  // same module as the tracks, e.g. from a spectrometer
  if( fVertexName.IsNull())  fVertexName = fSpectroName;

  fVertexModule = dynamic_cast<THaVertexModule*>
    ( FindModule( fVertexName.Data(), "THaVertexModule" ));
  if( !fVertexModule )
    return fStatus;
    
  // Standard initialization. Calls this object's DefineVariables().
  THaPhysicsModule::Init( run_time );

  return fStatus;
}

//_____________________________________________________________________________
Int_t THaExtTarCor::DefineVariables( EMode mode )
{
  // Define/delete global variables.

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  DefineVarsFromList( THaTrackingModule::GetRVarDef(), mode );

  const RVarDef var2[] = {
    { "delta_p",  "Size of momentum correction",    "fDeltaP" },
    { "delta_dp", "Size of delta correction",       "fDeltaDp" },
    { "delta_th", "Size of theta correction (rad)", "fDeltaTh" },
    { 0 }
  };
  DefineVarsFromList( var2, mode );
  return 0;
}

//_____________________________________________________________________________
Int_t THaExtTarCor::Process( const THaEvData& )
{
  // Calculate corrections and adjust the track parameters.

  if( !IsOK() ) return -1;

  THaTrackInfo* trkifo = fTrackModule->GetTrackInfo();
  if( !trkifo->IsOK() ) return 2;
  THaSpectrometer* spectro = trkifo->GetSpectrometer();
  if( !spectro ) return 3;

  Double_t ray[6];
  spectro->LabToTransport( fVertexModule->GetVertex(), 
			   trkifo->GetPvect(), ray );
  // Ignore junk
  if( TMath::Abs(ray[0]) > 0.1 || TMath::Abs(ray[1]) > 1.0 ||
      TMath::Abs(ray[2]) > 0.1 || TMath::Abs(ray[3]) > 1.0 ||
      TMath::Abs(ray[5]) > 1.0 ) 
    return 3;

  TVector3 pvect;
  // Calculate corrections & recalculate track parameters
  Double_t x_tg = ray[0];
  fDeltaTh = fThetaCorr * x_tg;
  fDeltaDp = x_tg / fDeltaCorr;
  Double_t theta = trkifo->GetTheta() + fDeltaTh;

  Double_t dp = trkifo->GetDp() + fDeltaDp;
  Double_t p  = spectro->GetPcentral() * ( 1.0+dp );
  fDeltaP = p - trkifo->GetP();
  spectro->TransportToLab( p, theta, trkifo->GetPhi(), pvect );

  // Get a second-iteration value for x_tg based on the 
  // corrected momentum vector
  spectro->LabToTransport( fVertexModule->GetVertex(), pvect, ray );

  // Save results in our TrackInfo
  fTrkIfo.Set( p, dp, ray[0], trkifo->GetY(), theta, trkifo->GetPhi(), pvect );

  fDataValid = true;
  return 0;
}

//_____________________________________________________________________________
Int_t THaExtTarCor::ReadRunDatabase( const TDatime& date )
{
  // Qeury the run database for the correction coefficients.
  // Use reasonable defaults if not found.
  // First try tags "<prefix>.theta_corr" and "<prefix>.delta_corr", then
  // global names "theta_corr" and "delta_corr".

  Int_t err = THaPhysicsModule::ReadRunDatabase( date );
  if( err ) return err;

  FILE* f = OpenRunDBFile( date );
  if( !f ) return kFileError;

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
  fclose(f);
  return kOK;
}
  
//_____________________________________________________________________________
ClassImp(THaExtTarCor)

