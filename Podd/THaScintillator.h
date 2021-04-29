#ifndef Podd_THaScintillator_h_
#define Podd_THaScintillator_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaScintillator                                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaNonTrackingDetector.h"
#include "DetectorData.h"
#include <vector>
#include <set>

class TClonesArray;

class THaScintillator : public THaNonTrackingDetector {

public:
  explicit THaScintillator( const char* name, const char* description = "",
                            THaApparatus* a = nullptr );
  THaScintillator();
  virtual ~THaScintillator();

  enum ESide { kNone = -1, kRight = 0, kLeft = 1 };
  using Idx_t  = std::pair<ESide,Int_t>;

  class HitData_t {
  public:
    // A "hit" is defined as a paddle with TDC hits on both sides
    HitData_t()
      : pad(-1), time(kBig), dtime(kBig), yt(kBig), ya(kBig), ampl(kBig) {}
    HitData_t( Int_t pad, Data_t time, Data_t dtime, Data_t yt, Data_t ya, Data_t ampl )
      : pad(pad), time(time), dtime(dtime), yt(yt), ya(ya), ampl(ampl) {}
    void clear() { time = dtime = yt = ampl = ya = kBig; }
    bool operator<( const HitData_t& rhs ) const { return time < rhs.time; }
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

  Int_t             GetNHits() const  { return static_cast<Int_t>(fHits.size()); }
  const HitData_t&  GetHit( Int_t i ) { return fHits[i]; }
  const HitData_t&  GetPad( Int_t i ) { return fPadData[i]; }

protected:

  // Calibration
  Data_t    fCn;          // speed of light in material (m/s)
  Data_t    fAttenuation; // attenuation length of material (1/m)
  Data_t    fResolution;  // average time resolution per PMT (s)

  // Per-event data
  Podd::PMTData*         fRightPMTs;      // Raw PMT data (right side)
  Podd::PMTData*         fLeftPMTs;       // Raw PMT data (left side)
  std::set<Idx_t>        fHitIdx;         // Indices of PMTs with data
  std::vector<HitData_t> fHits;           // Calculated hit data, per hit
  // fPadData duplicates the info in fHits for direct access via paddle number
  std::vector<HitData_t> fPadData;        // Calculated hit data, per paddle

  virtual Int_t  StoreHit( const DigitizerHitInfo_t& hitinfo, UInt_t data );
  virtual void   PrintDecodedData( const THaEvData& evdata ) const;

  virtual Int_t  ApplyCorrections();
  virtual Data_t TimeWalkCorrection( Idx_t idx, Data_t adc );
  virtual Int_t  FindPaddleHits();

  virtual Int_t  ReadDatabase( const TDatime& date );
  virtual Int_t  DefineVariables( EMode mode = kDefine );

  ClassDef(THaScintillator,1)   // Generic scintillator class
};

////////////////////////////////////////////////////////////////////////////////

#endif
