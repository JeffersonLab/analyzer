//*-- Author :    Ole Hansen   26-Mar-03

//////////////////////////////////////////////////////////////////////////
//
// THaDebugModule
//
// Diagnostic class to help troubleshoot physics modules.
// Prints one or more global variables for every event.
//
//////////////////////////////////////////////////////////////////////////

#include "THaDebugModule.h"
#include "THaVar.h"
#include "THaVarList.h"
#include "THaGlobals.h"
#include "TRegexp.h"

#include <cstring>
#include <cctype>
#include <iostream>

ClassImp(THaDebugModule)

//_____________________________________________________________________________
THaDebugModule::THaDebugModule( const char* var_list ) :
  THaPhysicsModule("DebugModule","Global variable inspector"), 
  fVarString(var_list), fFlags(kStop), fCount(0)
{
  // Normal constructor.
}

//_____________________________________________________________________________
THaDebugModule::~THaDebugModule()
{
  // Destructor
}

//_____________________________________________________________________________
THaAnalysisObject::EStatus THaDebugModule::Init( const TDatime& run_time )
{
  // Initialize the module.

  if( THaPhysicsModule::Init( run_time ))
    return fStatus;

  fVars.clear();

  Int_t nopt = fVarString.GetNOptions();
  if( nopt > 0 ) {
    if( !gHaVars )
      return fStatus = kInitError;
    TIter next( gHaVars );
    TObject* obj;
    for( Int_t i=0; i<nopt; i++ ) {
      const char* opt = fVarString.GetOption(i);
      if( !strcmp(opt,"NOSTOP") )
	fFlags &= ~kStop;
      else {
	// Regexp matching
	bool found = false;
	TRegexp re( opt, kTRUE);
	while( (obj = next()) ) {
	  TString s = obj->GetName();
	  if( s.Index(re) != kNPOS ) {
	    found = true;
	    fVars.push_back( static_cast<THaVar*>(obj) );
	  }
	}
	if( !found ) {
	  Warning( Here("Init()"), 
		   "Global variable %s not found. Skipped.", opt );
	}
      }
    }
  }
  return fStatus = kOK;
}

//_____________________________________________________________________________
Int_t THaDebugModule::Process()
{
  // Print() the variables

  vector<THaVar*>::iterator it = fVars.begin();
  for( ; it != fVars.end(); it++ ) {
    (*it)->Print();
  }

  if( (fFlags & kCount) && --fCount <= 0 ) {
    fFlags |= kStop;
    fFlags &= ~kCount;
  }
  if( fFlags & kStop ) {
    // Wait for user input
    cout << "RETURN: continue, H: run 100 events, R: run to end, Q: quit\n";
    char c;
    cin.clear();
    while( !cin.eof() && cin.get(c) && !strchr("\nqQhHrR",c));
    if( c != '\n' ) while( !cin.eof() && cin.get() != '\n');
    if( tolower(c) == 'q' )
      return kTerminate;
    else if( tolower(c) == 'h' ) {
      fFlags |= kCount;
      fFlags &= ~kStop;
      fCount = 100;
    }
    else if( tolower(c) == 'r' )
      fFlags &= ~kStop;
  }

  return 0;
}
