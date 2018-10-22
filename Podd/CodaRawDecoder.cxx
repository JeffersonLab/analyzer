////////////////////////////////////////////////////////////////////
//
//   Podd::CodaRawDecoder
//
//   Version of Decoder::CodaDecoder raw event data decoder that
//   exports Podd global variables for basic event data
//
//   Ole Hansen, August 2018
//
/////////////////////////////////////////////////////////////////////

#include "CodaRawDecoder.h"
#include "THaAnalysisObject.h"
#include "THaVarList.h"
#include "THaGlobals.h"

using namespace std;

namespace Podd {

//_____________________________________________________________________________
CodaRawDecoder::CodaRawDecoder()
{
  const char* const here = "CodaRawDecoder::CodaRawDecoder";

  // Register standard global variables for event header data
  if( gHaVars ) {
    VarDef vars[] = {
        { "runnum",    "Run number",     kInt,    0, &run_num },
        { "runtype",   "CODA run type",  kInt,    0, &run_type },
        { "runtime",   "CODA run time",  kULong,  0, &fRunTime },
        { "evnum",     "Event number",   kInt,    0, &event_num },
        { "evtyp",     "Event type",     kInt,    0, &event_type },
        { "evlen",     "Event Length",   kInt,    0, &event_length },
        { "evtime",    "Event time",     kULong,  0, &evt_time },
        { 0 }
    };
    TString prefix("g");
    // Prevent global variable clash if there are several instances of us
    if( fInstance > 1 )
      prefix.Append(Form("%d",fInstance));
    prefix.Append(".");
    gHaVars->DefineVariables( vars, prefix, here );
  } else
    Warning(here,"No global variable list found. Variables not registered.");
}

//_____________________________________________________________________________
CodaRawDecoder::~CodaRawDecoder()
{
  // Destructor. Unregister global variables

  if( gHaVars ) {
    TString prefix("g");
    if( fInstance > 1 )
      prefix.Append(Form("%d",fInstance));
    prefix.Append(".*");
    gHaVars->RemoveRegexp( prefix );
  }
}

//_____________________________________________________________________________
Int_t CodaRawDecoder::init_cmap_openfile( FILE*& fi, TString& fname )
{
  // Use the analyzer file name search logic to find the crate map database file

  const char* const here = "CodaRawDecoder::init_cmap";

  TDatime date(GetRunTime());    //FIXME: replace with TTimeStamp
  fi = THaAnalysisObject::OpenFile(fCrateMapName,date,here,"r",0);
  fname = "db_" + fCrateMapName + ".dat";
  return 1;
}
//_____________________________________________________________________________

} // namespace Podd

ClassImp(Podd::CodaRawDecoder)
