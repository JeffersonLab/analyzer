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

  Double_t dz   = fUVPlane->GetSpacing();  // Space between U and V planes
  Double_t u    = fUClust->GetIntercept(); // Intercept for U plane
  Double_t vInt = fVClust->GetIntercept(); // Intercept for V plane
  Double_t mu   = fUClust->GetSlope();     // Change in u / change in z_det
  Double_t mv   = fVClust->GetSlope();     // Change in v / change in z_det
    
  // Project vInt into the u plane
  Double_t v = vInt - mv * dz;

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
