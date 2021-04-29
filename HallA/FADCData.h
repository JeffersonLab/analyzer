//////////////////////////////////////////////////////////////////////////
//
// HallA::FADCData
//
// Support for data from JLab FADC250 modules
//
//////////////////////////////////////////////////////////////////////////

#ifndef HALLA_FADCDATA_H
#define HALLA_FADCDATA_H

#include "DetectorData.h"
#include "DataType.h"  // for Data_t
#include "OptionalType.h"
#include "THaAnalysisObject.h"   // for enums (EMode etc.)
#include "THaDetMap.h"
#include <memory>
#include <utility>

class TDatime;
class THaDetectorBase;

namespace HallA {

//_____________________________________________________________________________
class FADCData_t {
public:
  //TODO: init/clear to kBig?
  FADCData_t() : fIntegral(0), fPeak(0), fT(0), fT_c(0),
                 fOverflow(0), fUnderflow(0), fPedq(0), fPedestal(0) {}
  void clear() {
    // FIXME: init some of these to kBig?
    fIntegral = fPeak = fT = fT_c = fPedestal = 0.0;
    fOverflow = fUnderflow = fPedq = 0;
  }
  Data_t fIntegral;  // ADC peak integral
  Data_t fPeak;      // ADC peak value
  Data_t fT;         // TDC time (channels)
  Data_t fT_c;       // Offset-corrected TDC time (s)
  UInt_t fOverflow;  // FADC overflow bit
  UInt_t fUnderflow; // FADC underflow bit
  UInt_t fPedq;      // FADC pedestal quality bit
  Data_t fPedestal;  // Extracted pedestal value
};

// Calibration
static const Data_t kDefaultTDCscale = 0.0625;
class FADCConfig_t {
public:
  FADCConfig_t() : nped(1), nsa(1), nsb(1), win(1), tflag(true),
                   tdcscale(kDefaultTDCscale) {}
  void reset() {
    nped = nsa = nsb = win = 1;
    tflag = true;
    tdcscale = kDefaultTDCscale;
  }
  Int_t  nped;       // Number of samples included in FADC pedestal sum
  Int_t  nsa;        // Number of integrated samples after threshold crossing
  Int_t  nsb;        // Number of integrated samples before threshold crossing
  Int_t  win;        // Total number of samples in FADC window
  Bool_t tflag;      // If true, threshold on
  Data_t tdcscale;   // TDC scaling factor
};

//_____________________________________________________________________________
class FADCData : public Podd::DetectorData {
public:
  FADCData( const char* name, const char* desc, Int_t nelem );

  static OptUInt_t LoadFADCData( const DigitizerHitInfo_t& hitinfo );
  Int_t StoreHit( const DigitizerHitInfo_t& hitinfo, UInt_t data ) override;

  void  Clear( Option_t* ="" ) override;
  void  Reset( Option_t* ="" ) override;

  UInt_t          GetSize() const override { return fFADCData.size(); }
  FADCConfig_t&   GetConfig()              { return fConfig; }
#ifdef NDEBUG
  FADCData_t&     GetData( size_t i )      { return fFADCData[i]; }
#else
  FADCData_t&     GetData( size_t i )      { return fFADCData.at(i); }
#endif
  FADCData_t&     GetData( const DigitizerHitInfo_t& hitinfo )
  { return GetData(GetLogicalChannel(hitinfo)); }

  Int_t ReadConfig( FILE* file, const TDatime& date, const char* prefix );

protected:
  // Configuration for this module or group of modules
  FADCConfig_t              fConfig;    // FADC configuration parameters

  // Per-event data
  std::vector<FADCData_t>   fFADCData;  // FADC per-event readout data

  Int_t DefineVariablesImpl(
    THaAnalysisObject::EMode mode = THaAnalysisObject::kDefine,
    const char* key_prefix = "",
    const char* comment_subst = "" ) override;

  ClassDef(FADCData,1)  // FADC raw data
};

//_____________________________________________________________________________
// Create and initialize an FADCData object to be used with detector 'det'
std::pair<std::unique_ptr<FADCData>, Int_t>
MakeFADCData( const TDatime& date, THaDetectorBase* det );

//_____________________________________________________________________________
} // namespace HallA

#endif //HALLA_FADCDATA_H
