///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCWire                                                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaVDCWire.h"
#include "THaVDCTimeToDistConv.h"
#include "THaVDCT0CalTable.h"

ClassImp(THaVDCWire)


//_____________________________________________________________________________
THaVDCWire::~THaVDCWire()
{
  // Destructor. Does not delete time-to-distance converter since it
  // has been allocated by the plane object.
}


///////////////////////////////////////////////////////////////////////////////
