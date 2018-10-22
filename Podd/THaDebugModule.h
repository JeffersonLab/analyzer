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
  THaDebugModule( const char* var_list, const char* test="" );
  virtual ~THaDebugModule();
  
  virtual EStatus   Init( const TDatime& run_time );
  virtual void      Print( Option_t* opt="" ) const;
  virtual Int_t     Process( const THaEvData& evdata );

protected:

  enum EFlags { 
    kStop  = BIT(0),    // Wait for key press after every event
    kCount = BIT(1),    // Run for fCount events   
    kQuiet = BIT(2)     // Run quietly (don't print variables)
  };

  std::vector<const TObject*> fVars; // Array of pointers to variables 
  THaPrintOption  fVarString; // Set of strings with variable/cut names
  Int_t           fFlags;     // Option flags
  Int_t           fCount;     // Event counter
  TString         fTestExpr;  // Definition of test to evaluate before printing
  THaCut*         fTest;      // Pointer to test object to evaulate

  void    PrintEvNum( const THaEvData& ) const;
  Int_t   ParseList();

private:

  THaDebugModule();
  THaDebugModule( const THaDebugModule& );
  THaDebugModule& operator=( const THaDebugModule& );

  ClassDef(THaDebugModule,0)  // Physics module for debugging
};

#endif
