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

  const THaVDCUVPlane::PointCoords_t c =
    fUVPlane->CalcDetCoords( fUClust, fVClust );

  Set( c.x, c.y, c.theta, c.phi, fUVPlane->GetOrigin() );
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
  
////////////////////////////////////////////////////////////////////////////////
