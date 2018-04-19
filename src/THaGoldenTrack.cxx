//*-- Author :    Ole Hansen   04-Apr-03

//////////////////////////////////////////////////////////////////////////
//
// THaGoldenTrack
//
// Simple convenience class that makes a spectrometer's GoldenTrack
// directly available through global variables.
//
//////////////////////////////////////////////////////////////////////////

#include "THaGoldenTrack.h"
#include "THaSpectrometer.h"
#include "THaTrack.h"
#include "TClonesArray.h"
#include "VarDef.h"

ClassImp(THaGoldenTrack)

//_____________________________________________________________________________
THaGoldenTrack::THaGoldenTrack( const char* name, const char* description,
				const char* spectro ) :
  THaPhysicsModule(name,description), fIndex(-1), fGoldBeta(kBig), fTrack(0),
  fSpectroName(spectro), fSpectro(0)
{
  // Normal constructor.

}

//_____________________________________________________________________________
THaGoldenTrack::~THaGoldenTrack()
{
  // Destructor

  DefineVariables( kDelete );
}

//_____________________________________________________________________________
void THaGoldenTrack::Clear( Option_t* opt )
{
  THaPhysicsModule::Clear(opt);
  fTrkIfo.Clear(opt); fIndex = -1; fGoldBeta = kBig; fTrack = 0;
}

//_____________________________________________________________________________
THaAnalysisObject::EStatus THaGoldenTrack::Init( const TDatime& run_time )
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
Int_t THaGoldenTrack::DefineVariables( EMode mode )
{
  // Define/delete global variables.

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  const char* var_prefix = "fTrkIfo.";

  const RVarDef var1[] = {
    { "x",        "Target x coordinate",            "fX"},
    { "y",        "Target y coordinate",            "fY"},
    { "th",       "Tangent of target theta angle",  "fTheta"},
    { "ph",       "Tangent of target phi angle",    "fPhi"},    
    { "dp",       "Target delta",                   "fDp"},
    { "p",        "Lab momentum x (GeV)",           "fP"},
    { "px",       "Lab momentum x (GeV)",           "GetPx()"},
    { "py",       "Lab momentum y (GeV)",           "GetPy()"},
    { "pz",       "Lab momentum z (GeV)",           "GetPz()"},
    { "ok",       "Data valid status flag (1=ok)",  "fOK" },
    { 0 }
  };
  DefineVarsFromList( var1, mode, var_prefix );

  const RVarDef var2[] = {
    { "index",    "Index of Golden Track",         "fIndex" },
    { "beta", "Beta of Golden Track",          "fGoldBeta" },
    { 0 }
  };
  DefineVarsFromList( var2, mode );
  return 0;
}

//_____________________________________________________________________________
Int_t THaGoldenTrack::Process( const THaEvData& )
{
  // Calculate corrections and adjust the track parameters.

  if( !IsOK() ) return -1;

  fTrack = fSpectro->GetGoldenTrack();
  if( !fTrack ) return 1;
  if( !(fTrack->HasTarget())) return 2;

  // Save data in out TrackInfo (copying necessary because the track
  // pointer may change every event, and the global variable system
  // currently does not handle this)
  fTrkIfo = *fTrack;

  fGoldBeta = fTrack->GetBeta();

  // Find the track's index
  Int_t ntracks = fSpectro->GetNTracks();
  if( ntracks == 1 )
    fIndex = 0;
  else {  // ntracks>1
    TClonesArray* tracks = fSpectro->GetTracks();
    for( Int_t i=0; i<ntracks; i++ ) {
      THaTrack* theTrack = static_cast<THaTrack*>( tracks->At(i));
      if( theTrack == fTrack ) {
	fIndex = i;
	break;
      }
    }
  }

  fDataValid = true;
  return 0;
}
