#ifndef ROOT_THaPhysicsModule
#define ROOT_THaPhysicsModule

//////////////////////////////////////////////////////////////////////////
//
// THaPhysicsModule
//
//////////////////////////////////////////////////////////////////////////

#include "THaAnalysisObject.h"

class THaPhysicsModule : public THaAnalysisObject {
  
public:
  virtual ~THaPhysicsModule();
  
          bool  IsSingleTrack() const { return fSingleTrk; }
          bool  IsMultiTrack()  const { return !IsSingleTrack(); }

  virtual Int_t Process() = 0;

protected:

  //Only derived classes may construct me
  THaPhysicsModule() : fSingleTrk(false) {}
  THaPhysicsModule( const char* name, const char* description );
  THaPhysicsModule( const THaPhysicsModule& ) {};
  THaPhysicsModule& operator=( const THaPhysicsModule& ) { return *this; }

  virtual void MakePrefix() { THaAnalysisObject::MakePrefix( NULL ); }

  bool  fSingleTrk;              //Flag for single track mode

  ClassDef(THaPhysicsModule,0)   //ABC for a physics/kinematics module

};

#endif
