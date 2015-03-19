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
  
  virtual void  Clear( Option_t* opt="" ) { 
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

  THaPhysicsModule( const char* name, const char* description );

  void PrintInitError( const char* here );

  bool  fMultiTrk;               // Flag for multi-track mode
  bool  fDataValid;              // Data valid

  ClassDef(THaPhysicsModule,1)   //ABC for a physics/kinematics module
};

#endif
