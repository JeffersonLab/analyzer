// a class to handle event filtering
// Each filter has an associated global 'cut' variable
// to test against

#include <iostream>

#include "THaFilter.h"

#include "TError.h"
#include "TString.h"
#include "THaCutList.h"
#include "THaCut.h"
#include "THaIORun.h"

using namespace std;

//_____________________________________________________________________________
THaFilter::THaFilter( const char *cutname, const char* filename ) :
  fCutName(cutname), fFileName(filename), fRun(NULL), fCut(NULL)
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
}

//_____________________________________________________________________________
Int_t THaFilter::Close() 
{
  // Close this filter. Closes output file if open.

  cout << "Flushing and Closing " << fFileName << endl;
  if ( fRun->IsOpen() )
    return fRun->CloseFile();
  return 0;
}

//_____________________________________________________________________________
Int_t THaFilter::Init(const TDatime& date)
{
  // Init the filter

  if (fIsInit)
    return 0;

  fRun = new THaIORun(fFileName,"filtered file","w");
  if (!fRun) {
    Error("THaFilter::Process","Cannot create run %s for cut %s",
	  fFileName.Data(),fCutName.Data());
    return -2;
  }
  if ( fRun->OpenFile() ) {
    Error("THaFilter::Process","Cannot open file %s.",
	  fFileName.Data(),fCutName.Data());
    return -3;
  }      
  fCut = gHaCuts->FindCut(fCutName);
  if (!fCut) {
    Warning("THaFilter::Process","Cut %s does not exist. Filter is inactive.",
	    fCutName.Data());
  } else {
    fIsInit = 1;
  }
  return 0;
}

//_____________________________________________________________________________
Int_t THaFilter::Process( const THaEvData* evdata, const THaRun* run,
			  Int_t code ) 
{
  // Process event. Write the event to output CODA file if and only if
  // the event passes the filter cut.

  if (!fIsInit) return 0;
  
  if ( fCut->GetResult() ) {
    // write out the event
    Int_t *buff = const_cast<Int_t*>(run->GetEvBuffer());
    return fRun->PutEvent(buff);
  }
  return 0;
}

//_____________________________________________________________________________
ClassImp(THaFilter)
