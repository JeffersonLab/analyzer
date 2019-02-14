#ifndef Podd_UserApparatus_h_
#define Podd_UserApparatus_h_

//////////////////////////////////////////////////////////////////////////
//
// UserApparatus
//
//////////////////////////////////////////////////////////////////////////

#include "THaApparatus.h"

class UserApparatus : public THaApparatus {

public:
  UserApparatus( const char* name="U",
		 const char* description="User Apparatus" );
  virtual ~UserApparatus();

  virtual void  Clear( Option_t* opt="");
  virtual Int_t Reconstruct();

protected:

  Int_t  fNtotal;  // Total number of hits in all detectors

  virtual Int_t DefineVariables( EMode mode = kDefine );

  ClassDef(UserApparatus,0) // An example apparatus
};

#endif
