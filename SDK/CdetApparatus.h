#ifndef ROOT_CdetApparatus
#define ROOT_CdetApparatus

//////////////////////////////////////////////////////////////////////////
//
// CdetApparatus
//
//////////////////////////////////////////////////////////////////////////

#include "THaApparatus.h"

class CdetApparatus : public THaApparatus {
  
public:
  CdetApparatus( const char* name="U", 
		 const char* description="Cdet Apparatus" );
  virtual ~CdetApparatus();

  virtual void  Clear( Option_t* opt="");
  virtual Int_t Reconstruct();

protected:
  
  Int_t  fNtotal;  // Total number of hits in all detectors

  virtual Int_t DefineVariables( EMode mode = kDefine );

  ClassDef(CdetApparatus,0) // Fastbus test stand
};

#endif

