///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCPoint                                                               //
//                                                                           //
// A pair of one U and one V VDC cluster in a given VDC chamber              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaVDCPoint.h"
#include "THaVDCChamber.h"
#include "THaTrack.h"
#include <cassert>

//_____________________________________________________________________________
THaVDCPoint::THaVDCPoint( THaVDCCluster* u_cl, THaVDCCluster* v_cl,
			  THaVDCChamber* chamber )
  : fUClust(u_cl), fVClust(v_cl), fChamber(chamber), fTrack(0), fPartner(0)
{
  // Constructor

  assert( fUClust && fVClust && fChamber );

  CalcDetCoords();
}

//_____________________________________________________________________________
void THaVDCPoint::CalcDetCoords()
{
  // Convert U,V coordinates of our two clusters to the detector
  // coordinate system.

  const PointCoords_t c = fChamber->CalcDetCoords(fUClust,fVClust);
  Set( c.x, c.y, c.theta, c.phi );
  SetCenter( fChamber->DetToTrackCoord(c.x,c.y) );
}

//_____________________________________________________________________________
Double_t THaVDCPoint::GetZU() const
{
  // Return z-position of u cluster

  return fChamber->GetZ();
}

//_____________________________________________________________________________
Double_t THaVDCPoint::GetZV() const
{
  // Return z-position of v cluster

  return fChamber->GetZ() + fChamber->GetSpacing();
}

//_____________________________________________________________________________
void THaVDCPoint::SetSlopes( Double_t mu, Double_t mv )
{
  // Set global slopes of U and V clusters to mu, mv and recalculate
  // detector coordinates

  fUClust->SetSlope( mu );
  fVClust->SetSlope( mv );
  CalcDetCoords();
}

//_____________________________________________________________________________
void THaVDCPoint::CalcChisquare(Double_t& chi2, Int_t& nhits) const
{
  // Accumulate the chi2 from the clusters making up this pair,
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
void THaVDCPoint::SetTrack( THaTrack* track )
{
  // Associate this cluster pair as well as the underlying U and V clusters
  // with a reconstructed track

  fTrack = track;
  if( fUClust ) fUClust->SetTrack(track);
  if( fVClust ) fVClust->SetTrack(track);
}

//_____________________________________________________________________________
Int_t THaVDCPoint::GetTrackIndex() const
{
  // Return index of assigned track (-1 = none, 0 = first/best, etc.)

  if( !fTrack ) return -1;
  return fTrack->GetIndex();
}

//_____________________________________________________________________________
ClassImp(THaVDCPoint)
