//*-- Author :    Ole Hansen   13-Mar-02

//////////////////////////////////////////////////////////////////////////
//
// THaVDCTrackPair
//
// This is a utility class for the VDC tracking classes.
// It allows unique identification of tracks.
//
//////////////////////////////////////////////////////////////////////////

#include "THaVDCTrackPair.h"
#include "THaVDCUVTrack.h"
#include "TClass.h"

#include <iostream>

ClassImp(THaVDCTrackPair)

//_____________________________________________________________________________
THaVDCTrackPair::THaVDCTrackPair( THaVDCUVTrack* lower, THaVDCUVTrack* upper) :
  fLowerTrack(lower), fUpperTrack(upper), fError(1e307)
{
  // Constructor

  lower->SetPartner( upper );
  upper->SetPartner( lower );
}

//_____________________________________________________________________________
THaVDCTrackPair::THaVDCTrackPair( const THaVDCTrackPair& rhs ) :
  fLowerTrack(rhs.fLowerTrack), fUpperTrack(rhs.fUpperTrack),
  fError(rhs.fError)
{
  // Copy constructor.
}

//_____________________________________________________________________________
THaVDCTrackPair& THaVDCTrackPair::operator=( const THaVDCTrackPair& rhs )
{
  // Assignment operator.

  TObject::operator=(rhs);
  if ( this != &rhs ) {
    fLowerTrack = rhs.fLowerTrack;
    fUpperTrack = rhs.fUpperTrack;
    fError      = rhs.fError;
  }
  return *this;
}

//_____________________________________________________________________________
void THaVDCTrackPair::Analyze( Double_t spacing )
{
  // Compute goodness of match paremeter for the two UV tracks.
  // Essentially, this is a measure of how closely the two tracks
  // point at each other. 'spacing' is the separation of the 
  // upper and lower UV planes (in m).

  fError = 1e307;
}

//_____________________________________________________________________________
Int_t THaVDCTrackPair::Compare( const TObject* obj ) const
{
  // Compare this object to another THaVDCTrackPair

  if( !obj || IsA() != obj->IsA() )
    return -1;

  const THaVDCTrackPair* rhs = static_cast<const THaVDCTrackPair*>( obj );

  if( fError < rhs->fError )
    return -1;
  if( fError > rhs->fError )
    return 1;

  return 0;
}

//_____________________________________________________________________________
void THaVDCTrackPair::Print( Option_t* opt ) const
{
  // Print this object

  cout << fError << endl;
}
