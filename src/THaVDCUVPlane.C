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

ClassImp(THaVDCUVPlane)


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

  // Quick-and-dirty algorithm pushed by Bogdan Wojtsekhowski.
  // Probably not the way to go for the real thing, but it will give 
  // relatively clean (if inefficient) results:
  //
  // Match best in-time clusters (t_0 close to zero).
  // Discard any out-of-time clusters (|t_0| > N sigma, N = 3, configurable).
  // Bail if any ambiguity, marking the event as not analyzable

  Double_t max_u_t0 = fU->GetT0Resolution();
  Double_t max_v_t0 = fV->GetT0Resolution();
  THaVDCUVTrack* uvTrack = 0;

  for( Int_t iu = 0; iu < nu; ++iu ) {
    THaVDCCluster* uClust = fU->GetCluster(iu);
    if( uClust->GetT0() > max_u_t0 )
      continue;

    for( Int_t iv = 0; iv < nv; ++iv ) {
      THaVDCCluster* vClust = fV->GetCluster(iv);
      if( vClust->GetT0() > max_v_t0 )
	continue;

      if( uvTrack == 0 ) {
	// Found two clusters with "small" t0s, and none were found before.
	// So we pair them and hope for the best.
	uvTrack = new ( (*fUVTracks)[0] )
	  THaVDCUVTrack( fU->GetCluster(0), fV->GetCluster(0), this );
	uvTrack->CalcDetCoords();
      } else {
	// Too bad, another combination with small t0s found. 
	// This situation is ambiguous (within the limits of this algorithm),
	// and so we mark the event as not analyzable.
	MarkBad();
      }
    }
  }

  // return the number of UV tracks found
  return GetNUVTracks();
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
  UnmarkBad();
}

//_____________________________________________________________________________
Int_t THaVDCUVPlane::Decode( const THaEvData& evData )
{
  // Convert raw data into good stuff

  fU->Decode(evData);
  fV->Decode(evData);  
  return 0;
}

//_____________________________________________________________________________
void THaVDCUVPlane::FindClusters()
{
  // Find clusters in U & V planes

  fU->FindClusters();
  fV->FindClusters();
}

//_____________________________________________________________________________
void THaVDCUVPlane::FitTracks()
{
  // Fit data to recalculate cluster position

  fU->FitTracks();
  fV->FitTracks();
}

//_____________________________________________________________________________
Int_t THaVDCUVPlane::CoarseTrack()
{
  // Coarse computation of tracks
  
  // Find clusters and estimate their positions
  FindClusters();

  // Fit tracks through the hit coordinates of each cluster
  FitTracks();
  
  // FIXME: The fit may fail, so we might want to call a function here
  // that deletes UV tracks whose clusters have bad fits, or with
  // clusters that are too small (e.g. 1 hit), etc. 
  // Right now, we keep them, preferring efficiency over precision.

  // Pair U and V clusters
#ifndef NDEBUG
  UInt_t n_pairs =
#endif
    MatchUVClusters();

  // The current logic in MatchUVClusters should always produces at most
  // one cluster pair or mark the event as ambiguous
  assert( n_pairs <= 1 || IsBad() );

  return 0;
}

//_____________________________________________________________________________
Int_t THaVDCUVPlane::FineTrack()
{
  // High precision computation of tracks

  //TODO:
  //- Redo time-to-distance conversion based on "global" track slope
  //- Refit cluster of the track (if any), using new drift distances
  //- Recalculate cluster UV coordinates, so new "gobal" slopes can be
  //  computed

  return 0;
}


///////////////////////////////////////////////////////////////////////////////
