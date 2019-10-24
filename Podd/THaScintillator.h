#ifndef Podd_THaScintillator_h_
#define Podd_THaScintillator_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaScintillator                                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaNonTrackingDetector.h"
#include <vector>

class TClonesArray;

class THaScintillator : public THaNonTrackingDetector {

public:
  explicit THaScintillator( const char* name, const char* description = "",
                            THaApparatus* a = nullptr );
  THaScintillator();
  virtual ~THaScintillator();

  virtual void       Clear( Option_t* opt="" );
  virtual Int_t      Decode( const THaEvData& );
  virtual Int_t      CoarseProcess( TClonesArray& tracks );
  virtual Int_t      FineProcess( TClonesArray& tracks );

  virtual Int_t      ApplyCorrections();

  Int_t GetNHits() const { return fHits.size(); }

protected:

  static const Int_t NSIDES = 2;
  struct PMTCalib_t {
    PMTCalib_t() : off(0.f), ped(0.f), gain(1.f) {}
    void clear() { off = 0.f; ped = 0.f; gain = 1.f; }
    Float_t off;
    Float_t ped;
    Float_t gain;
  };
  struct PMTData_t {
    PMTData_t()
      : tdc(kMinInt), tdc_c(kBig), adc(kMinInt), adc_p(kBig), adc_c(kBig),
        tdc_set(false), adc_set(false) {}
    void clear() { tdc = adc = kMinInt; tdc_c = adc_p = adc_c = kBig;
      tdc_set = adc_set = false; }
    Int_t   tdc;
    Float_t tdc_c;
    Int_t   adc;
    Float_t adc_p;
    Float_t adc_c;
    Bool_t  tdc_set;
    Bool_t  adc_set;
  };
  struct HitCount_t {
    HitCount_t() : tdc(0), adc(0) {}
    void clear() { tdc = adc = 0; }
    Int_t   tdc;
    Int_t   adc;
  };
  struct HitData_t {
    Int_t   pad;
    Float_t time;
    Float_t dtime;
    Float_t ampl;
    Float_t yt;
    Float_t ya;
  };

  // Calibration
  std::vector<PMTCalib_t> fCalib[NSIDES];

  Float_t    fTdc2T;      // linear conversion between TDC and time (s/ch)
  Float_t    fCn;         // speed of light in material  (meters/second)

  Int_t      fNTWalkPar;  // number of timewalk correction parameters
  Float_t*   fTWalkPar;   // [fNTWalkPar] time walk correction parameters
  Float_t    fAdcMIP;     // nominal ADC above pedestal for MIP

  Float_t*   fTrigOff;     // [fNelem] Induced offset of trigger time from
			   // diff between trigger and retiming.
			   // Visible in coincidence data.

  Float_t    fAttenuation; // in m^-1: attenuation length of material
  Float_t    fResolution;  // average time resolution per PMT (s)

  // Per-event data
  HitCount_t             fNHits[NSIDES];
  std::vector<PMTData_t> fRightPMTs;
  std::vector<PMTData_t> fLeftPMTs;
  std::vector<HitData_t> fHits;

//  Int_t       fLTNhit;     // Number of Left paddles with TDC hits
//  Double_t*   fLT;         // [fNelem] Left paddles TDC times (channels)
//  Double_t*   fLT_c;       // [fNelem] Left PMT corrected TDC times (s)
//  Int_t       fRTNhit;     // Number of Right paddles with TDC signals
//  Double_t*   fRT;         // [fNelem] Right paddles TDC times (channels)
//  Double_t*   fRT_c;       // [fNelem] Right PMT corrected TDC times (s)
//  Int_t       fLANhit;     // Number of Left paddles with ADC hits
//  Double_t*   fLA;         // [fNelem] Left paddles ADC amplitudes
//  Double_t*   fLA_p;       // [fNelem] Left paddles ADC minus ped values
//  Double_t*   fLA_c;       // [fNelem] Left paddles corrected ADC ampl-s
//  Int_t       fRANhit;     // Number of Right paddles with ADC hits
//  Double_t*   fRA;         // [fNelem] Right paddles ADC amplitudes
//  Double_t*   fRA_p;       // [fNelem] Right paddles ADC minus ped values
//  Double_t*   fRA_c;       // [fNelem] Right paddles corrected ADC ampl-s
//
//
//  Int_t       fNhit;       // Number of paddles with complete TDC hits (l&r)
//  Int_t*      fHitPad;     // [fNhit] list of paddles with complete TDC hits
//
//  // could be done on a per-hit basis instead
//  Double_t*   fTime;       // [fNhit] corrected time for the paddle (s)
//  Double_t*   fdTime;      // [fNhit] uncertainty in time (s)
//  Double_t*   fAmpl;       // [fNhit] overall amplitude for the paddle
//  Double_t*   fYt;         // [fNhit] y-position of hit in paddle from TDC (m)
//  Double_t*   fYa;         // [fNhit] y-position of hit in paddle from ADC (m)

  void           DeleteArrays();
  virtual Int_t  ReadDatabase( const TDatime& date );
  virtual Int_t  DefineVariables( EMode mode = kDefine );

  enum ESide { kRight = 0, kLeft = 1 };

  virtual  Float_t TimeWalkCorrection( Int_t paddle, ESide side );

  ClassDef(THaScintillator,1)   // Generic scintillator class
};

////////////////////////////////////////////////////////////////////////////////

#endif
