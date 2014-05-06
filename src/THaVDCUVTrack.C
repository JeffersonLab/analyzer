///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCUVTrack                                                             //
//                                                                           //
// Class for UV Tracks                                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaVDCUVTrack.h"
#include "THaVDCUVPlane.h"
#include "THaVDCCluster.h"
#include "THaTrack.h"

const Double_t THaVDCUVTrack::kBig = 1e38;  // Arbitrary large value

//_____________________________________________________________________________
void THaVDCUVTrack::CalcDetCoords()
{
  // Convert U,V coordinates of our two clusters to the detector coordinate
  // system.
  //
  // Note that the slopes of our clusters may have been replaced
  // with global angles computed in a higher-level class.
  // See, for example, THaVDC::ConstructTracks()
  //
  // This routine requires several parameters from the THaVDCUVPlane that
  // this track belongs to. fUVPlane must be set!

  Double_t dz = fUVPlane->GetSpacing();  // Space between U and V planes
  Double_t u  = GetU();                  // Intercept for U plane
  Double_t v0 = GetV();                  // Intercept for V plane
  Double_t mu = fUClust->GetSlope();     // Slope of U cluster
  Double_t mv = fVClust->GetSlope();     // Slope of V cluster

  // Project v0 into the u plane
  Double_t v = v0 - mv * dz;

  // Now calculate track parameters in the detector cs
  Double_t detX     = (u*fUVPlane->fSin_v - v*fUVPlane->fSin_u) *
    fUVPlane->fInv_sin_vu;
  Double_t detY     = (v*fUVPlane->fCos_u - u*fUVPlane->fCos_v) *
    fUVPlane->fInv_sin_vu;
  Double_t detTheta = (mu*fUVPlane->fSin_v - mv*fUVPlane->fSin_u) *
    fUVPlane->fInv_sin_vu;
  Double_t detPhi   = (mv*fUVPlane->fCos_u - mu*fUVPlane->fCos_v) *
    fUVPlane->fInv_sin_vu;

  Set( detX, detY, detTheta, detPhi, fUVPlane->GetOrigin() );

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
  // Set global slopes of U and V clusters to mu, mv
  // Recalculate detector coordinates (v projection may have changed slightly)

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
