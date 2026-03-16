//*-- Author :    Ole Hansen   7-Sep-00

//////////////////////////////////////////////////////////////////////////
//
// THaPidDetector
//
//////////////////////////////////////////////////////////////////////////

#include "THaPidDetector.h"

#if ROOT_VERSION_CODE < ROOT_VERSION(6,36,0)
ClassImp(THaPidDetector)
#endif

//______________________________________________________________________________
THaPidDetector::THaPidDetector( const char* name, const char* description,
				THaApparatus* apparatus )
  : THaNonTrackingDetector(name,description,apparatus)
{
  // Normal constructor with name and description

}

