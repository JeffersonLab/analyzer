//*-- Author :    Ole Hansen   12-Mar-02

//////////////////////////////////////////////////////////////////////////
//
// THaTrackID
//
// Abstract base class for all track ID classes
//
//////////////////////////////////////////////////////////////////////////

#include "THaTrackID.h"

ClassImp(THaTrackID)

//_____________________________________________________________________________
THaTrackID& THaTrackID::operator=( const THaTrackID& rhs )
{
  // Assignment operator.

  TObject::operator=(rhs);
  return *this;
}
