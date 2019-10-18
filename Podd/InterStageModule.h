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

  virtual void  Clear( Option_t* opt="" ) {
    THaAnalysisObject::Clear(opt);
    fDataValid = false;
  }
  virtual Int_t Process( const THaEvData& ) = 0;

  bool  DataValid() const { return fDataValid; }
  Int_t GetStage()  const { return fStage; }

  // Special return codes for Process()
  enum ESpecialRetval { kFatal     = -16768,
			kTerminate = -16767 };
protected:

  InterStageModule( const char* name, const char* description,
                    Int_t stage );

  void PrintInitError( const char* here );

  Int_t fStage;                  // Stage after which to run
  bool  fDataValid;              // Data valid

  ClassDef(InterStageModule,1)   //ABC for a physics/kinematics module
};

} // namespace Podd

#endif
