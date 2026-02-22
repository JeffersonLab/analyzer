#ifndef Podd_THaPhysicsModule_h_
#define Podd_THaPhysicsModule_h_

//////////////////////////////////////////////////////////////////////////
//
// THaPhysicsModule
//
//////////////////////////////////////////////////////////////////////////

#include "THaAnalysisObject.h"

class THaPhysicsModule : public THaAnalysisObject {
  
public:
  void  Clear( Option_t* opt="" ) override {
    THaAnalysisObject::Clear(opt); 
    fDataValid = false;
  }

  bool  IsSingleTrack() const { return !IsMultiTrack(); }
  bool  IsMultiTrack()  const { return fMultiTrk; }
  bool  DataValid()     const { return fDataValid; }

  virtual Int_t Process( const THaEvData& ) = 0;

  // Special return codes for Process()
  enum ESpecialRetval { kFatal     = -16768,
			kTerminate = -16767 };
protected:
//TODO: DefineVariables fDataValid etc.
  THaPhysicsModule( const char* name, const char* description );

  void PrintInitError( const char* here );

  bool  fMultiTrk;               // Flag for multi-track mode
  bool  fDataValid;              // Data valid

  ClassDefOverride(THaPhysicsModule,1)   //ABC for a physics/kinematics module
};

#endif
