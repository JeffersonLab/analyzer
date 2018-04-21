#ifndef Podd_UserScintillator_h_
#define Podd_UserScintillator_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// UserScintillator                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaScintillator.h"

class UserScintillator : public THaScintillator {

public:
  UserScintillator( const char* name, const char* description = "",
		   THaApparatus* a = NULL );
  virtual ~UserScintillator();

  virtual void       Clear( Option_t* ="" );
  //  virtual Int_t      Decode( const THaEvData& );
  virtual Int_t      FineProcess( TClonesArray& tracks );

protected:
  Int_t     fPaddle;      // Paddle number hit by first track
  Double_t  fYtdc;        // Transverse position within paddle from TDCs (m)
  Double_t  fYtrk;        // Transverse position within paddle from track (m)

  Bool_t    fGoodToGo;    // True if all required parameters available

  // Parameters from database
  Bool_t    fCommonStop;  // True if TDCs use common stop mode

  virtual Int_t  ReadDatabase( const TDatime& date );
  virtual Int_t  DefineVariables( EMode mode = kDefine );

  ClassDef(UserScintillator,0)   // User-extended version of THaScintillator
};

////////////////////////////////////////////////////////////////////////////////

#endif
