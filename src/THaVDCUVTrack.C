///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCUVTrack                                                             //
//                                                                           //
// Class for UV Tracks                                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaVDCUVTrack.h"

class THaTrack;

ClassImp(THaVDCUVTrack)

//_____________________________________________________________________________
void THaVDCUVTrack::SetTrack( THaTrack* track )
{
  fTrack = track;
//    fUClust->SetTrack(track);
//    fVClust->SetTrack(track);
}

////////////////////////////////////////////////////////////////////////////////
