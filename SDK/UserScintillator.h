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
  explicit UserScintillator( const char* name, const char* description = "",
                             THaApparatus* a = nullptr );
  ~UserScintillator() override;

  void       Clear( Option_t* ="" ) override;
  //  Int_t      Decode( const THaEvData& ) override;
  Int_t      FineProcess( TClonesArray& tracks ) override;

protected:
  Int_t     fPaddle;      // Paddle number hit by first track
  Data_t    fYtdc;        // Transverse position within paddle from TDCs (m)
  Data_t    fYtrk;        // Transverse position within paddle from track (m)

  Bool_t    fGoodToGo;    // True if all required parameters available

  // Parameters from database
  Bool_t    fCommonStop;  // True if TDCs use common stop mode

  Int_t  ReadDatabase( const TDatime& date ) override;
  Int_t  DefineVariables( EMode mode = kDefine ) override;

  ClassDefOverride(UserScintillator,0)   // User-extended version of THaScintillator
};

////////////////////////////////////////////////////////////////////////////////

#endif
