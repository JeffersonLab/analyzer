//*-- Author :    Ole Hansen   13-Mar-03

//////////////////////////////////////////////////////////////////////////
//
// THaReactionPoint
//
// Calculate vertex for all the tracks from a single spectrometer.
//
// This module assumes a small-acceptance pointing-type spectrometer
// that generates tracks in TRANSPORT coordinates.
// It is assumed that the spectrometer only reconstucts the 
// position at the target transverse to the bend plane, i.e. y_tg, with 
// relatively high precision. Any reconstructed or estimated x_tg is ignored.
// The vertex is then defined as the intersection point of the 
// "track plane" and the beam ray. The "track plane" is the plane 
// spanned by the reconstructed momentum vector and the TRANSPORT x-axis
// (vertical) whose origin is the y_tg point.
// The beam ray is given by the measured beam positions and angles
// for this event. If no beam position measurement is available, beam 
// positions and angles are assumed to be zero.
// The spectrometer pointing offset is taken into account.
//
//////////////////////////////////////////////////////////////////////////

#include "THaReactionPoint.h"
#include "THaSpectrometer.h"
#include "THaTrack.h"
#include "THaBeam.h"
#include "TMath.h"

ClassImp(THaReactionPoint)

//_____________________________________________________________________________
THaReactionPoint::THaReactionPoint( const char* name, const char* description,
				    const char* spectro, const char* beam ) :
  THaPhysicsModule(name,description), fSpectroName(spectro), 
  fBeamName(beam), fSpectro(NULL), fBeam(NULL)
{
  // Normal constructor.

}

//_____________________________________________________________________________
THaReactionPoint::~THaReactionPoint()
{
  // Destructor

  DefineVariables( kDelete );
}

//_____________________________________________________________________________
void THaReactionPoint::Clear( Option_t* opt )
{
  // Clear all event-by-event variables.
  
  THaPhysicsModule::Clear(opt);
  VertexClear();
}

//_____________________________________________________________________________
Int_t THaReactionPoint::DefineVariables( EMode mode )
{
  // Define/delete analysis variables

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  return DefineVarsFromList( THaVertexModule::GetRVarDef(), mode );
}

//_____________________________________________________________________________
THaAnalysisObject::EStatus THaReactionPoint::Init( const TDatime& run_time )
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
Int_t THaReactionPoint::Process( const THaEvData& )
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
  TVector3 org, v; 
  Double_t t;

  for( Int_t i = 0; i<ntracks; i++ ) {
    THaTrack* theTrack = static_cast<THaTrack*>( tracks->At(i) );
    // Ignore junk tracks
    if( !theTrack || !theTrack->HasTarget() ) 
      continue;  
    org.SetXYZ( 0.0, theTrack->GetTY(), 0.0 );
    org *= fSpectro->GetToLabRot();
    org += fSpectro->GetPointingOffset();
    if( !IntersectPlaneWithRay( theTrack->GetPvect(), yax, org, 
				beam_org, beam_ray, t, v ))
      continue; // Oops, track and beam parallel?
    theTrack->SetVertex(v);

    // FIXME: preliminary
    if( theTrack == fSpectro->GetGoldenTrack() ) {
      fVertex = theTrack->GetVertex();
      fVertexOK = kTRUE;
    }
    // FIXME: calculate vertex coordinate errors here (need beam errors)


    // The following is more efficient but less general
    // and requires the tangents of the beam angles (TRANSPORT style)

//      Double_t x0 = (fSpectro->GetPointingOffset()).X() + 
//        (fSpectro->GetToLabRot()).XY() * theTrack->GetTY();
//      Double_t z0 = (fSpectro->GetPointingOffset()).Z() + 
//        (fSpectro->GetToLabRot()).ZY() * theTrack->GetTY();
//      Double_t dx  = beam_x + beam_th*z0 - x0;
//      Double_t dz  = dx /( theTrack->GetLabPx()/theTrack->GetLabPz() - beam_th );
//      Double_t z   = z0 + dz;
//      theTrack->SetVertex( beam_x+beam_th*z, beam_y+beam_ph*z, z );
  }
  return 0;
}
  
