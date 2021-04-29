//////////////////////////////////////////////////////////////////////////
//
// Podd::DetectorData
//
//////////////////////////////////////////////////////////////////////////

#include "DetectorData.h"
#include "Decoder.h"
#include "THaAnalysisObject.h" // For DefineVarsFromList

#include <stdexcept>
#include <sstream>

using namespace std;
using Decoder::ChannelType;

namespace Podd {

//_____________________________________________________________________________
DetectorData::DetectorData( const char* name, const char* desc )
  : TNamed(name,desc), fVarOK(false), fHitDone(false)
{
  // Base class constructor
}

//_____________________________________________________________________________
void DetectorData::Clear( Option_t* )
{
  ClearHitDone();
}

//_____________________________________________________________________________
Int_t DetectorData::GetLogicalChannel( const DigitizerHitInfo_t& hitinfo ) const
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
    throw logic_error(msg(hitinfo, "DetectorData: bad size. "
                                   "Should never happen. Call expert."));
  return hitinfo.lchan % static_cast<Int_t>(nelem);
}

//_____________________________________________________________________________
Int_t DetectorData::DefineVariables( THaAnalysisObject::EMode mode,
                                     const char* key_prefix,
                                     const char* comment_subst )
{
  // Define variables for DetectorData and subclasses. The actual work is done
  // in the virtual function DefineVariablesImpl.

  if( mode == THaAnalysisObject::kDefine && fVarOK )
    return THaAnalysisObject::kOK;

  Int_t ret = DefineVariablesImpl(mode, key_prefix, comment_subst);
  if( ret )
    return ret;

  fVarOK = (mode == THaAnalysisObject::kDefine);
  return THaAnalysisObject::kOK;
}

//_____________________________________________________________________________
Int_t DetectorData::DefineVariablesImpl( THaAnalysisObject::EMode /*mode*/,
                                         const char* /*key_prefix*/,
                                         const char* /*comment_subst*/ )
{
  // Define global variables for these detector data. This is the base class
  // version, which currently does not define anything. Derived classes will
  // typically override this function. (DetectorData without variables would be
  // rather pointless.)

  return THaAnalysisObject::kOK;
}

//_____________________________________________________________________________
Int_t DetectorData::StdDefineVariables( const RVarDef* vars,
                                        THaAnalysisObject::EMode mode,
                                        const char* key_prefix,
                                        const char* here,
                                        const char* comment_subst )
{
  // Standard implementation of DefineVariables for DetectorData.
  // Avoids code duplication and ensures consistent behavior.

  TString prefix = fName + ".";
  if( key_prefix && *key_prefix )
    prefix += key_prefix;
  return THaAnalysisObject::DefineVarsFromList(
    vars, THaAnalysisObject::kRVarDef, mode, "", this, prefix.Data(),
    here, comment_subst);
}

//_____________________________________________________________________________
string DetectorData::msg( const DigitizerHitInfo_t& hitinfo, const char* txt )
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
  : DetectorData(name, desc), fCalib(nelem), fADCs(nelem), fNHits(0)
{
  // Constructor. Creates data structures for 'nelem' ADC channels, i.e.
  // each channel is assumed to have one ADC reading plus pedestal-corrected
  // and calibrated values and associated calibration constants.
  // See the ADCCalib_t and ADCData_t structures for details.
}

//_____________________________________________________________________________
void ADCData::Clear( Option_t* opt )
{
  // Clear event-by-event data
  DetectorData::Clear(opt);

  for( auto& adc : fADCs ) {
    adc.adc_clear();
  }
  fNHits = 0;
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
static void StoreADC( ADCData_t& ADC, const ADCCalib_t& CALIB,
                      const DigitizerHitInfo_t& hitinfo, UInt_t data )
{
  ADC.adc   = data;
  ADC.adc_p = ADC.adc - CALIB.ped;
  ADC.adc_c = ADC.adc_p * CALIB.gain;
  ADC.nadc  = hitinfo.nhit;
}

//_____________________________________________________________________________
static void StoreTDC( TDCData_t& TDC, const TDCCalib_t& CALIB,
                      const DigitizerHitInfo_t& hitinfo, UInt_t data )
{
  TDC.tdc   = data;
  TDC.tdc_c = (TDC.tdc - CALIB.off) * CALIB.tdc2t;
  assert(CALIB.tdc2t >= 0.0);  // else bug in ReadDatabase
  if( hitinfo.type == ChannelType::kCommonStopTDC ) {
    // For common stop TDCs, time is negatively correlated to raw data
    // time = (offset-data)*res, so reverse the sign.
    TDC.tdc_c *= -1.0;
  }
  TDC.ntdc  = hitinfo.nhit;
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
    case ChannelType::kADC:
    case ChannelType::kMultiFunctionADC:
      StoreADC(fADCs[k], fCalib[k], hitinfo, data);
      fNHits++;
      fHitDone = true;
      break;

    case ChannelType::kUndefined:
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
  Int_t ret = DetectorData::DefineVariablesImpl(mode, key_prefix, comment_subst);
  if( ret )
    return ret;

  const RVarDef vars[] = {
    { "nhit",  "Number of %s PMTs with ADC signal",     "fNHits"  },
    { "a",     "Raw %s ADC amplitudes",                 "fADCs.adc"   },
    { "a_p",   "Pedestal-subtracted %s ADC amplitudes", "fADCs.adc_p" },
    { "a_c",   "Gain-corrected %s ADC amplitudes",      "fADCs.adc_c" },
    { nullptr }
  };
  return StdDefineVariables(vars, mode, key_prefix, here, comment_subst);
}

//=============================================================================
PMTData::PMTData( const char* name, const char* desc, UInt_t nelem )
  : DetectorData(name, desc), fCalib(nelem), fPMTs(nelem)
{
  // Constructor. Creates data structures for 'nelem' ADC+TDC channels, i.e.
  // each channel is assumed to have one ADC and one TDC reading plus pedestal-
  // corrected and calibrated values and associated calibration constants.
  // See the ADC/TDCCalib_t and ADC/TDCData_t structures for details.
}

//_____________________________________________________________________________
void PMTData::Clear( Option_t* opt )
{
  // Clear event-by-event data
  DetectorData::Clear(opt);

  for( auto& pmt : fPMTs ) {
    pmt.clear();
  }
  fNHits.clear();
}

//_____________________________________________________________________________
void PMTData::Reset( Option_t* opt )
{
  // Clear event-by-event data and reset calibration values to defaults.
  Clear(opt);

  for( auto& calib : fCalib ) {
    calib.reset();
  }
}

//_____________________________________________________________________________
Int_t PMTData::StoreHit( const DigitizerHitInfo_t& hitinfo, UInt_t data )
{
  // Copy the raw and calibrated ADC/TDC data to the member variables.
  // This is the single-hit version of this function.

  size_t k = GetLogicalChannel(hitinfo);
  if( k >= fPMTs.size() )
    throw
      std::invalid_argument(msg(hitinfo, "Logical channel number out of range"));

  switch( hitinfo.type ) {
    case ChannelType::kADC:
    case ChannelType::kMultiFunctionADC:
      StoreADC(fPMTs[k], fCalib[k], hitinfo, data);
      fNHits.adc++;
      fHitDone = true;
      break;

    case ChannelType::kCommonStopTDC:
    case ChannelType::kCommonStartTDC:
    case ChannelType::kMultiFunctionTDC:
      StoreTDC(fPMTs[k], fCalib[k], hitinfo, data);
      fNHits.tdc++;
      fHitDone = true;
      break;

    default:
      break;
    case ChannelType::kUndefined:
      throw logic_error(msg(hitinfo, "Invalid channel type"));
  }
  return 0;
}

//_____________________________________________________________________________
Int_t PMTData::DefineVariablesImpl( THaAnalysisObject::EMode mode,
                                    const char* key_prefix,
                                    const char* comment_subst )
{
  // Export PMTData results as global variables

  const char* const here = "PMTData::DefineVariables";

  // Define variables of the base class, if any
  Int_t ret = DetectorData::DefineVariablesImpl(mode, key_prefix, comment_subst);
  if( ret )
    return ret;

  const RVarDef vars[] = {
    { "nthit",  "Number of %s PMTs with valid TDC",      "fNHits.tdc"  },
    { "nahit",  "Number of %s PMTs with ADC signal",     "fNHits.adc"  },
    { "nhits",  "Total number of %s TDC hits",           "fPMTs.ntdc"  },
    { "t",      "Raw %s TDC times",                      "fPMTs.tdc"   },
    { "t_c",    "Calibrated %s TDC times (s)",           "fPMTs.tdc_c" },
    { "a",      "Raw %s ADC amplitudes",                 "fPMTs.adc"   },
    { "a_p",    "Pedestal-subtracted %s ADC amplitudes", "fPMTs.adc_p" },
    { "a_c",    "Gain-corrected %s ADC amplitudes",      "fPMTs.adc_c" },
    { nullptr }
  };
  return StdDefineVariables(vars, mode, key_prefix, here, comment_subst);
}

//_____________________________________________________________________________

} // namespace Podd

ClassImp(Podd::DetectorData)
ClassImp(Podd::PMTData)
ClassImp(Podd::ADCData)
