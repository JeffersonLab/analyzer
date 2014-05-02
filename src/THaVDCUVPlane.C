///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCUVPlane                                                             //
//                                                                           //
// Class for a UV-Plane in the VDC.  Each UV-Plane consists of two planes    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaVDC.h"
#include "THaVDCUVPlane.h"
#include "THaVDCPlane.h"
#include "THaVDCUVTrack.h"
#include "THaVDCCluster.h"
#include "THaVDCHit.h"
#include "TMath.h"

#include <cstring>
#include <cstdio>
#include <cassert>

//_____________________________________________________________________________
THaVDCUVPlane::THaVDCUVPlane( const char* name, const char* description,
			      THaDetectorBase* parent )
  : THaSubDetector(name,description,parent)
{
  // Constructor

  // Create the U and V planes
  fU = new THaVDCPlane( "u", "U plane", this );
  fV = new THaVDCPlane( "v", "V plane", this );

  // Create the UV tracks array
  fUVTracks = new TClonesArray("THaVDCUVTrack", 10); // 10 is arbitrary
}


//_____________________________________________________________________________
THaVDCUVPlane::~THaVDCUVPlane()
{
  // Destructor. Delete planes and track array.

  delete fU;
  delete fV;
  delete fUVTracks;
}

//_____________________________________________________________________________
THaDetectorBase::EStatus THaVDCUVPlane::Init( const TDatime& date )
{
  // Initialize the UV plane object.
  // Calls its own Init(), then initializes subdetectors, then calculates
  // some local geometry data.

  if( IsZombie() || !fV || !fU )
    return fStatus = kInitError;

  if( GetParent() )
    fOrigin = GetParent()->GetOrigin();

  EStatus status;
  if( (status = THaSubDetector::Init( date )) ||
      (status = fU->Init( date )) ||
      (status = fV->Init( date )))
    return fStatus = status;

  fSpacing = fV->GetZ() - fU->GetZ();  // Space between U & V planes

  TVector3 z( 0.0, 0.0, fU->GetZ() );
  fOrigin += z;

  Double_t uwAngle  = fU->GetWAngle();      // Get U plane Wire angle
  Double_t vwAngle  = fV->GetWAngle();      // Get V plane Wire angle

  // Precompute and store values for efficiency
  fSin_u   = TMath::Sin( uwAngle );
  fCos_u   = TMath::Cos( uwAngle );
  fSin_v   = TMath::Sin( vwAngle );
  fCos_v   = TMath::Cos( vwAngle );
  fInv_sin_vu = 1.0/TMath::Sin( vwAngle-uwAngle );

  return fStatus = kOK;
}

//_____________________________________________________________________________
Int_t THaVDCUVPlane::MatchUVClusters()
{
  // Match clusters in the U plane with cluster in the V plane
  // Creates THaVDCUVTrack object for each pair

  Int_t nu = fU->GetNClusters();
  Int_t nv = fV->GetNClusters();
  Int_t nuv = 0;

  for( Int_t i = 0; i < nu; i++ ) {
    THaVDCCluster* uClust = fU->GetCluster(i);
    for( Int_t j = 0; j < nv; j++ ) {
      THaVDCCluster* vClust = fV->GetCluster(j);

      THaVDCUVTrack* uvTrack =
	new((*fUVTracks)[nuv++]) THaVDCUVTrack(uClust, vClust, this);
    }
  }
  assert( GetNUVTracks() == nuv );
  return nuv;
}

//_____________________________________________________________________________
Int_t THaVDCUVPlane::CalcUVTrackCoords()
{
  // Compute track info (x, y, theta, phi) for the tracks based on
  // information contained in the clusters of the tracks and on
  // geometry information from this UV plane.
  //
  // Uses TRANSPORT coordinates.

  Int_t nUVTracks = GetNUVTracks();
  for (int i = 0; i < nUVTracks; i++) {
    THaVDCUVTrack* track  = GetUVTrack(i);
    if( track )
      track->CalcDetCoords();
  }
  return nUVTracks;
}

//_____________________________________________________________________________
void THaVDCUVPlane::Clear( Option_t* opt )
{
  // Clear event-by-event data
  fU->Clear(opt);
  fV->Clear(opt);
  fUVTracks->Clear();
}

//_____________________________________________________________________________
Int_t THaVDCUVPlane::Decode( const THaEvData& evData )
{
  // Decode both wire planes

  fU->Decode(evData);
  fV->Decode(evData);
  return 0;
}

//_____________________________________________________________________________
Int_t THaVDCUVPlane::CoarseTrack( )
{
  // Coarse computation of tracks

  // Find clusters and estimate their positions
  FindClusters();

  // Pair U and V clusters
  MatchUVClusters();

  // Construct UV tracks
  CalcUVTrackCoords();

  return 0;
}

//_____________________________________________________________________________
Int_t THaVDCUVPlane::FineTrack( )
{
  // High precision computation of tracks

  // Refine cluster position
  FitTracks();

  // FIXME: The fit may fail, so we might want to call a function here
  // that deletes UV tracks whose clusters have bad fits, or with
  // clusters that are too small (e.g. 1 hit), etc.
  // Right now, we keep them, preferring efficiency over precision.

  // Reconstruct the UV tracks, based on the refined cluster positions
  CalcUVTrackCoords();

  return 0;
}

//_____________________________________________________________________________
ClassImp(THaVDCUVPlane)
