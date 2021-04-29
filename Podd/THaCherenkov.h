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
#include "DetectorData.h"

class TClonesArray;

class THaCherenkov : public THaPidDetector {

public:
  explicit THaCherenkov( const char* name, const char* description = "",
                         THaApparatus* a = nullptr );
  THaCherenkov(); // for ROOT I/O
  virtual ~THaCherenkov();

  // THaCherenkov now uses THaDetectorBase::Decode()
  virtual void       Clear( Option_t* ="" );
  virtual Int_t      CoarseProcess( TClonesArray& tracks );
  virtual Int_t      FineProcess( TClonesArray& tracks );
          Data_t     GetAsum() const { return fASUM_c; }

protected:

  Podd::PMTData* fPMTData;   // PMT ADC & TDC calibration and per-event data
  Data_t         fASUM_p;    // Sum of ADC minus pedestal values of channels
  Data_t         fASUM_c;    // Sum of corrected ADC amplitudes of channels

  virtual Int_t    StoreHit( const DigitizerHitInfo_t& hitinfo, UInt_t data );
  virtual void     PrintDecodedData( const THaEvData& evdata ) const;

  virtual Int_t    DefineVariables( EMode mode = kDefine );
  virtual Int_t    ReadDatabase( const TDatime& date );

  ClassDef(THaCherenkov,0)    //Generic Cherenkov class
};

////////////////////////////////////////////////////////////////////////////////

#endif
