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
  // Destructor. 
  // Delete the Time to Distance Converter
  delete fTTDConv;  fTTDConv = NULL;

}


///////////////////////////////////////////////////////////////////////////////
