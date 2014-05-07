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
UVPlaneCoords_t THaVDCUVPlane::CalcDetCoords( const THaVDCCluster* ucl,
					      const THaVDCCluster* vcl ) const
{
  // Convert U,V coordinates of the given uv cluster pair to the detector
  // coordinate system of this plane. Assumes that u is the reference plane.

  Double_t u  = ucl->GetIntercept();  // Intercept for U plane
  Double_t v0 = vcl->GetIntercept();  // Intercept for V plane
  Double_t mu = ucl->GetSlope();      // Slope of U cluster
  Double_t mv = vcl->GetSlope();      // Slope of V cluster

  // Project v0 into the u plane
  Double_t v = v0 - mv * GetSpacing();

  UVPlaneCoords_t c;
  c.x     = (u*fSin_v - v*fSin_u) * fInv_sin_vu;
  c.y     = (v*fCos_u - u*fCos_v) * fInv_sin_vu;
  c.theta = (mu*fSin_v - mv*fSin_u) * fInv_sin_vu;
  c.phi   = (mv*fCos_u - mu*fCos_v) * fInv_sin_vu;

  return c;
}

//_____________________________________________________________________________
Int_t THaVDCUVPlane::MatchUVClusters()
{
  // Match clusters in the U plane with cluster in the V plane by t0 and
  // geometry. Fills fUVTracks with good pairs.

  Int_t nu = fU->GetNClusters();
  Int_t nv = fV->GetNClusters();

  // Match best in-time clusters (t_0 close to zero).
  // Discard any out-of-time clusters (|t_0| > N sigma, N = 3, configurable).

  Double_t max_u_t0 = fU->GetT0Resolution();
  Double_t max_v_t0 = fV->GetT0Resolution();

  Int_t nuv = 0;

  // Consider all possible uv cluster combinations
  for( Int_t iu = 0; iu < nu; ++iu ) {
    THaVDCCluster* uClust = fU->GetCluster(iu);
    if( TMath::Abs(uClust->GetT0()) > max_u_t0 )
      continue;

    for( Int_t iv = 0; iv < nv; ++iv ) {
      THaVDCCluster* vClust = fV->GetCluster(iv);
      if( TMath::Abs(vClust->GetT0()) > max_v_t0 )
	continue;

      // Test position to be within drift chambers
      const UVPlaneCoords_t c = CalcDetCoords(uClust,vClust);
      if( !fU->IsInActiveArea( c.x, c.y ) ) {
	continue;
      }

      // This cluster pair passes preliminary tests, so we save it, regardless
      // of possible ambiguities, which will be sorted out later
      new( (*fUVTracks)[nuv++] ) THaVDCUVTrack( uClust, vClust, this );
    }
  }

  // return the number of UV pairs found
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
  MatchUVClusters();

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

//_____________________________________________________________________________
ClassImp(THaVDCUVPlane)
