#ifndef Podd_THaScintillator_h_
#define Podd_THaScintillator_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaScintillator                                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaNonTrackingDetector.h"

class TClonesArray;

class THaScintillator : public THaNonTrackingDetector {

public:
  THaScintillator( const char* name, const char* description = "",
		   THaApparatus* a = NULL );
  THaScintillator();
  virtual ~THaScintillator();

  virtual void       Clear( Option_t* opt="" );
  virtual Int_t      Decode( const THaEvData& );
  virtual EStatus    Init( const TDatime& run_time );
  virtual Int_t      CoarseProcess( TClonesArray& tracks );
  virtual Int_t      FineProcess( TClonesArray& tracks );

  virtual Int_t      ApplyCorrections( void );

  Int_t GetNHits() const { return fNhit; }
  const Double_t* GetTimes() const { return fTime; }
  const Double_t* GetTuncer() const { return fdTime; }
  const Double_t* GetAmplitudes() const { return fAmpl; }
  const Double_t* GetYtime() const { return fYt; }
  const Double_t* GetYampl() const { return fYa; }

protected:

  static const Int_t NDEST = 2;
  struct DataDest {
    Int_t*    nthit;
    Int_t*    nahit;
    Double_t*  tdc;
    Double_t*  tdc_c;
    Double_t*  adc;
    Double_t*  adc_p;
    Double_t*  adc_c;
    Double_t*  offset;
    Double_t*  ped;
    Double_t*  gain;
  } fDataDest[NDEST];      // Lookup table for decoder

  // Calibration
  Double_t*   fLOff;       // [fNelem] TDC offsets for left paddles
  Double_t*   fROff;       // [fNelem] TDC offsets for right paddles
  Double_t*   fLPed;       // [fNelem] ADC pedestals for left paddles
  Double_t*   fRPed;       // [fNelem] ADC pedestals for right paddles
  Double_t*   fLGain;      // [fNelem] ADC gains for left paddles
  Double_t*   fRGain;      // [fNelem] ADC gains for right paddles

  Double_t    fTdc2T;      // linear conversion between TDC and time (s/ch)
  Double_t    fCn;         // speed of light in material  (meters/second)

  Int_t       fNTWalkPar;  // number of timewalk correction parameters
  Double_t*   fTWalkPar;   // [fNTWalkPar] time walk correction parameters
  Double_t    fAdcMIP;     // nominal ADC above pedestal for MIP

  Double_t*   fTrigOff;     // [fNelem] Induced offset of trigger time from
			    // diff between trigger and retiming.
			    // Visible in coincidence data.

  Double_t    fAttenuation; // in m^-1: attenuation length of material
  Double_t    fResolution;  // average time resolution per PMT (s)

  // Per-event data
  Int_t       fLTNhit;     // Number of Left paddles with TDC hits
  Double_t*   fLT;         // [fNelem] Left paddles TDC times (channels)
  Double_t*   fLT_c;       // [fNelem] Left PMT corrected TDC times (s)
  Int_t       fRTNhit;     // Number of Right paddles with TDC signals
  Double_t*   fRT;         // [fNelem] Right paddles TDC times (channels)
  Double_t*   fRT_c;       // [fNelem] Right PMT corrected TDC times (s)
  Int_t       fLANhit;     // Number of Left paddles with ADC hits
  Double_t*   fLA;         // [fNelem] Left paddles ADC amplitudes
  Double_t*   fLA_p;       // [fNelem] Left paddles ADC minus ped values
  Double_t*   fLA_c;       // [fNelem] Left paddles corrected ADC ampl-s
  Int_t       fRANhit;     // Number of Right paddles with ADC hits
  Double_t*   fRA;         // [fNelem] Right paddles ADC amplitudes
  Double_t*   fRA_p;       // [fNelem] Right paddles ADC minus ped values
  Double_t*   fRA_c;       // [fNelem] Right paddles corrected ADC ampl-s


  Int_t       fNhit;       // Number of paddles with complete TDC hits (l&r)
  Int_t*      fHitPad;     // [fNhit] list of paddles with complete TDC hits

  // could be done on a per-hit basis instead
  Double_t*   fTime;       // [fNelem] corrected time for the paddle (s)
  Double_t*   fdTime;      // [fNelem] uncertainty in time (s)
  Double_t*   fAmpl;       // [fNelem] overall amplitude for the paddle
  Double_t*   fYt;         // [fNelem] y-position of hit in paddle from TDC (m)
  Double_t*   fYa;         // [fNelem] y-position of hit in paddle from ADC (m)

  void           DeleteArrays();
  virtual Int_t  ReadDatabase( const TDatime& date );
  virtual Int_t  DefineVariables( EMode mode = kDefine );

  enum ESide { kLeft = 0, kRight = 1 };

  virtual  Double_t TimeWalkCorrection( const Int_t& paddle,
					const ESide side );

  ClassDef(THaScintillator,1)   // Generic scintillator class
};

////////////////////////////////////////////////////////////////////////////////

#endif
