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
  virtual ~THaPhysicsModule() {}
  
          bool  IsSingleTrack() const { return !IsMultiTrack(); }
          bool  IsMultiTrack()  const { return fMultiTrk; }

  virtual Int_t Process() = 0;

protected:

  //Only derived classes may construct me
  THaPhysicsModule() : fMultiTrk(false) {}
  THaPhysicsModule( const char* name, const char* description ) :
    THaAnalysisObject(name,description), fMultiTrk(false) {}
  THaPhysicsModule( const THaPhysicsModule& ) {};
  THaPhysicsModule& operator=( const THaPhysicsModule& ) { return *this; }

  virtual void MakePrefix() { THaAnalysisObject::MakePrefix( NULL ); }

  bool  fMultiTrk;               //Flag for multi-track mode

  ClassDef(THaPhysicsModule,1)   //ABC for a physics/kinematics module
};

#endif
