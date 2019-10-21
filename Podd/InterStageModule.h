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
  virtual ~InterStageModule() = default;

  virtual void  Clear( Option_t* opt="" );
  virtual Int_t Process( const THaEvData& ) = 0;

  bool  DataValid() const { return fDataValid; }
  Int_t GetStage()  const { return fStage; }

protected:
  Int_t fStage;                  // Stage after which to run
  bool  fDataValid;              // Data valid

  InterStageModule( const char* name, const char* description,
                    Int_t stage );

  void PrintInitError( const char* here );
  virtual Int_t DefineVariables( EMode mode );

  ClassDef(InterStageModule,1)   //ABC for a physics/kinematics module
};

} // namespace Podd

#endif
