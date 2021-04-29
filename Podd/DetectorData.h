//////////////////////////////////////////////////////////////////////////
//
// Podd::DetectorData
//
// Generic detector data (ADCs and TDCs)
//
//////////////////////////////////////////////////////////////////////////

#ifndef PODD_DETECTORDATA_H
#define PODD_DETECTORDATA_H

#include "TNamed.h"
#include "DataType.h"  // for Data_t
#include "THaAnalysisObject.h"   // for enums (EMode etc.)
#include "VarDef.h"    // for RVarDef
#include "THaDetMap.h"
#include <string>

// Silence rootcling warnings from ClassDef macros
#ifdef __clang__
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
#endif

namespace Podd {

//_____________________________________________________________________________
class DetectorData : public TNamed {
public:
  DetectorData() = delete;

  virtual Int_t  StoreHit( const DigitizerHitInfo_t& hitinfo, UInt_t data ) = 0;
  virtual Int_t  GetLogicalChannel( const DigitizerHitInfo_t& hitinfo ) const;
  virtual UInt_t GetSize() const = 0;

  void          Clear( Option_t* ="" ) override;
  virtual void  Reset( Option_t* ="" ) {};
  Int_t         DefineVariables(
    THaAnalysisObject::EMode mode = THaAnalysisObject::kDefine,
    const char* key_prefix = "",
    const char* comment_subst = "" );

  Bool_t        HitDone() const { return fHitDone; }
  Bool_t        IsSetup() const { return fVarOK; }
  void          ClearHitDone()  { fHitDone = false; }

protected:
  // Only derived classes may construct
  DetectorData( const char* name, const char* desc );
  // For exception message formatting
  static std::string msg( const DigitizerHitInfo_t& hitinfo, const char* txt );
  // Actual DefineVariables implementation, called from DefineVariables
  virtual Int_t DefineVariablesImpl(
    THaAnalysisObject::EMode mode = THaAnalysisObject::kDefine,
    const char* key_prefix = "",
    const char* comment_subst = "" );
  // Standard DefineVariables implementation
  Int_t StdDefineVariables( const RVarDef* vars, THaAnalysisObject::EMode mode,
                            const char* key_prefix, const char* here,
                            const char* comment_subst );

  Bool_t  fVarOK;   // Global variables are set up
  Bool_t  fHitDone; // StoreHit called for current hit

  ClassDef(DetectorData, 1)  // Base class for detector raw data
};

//_____________________________________________________________________________
// ADC info
struct ADCCalib_t {
  ADCCalib_t() : ped(0), gain(1), mip(1e10), twalk(0) {}
  void adc_calib_reset() { ped = 0; gain = 1; mip = 1e10; twalk = 0; }
  Data_t ped;       // ADC pedestal (channels)
  Data_t gain;      // ADC gain (a.u.)
  Data_t mip;       // ADC MIP for twalk correction (channels)
  Data_t twalk;     // Timewalk correction coefficient
};

struct ADCData_t {
  ADCData_t()
    : adc(kMaxUInt), adc_p(kBig), adc_c(kBig), nadc(0) {}
  void adc_clear() { adc = kMinInt; adc_p = adc_c = kBig; nadc = 0; }
  UInt_t adc;       // Raw ADC amplitude (channels)
  Data_t adc_p;     // Pedestal-subtracted ADC amplitude
  Data_t adc_c;     // Gain-corrected ADC amplitude
  UInt_t nadc;      // Number of hits on this ADC
};

// TDC info
struct TDCCalib_t {
  TDCCalib_t() : tdc2t(5e-10), off(0) {}
  void tdc_calib_reset() { tdc2t = 5e-10; off = 0; }
  Data_t tdc2t;     // Conversion coefficient from raw TDC to time (s/ch)
  Data_t off;       // TDC offset (channels)
};

struct TDCData_t {
  TDCData_t() : tdc(kMaxUInt), tdc_c(kBig), ntdc(0) {}
  void tdc_clear() { tdc = kMinInt; tdc_c = kBig; ntdc = 0; }
  UInt_t tdc;       // Raw TDC time (channels)
  Data_t tdc_c;     // Converted TDC time, corrected for offset (s)
  UInt_t ntdc;      // Number of hits on this TDC
};

// ADC and TDC combined
struct PMTCalib_t : public ADCCalib_t, public TDCCalib_t {
  PMTCalib_t() : ADCCalib_t(), TDCCalib_t() {}
  void reset() { adc_calib_reset(); tdc_calib_reset(); }
};

struct PMTData_t : public ADCData_t, public TDCData_t {
  PMTData_t() : ADCData_t(), TDCData_t() {}
  void clear() { adc_clear(); tdc_clear(); }
};

struct HitCount_t {
  HitCount_t() : tdc(0), adc(0) {}
  void clear() { tdc = adc = 0; }
  // These counters indicate for how many TDC/ADC channels, respectively,
  // any data was found in the raw data stream, even if uninteresting
  // (below pedestal etc.)
  UInt_t tdc;     // Count of TDCs with one or more hits
  UInt_t adc;     // Count of ADCs with one or more hits
};

//_____________________________________________________________________________
class ADCData : public DetectorData {
public:
  ADCData( const char* name, const char* desc, UInt_t nelem );

  Int_t       StoreHit( const DigitizerHitInfo_t& hitinfo,
                        UInt_t data ) override;

  void        Clear( Option_t* ="" ) override;
  void        Reset( Option_t* ="" ) override;

  UInt_t      GetHitCount() const      { return fNHits; }
  UInt_t      GetSize() const override { return fCalib.size(); }
#ifdef NDEBUG
  ADCCalib_t& GetCalib( size_t i )     { return fCalib[i]; }
  ADCData_t&  GetADC( size_t i )       { return fADCs[i]; }
#else
  ADCCalib_t& GetCalib( size_t i )     { return fCalib.at(i); }
  ADCData_t&  GetADC( size_t i )       { return fADCs.at(i); }
#endif
  ADCCalib_t& GetCalib( const DigitizerHitInfo_t& hitinfo )
  { return GetCalib(GetLogicalChannel(hitinfo)); }
  ADCData_t&  GetADC( const DigitizerHitInfo_t& hitinfo )
  { return GetADC(GetLogicalChannel(hitinfo)); }

protected:
  // Calibration
  std::vector<ADCCalib_t> fCalib;   // Calibration constants

  // Per-event data
  std::vector<ADCData_t>  fADCs;    // ADC data
  UInt_t                  fNHits;   // Number of ADCs with signal

  Int_t DefineVariablesImpl(
    THaAnalysisObject::EMode mode = THaAnalysisObject::kDefine,
    const char* key_prefix = "",
    const char* comment_subst = "" ) override;

  ClassDef(ADCData, 1)  // ADC raw data
};

//_____________________________________________________________________________
class PMTData : public DetectorData {
public:
  PMTData( const char* name, const char* desc, UInt_t nelem );

  Int_t       StoreHit( const DigitizerHitInfo_t& hitinfo,
                        UInt_t data ) override;

  void        Clear( Option_t* ="" ) override;
  void        Reset( Option_t* ="" ) override;

  HitCount_t& GetHitCount()            { return fNHits; }
  UInt_t      GetSize() const override { return fCalib.size(); }
#ifdef NDEBUG
  PMTCalib_t& GetCalib( size_t i ) { return fCalib[i]; }
  PMTData_t&  GetPMT( size_t i )   { return fPMTs[i]; }
#else
  PMTCalib_t& GetCalib( size_t i ) { return fCalib.at(i); }
  PMTData_t&  GetPMT( size_t i )   { return fPMTs.at(i); }
#endif
  PMTCalib_t& GetCalib( const DigitizerHitInfo_t& hitinfo )
  { return GetCalib(GetLogicalChannel(hitinfo)); }
  PMTData_t&  GetPMT( const DigitizerHitInfo_t& hitinfo )
  { return GetPMT(GetLogicalChannel(hitinfo)); }

protected:
  // Calibration
  std::vector<PMTCalib_t> fCalib;   // Calibration constants

  // Per-event data
  std::vector<PMTData_t>  fPMTs;    // PMT data (ADCs & TDCs)
  HitCount_t              fNHits;   // Number of ADCs/TDCs with signal

  Int_t DefineVariablesImpl(
    THaAnalysisObject::EMode mode = THaAnalysisObject::kDefine,
    const char* key_prefix = "",
    const char* comment_subst = "" ) override;

  ClassDef(PMTData, 1)  // Photomultiplier tube raw data (ADC & TDC)
};

} // namespace Podd

#endif //PODD_DETECTORDATA_H
