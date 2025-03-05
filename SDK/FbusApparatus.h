#ifndef ROOT_FbusApparatus
#define ROOT_FbusApparatus

//////////////////////////////////////////////////////////////////////////
//
// FbusApparatus
//
//////////////////////////////////////////////////////////////////////////

#include "THaApparatus.h"

class FbusApparatus : public THaApparatus {
  
public:
  FbusApparatus( const char* name="U", 
		 const char* description="Fbus Apparatus" );
  virtual ~FbusApparatus();

  virtual void  Clear( Option_t* opt="");
  virtual Int_t Reconstruct();

protected:
  
  Int_t  fNtotal;  // Total number of hits in all detectors

  virtual Int_t DefineVariables( EMode mode = kDefine );

  ClassDef(FbusApparatus,0) // Fastbus test stand
};

#endif

