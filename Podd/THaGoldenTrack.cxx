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
#include <vector>

using namespace std;

ClassImp(THaGoldenTrack)

//_____________________________________________________________________________
THaGoldenTrack::THaGoldenTrack( const char* name, const char* description,
				const char* spectro ) :
  THaPhysicsModule(name,description), fIndex(-1), fGoldBeta(kBig),
  fTrack(nullptr), fSpectroName(spectro), fSpectro(nullptr)
{
  // Normal constructor.

}

//_____________________________________________________________________________
THaGoldenTrack::~THaGoldenTrack()
{
  // Destructor

  RemoveVariables();
}

//_____________________________________________________________________________
void THaGoldenTrack::Clear( Option_t* opt )
{
  THaPhysicsModule::Clear(opt);
  fTrkIfo.Clear(opt); fIndex = -1; fGoldBeta = kBig; fTrack = nullptr;
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

  const char* var_prefix = "fTrkIfo.";

  const vector<RVarDef> var1= {
    { .name = "x",   .desc = "Target x coordinate",            .def = "fX"},
    { .name = "y",   .desc = "Target y coordinate",            .def = "fY"},
    { .name = "th",  .desc = "Tangent of target theta angle",  .def = "fTheta"},
    { .name = "ph",  .desc = "Tangent of target phi angle",    .def = "fPhi"},
    { .name = "dp",  .desc = "Target delta",                   .def = "fDp"},
    { .name = "p",   .desc = "Lab momentum x (GeV)",           .def = "fP"},
    { .name = "px",  .desc = "Lab momentum x (GeV)",           .def = "GetPx()"},
    { .name = "py",  .desc = "Lab momentum y (GeV)",           .def = "GetPy()"},
    { .name = "pz",  .desc = "Lab momentum z (GeV)",           .def = "GetPz()"},
    { .name = "ok",  .desc = "Data valid status flag (1=ok)",  .def = "fOK" },
  };
  Int_t ret = DefineVarsFromList( var1, mode, var_prefix );
  if( ret )
    return ret;

  const vector<RVarDef> var2 = {
    { .name = "index", .desc = "Index of Golden Track", .def = "fIndex" },
    { .name = "beta",  .desc = "Beta of Golden Track",  .def = "fGoldBeta" },
  };
  return DefineVarsFromList( var2, mode );
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
      if( tracks->At(i) == fTrack ) {
	fIndex = i;
	break;
      }
    }
  }

  fDataValid = true;
  return 0;
}
