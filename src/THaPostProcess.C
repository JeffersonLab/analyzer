//
// abstract class to optionally handle the raw buffer of
// EVERY EVENT (regardless of type) that comes through
// the pipeline/

#include "THaPostProcess.h"
#include "TList.h"

TList* THaPostProcess::fgModules = NULL;

using namespace std;

//_____________________________________________________________________________
THaPostProcess::THaPostProcess() : fIsInit(0) 
{
  // Constructor

  if( !fgModules ) fgModules = new TList;
  fgModules->Add( this );
}

//_____________________________________________________________________________
THaPostProcess::~THaPostProcess() 
{
  // Destructor

  fgModules->Remove( this );
  if( fgModules->GetSize() == 0 ) {
    delete fgModules; fgModules = NULL;
  }
}

//_____________________________________________________________________________
ClassImp(THaPostProcess)
