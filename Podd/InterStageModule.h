#ifndef Podd_InterStageModule_h_
#define Podd_InterStageModule_h_

//////////////////////////////////////////////////////////////////////////
//
// InterStageModule
//
// Similar to a THaPhysicsModule, but with processing after a given
// analysis stage (Decode, CoarseReconstruct, etc.)
//
//////////////////////////////////////////////////////////////////////////

#include "THaAnalysisObject.h"

namespace Podd {

class InterStageModule : public THaAnalysisObject {

public:
  virtual ~InterStageModule();

  virtual void  Clear( Option_t* opt="" );
  virtual Int_t Process( const THaEvData& ) = 0;

  bool  DataValid() const { return fDataValid; }
  Int_t GetStage()  const { return fStage; }

protected:
  Int_t fStage;             // Stage(s) after which to run (bit field)
  bool  fDataValid;         // Data valid

  InterStageModule( const char* name, const char* description, Int_t stage );

  void PrintInitError( const char* here );
  virtual Int_t DefineVariables( EMode mode = kDefine );

  ClassDef(InterStageModule,1)   //ABC for an inter-stage processing module
};

} // namespace Podd

#endif
