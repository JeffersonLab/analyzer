#ifndef Podd_THaCherenkov_h_
#define Podd_THaCherenkov_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaCherenkov                                                              //
//                                                                           //
// Generic Cherenkov detector.                                               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaPidDetector.h"

class TClonesArray;

class THaCherenkov : public THaPidDetector {

public:
  THaCherenkov( const char* name, const char* description = "",
	      THaApparatus* a = NULL );
  THaCherenkov(); // for ROOT I/O
  virtual ~THaCherenkov();

  virtual void       Clear( Option_t* ="" );
  virtual Int_t      Decode( const THaEvData& );
  virtual Int_t      CoarseProcess( TClonesArray& tracks );
  virtual Int_t      FineProcess( TClonesArray& tracks );
          Float_t    GetAsum() const { return fASUM_c; }

protected:

  // Configuration
  Double_t   fTdc2T;      // Conversion coefficient from raw TDC to time (s/ch)

  // Calibration
  Float_t*   fOff;        // [fNelem] TDC offsets (chan)
  Float_t*   fPed;        // [fNelem] ADC pedestals (chan)
  Float_t*   fGain;       // [fNelem] ADC gains

  // Per-event data
  Int_t      fNThit;      // Number of mirrors with non zero TDC times
  Float_t*   fT;          // [fNelem] Array of TDC times of channels
  Float_t*   fT_c;        // [fNelem] Array of corrected TDC times of channels
  Int_t      fNAhit;      // Number of mirrors with non zero ADC amplitudes
  Float_t*   fA;          // [fNelem] Array of ADC amplitudes of channels
  Float_t*   fA_p;        // [fNelem] Array of ADC minus pedestal values of chans
  Float_t*   fA_c;        // [fNelem] Array of corrected ADC amplitudes of chans
  Float_t    fASUM_p;     // Sum of ADC minus pedestal values of channels
  Float_t    fASUM_c;     // Sum of corrected ADC amplitudes of channels

  virtual Int_t  DefineVariables( EMode mode = kDefine );
          void   DeleteArrays();
  virtual Int_t  ReadDatabase( const TDatime& date );

  ClassDef(THaCherenkov,0)    //Generic Cherenkov class
};

////////////////////////////////////////////////////////////////////////////////

#endif
