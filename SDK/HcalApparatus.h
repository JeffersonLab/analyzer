#ifndef ROOT_HcalApparatus
#define ROOT_HcalApparatus

//////////////////////////////////////////////////////////////////////////
//
// HcalApparatus
//
//////////////////////////////////////////////////////////////////////////

#include "THaApparatus.h"

class HcalApparatus : public THaApparatus {
  
public:
  HcalApparatus( const char* name="U", 
		 const char* description="Hadron Arm Apparatus" );
  virtual ~HcalApparatus();

  virtual void  Clear( Option_t* opt="");
  virtual Int_t Reconstruct();

protected:
  
  Int_t  fNtotal;  // Total number of hits in all detectors

  virtual Int_t DefineVariables( EMode mode = kDefine );

  ClassDef(HcalApparatus,0) // Hadron Arm
};

#endif

