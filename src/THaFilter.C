#include <iostream>

#include "THaFilter.h"

#include "TError.h"
#include "TString.h"
#include "THaCutList.h"
#include "THaCut.h"
#include "THaIORun.h"
#include "TList.h"

using namespace std;

TList* gFilterList=0;

THaFilter::THaFilter( const char *cutname, const char* filename ) :
  THaPostProcess(), fCutName(cutname), fFileName(filename), fRun(NULL),
  fCut(NULL)
{
  if (!gFilterList)
    gFilterList = new TList();
  
  gFilterList->Add(this);
}

Int_t THaFilter::Init(const TDatime& date) {
  if (!fIsInit) {
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
  }
  return 0;
}

Int_t THaFilter::Process( const THaRun *run, int force ) {
  if (!fIsInit) return 0;
  
  if ( force || fCut->GetResult() ) {
    // write out the event
    Int_t *buff = const_cast<Int_t*>(run->GetEvBuffer());
    return fRun->PutEvent(buff);
  }
  return 0;
}

Int_t THaFilter::CleanUp() {
  cout << "Flushing and Closing " << fFileName << endl;
  if ( fRun->IsOpen() )
    return fRun->CloseFile();
  return 0;
}

ClassImp(THaFilter)
