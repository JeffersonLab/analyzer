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

ClassImp(THaVDCUVPlane)

using namespace std;

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
THaVDCUVPlane::PointCoords_t
THaVDCUVPlane::CalcDetCoords( const THaVDCCluster* ucl,
			      const THaVDCCluster* vcl ) const
{
  // Convert U,V coordinates of the given uv cluster pair to the detector
  // coordinate system of this chamber. Takes u as the reference plane.

  Double_t u  = ucl->GetIntercept();  // Intercept in U plane
  Double_t v0 = vcl->GetIntercept();  // Intercept in V plane
  Double_t mu = ucl->GetSlope();      // Slope of U cluster
  Double_t mv = vcl->GetSlope();      // Slope of V cluster

  // Project v0 into the u plane
  Double_t v = v0 - mv * GetSpacing();

  PointCoords_t c;
  c.x     = (u*fSin_v - v*fSin_u) * fInv_sin_vu;
  c.y     = (v*fCos_u - u*fCos_v) * fInv_sin_vu;
  c.theta = (mu*fSin_v - mv*fSin_u) * fInv_sin_vu;
  c.phi   = (mv*fCos_u - mu*fCos_v) * fInv_sin_vu;

  return c;
}

//_____________________________________________________________________________
Int_t THaVDCUVPlane::MatchUVClusters()
{
  // Match clusters in the U plane with cluster in the V plane
  // Creates THaVDCUVTrack object for each pair

  Int_t nu = fU->GetNClusters();
  Int_t nv = fV->GetNClusters();

  // No clusters in one or both planes.
  // It is not realistic to recover from this situation since no accurate
  // UV position can be determined for this chamber. Accordingly, this
  // chamber should report zero UV matches.
  if( nu == 0 || nv == 0 )
    return 0;

  Bool_t multi_cluster = false, old_algorithm = false;
  THaDetector* vdc = GetMainDetector();
  if( vdc ) {
    multi_cluster = vdc->TestBit(THaVDC::kAllowMultiClusters);
    old_algorithm = vdc->TestBit(THaVDC::kBadOldAlgorithm);
  }

  // Multiple clusters in both planes are not supported, unless
  // (a) we turn on experimental multi-cluster analysis or
  // (b) we insist on using the old algorithm, which is known to be incorrect
  if( nu > 1 && nv > 1 && !(multi_cluster || old_algorithm) )
    return 0;

  Int_t nuv = 0;

  if( !old_algorithm ) {
    // Consider all possible uv cluster combinations
    for( Int_t iu = 0; iu < nu; ++iu ) {
      THaVDCCluster* uClust = fU->GetCluster(iu);

      for( Int_t iv = 0; iv < nv; ++iv ) {
	THaVDCCluster* vClust = fV->GetCluster(iv);

	// Test position to be within drift chambers
	const PointCoords_t c = CalcDetCoords(uClust,vClust);
	if( !fU->IsInActiveArea( c.x, c.y ) ) {
	  continue;
	}

	// This cluster pair passes preliminary tests, so we save it, regardless
	// of possible ambiguities, which will be sorted out later
#ifndef NDEBUG
	THaVDCUVTrack* uvTrack =
#endif
	  new( (*fUVTracks)[nuv++] ) THaVDCUVTrack( uClust, vClust, this );
	assert( uvTrack->GetUCluster() != 0 && uvTrack->GetVCluster() != 0 );
      }
    }
  }
  else {
    // BAD old algorithm, for comparison studies and similar

    // Select the higher-occupancy plane: p1 has more clusters than p2
    THaVDCPlane *p1, *p2;
    if ( nu > nv ) {
      p1 = fU;
      p2 = fV;
    } else {
      p1 = fV;
      p2 = fU;
    }

    // Match clusters by time (Note: this is known to be wrong)
    for (int i = 0; i < p1->GetNClusters(); i++) {
      THaVDCCluster* p1Clust = p1->GetCluster(i);
      THaVDCHit* p1Pivot = p1Clust->GetPivot();
      assert( p1Pivot );

      THaVDCCluster* minClust = 0;
      Double_t minTimeDif = 1e307;  //Arbitrary large value
      for (int j = 0; j < p2->GetNClusters(); j++) {
	THaVDCCluster* p2Clust = p2->GetCluster(j);
	THaVDCHit* p2Pivot = p2Clust->GetPivot();
	assert( p2Pivot );

	Double_t timeDif = TMath::Abs(p1Pivot->GetTime() - p2Pivot->GetTime());
	if (timeDif < minTimeDif) {
	  minTimeDif = timeDif;
	  minClust = p2Clust;
	}
      }
      assert( nuv == i );
      THaVDCUVTrack* uvTrack = new ( (*fUVTracks)[nuv++] ) THaVDCUVTrack();

      if (p1 == fU) {
	uvTrack->SetUCluster( p1Clust );
	uvTrack->SetVCluster( minClust );
      } else {
	uvTrack->SetUCluster( minClust );
	uvTrack->SetVCluster( p1Clust );
      }
    } // p1 clusters
  } // BAD old algorithm

  assert( nuv == GetNUVTracks() );
  // return the number of UV tracks found
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
  // Convert raw data into good stuff

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
void THaVDCUVPlane::SetNMaxGap( Int_t val )
{
  fU->SetNMaxGap(val);
  fV->SetNMaxGap(val);
}

//_____________________________________________________________________________
void THaVDCUVPlane::SetMinTime( Int_t val )
{
  fU->SetMinTime(val);
  fV->SetMinTime(val);
}

//_____________________________________________________________________________
void THaVDCUVPlane::SetMaxTime( Int_t val )
{
  fU->SetMaxTime(val);
  fV->SetMaxTime(val);
}

//_____________________________________________________________________________
void THaVDCUVPlane::SetTDCRes( Double_t val )
{
  fU->SetTDCRes(val);
  fV->SetTDCRes(val);
}


///////////////////////////////////////////////////////////////////////////////
