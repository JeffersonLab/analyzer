//*-- Author :    Ole Hansen   7-Sep-00

//////////////////////////////////////////////////////////////////////////
//
// THaPidDetector
//
//////////////////////////////////////////////////////////////////////////

#include "THaPidDetector.h"

ClassImp(THaPidDetector)

//______________________________________________________________________________
THaPidDetector::THaPidDetector( const char* name, const char* description,
				THaApparatus* apparatus )
  : THaNonTrackingDetector(name,description,apparatus)
{
  // Normal constructor with name and description

}

//______________________________________________________________________________
THaPidDetector::~THaPidDetector()
{
  // Destructor

}

