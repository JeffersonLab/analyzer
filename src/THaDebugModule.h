#ifndef ROOT_THaDebugModule
#define ROOT_THaDebugModule

//////////////////////////////////////////////////////////////////////////
//
// THaDebugModule
//
//////////////////////////////////////////////////////////////////////////

#include "THaPhysicsModule.h"
#include "THaPrintOption.h"
#include <vector>

class THaVar;

class THaDebugModule : public THaPhysicsModule {
  
public:
  THaDebugModule( const char* var_list );
  virtual ~THaDebugModule();
  
          Int_t     GetNvars() const { return fVars.size(); }
  virtual EStatus   Init( const TDatime& run_time );
  virtual Int_t     Process();

protected:

  enum EFlags { 
    kStop  = BIT(0),    // Wait for key press after every event
    kCount = BIT(1)     // Run for fCount events   
  };

  vector<THaVar*> fVars;      // Array of pointers to variables 
  THaPrintOption  fVarString; // Set of strings with variable names
  Int_t           fFlags;     // Option flags
  Int_t           fCount;     // Event counter

private:
  THaDebugModule();
  THaDebugModule( const THaDebugModule& );
  THaDebugModule& operator=( const THaDebugModule& );

  ClassDef(THaDebugModule,0)  // Physics module for debugging
};

#endif
