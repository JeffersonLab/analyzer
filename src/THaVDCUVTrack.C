///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCUVTrack                                                             //
//                                                                           //
// Class for UV Tracks                                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaVDCUVTrack.h"
#include "THaVDCUVPlane.h"
#include "THaTrack.h"

const Double_t THaVDCUVTrack::kBig = 1e38;  // Arbitrary large value

//_____________________________________________________________________________
THaVDCUVTrack::THaVDCUVTrack( THaVDCCluster* u_cl, THaVDCCluster* v_cl,
			      THaVDCUVPlane* plane )
  : fUClust(u_cl), fVClust(v_cl), fUVPlane(plane), fTrack(0), fPartner(0)
{
  // Constructor

  assert( fUClust && fVClust && fUVPlane );

  CalcDetCoords();
}

//_____________________________________________________________________________
void THaVDCUVTrack::CalcDetCoords()
{
  // Convert U,V coordinates of our two clusters to the detector
  // coordinate system.

  const UVPlaneCoords_t c = fUVPlane->CalcDetCoords(fUClust,fVClust);
  Set( c.x, c.y, c.theta, c.phi, fUVPlane->GetOrigin() );
}

//_____________________________________________________________________________
Double_t THaVDCUVTrack::GetZU() const
{
  // Return z-position of u cluster

  return fUVPlane->GetZ();
}

//_____________________________________________________________________________
Double_t THaVDCUVTrack::GetZV() const
{
  // Return intercept of u cluster

  return fUVPlane->GetZ() + fUVPlane->GetSpacing();
}

//_____________________________________________________________________________
void THaVDCUVTrack::SetSlopes( Double_t mu, Double_t mv )
{
  // Set global slopes of U and V clusters to mu, mv and recalculate
  // detector coordinates

  fUClust->SetSlope( mu );
  fVClust->SetSlope( mv );
  CalcDetCoords();
}

//_____________________________________________________________________________
void THaVDCUVTrack::CalcChisquare(Double_t& chi2, Int_t& nhits) const
{
  // Accumulate the chi2 from the clusters making up this track,
  // adding their terms to the chi2 and nhits.
  //
  //  The global slope and intercept, derived from the entire track,
  //  must already have been set for each cluster
  //   (as done in THaVDC::ConstructTracks)
  //
  fUClust->CalcChisquare(chi2,nhits);
  fVClust->CalcChisquare(chi2,nhits);
}

//_____________________________________________________________________________
void THaVDCUVTrack::SetTrack( THaTrack* track )
{
  // Associate this cluster as well as the underlying U and V clusters
  // with a reconstructed track

  fTrack = track;
  if( fUClust ) fUClust->SetTrack(track);
  if( fVClust ) fVClust->SetTrack(track);
}

//_____________________________________________________________________________
Int_t THaVDCUVTrack::GetTrackIndex() const
{
  // Return index of assigned track (-1 = none, 0 = first/best, etc.)

  if( !fTrack ) return -1;
  return fTrack->GetIndex();
}

//_____________________________________________________________________________
ClassImp(THaVDCUVTrack)
