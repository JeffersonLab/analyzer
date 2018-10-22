//*-- Author :    Ole Hansen   12-Mar-02

//////////////////////////////////////////////////////////////////////////
//
// THaTrackID
//
// Abstract base class for all track ID classes.
//
// This class is used to identify tracks created by THaTrackingDetectors 
// uniquely. In general, each THaTrackingDetector may implement its own
// type of THaTrackID, depending on how tracks can be uniquely identified
// (wire numbers, cluster numbers, hit positions, etc.)  This information
// should be used to match tracks found in the CoarseTrack() stage with
// those found in FineTrack(). 
//
// Derived classes must implement at least operator== and operator!=.
// These operators should be protected against comparison of 
// dissimilar derived objects with a line like
//
// THaMyTrackID::operator==( const THaTrackID& rhs ) {
//   if( IsA() != rhs.IsA() ) return kFALSE;
//   ...
// }
//
//////////////////////////////////////////////////////////////////////////

#include "THaTrackID.h"

ClassImp(THaTrackID)

//_____________________________________________________________________________
