//*-- Author :    Ole Hansen   13-Mar-02

//////////////////////////////////////////////////////////////////////////
//
// THaVDCPointPair
//
// A pair of THaVDCPoints, candidate for a reconstructed track.
//
//////////////////////////////////////////////////////////////////////////

#include "THaVDCPointPair.h"
#include "THaVDCPoint.h"
#include "TClass.h"

#include <iostream>
#include <cassert>

using namespace std;

//_____________________________________________________________________________
void THaVDCPointPair::Analyze( Double_t spacing )
{
  // Compute goodness of match parameter between upper and lower point.
  // Essentially, this is a measure of how closely the two local tracks
  // point at each other. 'spacing' is the separation of the
  // upper and lower UV planes (in m).
  //
  // Calculate projected positions of UV tracks in opposite planes, measure
  // how far they differ from partner UV track intercept, and store the sum
  // of the squares of the distances

  // Project the lower track into the upper plane
  fError  = GetProjectedDistance( fLowerPoint, fUpperPoint, spacing );
  fError += GetProjectedDistance( fUpperPoint, fLowerPoint, -spacing );

  return;
}

//_____________________________________________________________________________
void THaVDCPointPair::Associate( THaTrack* track )
{
  // Mark upper and lower points as well as their clusters
  // as belonging to the given track

  THaVDCPoint* point[2] = { fLowerPoint, fUpperPoint };

  for( int i=0; i<2; i++ ) {
    point[i]->SetTrack( track );
    point[i]->GetUCluster()->SetTrack( track );
    point[i]->GetVCluster()->SetTrack( track );
  }
}

//_____________________________________________________________________________
chi2_t THaVDCPointPair::CalcChi2() const
{
  // Calculate chi2 and number of data points for the associated track
  // and clusters

  THaVDCCluster* clust[4] =
    { fLowerPoint->GetUCluster(), fLowerPoint->GetVCluster(),
      fUpperPoint->GetUCluster(), fUpperPoint->GetVCluster() };

  chi2_t res(0,0);
  for( int i=0; i<4; i++ ) {
    chi2_t r = clust[i]->CalcDist();
    res.first  += r.first;
    res.second += r.second;
  }
  return res;
}

//_____________________________________________________________________________
Int_t THaVDCPointPair::Compare( const TObject* obj ) const
{
  // Compare this object to another THaVDCPointPair.
  // Used for sorting tracks.

  if( !obj || IsA() != obj->IsA() )
    return -1;

  const THaVDCPointPair* rhs = static_cast<const THaVDCPointPair*>( obj );

  if( fError < rhs->fError )
    return -1;
  if( fError > rhs->fError )
    return 1;

  return 0;
}

//_____________________________________________________________________________
Double_t THaVDCPointPair::GetProjectedDistance( THaVDCPoint* here,
						THaVDCPoint* there,
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
THaTrack* THaVDCPointPair::GetTrack() const
{
  // Return track associated with this pair

  assert( fLowerPoint->GetTrack() == fUpperPoint->GetTrack() );
  return fLowerPoint->GetTrack();
}

//_____________________________________________________________________________
void THaVDCPointPair::Print( Option_t* ) const
{
  // Print this object

  cout << fError << endl;
}

//_____________________________________________________________________________
void THaVDCPointPair::Use()
{
  // Mark this coordinate pair as used (i.e. associated with a track)
  // This does the following:
  //
  // (a) upper and lower points are marked as each other's partners
  // (b) the global slope resulting from the coordinate pair is calculated
  //     and stored in the clusters associated with the points
  // (c) This pair is marked as used

  // Make the points of this pair each other's partners. This prevents
  // tracks from being associated with more than one valid pair.
  fLowerPoint->SetPartner( fUpperPoint );
  fUpperPoint->SetPartner( fLowerPoint );

  // Compute global track values and get TRANSPORT coordinates for tracks. Re-
  // place local cluster slopes with global ones, which have higher precision.
  Double_t mu =
    (fUpperPoint->GetU() - fLowerPoint->GetU()) /
    (fUpperPoint->GetZU() - fLowerPoint->GetZU());
  Double_t mv =
    (fUpperPoint->GetV() - fLowerPoint->GetV()) /
    (fUpperPoint->GetZV() - fLowerPoint->GetZV());

  fLowerPoint->SetSlopes( mu, mv );
  fUpperPoint->SetSlopes( mu, mv );

  SetStatus(1);
}

//_____________________________________________________________________________
void THaVDCPointPair::Release()
{
  // Mark this track pair as unused

}

///////////////////////////////////////////////////////////////////////////////
ClassImp(THaVDCPointPair)
