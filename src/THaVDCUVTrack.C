///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCUVTrack                                                             //
//                                                                           //
// Class for UV Tracks 
///////////////////////////////////////////////////////////////////////////////

#include "THaVDC.h"
#include "THaVDCUVTrack.h"
#include "THaVDCCluster.h"
#include "THaVDCUVPlane.h"
#include "THaTrack.h"


ClassImp(THaVDCUVTrack)


//______________________________________________________________________________
THaVDCUVTrack::THaVDCUVTrack()
{
  //Normal constructor
  fUClust = NULL;
  fVClust = NULL;
  fUVPlane = NULL;
  fTrack   = NULL;
  fPartner = NULL;
  fX = fY = fTheta = fPhi = 0.0;
}



//______________________________________________________________________________
THaVDCUVTrack::~THaVDCUVTrack()
{
  // Destructor. Remove variables from global list.

}
//______________________________________________________________________________
THaVDCUVTrack * THaVDCUVTrack::FindPartner(TClonesArray& trackList, Int_t length)
{
  // Find the UV Track in tracklist which is closest to the projected position
  // of this track in the other plane

  if (length == 0)
    return NULL;  // There can't be a partner if the list is empty!

  if (length == 1) {
    //Only one track, so it must be the partner
    fPartner = (THaVDCUVTrack*)trackList[0];
    return fPartner; 
  }

  Double_t px; //Projected x
  Double_t py; //Projected y
  // Get distance between upper and lower uv planes
  Double_t detZ = fUVPlane->GetVDC()->GetSpacing(); 

  // First, determine which UV plane this track belongs to
  if (fUVPlane ==  fUVPlane->GetVDC()->GetLower()) { 
    // Lower UV Track, so project into upper UV plane
    px = fX + detZ * fTheta;
    py = fY + detZ * fPhi;
  } else {
    // Upper UV track, to project into the lower UV plane
    px = fX - detZ * fTheta;
    py = fY - detZ * fPhi;
  }
  
  //Now find the track in trackList which is closest to the projected coordinates
  Double_t minDist = 1e307; // Arbitrary very large value
 
  THaVDCUVTrack * minTrack;
  for (int i = 0; i < length; i++) {
    THaVDCUVTrack * track = (THaVDCUVTrack *)trackList[i];
    Double_t dist = (track->fX - px) * (track->fX - px) + 
      (track->fY - py) * (track->fY - py);
    if (dist < minDist) {
      minDist = dist;
      minTrack = track;
    }
  }
  fPartner = minTrack;
  return minTrack;
    
}

//_____________________________________________________________________________
void THaVDCUVTrack::SetTrack(THaTrack * track)
{
  fTrack = track;
//    fUClust->SetTrack(track);
//    fVClust->SetTrack(track);
}

////////////////////////////////////////////////////////////////////////////////
