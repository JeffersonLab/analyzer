//*-- Author :    Ole Hansen   25-Mar-21
//
///////////////////////////////////////////////////////////////////////////
//
// Podd::ChannelData
//
// Abstract base class and basic implementations for data from DAQ readout
// channels.
//
// ChannelData holds information for all readout channels of the same type
// that are associated with a detector. For example, if a detector is
// configured to use data from 3 identical ADC modules with 12 channels
// each, these channels can be represented by a single ChannelData object
// of "size" (number of elements) 3 x 12 = 36. The ChannelData object holds
//
// - configuration and calibration constants for each channel.
// - a vector of event-by-event data, whose size is the number of hits seen
//   in all modules for each event.
//
///////////////////////////////////////////////////////////////////////////

#ifndef PODD_CHANNELDATA_H
#define PODD_CHANNELDATA_H

#include "TNamed.h"             // for kMaxUInt, UInt_t, Int_t, Option_t, ClassDefOverride, TNamed
#include "DataType.h"           // for kBig, Data_t
#include "THaAnalysisObject.h"  // for THaAnalysisObject, enums (EMode etc.)
#include "THaDetMap.h"          // for DigitizerHitInfo_t
#include <memory>               // for unique_ptr, make_unique
#include <string>               // for string
#include <type_traits>          // for is_same_v
#include <utility>              // for pair, move
#include <vector>               // for vector

class TDatime;
class THaDetectorBase;
struct RVarDef;

namespace Podd {

//_____________________________________________________________________________
class ChannelData : public TNamed {
public:
  using THaAnalysisObject::kFileError, THaAnalysisObject::kOK;
  using THaAnalysisObject::kDefine, THaAnalysisObject::kDelete;

  ChannelData() = delete;

  virtual Int_t  StoreHit( const DigitizerHitInfo_t& hitinfo, UInt_t data ) = 0;
  virtual Int_t  GetLogicalChannel( const DigitizerHitInfo_t& hitinfo ) const;
  virtual UInt_t GetSize() const = 0;

  void          Clear( Option_t* ="" ) override;
  virtual void  Reset( Option_t* ="" ) {};
  virtual Int_t ReadConfig( THaDetectorBase* det, const TDatime& date,
                            const char* key_prefix = nullptr );
  Int_t         DefineVariables( THaAnalysisObject::EMode mode = kDefine,
                                 const char* key_prefix = "",
                                 const char* comment_subst = "" );

  bool          HitDone() const { return fHitDone; }
  bool          IsInit()  const { return fIsInit; }
  bool          IsSetup() const { return fVarOK; }
  void          ClearHitDone()  { fHitDone = false; }

protected:
  // Only derived classes may construct
  ChannelData( const char* name, const char* desc );
  // For exception message formatting
  static std::string msg( const DigitizerHitInfo_t& hitinfo, const char* txt );
  // Actual DefineVariables implementation, called from DefineVariables
  virtual Int_t DefineVariablesImpl( THaAnalysisObject::EMode mode = kDefine,
                                     const char* key_prefix = "",
                                     const char* comment_subst = "" );
  // Standard DefineVariables implementation
  Int_t StdDefineVariables( const RVarDef* vars, THaAnalysisObject::EMode mode,
                            const char* key_prefix, const char* here,
                            const char* comment_subst );

  bool  fIsInit;  // ReadDatabase successfully done
  bool  fVarOK;   // Global variables are set up
  bool  fHitDone; // StoreHit called for current hit

  ClassDefOverride(ChannelData, 0)  // Base class for detector raw data
};

//_____________________________________________________________________________
// ADC info

// There's apparently no consensus in the literature about the units of ADC
// values. One finds "counts", "channels" (HEP/NP), "codes" (LeCroy), "LSBs",
// "ADUs", "quants" and more. We'll use "counts" here, even though many ADCs
// don't actually count.

// Calibration coefficients for a single logical channel
struct ADCCalib_t {
  void adc_calib_reset() { gain = 1; ped = mip = 0; twalk = 0; }
  Data_t gain   {1};         // ADC gain calibration (counts -> arb.u. e.g. npe)
  UInt_t ped    {0};         // Static ADC pedestal (counts)
  UInt_t mip    {0};         // Reference ADC value (adc_p) for twalk correction
  Data_t twalk  {0};         // Timewalk correction coefficient (s * sqrt(counts))
//Data_t gate   {0};         // Integration gate (s) -> provided by decoder module
//Data_t cal    {1};         // Conversion factor to charge (pC/count) -> provided by decoder module
};

// Per-event data of a single ADC hit
struct ADCHit_t {
  UInt_t chan   {kMaxUInt};  // Logical channel number of this hit
  UInt_t flag   {0};         // Status bits (application-dependent, e.g. overflow)
  UInt_t adc    {0};         // Pulse integral (counts)
  Int_t  adc_p  {0};         // Integral (adc) minus pedestal (counts)
  Data_t adc_c  {kBig};      // Calibrated ADC: adc_p scaled by gain factor
};

// TDC info

// Calibration coefficients for a single logical channel
struct TDCCalib_t {
  void tdc_calib_reset() { off = cutlo = 0; cuthi = kMaxUInt; }
//Data_t tdc2t  {5e-10};     // TDC scale (s/count) - provided by decoder module
  UInt_t off    {0};         // TDC offset (counts)
  UInt_t cutlo  {0};         // Time cut lower limit (counts)
  UInt_t cuthi  {kMaxUInt};  // Time cut upper limit (counts)
};

// A single structure for both single and dual edge TDC hit data wastes 8 bytes
// per hit for single edge TDCs. Optimizing this would cost more than it gains.

// Per-event data of a single TDC hit (single or dual edge)
struct TDCHit_t {
  UInt_t chan   {kMaxUInt};  // Logical channel number of this hit
  UInt_t flag   {0};         // Status bits (application-dependent)
  UInt_t tdc    {0};         // Leading-edge TDC reading (counts)
  UInt_t tdc_t  {0};         // Trailing-edge TDC reading (counts)
  Data_t tdc_c  {kBig};      // Converted leading edge time corrected for offset (s)
  Data_t tot    {kBig};      // Time-over-threshold (s)
};

struct HitCount_t {
  void clear() { any = all = good = 0; }
  void reset() { clear(); sum_any = sum_all = sum_good = 0; }
  UInt_t    any      {0};    // One or more hits in this event (coverage)
  UInt_t    all      {0};    // Total number of hits in this event (multiplicity)
  UInt_t    good     {0};    // Number of "good" hits (application-defined)
  ULong64_t sum_any  {0};    // Sum of "any" over all events
  ULong64_t sum_all  {0};    // Sum of "all" over all events
  ULong64_t sum_good {0};    // Sum of "good" over all events
};

//_____________________________________________________________________________
class ADCData : public ChannelData {
public:
  ADCData( const char* name, const char* desc, UInt_t nelem );
  explicit ADCData( const THaDetectorBase* det );

  Int_t       StoreHit( const DigitizerHitInfo_t& hitinfo,
                        UInt_t data ) override;

  void        Clear( Option_t* ="" ) override;
  void        Reset( Option_t* ="" ) override;
  Int_t       ReadConfig( THaDetectorBase* det, const TDatime& date,
                          const char* key_prefix = nullptr ) override;

  UInt_t      GetHitCount() const      { return fNHits; }
  UInt_t      GetSize() const override { return fCalib.size(); }
#ifdef NDEBUG
  ADCCalib_t& GetCalib( size_t i )     { return fCalib[i]; }
  ADCHit_t&  GetADC( size_t i )       { return fADCs[i]; }
#else
  ADCCalib_t& GetCalib( size_t i )     { return fCalib.at(i); }
  ADCHit_t&  GetADC( size_t i )       { return fADCs.at(i); }
#endif
  ADCCalib_t& GetCalib( const DigitizerHitInfo_t& hitinfo )
  { return GetCalib(GetLogicalChannel(hitinfo)); }
  ADCHit_t&  GetADC( const DigitizerHitInfo_t& hitinfo )
  { return GetADC(GetLogicalChannel(hitinfo)); }

protected:
  // Calibration
  std::vector<ADCCalib_t> fCalib;   // [nelem] Calibration constants

  // Per-event data
  std::vector<ADCHit_t>  fADCs;    // ADC data
  UInt_t                  fNHits;   // Number of ADCs with signal

  Int_t DefineVariablesImpl( THaAnalysisObject::EMode mode = kDefine,
                             const char* key_prefix = "",
                             const char* comment_subst = "" ) override;

  ClassDefOverride(ADCData, 0)  // ADC raw data
};

//_____________________________________________________________________________
class TDCData : public ChannelData {
public:
  TDCData( const char* name, const char* desc, UInt_t nelem );
  explicit TDCData( const THaDetectorBase* det );

  Int_t       StoreHit( const DigitizerHitInfo_t& hitinfo,
                        UInt_t data ) override;

  void        Clear( Option_t* ="" ) override;
  void        Reset( Option_t* ="" ) override;
  Int_t       ReadConfig( THaDetectorBase* det, const TDatime& date,
                          const char* key_prefix = nullptr ) override;

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

  Int_t DefineVariablesImpl( THaAnalysisObject::EMode mode = kDefine,
                             const char* key_prefix = "",
                             const char* comment_subst = "" ) override;

  ClassDefOverride(TDCData, 0)  // Photomultiplier tube raw data (ADC & TDC)
};

//_____________________________________________________________________________
// Create and initialize a ChannelData object to be used with detector 'det'.
// Explicit instantiations provided in implementation files.
template<class ChanDat>
std::pair<std::unique_ptr<ChanDat>, Int_t>
MakeChanDat( const TDatime& date, THaDetectorBase* det )
{
  // Instantiate a ChanDat object and read its configuration from the database.
  // 'date' is the initialization time, 'det', the owning detector class.
  // Return value:
  //  ret.first:   unique_ptr to ChanDat object
  //  ret.second:  return value from ReadConfig (a ReadDatabase EStatus)

  using THaAnalysisObject::kFileError, THaAnalysisObject::kOK;

  std::pair<std::unique_ptr<ChanDat>, Int_t> ret{nullptr, kFileError};
  if( !det )
    return ret;

  // Create new ChanDat object
  auto chandat = std::make_unique<ChanDat>(det);

  // Configure ChanDat using the detector's database. However, don't bother
  // if the concrete ChanDat class doesn't implement its own database reader.
  if constexpr( not std::is_same_v
    <decltype(&ChanDat::ReadConfig), decltype(&Podd::ChannelData::ReadConfig)> )
  {
    auto err = chandat->ReadConfig(det, date, nullptr);
    if( err != kOK ) {
      ret.second = err;
      return ret;
    }
  }
  return {std::move(chandat), kOK};
}

} // namespace Podd

#endif //PODD_CHANNELDATA_H
