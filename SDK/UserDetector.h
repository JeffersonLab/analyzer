#ifndef ROOT_UserDetector
#define ROOT_UserDetector

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// UserDetector                                                              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaNonTrackingDetector.h"

class UserDetector : public THaNonTrackingDetector {

public:
  UserDetector( const char* name, const char* description = "",
		THaApparatus* a = NULL );
  virtual ~UserDetector();

  virtual void       Clear( Option_t* opt="" );
  virtual Int_t      Decode( const THaEvData& );
  virtual Int_t      CoarseProcess( TClonesArray& tracks );
  virtual Int_t      FineProcess( TClonesArray& tracks );

  Int_t GetNhits() const { return fNhit; }

protected:

  // Calibration
  Double_t*  fPed;       // [fNelem] ADC pedestals
  Double_t*  fGain;      // [fNelem] ADC gains
  
  // Per-event data
  Int_t      fNhit;      // Number of hits
  Double_t*  fRawADC;    // [fNelem] Raw ADC data
  Double_t*  fCorADC;    // [fNelem] Corrected ADC data
 
  void           DeleteArrays();
  virtual Int_t  ReadDatabase( const TDatime& date );
  virtual Int_t  DefineVariables( EMode mode );

  ClassDef(UserDetector,0)   // Example detector
};

////////////////////////////////////////////////////////////////////////////////

#endif
