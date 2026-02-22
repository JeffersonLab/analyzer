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
#include "ChannelData.h"

class TClonesArray;

class THaCherenkov : public THaPidDetector {

public:
  explicit THaCherenkov( const char* name, const char* description = "",
                         THaApparatus* a = nullptr );
  THaCherenkov(); // for ROOT I/O
  ~THaCherenkov() override;

  // THaCherenkov now uses THaDetectorBase::Decode()
  void       Clear( Option_t* ="" ) override;
  Int_t      CoarseProcess( TClonesArray& tracks ) override;
  Int_t      FineProcess( TClonesArray& tracks ) override;
  Data_t     GetAsum() const { return fASUM_c; }

protected:

  Podd::TDCData* fPMTData;   // PMT ADC & TDC calibration and per-event data
  Data_t         fASUM_p;    // Sum of ADC minus pedestal values of channels
  Data_t         fASUM_c;    // Sum of corrected ADC amplitudes of channels

  Int_t    StoreHit( const DigitizerHitInfo_t& hitinfo, UInt_t data ) override;
  void     PrintDecodedData( const THaEvData& evdata ) const override;

  Int_t    DefineVariables( EMode mode = kDefine ) override;
  Int_t    ReadDatabase( const TDatime& date ) override;

  ClassDefOverride(THaCherenkov,0)    //Generic Cherenkov class
};

////////////////////////////////////////////////////////////////////////////////

#endif
