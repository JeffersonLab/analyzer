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

ClassImp(THaVDCUVTrack)

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
  Double_t u  = fUClust->GetIntercept(); // Intercept for U plane
  Double_t v0 = fVClust->GetIntercept(); // Intercept for V plane
  Double_t mu = fUClust->GetSlope();     // Slope of U cluster
  Double_t mv = fVClust->GetSlope();     // Slope of V cluster
    
  // Project v0 into the u plane
  Double_t v = v0 - mv * dz;

  // Now calculate track parameters in the detector cs
  Double_t detX     = (u*fUVPlane->fSin_v - v*fUVPlane->fSin_u) / 
    fUVPlane->fSin_vu;
  Double_t detY     = (v*fUVPlane->fCos_u - u*fUVPlane->fCos_v) / 
    fUVPlane->fSin_vu;
  Double_t detTheta = (mu*fUVPlane->fSin_v - mv*fUVPlane->fSin_u) / 
    fUVPlane->fSin_vu;
  Double_t detPhi   = (mv*fUVPlane->fCos_u - mu*fUVPlane->fCos_v) / 
    fUVPlane->fSin_vu;

  Set( detX, detY, detTheta, detPhi, fUVPlane->GetOrigin() );

}

////////////////////////////////////////////////////////////////////////////////
