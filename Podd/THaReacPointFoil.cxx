//*-- Author :    BR

//////////////////////////////////////////////////////////////////////////
//
// THaReacPointFoil
//
// Calculate vertex for all the tracks from the beam
// apparatus, assuming a single foil target at z = 0
//
//
//////////////////////////////////////////////////////////////////////////

#include "THaReacPointFoil.h"
#include "THaSpectrometer.h"
#include "THaTrack.h"
#include "THaBeam.h"
#include "TMath.h"

ClassImp(THaReacPointFoil)

//_____________________________________________________________________________
THaReacPointFoil::THaReacPointFoil( const char* name, const char* description,
				    const char* spectro, const char* beam ) :
  THaPhysicsModule(name,description), fSpectroName(spectro), 
  fBeamName(beam), fSpectro(NULL), fBeam(NULL)
{
  // Normal constructor.

}

//_____________________________________________________________________________
THaReacPointFoil::~THaReacPointFoil()
{
  // Destructor

  DefineVariables( kDelete );
}

//_____________________________________________________________________________
void THaReacPointFoil::Clear( Option_t* opt )
{
  // Clear all event-by-event variables.
  
  THaPhysicsModule::Clear(opt);
  VertexClear();
}

//_____________________________________________________________________________
Int_t THaReacPointFoil::DefineVariables( EMode mode )
{
  // Define/delete analysis variables

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  return DefineVarsFromList( THaVertexModule::GetRVarDef(), mode );
}

//_____________________________________________________________________________
THaAnalysisObject::EStatus THaReacPointFoil::Init( const TDatime& run_time )
{
  // Initialize the module.
  // Locate the spectrometer apparatus named in fSpectroName and save
  // pointer to it.

  // Standard initialization. Calls this object's DefineVariables().
  if( THaPhysicsModule::Init( run_time ) != kOK )
    return fStatus;

  fSpectro = static_cast<THaSpectrometer*>
    ( FindModule( fSpectroName.Data(), "THaSpectrometer"));
  if( !fSpectro )
    return fStatus;

  if( fBeamName.Length() > 0 )
    fBeam = static_cast<THaBeam*>( FindModule( fBeamName.Data(), "THaBeam") );
  
  return fStatus;
}

//_____________________________________________________________________________
Int_t THaReacPointFoil::Process( const THaEvData& )
{
  // Calculate the vertex coordinates.

  if( !IsOK() ) return -1;

  Int_t ntracks = fSpectro->GetNTracks();
  if( ntracks == 0 ) return 0;

  TClonesArray* tracks = fSpectro->GetTracks();
  if( !tracks ) return -2;

  TVector3 beam_org, beam_ray( 0.0, 0.0, 1.0 );
  if( fBeam ) {
    beam_org = fBeam->GetPosition();
    beam_ray = fBeam->GetDirection();
  }
  static const TVector3 yax( 0.0, 1.0, 0.0 );
  static const TVector3 xax( 1.0, 0.0, 0.0 );
  TVector3 org, v; 
  Double_t t;

  for( Int_t i = 0; i<ntracks; i++ ) {
    THaTrack* theTrack = static_cast<THaTrack*>( tracks->At(i) );
    // Ignore junk tracks
    if( !theTrack || !theTrack->HasTarget() ) 
      continue;  
    org.SetX( 0. );
    org.SetZ( 0. ); 
    org.SetY( 0. );
    if( !IntersectPlaneWithRay( xax, yax, org, 
				beam_org, beam_ray, t, v ))
      continue; // Oops, track and beam parallel?
    theTrack->SetVertex(v);

    // FIXME: preliminary
    if( theTrack == fSpectro->GetGoldenTrack() ) {
      fVertex = theTrack->GetVertex();
      fVertexOK = kTRUE;
    }
    // FIXME: calculate vertex coordinate errors here (need beam errors)


  }
  return 0;
}
  
