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

const Double_t THaTrackInfo::kBig = 1e38;

//_____________________________________________________________________________
THaTrackInfo& THaTrackInfo::operator=( const THaTrackInfo& rhs )
{
  // Assignment operator

  if( this != &rhs ) {
    fP     = rhs.fP;
    fDp    = rhs.fDp;
    fX     = rhs.fX;
    fY     = rhs.fY;
    fTheta = rhs.fTheta;
    fPhi   = rhs.fPhi;
    fPvect = rhs.fPvect;
    fOK    = rhs.fOK;
    fSpectro = rhs.fSpectro;
  }
  return *this;
}

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
