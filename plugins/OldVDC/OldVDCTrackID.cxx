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

using namespace std;

//_____________________________________________________________________________
OldVDCTrackID::OldVDCTrackID( const OldVDCUVTrack* lower,
			      const OldVDCUVTrack* upper)
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
TObject* OldVDCTrackID::Clone( const char* ) const
{
  return new OldVDCTrackID(*this);
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
