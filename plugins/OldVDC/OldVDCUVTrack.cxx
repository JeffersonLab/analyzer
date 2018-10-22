///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// OldVDCUVTrack                                                             //
//                                                                           //
// Class for UV Tracks                                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "OldVDCUVTrack.h"
#include "OldVDCUVPlane.h"
#include "OldVDCCluster.h"

ClassImp(OldVDCUVTrack)

//_____________________________________________________________________________
void OldVDCUVTrack::CalcDetCoords()
{
  // Convert U,V coordinates of our two clusters to the detector coordinate
  // system. 
  //
  // Note that the slopes of our clusters may have been replaced
  // with global angles computed in a higher-level class. 
  // See, for example, OldVDC::ConstructTracks()
  // 
  // This routine requires several parameters from the OldVDCUVPlane that
  // this track belongs to. fUVPlane must be set!

  Double_t dz = fUVPlane->GetSpacing();  // Space between U and V planes
  Double_t u  = fUClust->GetIntercept(); // Intercept for U plane
  Double_t v0 = fVClust->GetIntercept(); // Intercept for V plane
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
void OldVDCUVTrack::CalcChisquare(Double_t& chi2, Int_t& nhits) const
{
  // Accumulate the chi2 from the clusters making up this track,
  // adding their terms to the chi2 and nhits.
  //
  //  The global slope and intercept, derived from the entire track,
  //  must already have been set for each cluster
  //   (as done in OldVDC::ConstructTracks)
  //
  fUClust->CalcChisquare(chi2,nhits);
  fVClust->CalcChisquare(chi2,nhits);
}
  
////////////////////////////////////////////////////////////////////////////////
