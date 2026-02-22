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
  explicit UserApparatus( const char* name="U",
                          const char* description="User Apparatus" );
  ~UserApparatus() override;

  void  Clear( Option_t* opt="") override;
  Int_t Reconstruct() override;

protected:

  Int_t  fNtotal;  // Total number of hits in all detectors

  Int_t DefineVariables( EMode mode = kDefine ) override;

  ClassDefOverride(UserApparatus,0) // An example apparatus
};

#endif
