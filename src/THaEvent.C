//*-- Authors :    Ole Hansen & Bob Michaels  16/08/2002

////////////////////////////////////////////////////////////////////////
//
// THaEvent
//
// This is event data that we ALWAYS want to appear on the output,
// regardless of what other global variables or formulas users want.
// 
// The present version (Sept 2002) is very simplified from the original
// because most of the output is handled by class THaOutput.
//
////////////////////////////////////////////////////////////////////////

#include "THaEvent.h"
#include "THaEvData.h"
#include "THaVarList.h"
#include "THaGlobals.h"
#include "TROOT.h"

#include <cstring>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>
#include <iomanip>
#include <strstream.h>

ClassImp(THaEventHeader)
ClassImp(THaEvent)

THaEvent::THaEvent() 
{
  event_number = 0;
  event_type = 0;
  event_length = 0;
  run_number = 0;
  helicity = 0;
}

//______________________________________________________________________________
THaEvent::~THaEvent()
{

}

//______________________________________________________________________________
Int_t THaEvent::Fill( THaEvData &data )
{  
  event_number = data.GetEvNum();
  event_type = data.GetEvType();
  event_length = data.GetEvLength();
  run_number = data.GetRunNum();
  helicity = data.GetHelicity();

// Add what you want here...

  return 0;
}









