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
#include <cstring>
#include <cctype>
#include <iostream>

ClassImp(THaDebugModule)

//_____________________________________________________________________________
THaDebugModule::THaDebugModule( const char* var_list ) :
  THaPhysicsModule("DebugModule","Global variable inspector"), 
  fNvars(0), fVars(NULL), fVarString(var_list), fFlags(0)
{
  // Normal constructor.
}

//_____________________________________________________________________________
THaDebugModule::~THaDebugModule()
{
  // Destructor

  delete [] fVars;
}

//_____________________________________________________________________________
THaAnalysisObject::EStatus THaDebugModule::Init( const TDatime& run_time )
{
  // Initialize the module.

  fNvars = 0;
  delete [] fVars; fVars = 0;

  if( THaPhysicsModule::Init( run_time ))
    return fStatus;

  Int_t nopt = fVarString.GetNOptions();
  if( nopt > 0 ) {
    if( !gHaVars )
      return fStatus = kInitError;
    fVars = new THaVar*[ nopt ];
    for( Int_t i=0; i<nopt; i++ ) {
      const char* opt = fVarString.GetOption(i);
      if( !strcmp( opt, "STOP" ))
	fFlags |= kStop;
      else {
	if( THaVar* var = gHaVars->Find( opt ))
	  fVars[fNvars++] = var;
	else {
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

  for( Int_t i=0; i<fNvars; i++ ) {
    fVars[i]->Print();
  }

  if( fFlags & kStop ) {
    // Wait for user to hit Return
    cout << "RETURN: continue, Q: quit\n";
    char c;
    cin.clear();
    while( !cin.eof() && cin.get(c) && !strchr("\nqQ",c));
    if( tolower(c) == 'q' )
      return kTerminate;
  }

  return 0;
}
