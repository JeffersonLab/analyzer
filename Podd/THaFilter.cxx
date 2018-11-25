// a class to handle event filtering
// Each filter has an associated global 'cut' variable
// to test against

#include "THaFilter.h"
#include "THaCodaFile.h"
#include "TError.h"
#include "TString.h"
#include "THaCutList.h"
#include "THaCut.h"
#include "THaRunBase.h"
#include "THaEvData.h"
// only for ERetVal, used by Process(). Should put ERetVal in a separate header
#include "THaAnalyzer.h"

using namespace std;
using namespace Decoder;

//_____________________________________________________________________________
THaFilter::THaFilter( const char *cutexpr, const char* filename ) :
  fCutExpr(cutexpr), fFileName(filename), fCodaOut(NULL), fCut(NULL)
{
  // Constructor

  // This module returns compatible return codes
  SetBit(kUseReturnCode);
}

//_____________________________________________________________________________
THaFilter::~THaFilter()
{
  // Destructor. Calls virtual Close(). NB: any derived class
  // that overrides Close() must implement its own destructor and call its
  // own Close() from there.

  Close();
  delete fCut;
  delete fCodaOut;
}

//_____________________________________________________________________________
Int_t THaFilter::Close()
{
  // Close this filter. Closes output file if open.

  if( !fCodaOut ) return 0;
  //cout << "Flushing and Closing " << fFileName << endl;
  return fCodaOut->codaClose();
}

//_____________________________________________________________________________
Int_t THaFilter::Init(const TDatime& )
{
  // Init the filter

  const char* const here = "THaFilter::Init";

  if (fIsInit)
    return 0;

  // Set up our cut
  fCut = new THaCut( "Filter_Test", fCutExpr, "PostProcess" );
  // Expression error?
  if( !fCut || fCut->IsZombie()) {
    delete fCut; fCut = NULL;
    Warning(here,"Illegal cut expression: %s.\nFilter is inactive.",
	    fCutExpr.Data());
  } else {
    fCodaOut = new THaCodaFile;
    if (!fCodaOut) {
      Error(here,"Cannot create CODA output file object. "
	    "Something is very wrong." );
      return -2;
    }

    if ( fCodaOut->codaOpen(fFileName, "w", 1) ) {
      Error(here,"Cannot open CODA file %s for writing.",fFileName.Data());
      delete fCodaOut;
      return -3;
    }
    fIsInit = 1;
  }
  return 0;
}

//_____________________________________________________________________________
Int_t THaFilter::Process( const THaEvData* evdata, const THaRunBase* run,
			  Int_t /* code */ )
{
  // Process event. Write the event to output CODA file if and only if
  // the event passes the filter cut.

  const char* const here = "THaFilter::Process";

  if (!fIsInit || !fCut->EvalCut())
    return THaAnalyzer::kOK;

  // write out the event
  Int_t ret = fCodaOut->codaWrite(run->GetEvBuffer());
  if( ret == CODA_FATAL ) {
    Error( here, "Fatal error writing to CODA output file %s. Check if you have "
	   "write permission", fFileName.Data() );
    return THaAnalyzer::kFatal;
  } else if( ret != CODA_OK ) {
    Error( here, "Error writing to CODA output file %s. Event %d not written",
	   fFileName.Data(), (evdata != NULL) ? evdata->GetEvNum() : -1 );
  }
  return THaAnalyzer::kOK;
}

//_____________________________________________________________________________
ClassImp(THaFilter)
