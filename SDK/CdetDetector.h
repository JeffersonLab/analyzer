#ifndef ROOT_CdetDetector
#define ROOT_CdetDetector

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CdetDetector                                                              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaNonTrackingDetector.h"
#include <vector>

class CdetDetector : public THaNonTrackingDetector {

public:
  CdetDetector( const char* name, const char* description = "",
		THaApparatus* a = NULL );
  CdetDetector();
  virtual ~CdetDetector();

  virtual void       Clear( Option_t* opt="" );
  virtual Int_t      Decode( const THaEvData& );
  virtual EStatus    Init( const TDatime& run_time );
  virtual Int_t      CoarseProcess( TClonesArray& tracks );
  virtual Int_t      FineProcess( TClonesArray& tracks );
  virtual void       Print( Option_t* opt="" ) const;

  Int_t GetNhits() const { return fNhit; }

  static const Int_t NDEST = 1; // Only one side instrumented, currently!!
  struct DataDest {
    Int_t*    nthit;
    Double_t*  tdcl;
    Double_t*  tdct;
    Double_t*  tdcl_c;
    Double_t*  tdct_c;
    Int_t*    nahit;
    Double_t*  adc;
    Double_t*  adc_c;
    Double_t*  offset;
    Double_t*  ped;
    Double_t*  gain;
  } fDataDest[NDEST];     // Lookup table for decoder  

protected:
  // Typical experimental data are far less certain than even single precision,
  // so floats are usually just fine for storing them, especially if there
  // are arrays of values.
  typedef std::vector<Float_t> Vflt_t;

  // Calibration data from database
  //Vflt_t     fPed;       // ADC pedestals
  //Vflt_t     fGain;      // ADC gains
  Double_t*   fOff;       // [fNelem] TDC offsets for paddles
  Double_t*   fPed;       // [fNelem] ADC pedestals for addles
  Double_t*   fGain;      // [fNelem] ADC gains for paddles

  Double_t    fTdc2T;      // linear conversion between TDC and time (s/ch)
  Double_t    fCn;         // speed of light in material  (meters/second)
  Double_t    fAdcMIP;     // nominal ADC above pedestal for MIP
  
  // Per-event data
  Int_t      fTNhit;      // Number of TDC times
  Double_t*  fRawLTDC;    // [fNelem] Raw Leading Edge TDC data
  Double_t*  fRawTTDC;    // [fNelem] Raw Trailing Edge TDC data
  Double_t*  fCorLTDC;    // [fNelem] Corrected Leading Edge TDC data
  Double_t*  fCorTTDC;    // [fNelem] Corrected Leading Edge TDC data
  Int_t      fANhit;      // Number of ADC amplitudes
  Double_t*  fRawADC;    // [fNelem] Raw ADC data
  Double_t*   fCorADC;    // [fNelem] Corrected ADC data

  Int_t      fNhit;	// Number of paddles with TDC hits
 
  virtual Int_t  ReadDatabase( const TDatime& date );
  virtual Int_t  DefineVariables( EMode mode );

  ClassDef(CdetDetector,0)   // Fastbus detector 1
};

////////////////////////////////////////////////////////////////////////////////

#endif
