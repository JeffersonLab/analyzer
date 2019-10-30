#ifndef Podd_THaScintillator_h_
#define Podd_THaScintillator_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaScintillator                                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaNonTrackingDetector.h"
#include "THaDetMap.h"
#include <vector>
#include <set>
#include <utility>

namespace Decoder {
  class Module;
}

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
    Data_t off;       // TDC offset (channels)
    Data_t ped;       // ADC pedestal (channels)
    Data_t gain;      // ADC gain (a.u.)
  };

  struct PMTData_t {
    PMTData_t()
      : tdc(kMinInt), tdc_c(kBig), adc(kMinInt), adc_p(kBig), adc_c(kBig),
        ntdc(0), nadc(0) {}
    void clear() { tdc = adc = kMinInt; tdc_c = adc_p = adc_c = kBig;
      ntdc = nadc = 0; }
    Int_t  tdc;       // Raw TDC time (channels)
    Data_t tdc_c;     // Converted TDC time, corrected for offset (s)
    Int_t  adc;       // Raw ADC amplitude (channels)
    Data_t adc_p;     // Pedestal-subtracted ADC amplitude
    Data_t adc_c;     // Gain-corrected ADC amplitude
    Int_t  ntdc;      // Number of TDC hits
    Int_t  nadc;      // Number of ADC hits

  };
  struct HitCount_t {
    HitCount_t() : tdc(0), adc(0) {}
    void clear() { tdc = adc = 0; }
    // These counters indicate for how many TDC/ADC channels, respectively,
    // any data was found in the raw data stream, even if uninteresting
    // (below pedestal etc.)
    Int_t   tdc;      // Count of TDCs with one or more hits
    Int_t   adc;      // Count of ADCs with one or more hits
  };

  struct HitData_t {
    // A "hit" is defined as a paddle with TDC hits on both sides
    HitData_t()
      : pad(-1), time(kBig), dtime(kBig), yt(kBig), ya(kBig), ampl(kBig) {}
    HitData_t( Int_t pad, Data_t time, Data_t dtime, Data_t yt, Data_t ya, Data_t ampl )
      : pad(pad), time(time), dtime(dtime), yt(yt), ya(ya), ampl(ampl) {}
    void clear() { time = dtime = yt = ampl = ya = kBig; }
    Int_t  pad;      // Paddle number
    Data_t time;     // Average of corrected TDC times (tdc_c) (s)
    Data_t dtime;    // Uncertainty of average time (s)
    Data_t yt;       // Transverse track position from TDC time difference (m)
    Data_t ya;       // Transverse track position from ADC amplitude ratio (m)
    Data_t ampl;     // Estimated energy deposition dE/dx (a.u.)
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
  Data_t    fCn;         // speed of light in material  (m/s)
  Data_t    fAdcMIP;     // nominal ADC above pedestal for MIP
  Data_t    fAttenuation; // attenuation length of material (1/m)
  Data_t    fResolution;  // average time resolution per PMT (s)
  std::vector<Data_t> fTWalkPar[NSIDES]; // timewalk correction parameters (s)

  // Per-event data
  HitCount_t             fNHits[NSIDES];  // Number of active ADCs/TDCs on each side
  std::set<Idx_t>        fHitIdx;         // Indices of PMTs with data
  // The PMT data are stored in two vectors because the global variable system
  // currently cannot handle vectors of vectors of structures.
  std::vector<PMTData_t> fRightPMTs;      // Raw PMT data (right side)
  std::vector<PMTData_t> fLeftPMTs;       // Raw PMT data (left side)
  // fPadData duplicates the info in fHits for direct access via paddle number
  std::vector<HitData_t> fPadData;        // Calculated hit data, per paddle
  std::vector<HitData_t> fHits;           // Calculated hit data, per hit

  Decoder::Module*       fModule;  // Hardware module currently being decoded

  using OptInt_t = std::pair<Int_t,bool>;
  using DetMapItem = THaDetMap::Module;
  virtual void     SetupModule( const THaEvData& evdata, DetMapItem* pModule );
  virtual OptInt_t LoadData( const THaEvData& evdata, DetMapItem* pModule,
    Bool_t adc, Int_t chan, Int_t hit, Int_t pad, ESide side );
  virtual Int_t    ApplyCorrections();
  virtual Data_t   TimeWalkCorrection( Idx_t idx, Data_t adc );
  virtual Int_t    FindPaddleHits();

  virtual Int_t    ReadDatabase( const TDatime& date );
  virtual Int_t    DefineVariables( EMode mode = kDefine );

  ClassDef(THaScintillator,1)   // Generic scintillator class
};

////////////////////////////////////////////////////////////////////////////////

#endif
