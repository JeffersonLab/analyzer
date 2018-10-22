///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// OldVDCUVPlane                                                             //
//                                                                           //
// Class for a UV-Plane in the VDC.  Each UV-Plane consists of two planes    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "OldVDC.h"
#include "OldVDCUVPlane.h"
#include "OldVDCPlane.h"
#include "OldVDCUVTrack.h"
#include "OldVDCCluster.h"
#include "OldVDCHit.h"
#include "TMath.h"

#include <cstring>
#include <cstdio>

ClassImp(OldVDCUVPlane)


//_____________________________________________________________________________
OldVDCUVPlane::OldVDCUVPlane( const char* name, const char* description,
			      THaDetectorBase* parent )
  : THaSubDetector(name,description,parent),
    fSpacing(0), fSin_u(0), fCos_u(1), fSin_v(1), fCos_v(0), fInv_sin_vu(0)
{
  // Constructor

  // Create the U and V planes
  fU = new OldVDCPlane( "u", "U plane", this );
  fV = new OldVDCPlane( "v", "V plane", this );

  // Create the UV tracks array
  fUVTracks = new TClonesArray("OldVDCUVTrack", 10); // 10 is arbitrary
}


//_____________________________________________________________________________
OldVDCUVPlane::~OldVDCUVPlane()
{
  // Destructor. Delete planes and track array.

  delete fU;
  delete fV;
  delete fUVTracks;
}

//_____________________________________________________________________________
THaDetectorBase::EStatus OldVDCUVPlane::Init( const TDatime& date )
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
void OldVDCUVPlane::SetDebug( Int_t level )
{
  // Set debug level of this chamber and its plane subdetectors

  THaSubDetector::SetDebug(level);
  fU->SetDebug(level);
  fV->SetDebug(level);
}

//_____________________________________________________________________________
Int_t OldVDCUVPlane::MatchUVClusters()
{
  // Match clusters in the U plane with cluster in the V plane
  // Creates OldVDCUVTrack object for each pair

  Int_t nu = fU->GetNClusters();
  Int_t nv = fV->GetNClusters();

  // Need at least one cluster per plane in order to match
  if ( nu < 1 || nv < 1) {
    return 0;
  }
  
  OldVDCUVTrack* uvTrack = NULL;
  
  // There are two cases:
  // 1) One cluster per plane
  // 2) Multiple clusters per plane

  // One cluster per plane case
  if ( nu == 1 && nv == 1) {
    uvTrack = new ( (*fUVTracks)[0] ) OldVDCUVTrack();
    uvTrack->SetUVPlane(this);

    // Set the U & V clusters
    uvTrack->SetUCluster( fU->GetCluster(0) );
    uvTrack->SetVCluster( fV->GetCluster(0) );

  } else { 
    // At least one plane has multiple clusters
    // FIXME: This is a crucial part of the algorithm. Really the right approach?
    // Cope with different numbers of clusters per plane by ensuring that
    // p1 has more clusters than p2
    OldVDCPlane *p1, *p2; // Plane 1 and plane 2
    if ( nu > nv ) { 
      // More U clusters than V clusters
      p1 = fU;
      p2 = fV;
    } else {  
      p1 = fV;
      p2 = fU;
    }       
    // Match clusters by time
    OldVDCCluster *p1Clust, *p2Clust; //Cluster from p1, p2 respectively
    OldVDCCluster *minClust;     //Cluster in p2 with time closest to p1Clust
    OldVDCHit     *p1Pivot, *p2Pivot;
    Double_t minTimeDif;      // Time difference between p1Clust and minClust
    for (int i = 0; i < p1->GetNClusters(); i++) {
      
      p1Clust = p1->GetCluster(i);
      p1Pivot = p1Clust->GetPivot();
      if( !p1Pivot ) {
	Warning( "OldVDCUVPlane", "Cluster without pivot in p1, %d!", i );
	continue;
      }
      minClust = NULL;
      minTimeDif = 1e307;  //Arbitrary large value
      for (int j = 0; j < p2->GetNClusters(); j++) {
	p2Clust = p2->GetCluster(j);
	p2Pivot = p2Clust->GetPivot();
	if( !p2Pivot ) {
	  Warning( "OldVDCUVPlane", 
		   "Cluster without pivot in p2, %d, %d!", i, j );
	  continue;
	}
	Double_t timeDif = TMath::Abs(p1Pivot->GetTime() - p2Pivot->GetTime());
	
	if (timeDif < minTimeDif) {
	  minTimeDif = timeDif;
	  minClust = p2Clust;
	}
	  
      }
      uvTrack = new ( (*fUVTracks)[i] ) OldVDCUVTrack();
      uvTrack->SetUVPlane(this);

      // Set the UV tracks U & V clusters
      if (p1 == fU) { // If p1 is the U plane	
	uvTrack->SetUCluster( p1Clust );
	uvTrack->SetVCluster( minClust );
      } else {  // p2 is the U plane
	uvTrack->SetUCluster( minClust );
	uvTrack->SetVCluster( p1Clust );
      }
    }   
  }
  // return the number of UV tracks found
  return GetNUVTracks();
}

//_____________________________________________________________________________
Int_t OldVDCUVPlane::CalcUVTrackCoords()
{
  // Compute track info (x, y, theta, phi) for the tracks based on
  // information contained in the clusters of the tracks and on 
  // geometry information from this UV plane.
  //
  // Uses TRANSPORT coordinates. 
  
  Int_t nUVTracks = GetNUVTracks();
  for (int i = 0; i < nUVTracks; i++) {
    OldVDCUVTrack* track  = GetUVTrack(i);
    if( track )
      track->CalcDetCoords();
  }
  return nUVTracks;
}

//_____________________________________________________________________________
void OldVDCUVPlane::Clear( Option_t* opt )
{ 
  // Clear event-by-event data
  THaSubDetector::Clear(opt);
  fU->Clear(opt);
  fV->Clear(opt);
  fUVTracks->Clear();
}

//_____________________________________________________________________________
Int_t OldVDCUVPlane::Decode( const THaEvData& evData )
{
  // Convert raw data into good stuff

  fU->Decode(evData);
  fV->Decode(evData);  
  return 0;
}

//_____________________________________________________________________________
Int_t OldVDCUVPlane::CoarseTrack( )
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
Int_t OldVDCUVPlane::FineTrack( )
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
void OldVDCUVPlane::SetNMaxGap( Int_t val )
{
  if( val < 0 || val > 2 ) {
    Error( Here("SetNMaxGap"),
	   "Invalid max_gap = %d, must be betwwen 0 and 2.", val );
    return;
  }
  fU->SetNMaxGap(val);
  fV->SetNMaxGap(val);
}

//_____________________________________________________________________________
void OldVDCUVPlane::SetMinTime( Int_t val )
{
  if( val < 0 || val > 4095 ) {
    Error( Here("SetMinTime"),
	   "Invalid min_time = %d, must be betwwen 0 and 4095.", val );
    return;
  }
  fU->SetMinTime(val);
  fV->SetMinTime(val);
}

//_____________________________________________________________________________
void OldVDCUVPlane::SetMaxTime( Int_t val )
{
  if( val < 1 || val > 4096 ) {
    Error( Here("SetMaxTime"),
	   "Invalid max_time = %d. Must be between 1 and 4096.", val );
    return;
  }
  fU->SetMaxTime(val);
  fV->SetMaxTime(val);
}

//_____________________________________________________________________________
void OldVDCUVPlane::SetTDCRes( Double_t val )
{
  if( val < 0 || val > 1e-6 ) {
    Error( Here("SetTDCRes"),
	   "Nonsense TDC resolution = %8.1le s/channel.", val );
    return;
  }
  fU->SetTDCRes(val);
  fV->SetTDCRes(val);
}


///////////////////////////////////////////////////////////////////////////////
