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
#include <vector>

using namespace std;

namespace Podd {

//_____________________________________________________________________________
CodaRawDecoder::CodaRawDecoder()
{
  const char* const here = "CodaRawDecoder::CodaRawDecoder";

  // Register standard global variables for event header data
  if( gHaVars ) {
    const vector<VarDef> vars = {
      { .name = "runnum",   .desc = "Run number",       .type = kUInt,  .loc = &run_num       },
      { .name = "runtype",  .desc = "CODA run type",    .type = kUInt,  .loc = &run_type      },
      { .name = "runtime",  .desc = "Run start time (Unix)", .type = kLong,  .loc = &fRunTime },
      { .name = "evnum",    .desc = "Event number",     .type = kULong, .loc = &event_num     },
      { .name = "evtyp",    .desc = "Event type",       .type = kUInt,  .loc = &event_type    },
      { .name = "evlen",    .desc = "Event length",     .type = kUInt,  .loc = &event_length  },
      { .name = "evtime",   .desc = "Event time",       .type = kULong, .loc = &evt_time      },
      { .name = "datatype", .desc = "Data type",        .type = kUInt,  .loc = &data_type     },
      { .name = "trigbits", .desc = "Trigger bits",     .type = kUInt,  .loc = &trigger_bits  },
      { .name = "tsevtyp",  .desc = "TS event type",    .type = kUInt,  .loc = &tsEvType      },
      { .name = "rawevnum", .desc = "Raw event number", .type = kULong, .loc = &raw_event_num }
    };
    TString prefix("g");
    // Prevent global variable clash if there are several instances of us
    if( fInstance > 1 )
      prefix.Append(Form("%d",fInstance));
    prefix.Append(".");
    gHaVars->DefineVariables(vars, {.prefix = prefix, .caller = here});
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

} // namespace Podd

#if ROOT_VERSION_CODE < ROOT_VERSION(6,36,0)
ClassImp(Podd::CodaRawDecoder)
#endif
