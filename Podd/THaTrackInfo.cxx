//*-- Author :    Ole Hansen   04-Apr-03

//////////////////////////////////////////////////////////////////////////
//
// THaTrackInfo
//
// Utility class/structure for holding track information.
//
//////////////////////////////////////////////////////////////////////////

#include "THaTrackInfo.h"
#include "THaTrack.h"
#include "THaSpectrometer.h"
#include "THaTrackingDetector.h"

using namespace std;

//_____________________________________________________________________________
THaTrackInfo& THaTrackInfo::operator=( const THaTrack& track )
{
  // Assignment to a track

  fP     = track.GetP();
  fDp    = track.GetDp();
  fX     = track.GetTX();
  fY     = track.GetTY();
  fTheta = track.GetTTheta();
  fPhi   = track.GetTPhi();
  fPvect = const_cast<THaTrack&>(track).GetPvect();
  fOK    = 1;
  THaTrackingDetector* td = track.GetCreator();
  if( td )
    fSpectro = static_cast<THaSpectrometer*>(td->GetApparatus());

  return *this;
}

//_____________________________________________________________________________
ClassImp(THaTrackInfo)
