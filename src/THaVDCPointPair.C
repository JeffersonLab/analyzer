//*-- Author :    Ole Hansen   13-Mar-02

//////////////////////////////////////////////////////////////////////////
//
// THaVDCPointPair
//
// A pair of THaVDCPoints, candidate for a reconstructed track.
//
//////////////////////////////////////////////////////////////////////////

#include "THaVDCPointPair.h"
#include "THaVDCChamber.h"
#include "TClass.h"       // for IsA()
#include "TString.h"

#include <iostream>
#include <cassert>

using namespace std;
using namespace VDC;

//_____________________________________________________________________________
void THaVDCPointPair::Analyze()
{
  // Compute goodness of match parameter between upper and lower point.
  // Essentially, this is a measure of how closely the two local tracks
  // point at each other.

  //FIXME: preliminary, just the old functionality
  fError = CalcError( fLowerPoint, fUpperPoint, fSpacing );
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
    res = res + clust[i]->CalcDist();
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
Double_t THaVDCPointPair::CalcError( THaVDCPoint* here,
				     THaVDCPoint* there,
				     Double_t spacing )
{
  // Calculate projected positions of points in opposite planes, measure
  // how far they differ from partner point intercept, and return the sum
  // of the squares of the distances

  Double_t error =
    GetProjectedDistance( here, there, spacing );
  error +=
    GetProjectedDistance( there, here, -spacing );

  return error;
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
Bool_t THaVDCPointPair::HasUsedCluster() const
{
  // Return true if any cluster underlying this point pair is also part of
  // another point pair that has already been chosen for making a track.
  // This ensures each cluster is only ever used in at most one final track.

  const THaVDCCluster* clust[4] =
    { fLowerPoint->GetUCluster(), fLowerPoint->GetVCluster(),
      fUpperPoint->GetUCluster(), fUpperPoint->GetVCluster() };

  for( int i=0; i<4; i++ ) {
    if( clust[i]->GetPointPair() != 0 )
      return kTRUE;
  }
  return kFALSE;
}

//_____________________________________________________________________________
void THaVDCPointPair::Print( Option_t* opt ) const
{
  // Print details about this point pair (for debugging)

  TString sopt(opt);
  if( sopt.Contains("TRACKP") ) {
    cout << "Global track parameters: mu/mv/th/ph = "
	 << GetLower()->GetUCluster()->GetSlope() << " "
	 << GetLower()->GetVCluster()->GetSlope() << " "
	 << GetLower()->GetTheta() << " "
	 << GetLower()->GetPhi()
	 << endl;
    return;
  }

  if( !sopt.Contains("NOHEAD") ) {
    cout << "Pair: ";
  }
  cout << "pivots = "
       << GetLower()->GetUCluster()->GetPivotWireNum() << " "
       << GetLower()->GetVCluster()->GetPivotWireNum() << " "
       << GetUpper()->GetUCluster()->GetPivotWireNum() << " "
       << GetUpper()->GetVCluster()->GetPivotWireNum()
       << ", dUpper/dLower = "
       << GetProjectedDistance( GetLower(), GetUpper(), fSpacing )
       << "  "
       << GetProjectedDistance( GetUpper(), GetLower(),-fSpacing )
       << ", error = "
       << GetError()
       << endl;
}

//_____________________________________________________________________________
void THaVDCPointPair::Use()
{
  // Mark this point pair as used (i.e. chosen for making a track)
  // This does the following:
  //
  // (a) upper and lower points are marked as each other's partners
  // (b) the underlying four clusters are marked as associated with this pair
  // (c) the global slope resulting from the coordinate pair is calculated
  //     and stored in the clusters associated with the points
  // (d) This pair is marked as used

  // Compute global track values and get TRANSPORT coordinates for tracks. Re-
  // place local cluster slopes with global ones, which have higher precision.
  Double_t mu =
    (fUpperPoint->GetU() - fLowerPoint->GetU()) /
    (fUpperPoint->GetZU() - fLowerPoint->GetZU());
  Double_t mv =
    (fUpperPoint->GetV() - fLowerPoint->GetV()) /
    (fUpperPoint->GetZV() - fLowerPoint->GetZV());

  // Set point an cluster data
  THaVDCPoint* point[2] = { fLowerPoint, fUpperPoint };
  for( int i = 0; i<2; ++i ) {
    point[i]->SetPartner( point[!i] );
    point[i]->SetSlopes( mu, mv );
    point[i]->GetUCluster()->SetPointPair( this );
    point[i]->GetVCluster()->SetPointPair( this );
  }

  SetStatus(1);
}

//_____________________________________________________________________________
void THaVDCPointPair::Release()
{
  // Mark this track pair as unused

}

///////////////////////////////////////////////////////////////////////////////
ClassImp(THaVDCPointPair)
