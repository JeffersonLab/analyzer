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
  }
  return *this;
}

//_____________________________________________________________________________
THaTrackInfo& THaTrackInfo::operator=( const THaTrack& rhs )
{
  // Assignment to a track

  fP     = rhs.GetP();
  fDp    = rhs.GetDp();
  fX     = rhs.GetTX();
  fY     = rhs.GetTY();
  fTheta = rhs.GetTTheta();
  fPhi   = rhs.GetTPhi();
  fPvect = const_cast<THaTrack&>(rhs).GetPvect();
  fOK    = 1;
  
  return *this;
}

//_____________________________________________________________________________
ClassImp(THaTrackInfo)
