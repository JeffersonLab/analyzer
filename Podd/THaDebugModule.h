#ifndef Podd_THaDebugModule_h_
#define Podd_THaDebugModule_h_

//////////////////////////////////////////////////////////////////////////
//
// THaDebugModule
//
//////////////////////////////////////////////////////////////////////////

#include "THaPhysicsModule.h"
#include "THaPrintOption.h"
#include <vector>

class THaCut;

class THaDebugModule : public THaPhysicsModule {

public:
  explicit THaDebugModule( const char* var_list, const char* test="" );
  THaDebugModule( const THaDebugModule& ) = delete;
  THaDebugModule& operator=( const THaDebugModule& ) = delete;
  virtual ~THaDebugModule();

  virtual EStatus   Init( const TDatime& run_time );
  virtual void      Print( Option_t* opt="" ) const;
  virtual Int_t     Process( const THaEvData& evdata );

protected:

  std::vector<const TObject*> fVars; // Array of pointers to variables
  THaPrintOption  fVarString; // Set of strings with variable/cut names
  Int_t           fFlags;     // Current operation mode (see implementation)
  Int_t           fCount;     // Event counter
  TString         fTestExpr;  // Definition of test to evaluate before printing
  THaCut*         fTest;      // Pointer to test object to evaluate

  void    PrintEvNum( const THaEvData& ) const;
  Int_t   ParseList();

  ClassDef(THaDebugModule,0)  // Physics module for debugging
};

#endif
