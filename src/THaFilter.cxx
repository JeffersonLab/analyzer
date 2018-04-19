// a class to handle event filtering
// Each filter has an associated global 'cut' variable
// to test against

#include <iostream>

#include "THaFilter.h"
#include "THaCodaFile.h"
#include "TError.h"
#include "TString.h"
#include "THaCutList.h"
#include "THaCut.h"
#include "THaRunBase.h"

using namespace std;
using namespace Decoder;

//_____________________________________________________________________________
THaFilter::THaFilter( const char *cutexpr, const char* filename ) :
  fCutExpr(cutexpr), fFileName(filename), fCodaOut(NULL), fCut(NULL)
{
  // Constructor
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
  cout << "Flushing and Closing " << fFileName << endl;
  return fCodaOut->codaClose();
}

//_____________________________________________________________________________
Int_t THaFilter::Init(const TDatime& )
{
  // Init the filter

  if (fIsInit)
    return 0;

  fCodaOut = new THaCodaFile;
  if (!fCodaOut) {
    Error("Init","Cannot create CODA output file object. "
	  "Something is very wrong." );
    return -2;
  }
  if ( fCodaOut->codaOpen(fFileName, "w", 1) ) {
    Error("Init","Cannot open CODA file %s for writing.",fFileName.Data());
    return -3;
  }

  // Set up our cut
  fCut = new THaCut( "Filter_Test", fCutExpr, "PostProcess" );
  // Expression error?
  if( !fCut || fCut->IsZombie()) {
    delete fCut; fCut = NULL;
    Warning("Init","Illegal cut expression: %s.\nFilter is inactive.",
	    fCutExpr.Data());
  } else {
    fIsInit = 1;
  }
  return 0;
}

//_____________________________________________________________________________
Int_t THaFilter::Process( const THaEvData* /* evdata */, const THaRunBase* run,
			  Int_t /* code */ )
{
  // Process event. Write the event to output CODA file if and only if
  // the event passes the filter cut.

  if (!fIsInit || !fCut->EvalCut())
    return 0;

  // write out the event
  return  fCodaOut->codaWrite(run->GetEvBuffer());
}

//_____________________________________________________________________________
ClassImp(THaFilter)
