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
#include "THaVarList.h"
#include "THaCutList.h"
#include "THaCut.h"
#include "THaGlobals.h"
#include "THaEvData.h"
#include "TRegexp.h"
#include "TClass.h"

#include <cstring>
#include <cctype>
#include <iostream>

using namespace std;

enum EFlags {
  kStop,     // Wait for key press after every event
  kCount,    // Run for fCount events
  kQuiet     // Run quietly (don't print variables)
};

//_____________________________________________________________________________
THaDebugModule::THaDebugModule( const char* var_list, const char* test )
  : THaPhysicsModule("DebugModule", var_list)
  , fVarString(var_list)
  , fFlags(0)
  , fCount(0)
  , fTestExpr(test)
  , fTest(nullptr)
{
  // Normal constructor.

  SETBIT(fFlags, kStop);
  fIsSetup = false;
}

//_____________________________________________________________________________
THaDebugModule::~THaDebugModule()
{
  // Destructor
  delete fTest;
}

//_____________________________________________________________________________
THaAnalysisObject::EStatus THaDebugModule::Init( const TDatime& run_time )
{
  // Initialize the module. This just clears the list of variables.
  // The actual initialization takes place in ParseList()

  fVars.clear();

  if( THaPhysicsModule::Init(run_time) )
    return fStatus;

  // fIsSetup is set by the default DefineVariables, but we use it as a flag
  // to trigger lazy initialization of our variables/cuts list in Process(),
  // so we need to unset it explicitly here.
  fIsSetup = false;

  return fStatus = kOK;
}


//_____________________________________________________________________________
Int_t THaDebugModule::THaDebugModule::ParseList()
{
  // Parse the list of variable/cut names and store pointers
  // to the found objects. Called from Process() the first time
  // this module is used.

  Int_t nopt = fVarString.GetNOptions();
  if( nopt > 0 ) {
    for( Int_t i = 0; i < nopt; i++ ) {
      const char* opt = fVarString.GetOption(i);
      if( !strcmp(opt, "NOSTOP") )
        CLRBIT(fFlags, kStop);
      else {
        // Regexp matching
        bool found = false;
        TRegexp re(opt, true);
        // We can inspect analysis variables and cuts/tests
        if( gHaVars ) {
          TIter next(gHaVars);
          while( TObject* obj = next() ) {
            TString s = obj->GetName();
            if( s.Index(re) != kNPOS ) {
              found = true;
              fVars.push_back(obj);
            }
          }
        }
        if( gHaCuts ) {
          const TList* lst = gHaCuts->GetCutList();
          if( lst ) {
            TIter next(lst);
            while( TObject* obj = next() ) {
              TString s = obj->GetName();
              if( s.Index(re) != kNPOS ) {
                found = true;
                fVars.push_back(obj);
              }
            }
          }
        }
        if( !found ) {
          Warning(Here("ParseList"),
                  "Global variable or cut %s not found. Skipped.", opt);
        }
      }
    }
  }
  return 0;
}

//_____________________________________________________________________________
void THaDebugModule::Print( Option_t* opt ) const
{
  // Print details of the defined variables

  THaPhysicsModule::Print(opt);
  if( fIsSetup ) {
    if( !fTestExpr.IsNull() ) {
      if( fTest )
        fTest->Print();
      else
        cout << "Test name: " << fTestExpr << " (undefined)\n";
    }
    cout << "Number of variables: " << fVars.size() << endl;
    for( const auto* obj: fVars ) {
      cout << obj->GetName() << "  ";
    }
    cout << endl;
  } else {
    if( !fTestExpr.IsNull() )
      cout << "Test name: " << fTestExpr << endl;
    cout << "(Module not initialized)\n";
  }
}

//_____________________________________________________________________________
void THaDebugModule::PrintEvNum( const THaEvData& evdata ) const
{
  // Print current event number
  cout << "======>>>>>> Event " << (UInt_t) evdata.GetEvNum() << endl;
}

//_____________________________________________________________________________
Int_t THaDebugModule::Process( const THaEvData& evdata )
{
  // Print the variables for every event and wait for user input.

  // We have to set up the test here because physics modules' Init() is
  // called before the analyzer's tests are loaded, and we want to be
  // able to use existing tests.
  if( !fIsSetup ) {
    ParseList();
    if( !fTestExpr.IsNull() ) {
      fTest = new THaCut(fName + "_Test", fTestExpr, fName + "_Block");
      // Expression error?
      if( !fTest || fTest->IsZombie() ) {
        delete fTest;
        fTest = nullptr;
      }
    }
    fIsSetup = true;
  }
  bool good = !fTest || fTest->EvalCut();

  // Print() the variables
  if( good && !TESTBIT(fFlags, kQuiet) ) {
    PrintEvNum(evdata);
    for( const auto& var: fVars ) {
      const char* opt = "";
      if( var->IsA()->InheritsFrom("THaVar") )
        opt = "FULL";
      var->Print(opt);
    }
  }

  if( TESTBIT(fFlags, kCount) && --fCount <= 0 ) {
    SETBIT(fFlags, kStop);
    CLRBIT(fFlags, kCount);
    if( !good )
      PrintEvNum(evdata);
    good = true;
  }
  if( good && TESTBIT(fFlags, kStop) ) {
    // Wait for user input
    cout << "RETURN: continue, H: run 100 events, R: run to end, F: finish quietly, Q: quit\n";
    char c = 0;
    cin.clear();
    while( !cin.eof() && cin.get(c) && !strchr("\nqQhHrRfF", c) ) {}
    if( c != '\n' )
      while( !cin.eof() && cin.get() != '\n' ) {}
    if( tolower(c) == 'q' )
      return kTerminate;
    else if( tolower(c) == 'h' ) {
      SETBIT(fFlags, kCount);
      CLRBIT(fFlags, kStop);
      fCount = 100;
    } else if( tolower(c) == 'r' )
      CLRBIT(fFlags, kStop);
    else if( tolower(c) == 'f' ) {
      CLRBIT(fFlags, kStop);
      SETBIT(fFlags, kQuiet);
    }
  }

  return 0;
}

ClassImp(THaDebugModule)
