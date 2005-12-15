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
  // FIXME: 'this' pointer is stored twice in the planes
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

  if( GetDetector() )
    fOrigin = GetDetector()->GetOrigin();

  EStatus status;
  if( (status = THaSubDetector::Init( date )) ||
      (status = fU->Init( date )) ||
      (status = fV->Init( date )))
    return fStatus = status;

  fSpacing = fV->GetZ() - fU->GetZ();  // Space between U & V planes

  TVector3 z( 0.0, 0.0, fU->GetZ() );
  fOrigin += z; 

  //Double_t vdcAngle = fVDC->GetVDCAngle();  // Get angle of the VDC
  Double_t uwAngle  = fU->GetWAngle();      // Get U plane Wire angle
  Double_t vwAngle  = fV->GetWAngle();      // Get V plane Wire angle
  fVUWireAngle = vwAngle - uwAngle;    // Difference between vwAngle and
                                       // uwAngle
  // Precompute and store values for efficiency
  fSin_u   = TMath::Sin( uwAngle );
  fCos_u   = TMath::Cos( uwAngle );
  fSin_v   = TMath::Sin( vwAngle );
  fCos_v   = TMath::Cos( vwAngle );
  fSin_vu  = TMath::Sin( fVUWireAngle );

  return fStatus = kOK;
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
  
  THaVDCUVTrack* uvTrack = NULL;
  
  // There are two cases:
  // 1) One cluster per plane
  // 2) Multiple clusters per plane

  // One cluster per plane case
  if ( nu == 1 && nv == 1) {
    uvTrack = new ( (*fUVTracks)[0] ) THaVDCUVTrack();
    uvTrack->SetUVPlane(this);

    // Set the U & V clusters
    uvTrack->SetUCluster( fU->GetCluster(0) );
    uvTrack->SetVCluster( fV->GetCluster(0) );

  } else { 
    // At least one plane has multiple clusters
    // FIXME: This is a crucial part of the algorithm. Really the right approach?
    // Cope with different numbers of clusters per plane by ensuring that
    // p1 has more clusters than p2
    THaVDCPlane *p1, *p2; // Plane 1 and plane 2
    if ( nu > nv ) { 
      // More U clusters than V clusters
      p1 = fU;
      p2 = fV;
    } else {  
      p1 = fV;
      p2 = fU;
    }       
    // Match clusters by time
    THaVDCCluster *p1Clust, *p2Clust; //Cluster from p1, p2 respectively
    THaVDCCluster *minClust;     //Cluster in p2 with time closest to p1Clust
    THaVDCHit     *p1Pivot, *p2Pivot;
    Double_t minTimeDif;      // Time difference between p1Clust and minClust
    for (int i = 0; i < p1->GetNClusters(); i++) {
      
      p1Clust = p1->GetCluster(i);
      p1Pivot = p1Clust->GetPivot();
      if( !p1Pivot ) {
	Warning( "THaVDCUVPlane", "Cluster without pivot in p1, %d!", i );
	continue;
      }
      minClust = NULL;
      minTimeDif = 1e307;  //Arbitrary large value
      for (int j = 0; j < p2->GetNClusters(); j++) {
	p2Clust = p2->GetCluster(j);
	p2Pivot = p2Clust->GetPivot();
	if( !p2Pivot ) {
	  Warning( "THaVDCUVPlane", 
		   "Cluster without pivot in p2, %d, %d!", i, j );
	  continue;
	}
	Double_t timeDif = TMath::Abs(p1Pivot->GetTime() - p2Pivot->GetTime());
	
	if (timeDif < minTimeDif) {
	  minTimeDif = timeDif;
	  minClust = p2Clust;
	}
	  
      }
      uvTrack = new ( (*fUVTracks)[i] ) THaVDCUVTrack();
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
Int_t THaVDCUVPlane::Decode( const THaEvData& evData )
{
  // Convert raw data into good stuff

  Clear();

  // Decode each plane
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


///////////////////////////////////////////////////////////////////////////////
