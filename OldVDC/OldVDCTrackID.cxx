//*-- Author :    Ole Hansen   09-Mar-02

//////////////////////////////////////////////////////////////////////////
//
// OldVDCTrackID
//
// This is a utility class for the VDC tracking classes.
// It allows unique identification of tracks.
//
//////////////////////////////////////////////////////////////////////////

#include "OldVDCTrackID.h"
#include "OldVDCCluster.h"
#include "OldVDCUVTrack.h"
#include <iostream>
#include <iomanip>
#include <cstring>

using namespace std;

//_____________________________________________________________________________
OldVDCTrackID::OldVDCTrackID( const OldVDCUVTrack* lower,
			      const OldVDCUVTrack* upper) :
  THaTrackID()
{
  // Constructor that automatically determines pivot numbers
  // from the given OldVDCUVTracks.

  OldVDCCluster* cluster;
  if( lower ) {
    if( (cluster = lower->GetUCluster()))
      fLowerU = cluster->GetPivotWireNum();
    if( (cluster = lower->GetVCluster()))
      fLowerV = cluster->GetPivotWireNum();
  }
  if( upper ) {
    if( (cluster = upper->GetUCluster()))
      fUpperU = cluster->GetPivotWireNum();
    if( (cluster = upper->GetVCluster()))
      fUpperV = cluster->GetPivotWireNum();
  }
}

//_____________________________________________________________________________
OldVDCTrackID::OldVDCTrackID( const OldVDCTrackID& rhs ) : THaTrackID(rhs),
  fLowerU(rhs.fLowerU), fLowerV(rhs.fLowerV),
  fUpperU(rhs.fUpperU), fUpperV(rhs.fUpperV)
{
  // Copy constructor.
}

//_____________________________________________________________________________
OldVDCTrackID& OldVDCTrackID::operator=( const OldVDCTrackID& rhs )
{
  // Assignment operator.

  THaTrackID::operator=(rhs);
  if ( this != &rhs ) {
    fLowerU = rhs.fLowerU;
    fLowerV = rhs.fLowerV;
    fUpperU = rhs.fUpperU;
    fUpperV = rhs.fUpperV;
  }
  return *this;
}

//_____________________________________________________________________________
void OldVDCTrackID::Print( Option_t* ) const
{
  // Print ID description

  cout << " " 
       << setw(5) << fLowerU
       << setw(5) << fLowerV
       << setw(5) << fUpperU
       << setw(5) << fUpperV
       << endl;
}
///////////////////////////////////////////////////////////////////////////////
ClassImp(OldVDCTrackID)
