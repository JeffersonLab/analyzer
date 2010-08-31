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

  fVDC = dynamic_cast<THaVDC*>( GetMainDetector() );
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

  // Precompute and store values for efficiency
  fSin_u   = fU->GetSinAngle();
  fCos_u   = fU->GetCosAngle();
  fSin_v   = fV->GetSinAngle();
  fCos_v   = fV->GetCosAngle();
  fInv_sin_vu = 1.0/TMath::Sin( fV->GetWAngle() - fU->GetWAngle() );

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
  THaVDCUVTrack* testTrack = 0;

  Int_t ntrk = 0;

  for( Int_t iu = 0; iu < nu; ++iu ) {
    THaVDCCluster* uClust = fU->GetCluster(iu);
    if( TMath::Abs(uClust->GetT0()) > max_u_t0 )
      continue;

    for( Int_t iv = 0; iv < nv; ++iv ) {
      THaVDCCluster* vClust = fV->GetCluster(iv);
      if( TMath::Abs(vClust->GetT0()) > max_v_t0 )
	continue;

      testTrack = new THaVDCUVTrack( uClust, vClust, this );
      testTrack->CalcDetCoords();
      Double_t xcoord = testTrack->GetX();
      Double_t ycoord = testTrack->GetY();
      delete testTrack;  testTrack = NULL;

      // Test position to be within drift chambers
      if( TMath::Abs(ycoord) > fU->GetYSize() ||
	  TMath::Abs(xcoord) > fU->GetXSize()){
	      continue;
      }

      delete testTrack; testTrack = NULL;

      // FIXME:  We should just mark this one "region" bad. 
      // Pairs that are sufficiently far away from this should
      // still be OK to use (if they're not ambiguious themselves)
      if( uClust->IsPaired() || vClust->IsPaired() ){
	      // Found a pair that was already some good match
	      // We say there is an ambiguity and we stop
	      MarkBad();
	      continue;
      }

      uClust->SetPaired(kTRUE);
      vClust->SetPaired(kTRUE);

	// Found two clusters with "small" t0s
	// So we pair them and hope for the best.
	
	uvTrack = new ( (*fUVTracks)[ntrk++] ) 
		THaVDCUVTrack( uClust, vClust, this );
	uvTrack->CalcDetCoords();
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
  
  // Find clusters and estimate their position/slope
  FindClusters();

  // Fit "local" tracks through the hit coordinates of each cluster
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

  return 0;
}

//_____________________________________________________________________________
Int_t THaVDCUVPlane::FineTrack()
{
  // High precision computation of tracks

  //TODO:
  //- Redo time-to-distance conversion based on "global" track slope
  //- Refit clusters of the track (if any), using new drift distances
  //- Recalculate cluster UV coordinates, so new "gobal" slopes can be
  //  computed

  return 0;
}


///////////////////////////////////////////////////////////////////////////////
