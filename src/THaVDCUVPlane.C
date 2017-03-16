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

  // Need at least one cluster per plane in order to match
  if ( nu < 1 || nv < 1) {
    return 0;
  }
  
  // There are two cases:
  // 1) One cluster per plane
  // 2) Multiple clusters per plane

  // One cluster per plane case
  if ( nu == 1 && nv == 1) {
    THaVDCUVTrack* uvTrack = new ( (*fUVTracks)[0] ) THaVDCUVTrack();
    uvTrack->SetUVPlane(this);

    // Set the U & V clusters
    uvTrack->SetUCluster( fU->GetCluster(0) );
    uvTrack->SetVCluster( fV->GetCluster(0) );

  } else if( nu == 1 || nv == 1 ) {
    // At least one plane has multiple clusters
    // The simple algorithm used here is not able to handle multiple clusters
    // in both planes. We don't create any UV tracks for such events.

    // Select the higher-occupancy plane: p1 has more clusters than p2
    THaVDCPlane *p1, *p2;
    if ( nu > nv ) { 
      p1 = fU;
      p2 = fV;
    } else {  
      p1 = fV;
      p2 = fU;
    }       
    assert( p1->GetNClusters() > 1 );
    assert( p2->GetNClusters() == 1 );
    // Create cluster pair candidates
    for (int i = 0; i < p1->GetNClusters(); i++) {
      THaVDCUVTrack* uvTrack = new ( (*fUVTracks)[i] ) THaVDCUVTrack();
      uvTrack->SetUVPlane(this);

      // Set the UV tracks U & V clusters
      THaVDCCluster* p1Clust = p1->GetCluster(i);
      THaVDCCluster* p2Clust = p2->GetCluster(0);
      if (p1 == fU) {
	uvTrack->SetUCluster( p1Clust );
	uvTrack->SetVCluster( p2Clust );
      } else {
	uvTrack->SetUCluster( p2Clust );
	uvTrack->SetVCluster( p1Clust );
      }
      assert( uvTrack->GetUCluster() != 0 && uvTrack->GetVCluster() != 0 );
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
