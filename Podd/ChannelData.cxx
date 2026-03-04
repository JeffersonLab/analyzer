//*-- Author :    Ole Hansen   25-Mar-21
//
//////////////////////////////////////////////////////////////////////////
//
// Podd::ChannelData
//
//////////////////////////////////////////////////////////////////////////

#include "ChannelData.h"
#include "DataType.h"           // for Data_t
#include "Database.h"           // for DBRequest, LoadDatabase, VarType, RVarDef
#include "Decoder.h"            // for EModuleType
#include "TDatime.h"            // for TDatime
#include "THaAnalysisObject.h"  // for THaAnalysisObject, enums (EMode etc.)
#include "THaDetMap.h"          // for DigitizerHitInfo_t
#include "THaDetectorBase.h"    // for THaDetectorBase
#include "TString.h"            // for TString
#include "Helper.h"             // for ToInt
#include <cassert>              // for assert
#include <cstdio>               // for fclose, FILE, size_t
#include <sstream>              // for ostringstream etc.
#include <stdexcept>            // for logic_error, invalid_argument
#include <string>               // for basic_string, allocator, string
#include <type_traits>          // for is_same_v
#include <vector>               // for vector

using namespace std;
using Decoder::EModuleType;

namespace Podd {

//_____________________________________________________________________________
ChannelData::ChannelData( const char* name, const char* desc )
  : TNamed(name,desc)
  , fIsInit(false)
  , fVarOK(false)
  , fHitDone(false)
{
  // Base class constructor
}

//_____________________________________________________________________________
void ChannelData::Clear( Option_t* )
{
  ClearHitDone();
}

//_____________________________________________________________________________
Int_t ChannelData::GetLogicalChannel( const DigitizerHitInfo_t& hitinfo ) const
{
  // Return the 'logical channel' for the hardware channel referenced by
  // 'hitinfo'. The logical channel is the user-facing channel number, the
  // index into whatever arrays the detector data are stored.
  //
  // By default, this is the sequence number of the channel in the detector
  // map modulo the number of channels defined.
  //
  // Derived classes may use a different mapping here, e.g. a lookup table to
  // handle an irregular wiring scheme.

  UInt_t nelem = GetSize();
  if( nelem == 0 )
    throw logic_error(msg(hitinfo,
    "ChannelData: bad size. Should never happen. Call expert."));
  return hitinfo.lchan % ToInt(nelem);
}

//_____________________________________________________________________________
Int_t ChannelData::ReadConfig( THaDetectorBase*, const TDatime& , const char* )
{
  // Default implementation of reading the database. Does nothing.

  fIsInit = true;
  return kOK;
}

//_____________________________________________________________________________
Int_t ChannelData::DefineVariables( THaAnalysisObject::EMode mode,
                                    const char* key_prefix,
                                    const char* comment_subst )
{
  // Define variables for ChannelData and subclasses. The actual work is done
  // in the virtual function DefineVariablesImpl.

  if( mode == kDefine && fVarOK )
    return kOK;

  if( Int_t ret = DefineVariablesImpl(mode, key_prefix, comment_subst) )
    return ret;

  fVarOK = (mode == kDefine);
  return kOK;
}

//_____________________________________________________________________________
Int_t ChannelData::DefineVariablesImpl( THaAnalysisObject::EMode /*mode*/,
                                        const char* /*key_prefix*/,
                                        const char* /*comment_subst*/ )
{
  // Define global variables for these detector data. This is the base class
  // version, which currently does not define anything. Derived classes will
  // typically override this function. (ChannelData without variables would be
  // rather pointless.)

  return kOK;
}

//_____________________________________________________________________________
Int_t ChannelData::StdDefineVariables( const vector<RVarDef>& vars,
                                       THaAnalysisObject::EMode mode,
                                       const char* key_prefix,
                                       const char* here,
                                       const char* comment_subst )
{
  // Standard implementation of DefineVariables for ChannelData.
  // Avoids code duplication and ensures consistent behavior.

  TString prefix = fName;
  if( key_prefix && *key_prefix ) {
    prefix += key_prefix;
    if( !prefix.EndsWith(".") )
      prefix += '.';
  }
  return THaAnalysisObject::DefineGlobalVariables(
    vars, mode, this, {.prefix = prefix, .caller = here,
      .def_prefix = "", .comment_subst = comment_subst});
}

//_____________________________________________________________________________
string ChannelData::msg( const DigitizerHitInfo_t& hitinfo, const char* txt )
{
  // Format message string for exceptions
  ostringstream ostr;
  ostr << "Event " << hitinfo.ev << ", ";
  ostr << "channel "
       << hitinfo.crate << "/" << hitinfo.slot << "/"
       << hitinfo.chan  << "/" << hitinfo.hit << ": ";
  if( txt and *txt ) ostr << txt; else ostr << "Unspecified error.";
  return ostr.str();
}

//=============================================================================
ADCData::ADCData( const char* name, const char* desc, UInt_t nelem )
  : ChannelData(name, desc), fCalib(nelem), fADCs(nelem), fNHits(0)
{
  // Constructor. Creates data structures for 'nelem' ADC channels, i.e.
  // each channel is assumed to have one ADC reading plus pedestal-corrected
  // and calibrated values and associated calibration constants.
  // See the ADCCalib_t and ADCData_t structures for details.
}

//_____________________________________________________________________________
ADCData::ADCData( const THaDetectorBase* det )
  : ADCData(det->GetPrefix(), det->GetTitle(), det->GetNelem())
{
}

//_____________________________________________________________________________
void ADCData::Clear( Option_t* opt )
{
  // Clear event-by-event data
  ChannelData::Clear(opt);

  // for( auto& adc : fADCs ) {
  //   adc.adc_clear();
  // }
  fNHits = 0;
}

//_____________________________________________________________________________
Int_t ADCData::ReadConfig( THaDetectorBase* det, const TDatime& date,
                           const char* key_prefix )
{
  // Load ADC calibration constants from database

//  VarType kDataType = std::is_same_v<Data_t, Float_t> ? kFloat : kDouble;
  constexpr VarType kDataTypeV = std::is_same_v<Data_t, Float_t> ? kFloatV : kDoubleV;

  FILE* file = det->OpenFile(date);
  if( !file )
    return kFileError;

  // Set defaults
  for( auto& c : fCalib )
    c.adc_calib_reset();

  const UInt_t nval = fCalib.size();
  vector<Data_t> ped, gain, twalk;
  ped.reserve(nval); gain.reserve(nval); twalk.reserve(nval);
  const vector<DBRequest> calib_request = {
    {
      .name = "adc.pedestals,pedestals,ped", .var = &ped,
      .type = kDataTypeV, .nelem = nval, .optional = true
    },
    {
      .name = "adc.gains,gains,gain", .var = &gain,
      .type = kDataTypeV, .nelem = nval, .optional = true
    },
    {
      .name = "adc.twalk,timewalk_params", .var = &twalk,
      .type = kDataTypeV, .nelem = nval, .optional = true
    },
  };
  Int_t err = LoadDatabase({.file = file, .date = date,
    .prefix = det->GetPrefix(), .key_prefix = key_prefix}, calib_request);
  (void)fclose(file);
  if( !err ) {
    //TODO Save results to calib struct(s)
  }
  return err;
}

//_____________________________________________________________________________
void ADCData::Reset( Option_t* opt )
{
  // Clear event-by-event data and reset calibration values to defaults.
  Clear(opt);

  for( auto& calib : fCalib ) {
    calib.adc_calib_reset();
  }
}

//_____________________________________________________________________________
static void StoreADC( ADCHit_t& ADC, const ADCCalib_t& CALIB,
                      const DigitizerHitInfo_t& hitinfo, UInt_t data )
{
  ADC.adc   = data;
  ADC.adc_p = ADC.adc - CALIB.ped;
  ADC.adc_c = ADC.adc_p * CALIB.gain;
  //ADC.nhits  = hitinfo.nhit;
}

//_____________________________________________________________________________
static void StoreTDC( TDCHit_t& TDC, const TDCCalib_t& CALIB,
                      const DigitizerHitInfo_t& hitinfo, UInt_t data )
{
  TDC.tdc   = data;
//  TDC.tdc_c = (TDC.tdc - CALIB.off) * CALIB.tdc2t;
//  assert(CALIB.tdc2t >= 0.0);  // else bug in ReadDatabase
  if( hitinfo.type == EModuleType::kCommonStopTDC ) {
    // For common stop TDCs, time is negatively correlated to raw data
    // time = (offset-data)*res, so reverse the sign.
    TDC.tdc_c *= -1.0;
  }
  //TDC.ntdc  = hitinfo.nhit;
}

//_____________________________________________________________________________
Int_t ADCData::StoreHit( const DigitizerHitInfo_t& hitinfo, UInt_t data )
{
  // Copy the raw and calibrated ADC/TDC data to the member variables.
  // This is the single-hit version of this function.

  size_t k = GetLogicalChannel(hitinfo);
  if( k >= fADCs.size() )
    throw
      std::invalid_argument(msg(hitinfo, "Logical channel number out of range"));

  switch( hitinfo.type ) {
    case EModuleType::kADC:
      StoreADC(fADCs[k], fCalib[k], hitinfo, data);
      fNHits++;
      fHitDone = true;
      break;

    case EModuleType::kUndefined:
      throw logic_error(msg(hitinfo, "Invalid channel type"));
    default:
      break;
  }
  return 0;
}

//_____________________________________________________________________________
Int_t ADCData::DefineVariablesImpl( THaAnalysisObject::EMode mode,
                                    const char* key_prefix,
                                    const char* comment_subst )
{
  // Export ADCData results as global variables

  const char* const here = "ADCData::DefineVariables";

  // Define variables of the base class, if any
  if( Int_t ret = ChannelData::DefineVariablesImpl(mode, key_prefix, comment_subst) )
    return ret;

  const vector<RVarDef> vars = {
    { .name = "nhit",  .desc = "Number of %s PMTs with ADC signal",     .def = "fNHits"  },
    { .name = "a",     .desc = "Raw %s ADC amplitudes",                 .def = "fADCs.adc"   },
    { .name = "a_p",   .desc = "Pedestal-subtracted %s ADC amplitudes", .def = "fADCs.adc_p" },
    { .name = "a_c",   .desc = "Gain-corrected %s ADC amplitudes",      .def = "fADCs.adc_c" },
  };
  return StdDefineVariables(vars, mode, key_prefix, here, comment_subst);
}

//=============================================================================
TDCData::TDCData( const char* name, const char* desc, UInt_t nelem )
  : ChannelData(name, desc), fCalib(nelem), fTDCs(nelem)
{
  // Constructor. Creates data structures for 'nelem' ADC+TDC channels, i.e.
  // each channel is assumed to have one ADC and one TDC reading plus pedestal-
  // corrected and calibrated values and associated calibration constants.
  // See the ADC/TDCCalib_t and ADC/TDCData_t structures for details.
}

//_____________________________________________________________________________
TDCData::TDCData( const THaDetectorBase* det )
  : TDCData(det->GetPrefixName(), det->GetTitle(), det->GetNelem())
{
}

//_____________________________________________________________________________
void TDCData::Clear( Option_t* opt )
{
  // Clear event-by-event data
  ChannelData::Clear(opt);

  // for( auto& pmt : fTDCs ) {
  //   pmt.clear();
  // }
  fNHits.clear();
}

//_____________________________________________________________________________
//FIXME: support key prefix
//FIXME: add alternative keys
Int_t TDCData::ReadConfig( THaDetectorBase* det, const TDatime& date,
                           const char* key_prefix )
{
  // Load ADC calibration constants from database

//  VarType kDataType = std::is_same_v<Data_t, Float_t> ? kFloat : kDouble;
  constexpr VarType kDataTypeV = std::is_same_v<Data_t, Float_t> ? kFloatV : kDoubleV;

  FILE* file = det->OpenFile(date);
  if( !file )
    return kFileError;

  // Set defaults
  for( auto& c : fCalib ) {
//    c.adc_calib_reset();
    c.tdc_calib_reset();
  }

  UInt_t nval = fCalib.size();
  vector<Data_t> ped, gain, twalk;
  ped.reserve(nval); gain.reserve(nval); twalk.reserve(nval);
  const vector<DBRequest> calib_request = {
    {
      .name = "pedestals", .var = &ped,
      .type = kDataTypeV, .nelem = nval, .optional = true
    },
    {
      .name = "gains", .var = &gain,
      .type = kDataTypeV, .nelem = nval, .optional = true
    },
    {
      .name = "timewalk_params", .var = &twalk,
      .type = kDataTypeV, .nelem = nval, .optional = true
    },
    {}
  };
  Int_t err = LoadDatabase({.file = file, .date = date,
    .prefix = det->GetPrefix(), .key_prefix = key_prefix}, calib_request);
  (void)fclose(file);
  if( !err ) {

  }
  return err;
}

//_____________________________________________________________________________
void TDCData::Reset( Option_t* opt )
{
  // Clear event-by-event data and reset calibration values to defaults.
  Clear(opt);

  // for( auto& calib : fCalib ) {
  //   calib.reset();
  // }
}

//_____________________________________________________________________________
Int_t TDCData::StoreHit( const DigitizerHitInfo_t& hitinfo, UInt_t data )
{
  // Copy the raw and calibrated ADC/TDC data to the member variables.
  // This is the single-hit version of this function.

  size_t k = GetLogicalChannel(hitinfo);
  if( k >= fTDCs.size() )
    throw
      std::invalid_argument(msg(hitinfo, "Logical channel number out of range"));

  switch( hitinfo.type ) {
    case EModuleType::kADC:
//      StoreADC(fTDCs[k], fCalib[k], hitinfo, data);
//      fNHits.adc++;
      fHitDone = true;
      break;

    case EModuleType::kCommonStopTDC:
    case EModuleType::kCommonStartTDC:
      StoreTDC(fTDCs[k], fCalib[k], hitinfo, data);
//      fNHits.tdc++;
      fHitDone = true;
      break;

    default:
      break;
    case EModuleType::kUndefined:
      throw logic_error(msg(hitinfo, "Invalid channel type"));
  }
  return 0;
}

//_____________________________________________________________________________
Int_t TDCData::DefineVariablesImpl( THaAnalysisObject::EMode mode,
                                    const char* key_prefix,
                                    const char* comment_subst )
{
  // Export PMTData results as global variables

  const char* const here = "PMTData::DefineVariables";

  // Define variables of the base class, if any
  if( Int_t ret = ChannelData::DefineVariablesImpl(mode, key_prefix, comment_subst) )
    return ret;

  const vector<RVarDef> vars = {
    { .name = "nthit",  .desc = "Number of %s PMTs with valid TDC",       .def = "fNHits.tdc"  },
    { .name = "nahit",  .desc = "Number of %s PMTs with ADC signal",      .def = "fNHits.adc"  },
    { .name = "nhits",  .desc = "Total number of %s TDC hits",            .def = "fPMTs.ntdc"  },
    { .name = "t",      .desc = "Raw %s TDC times",                       .def = "fPMTs.tdc"   },
    { .name = "t_c",    .desc = "Calibrated %s TDC times (s)",            .def = "fPMTs.tdc_c" },
    { .name = "a",      .desc = "Raw %s ADC amplitudes",                  .def = "fPMTs.adc"   },
    { .name = "a_p",    .desc = "Pedestal-subtracted %s ADC amplitudes",  .def = "fPMTs.adc_p" },
    { .name = "a_c",    .desc = "Gain-corrected %s ADC amplitudes",       .def = "fPMTs.adc_c" },
  };
  return StdDefineVariables(vars, mode, key_prefix, here, comment_subst);
}

//_____________________________________________________________________________

} // namespace Podd

ClassImp(Podd::ChannelData)
ClassImp(Podd::PMTData)
ClassImp(Podd::ADCData)
