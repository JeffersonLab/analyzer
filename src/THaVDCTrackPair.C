//*-- Author :    Ole Hansen   13-Mar-02

//////////////////////////////////////////////////////////////////////////
//
// THaVDCTrackPair
//
// This is a utility class for the VDC tracking classes.
// It allows unique identification of tracks.
//
//////////////////////////////////////////////////////////////////////////

#include "THaVDCTrackPair.h"
#include "THaVDCUVTrack.h"
#include "TClass.h"

#include <iostream>
#include <cassert>

using namespace std;

//_____________________________________________________________________________
THaVDCTrackPair& THaVDCTrackPair::operator=( const THaVDCTrackPair& rhs )
{
  // Assignment operator.

  TObject::operator=(rhs);
  if ( this != &rhs ) {
    fLowerTrack = rhs.fLowerTrack;
    fUpperTrack = rhs.fUpperTrack;
    fError      = rhs.fError;
    fStatus     = rhs.fStatus;
  }
  return *this;
}

//_____________________________________________________________________________
void THaVDCTrackPair::Analyze( Double_t spacing )
{
  // Compute goodness of match paremeter for the two UV tracks.
  // Essentially, this is a measure of how closely the two tracks
  // point at each other. 'spacing' is the separation of the 
  // upper and lower UV planes (in m).
  //
  // This method is critical for the quality of the VDC multi-track
  // reconstruction.

  // For now, a simple approach: Calculate projected positions of tracks in
  // opposite planes, measure how far they differ from partner track
  // intercept, and store the sum of the distances

  // Project the lower track into the upper plane
  fError  = GetProjectedDistance( fLowerTrack, fUpperTrack, spacing );
  fError += GetProjectedDistance( fUpperTrack, fLowerTrack, -spacing );

  return;
}

//_____________________________________________________________________________
void THaVDCTrackPair::Associate( THaTrack* track )
{
  // Mark upper and lower UV tracks as well as their clusters 
  // as belonging to this track

  THaVDCUVTrack* uvtrk[2] = { fLowerTrack, fUpperTrack };

  for( int i=0; i<2; i++ ) {
    uvtrk[i]->SetTrack( track );
    uvtrk[i]->GetUCluster()->SetTrack( track );
    uvtrk[i]->GetVCluster()->SetTrack( track );
  }
}

//_____________________________________________________________________________
chi2_t THaVDCTrackPair::CalcChi2() const
{
  // Calculate chi2 and number of data points for the associated track
  // and clusters

  THaVDCCluster* clust[4] = 
    { fLowerTrack->GetUCluster(), fLowerTrack->GetVCluster(),
      fUpperTrack->GetUCluster(), fUpperTrack->GetVCluster() };

  chi2_t res;
  for( int i=0; i<2; i++ ) {
    chi2_t r = clust[i]->CalcDist();
    res.first  += r.first;
    res.second += r.second;
  }
  return res;
}

//_____________________________________________________________________________
Int_t THaVDCTrackPair::Compare( const TObject* obj ) const
{
  // Compare this object to another THaVDCTrackPair.
  // Used for sorting tracks.

  if( !obj || IsA() != obj->IsA() )
    return -1;

  const THaVDCTrackPair* rhs = static_cast<const THaVDCTrackPair*>( obj );

  if( fError < rhs->fError )
    return -1;
  if( fError > rhs->fError )
    return 1;

  return 0;
}

//_____________________________________________________________________________
Double_t THaVDCTrackPair::GetProjectedDistance( THaVDCUVTrack* here,
						THaVDCUVTrack* there,
						Double_t spacing )
{
  // Project 'here' to plane of 'there' and return square of distance between
  // projected position and intercept of 'there'

  Double_t px = here->GetX() + spacing * here->GetTheta();  // Projected x
  Double_t py = here->GetY() + spacing * here->GetPhi();    // Projected y
  
  Double_t x = there->GetX();
  Double_t y = there->GetY();

  return (px-x)*(px-x) + (py-y)*(py-y);
}

//_____________________________________________________________________________
THaTrack* THaVDCTrackPair::GetTrack() const
{
  // Return track associated with this pair

  assert( fLowerTrack->GetTrack() == fUpperTrack->GetTrack() );
  return fLowerTrack->GetTrack();
}

//_____________________________________________________________________________
void THaVDCTrackPair::Print( Option_t* ) const
{
  // Print this object

  cout << fError << endl;
}

//_____________________________________________________________________________
void THaVDCTrackPair::Use()
{
  // Mark this track pair as used (i.e. associated with a track)
  // This does the following:
  //
  // (a) upper and lower tracks are marked as each other's partners
  // (b) the global slope resulting from the track pair is calculated
  //     and stored in the clusters associated with the UVtracks
  // (c) This pair is marked as used

  // Make the tracks of this pair each other's partners. This prevents
  // tracks from being associated with more than one valid pair.
  fLowerTrack->SetPartner( fUpperTrack );
  fUpperTrack->SetPartner( fLowerTrack );

  // Compute global track values and get TRANSPORT coordinates for tracks. Re-
  // place local cluster slopes with global ones, which have higher precision.
  Double_t mu =
    (fUpperTrack->GetU() - fLowerTrack->GetU()) /
    (fUpperTrack->GetZU() - fLowerTrack->GetZU());
  Double_t mv =
    (fUpperTrack->GetV() - fLowerTrack->GetV()) /
    (fUpperTrack->GetZV() - fLowerTrack->GetZV());

  fLowerTrack->SetSlopes( mu, mv );
  fUpperTrack->SetSlopes( mu, mv );

  SetStatus(1);
}

//_____________________________________________________________________________
void THaVDCTrackPair::Release()
{
  // Mark this track pair as unused

}

///////////////////////////////////////////////////////////////////////////////
ClassImp(THaVDCTrackPair)
