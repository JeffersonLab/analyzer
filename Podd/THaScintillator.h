#ifndef Podd_THaScintillator_h_
#define Podd_THaScintillator_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaScintillator                                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaNonTrackingDetector.h"
#include <vector>
#include <set>
#include <utility>

class TClonesArray;

class THaScintillator : public THaNonTrackingDetector {

public:
  explicit THaScintillator( const char* name, const char* description = "",
                            THaApparatus* a = nullptr );
  THaScintillator();
  virtual ~THaScintillator();


  enum ESide { kRight = 0, kLeft = 1 };
  using Idx_t  = std::pair<ESide,Int_t>;
  using Data_t = Float_t;  // Data type for physics quantities. Must be Float_t or Double_t

  struct PMTCalib_t {
    PMTCalib_t() : off(0.f), ped(0.f), gain(1.f) {}
    void clear() { off = 0.f; ped = 0.f; gain = 1.f; }
    Data_t off;
    Data_t ped;
    Data_t gain;
  };
  struct PMTData_t {
    PMTData_t()
      : tdc(kMinInt), tdc_c(kBig), adc(kMinInt), adc_p(kBig), adc_c(kBig),
        tdc_set(false), adc_set(false) {}
    void clear() { tdc = adc = kMinInt; tdc_c = adc_p = adc_c = kBig;
      tdc_set = adc_set = false; }
    Int_t  tdc;
    Data_t tdc_c;
    Int_t  adc;
    Data_t adc_p;
    Data_t adc_c;
    Bool_t tdc_set;
    Bool_t adc_set;
  };
  struct HitCount_t {
    HitCount_t() : tdc(0), adc(0) {}
    void clear() { tdc = adc = 0; }
    Int_t   tdc;
    Int_t   adc;
  };
  struct HitData_t {
    HitData_t()
      : pad(-1), time(kBig), dtime(kBig), yt(kBig), ya(kBig), ampl(kBig) {}
    HitData_t( Int_t pad, Data_t time, Data_t dtime, Data_t yt, Data_t ya, Data_t ampl )
      : pad(pad), time(time), dtime(dtime), yt(yt), ya(ya), ampl(ampl) {}
    void clear() { time = dtime = yt = ampl = ya = kBig; }
    Int_t  pad;
    Data_t time;
    Data_t dtime;
    Data_t yt;
    Data_t ya;
    Data_t ampl;
  };

  virtual void      Clear( Option_t* opt="" );
  virtual Int_t     Decode( const THaEvData& );
  virtual Int_t     CoarseProcess( TClonesArray& tracks );
  virtual Int_t     FineProcess( TClonesArray& tracks );

  Int_t             GetNHits() const { return fHits.size(); }
  const HitData_t&  GetHit( Int_t i ) { return fHits[i]; }
  const HitData_t&  GetPad( Int_t i ) { return fPadData[i]; }
  const PMTData_t&  GetPMT( ESide side, Int_t pad ) {
    return (side==kRight) ? fRightPMTs[pad] : fLeftPMTs[pad];
  }
protected:

  static const Int_t NSIDES = 2;

  // Calibration
  std::vector<PMTCalib_t> fCalib[NSIDES];

  Data_t    fTdc2T;      // linear conversion between TDC and time (s/ch)
  Data_t    fCn;         // speed of light in material  (meters/second)

  Int_t     fNTWalkPar;  // number of timewalk correction parameters
  Data_t*   fTWalkPar;   // [fNTWalkPar] time walk correction parameters
  Data_t    fAdcMIP;     // nominal ADC above pedestal for MIP

  Data_t*   fTrigOff;     // [fNelem] Induced offset of trigger time from
			   // diff between trigger and retiming.
			   // Visible in coincidence data.

  Data_t    fAttenuation; // in m^-1: attenuation length of material
  Data_t    fResolution;  // average time resolution per PMT (s)

  // Per-event data
  HitCount_t             fNHits[NSIDES];
  std::set<Idx_t>        fHitIdx;
  std::vector<PMTData_t> fRightPMTs;
  std::vector<PMTData_t> fLeftPMTs;
  std::vector<HitData_t> fPadData;
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

  virtual Int_t  ApplyCorrections();
  virtual Data_t TimeWalkCorrection( Idx_t idx, Data_t adc );
  virtual Int_t  FindPaddleHits();

  ClassDef(THaScintillator,1)   // Generic scintillator class
};

////////////////////////////////////////////////////////////////////////////////

#endif
