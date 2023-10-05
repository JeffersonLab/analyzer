//////////////////////////////////////////////////////////////////
//
//   Fadc250Module
//   JLab FADC 250 Module
//
//   Author: Eric Pooser, pooser@jlab.org
//
//   For documentation pertaining to the FADC250
//   modules refer to the following link:
//   https://coda.jlab.org/drupal/system/files/pdfs/HardwareManual/fADC250/FADC250_Processing_FPGA_Firmware_ver_0x0C01_Description_Instructions.pdf
//
//   The following data type summary corresponds to firmware version 0x0C00 (06/09/2016)
//   -----------------------------------------------------------------------------------
//   0 block header
//   1 block trailer
//   2 event header
//   3 trigger time
//   4 window raw data
//   5 (reserved)
//   6 pulse raw data -> Old firmware
//   7 pulse integral -> Old firmware
//   8 pulse time     -> Old firmware
//   9 pulse parameters
//   10 pulse parameters (pedestal) -> Old firmware
//   11 (reserved)
//   12 scaler data
//   13 (reserved)
//   14 data not valid (empty module)
//   15 filler (non-data) word
//
//   Mode Summary
//   ----------------------------------------------
//   Mode 1:  data type  4          -> Old firmware
//   Mode 2:  data type  6          -> Old firmware
//   Mode 3:  data types 7 & 8      -> Old firmware
//   Mode 4:  data types 8 & 10     -> Old firmware
//   Mode 7:  data types 7 & 8 & 10 -> Old firmware
//   Mode 8:  data types 4 & 8 & 10 -> Old firmware
//   Mode 9:  data type  9
//   Mode 10: data types 4 & 9
//
//   fFirmwareVers == 2
//   Current version (0x0C0D, 9/28/17) of the FADC firmware only
//   reports the pedestal once for up to four identifiable pulses in
//   in the readout window.  For now, it is best to tell THaEvData
//   that there are n pedestals associated with n identified pulses
//   and point the GetData method to the first entry in the pedestal
//   data vector.  This is now the non firmware specific behavior.
//
/////////////////////////////////////////////////////////////////////

#include "Fadc250Module.h"
#include "THaSlotData.h"
#include "TMath.h"

#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <numeric>
#include <cassert>
#include <stdexcept>
#include <map>
#include <sstream>

using namespace std;

// #define DEBUG
// #define WITH_DEBUG

#ifdef DEBUG
#include <fstream>
#endif

namespace Decoder {

using vsiz_t = vector<int>::size_type;

Module::TypeIter_t Fadc250Module::fgThisType =
  DoRegister(ModuleType("Decoder::Fadc250Module", 250));

//_____________________________________________________________________________
Fadc250Module::Fadc250Module( UInt_t crate, UInt_t slot )
  : PipeliningModule(crate, slot), fadc_data{}, fPulseData(NADCCHAN),
    data_type_4(false), data_type_6(false), data_type_7(false),
    data_type_8(false), data_type_9(false), data_type_10(false),
    block_header_found(false), block_trailer_found(false),
    event_header_found(false), slots_match(false)
{
  IsInit = false;
  Fadc250Module::Init();
}

//_____________________________________________________________________________
#if defined DEBUG && defined WITH_DEBUG
Fadc250Module::~Fadc250Module()
{
  // delete fDebugFile; fDebugFile = 0;
}
#else
Fadc250Module::~Fadc250Module() = default;
#endif

//_____________________________________________________________________________
Bool_t Fadc250Module::HasCapability( Decoder::EModuleType type )
{
  return (type == kSampleADC || type == kPulseIntegral || type == kPulseTime
          || type == kPulsePeak || type == kPulsePedestal
          || type == kCoarseTime || type == kFineTime);
}

//_____________________________________________________________________________
// Clear all data vectors
inline
void Fadc250Module::ClearDataVectors()
{
  // Clear all data objects
  assert(fPulseData.size() == NADCCHAN);  // Initialization error in constructor
  for( uint32_t i = 0; i < NADCCHAN; i++ ) {
    fPulseData[i].clear();
  }
}

//_____________________________________________________________________________
// Require that slot from base class and slot from
//   data match before populating data vectors
inline
void Fadc250Module::PopulateDataVector( vector<uint32_t>& data_vector,
                                        uint32_t data ) const
{
  if( slots_match )
    data_vector.push_back(data);
}

//_____________________________________________________________________________
// Sum elements contained in data vector
inline
uint32_t Fadc250Module::SumVectorElements( const vector<uint32_t>& data_vector )
{
  return accumulate(data_vector.begin(), data_vector.end(), 0U);
}

//_____________________________________________________________________________
void Fadc250Module::Clear( Option_t* opt )
{
  // Clear event-by-event data
  PipeliningModule::Clear(opt);
  fadc_data.clear();
  ClearDataVectors();
  // Initialize data_type_def to FILLER and data types to false
  data_type_def = 15;
  // Initialize data types to false
  data_type_4 = data_type_6 = data_type_7 = data_type_8 = data_type_9 = data_type_10 = false;
  block_header_found = block_trailer_found = event_header_found = slots_match = false;
}

//_____________________________________________________________________________
void Fadc250Module::Init()
{
#if defined DEBUG && defined WITH_DEBUG
  // This will make a HUGE output
  // delete fDebugFile; fDebugFile = 0;
  // fDebugFile = new ofstream;
  // fDebugFile->open("fadcdebug.dat");
#endif
  Clear();
  IsInit = true;
  fName = "FADC250 JLab Flash ADC Module";
}

//_____________________________________________________________________________
void Fadc250Module::CheckDecoderStatus() const
{
  cout << "FADC250 Decoder has been called" << endl;
}

//_____________________________________________________________________________
UInt_t Fadc250Module::GetNumEvents( Decoder::EModuleType emode,
                                    UInt_t chan ) const
{
  vsiz_t ret = 0;
  switch( emode ) {
    case kSampleADC:
      ret = fPulseData[chan].samples.size();
      break;
    case kPulseIntegral:
      ret = fPulseData[chan].integral.size();
      break;
    case kPulseTime:
      ret = fPulseData[chan].time.size();
      break;
    case kPulsePeak:
      ret = fPulseData[chan].peak.size();
      break;
    case kPulsePedestal:
      if( fFirmwareVers == 2 )
        ret = fPulseData[chan].pedestal.size();
      else
        ret = fPulseData[chan].integral.size();
      break;
    case kCoarseTime:
      ret = fPulseData[chan].coarse_time.size();
      break;
    case kFineTime:
      ret = fPulseData[chan].fine_time.size();
      break;
  }
  if( ret > kMaxUInt )
    throw overflow_error("ERROR! Fadc250Module::GetNumEvents: "
                         "integer overflow");
  return ret;
}

//_____________________________________________________________________________
UInt_t Fadc250Module::GetData( Decoder::EModuleType emode, UInt_t chan,
                               UInt_t ievent ) const
{
  switch( emode ) {
    case kSampleADC:
      return GetPulseSamplesData(chan, ievent);
    case kPulseIntegral:
      return GetPulseIntegralData(chan, ievent);
    case kPulseTime:
      return GetPulseTimeData(chan, ievent);
    case kPulsePeak:
      return GetPulsePeakData(chan, ievent);
    case kPulsePedestal:
      return GetPulsePedestalData(chan, ievent);
    case kCoarseTime:
      return GetPulseCoarseTimeData(chan, ievent);
    case kFineTime:
      return GetPulseFineTimeData(chan, ievent);
  }
  return 0;
}

//_____________________________________________________________________________
UInt_t Fadc250Module::GetPulseIntegralData( UInt_t chan, UInt_t ievent ) const
{
  vsiz_t nevent = fPulseData[chan].integral.size();
  if( ievent >= nevent ) {
    cout << "ERROR:: Fadc250Module:: GetPulseIntegralData:: invalid event number for slot = " << fSlot << ", channel = "
         << chan << endl;
    return kMaxUInt;
  }
  if( nevent == 0 ) {
    cout << "ERROR:: Fadc250Module:: GetPulseIntegralData:: data vector empty for slot = " << fSlot << ", channel = "
         << chan << endl;
    return kMaxUInt;
  } else {
#ifdef WITH_DEBUG
    if( fDebugFile )
      *fDebugFile << "Fadc250Module::GetPulseIntegralData channel "
                  << chan << ", event " << ievent << " = "
                  << fPulseData[chan].integral[ievent] << endl;
#endif
    return fPulseData[chan].integral[ievent];
  }
}

//_____________________________________________________________________________
UInt_t Fadc250Module::GetEmulatedPulseIntegralData( UInt_t chan ) const
{
  vsiz_t nevent = fPulseData[chan].samples.size();
  if( nevent == 0 ) {
    cout << "ERROR:: Fadc250Module:: GetEmulatedPulseIntegralData:: data vector empty  for slot = " << fSlot
         << ", channel = " << chan << "\n"
         << "Ensure that FADC is operating in mode 1 OR 8" << endl;
    return kMaxUInt;
  } else {
#ifdef WITH_DEBUG
    if( fDebugFile )
      *fDebugFile << "Fadc250Module::GetEmulatedPulseIntegralData channel "
                  << chan << " = " << SumVectorElements(fPulseData[chan].samples) << endl;
#endif
    return SumVectorElements(fPulseData[chan].samples);
  }
}

//_____________________________________________________________________________
UInt_t Fadc250Module::GetPulseTimeData( UInt_t chan, UInt_t ievent ) const
{
  vsiz_t nevent = fPulseData[chan].time.size();
  if( ievent >= nevent ) {
    cout << "ERROR:: Fadc250Module:: GetPulseTimeData:: invalid event number for slot = " << fSlot << ", channel = "
         << chan << endl;
    return kMaxUInt;
  }
  if( nevent == 0 ) {
    cout << "ERROR:: Fadc250Module:: GetPulseTimeData:: data vector empty for slot = " << fSlot << ", channel = "
         << chan << endl;
    return kMaxUInt;
  } else {
#ifdef WITH_DEBUG
    if( fDebugFile )
      *fDebugFile << "Fadc250Module::GetPulseTimeData channel "
                  << chan << ", event " << ievent << " = "
                  << fPulseData[chan].time[ievent] << endl;
#endif
    return fPulseData[chan].time[ievent];
  }
}

//_____________________________________________________________________________
UInt_t Fadc250Module::GetPulseCoarseTimeData( UInt_t chan, UInt_t ievent ) const
{
  vsiz_t nevent = fPulseData[chan].coarse_time.size();
  if( ievent >= nevent ) {
    cout << "ERROR:: Fadc250Module:: GetPulseCoarseTimeData:: invalid event number for slot = " << fSlot
         << ", channel = " << chan << endl;
    return kMaxUInt;
  }
  if( nevent == 0 ) {
    cout << "ERROR:: Fadc250Module:: GetPulseCoarseTimeData:: data vector empty for slot = " << fSlot << ", channel = "
         << chan << endl;
    return kMaxUInt;
  } else {
#ifdef WITH_DEBUG
    if( fDebugFile )
      *fDebugFile << "Fadc250Module::GetPulseCoarseTimeData channel "
                  << chan << ", event " << ievent << " = "
                  << fPulseData[chan].coarse_time[ievent] << endl;
#endif
    return fPulseData[chan].coarse_time[ievent];
  }
}

//_____________________________________________________________________________
UInt_t Fadc250Module::GetPulseFineTimeData( UInt_t chan, UInt_t ievent ) const
{
  vsiz_t nevent = fPulseData[chan].fine_time.size();
  if( ievent >= nevent ) {
    cout << "ERROR:: Fadc250Module:: GetPulseCoarseTimeData:: invalid event number for slot = " << fSlot
         << ", channel = " << chan << endl;
    return kMaxUInt;
  }
  if( nevent == 0 ) {
    cout << "ERROR:: Fadc250Module:: GetPulseCoarseTimeData:: data vector empty for slot = " << fSlot << ", channel = "
         << chan << endl;
    return kMaxUInt;
  } else {
#ifdef WITH_DEBUG
    if( fDebugFile )
      *fDebugFile << "Fadc250Module::GetPulseFineTimeData channel "
                  << chan << ", event " << ievent << " = "
                  << fPulseData[chan].fine_time[ievent] << endl;
#endif
    return fPulseData[chan].fine_time[ievent];
  }
}

//_____________________________________________________________________________
UInt_t Fadc250Module::GetPulsePeakData( UInt_t chan, UInt_t ievent ) const
{
  vsiz_t nevent = fPulseData[chan].peak.size();
  if( ievent >= nevent ) {
    cout << "ERROR:: Fadc250Module:: GetPulsePeakData:: invalid event number for slot = " << fSlot << ", channel = "
         << chan << endl;
    return kMaxUInt;
  }
  if( nevent == 0 ) {
    cout << "ERROR:: Fadc250Module:: GetPulsePeakData:: data vector empty for slot = " << fSlot << ", channel = "
         << chan << endl;
    return kMaxUInt;
  } else {
#ifdef WITH_DEBUG
    if( fDebugFile )
      *fDebugFile << "Fadc250Module::GetPulsePeakData channel "
                  << chan << ", event " << ievent << " = "
                  << fPulseData[chan].peak[ievent] << endl;
#endif
    return fPulseData[chan].peak[ievent];
  }
}

//_____________________________________________________________________________
UInt_t Fadc250Module::GetPulsePedestalData( UInt_t chan, UInt_t ievent ) const
{
  vsiz_t nevent = fPulseData[chan].pedestal.size();
  if( nevent == 0 ) {
    cout << "ERROR:: Fadc250Module:: GetPulsePedestalData:: data vector empty for slot = " << fSlot << ", channel = "
         << chan << endl;
    return kMaxUInt;
  }
  if( fFirmwareVers == 2 ) {
    if( ievent >= nevent ) {
      cout << "ERROR:: Fadc250Module:: GetPulsePedestalData:: invalid data vector size = " << fSlot << ", channel = "
           << chan << endl;
      return kMaxUInt;
    } else {
#ifdef WITH_DEBUG
      if( fDebugFile )
        *fDebugFile << "Fadc250Module::GetPulsePedestalData channel "
                    << chan << ", event " << ievent << " = "
                    << fPulseData[chan].pedestal[ievent] << endl;
#endif
      return fPulseData[chan].pedestal[ievent];
    }
  }
  if( nevent != 1 ) {
    cout << "ERROR:: Fadc250Module:: GetPulsePedestalData:: invalid data vector size = " << fSlot << ", channel = "
         << chan << endl;
    return kMaxUInt;
  } else {
#ifdef WITH_DEBUG
    if( fDebugFile )
      *fDebugFile << "Fadc250Module::GetPulsePedestalData channel "
                  << chan << ", event " << ievent << " = "
                  << fPulseData[chan].pedestal[0] << endl;
#endif
    return fPulseData[chan].pedestal[0];
  }
}

//_____________________________________________________________________________
UInt_t Fadc250Module::GetPedestalQuality( UInt_t chan, UInt_t ievent ) const
{
  vsiz_t nevent = fPulseData[chan].pedestal_quality.size();
  if( nevent == 0 ) {
    cout << "ERROR:: Fadc250Module:: GetPedestalQuality:: data vector empty for slot = " << fSlot << ", channel = "
         << chan << endl;
    return kMaxUInt;
  }
  if( nevent != 1 ) {
    cout << "ERROR:: Fadc250Module:: GetPedestalQuality:: invalid data vector size = " << fSlot << ", channel = "
         << chan << endl;
    return kMaxUInt;
  } else {
#ifdef WITH_DEBUG
    if( fDebugFile )
      *fDebugFile << "Fadc250Module::GetPedestalQuality channel "
                  << chan << ", event " << ievent << " = "
                  << fPulseData[chan].pedestal_quality[0] << endl;
#endif
    return fPulseData[chan].pedestal_quality[0];
  }
}

//_____________________________________________________________________________
UInt_t Fadc250Module::GetOverflowBit( UInt_t chan, UInt_t ievent ) const
{
  vsiz_t nevent = fPulseData[chan].overflow.size();
  if( ievent >= nevent ) {
    cout << "ERROR:: Fadc250Module:: GetOverflowBit:: invalid event number for slot = " << fSlot << ", channel = "
         << chan << endl;
    return kMaxUInt;
  }
  if( nevent == 0 ) {
    cout << "ERROR:: Fadc250Module:: GetOverflowBit:: data vector empty for slot = " << fSlot << ", channel = " << chan
         << endl;
    return kMaxUInt;
  } else {
#ifdef WITH_DEBUG
    if( fDebugFile )
      *fDebugFile << "Fadc250Module::GetOverflowBit channel "
                  << chan << ", event " << ievent << " = "
                  << fPulseData[chan].overflow[ievent] << endl;
#endif
    return fPulseData[chan].overflow[ievent];
  }
}

//_____________________________________________________________________________
UInt_t Fadc250Module::GetUnderflowBit( UInt_t chan, UInt_t ievent ) const
{
  vsiz_t nevent = fPulseData[chan].underflow.size();
  if( ievent >= nevent ) {
    cout << "ERROR:: Fadc250Module:: GetUnderflowBit:: invalid event number for slot = " << fSlot << ", channel = "
         << chan << endl;
    return kMaxUInt;
  }
  if( nevent == 0 ) {
    cout << "ERROR:: Fadc250Module:: GetUnderflowBit:: data vector empty for slot = " << fSlot << ", channel = " << chan
         << endl;
    return kMaxUInt;
  } else {
#ifdef WITH_DEBUG
    if( fDebugFile )
      *fDebugFile << "Fadc250Module::GetUnderflowBit channel "
                  << chan << ", event " << ievent << " = "
                  << fPulseData[chan].underflow[ievent] << endl;
#endif
    return fPulseData[chan].underflow[ievent];
  }
}

//_____________________________________________________________________________
UInt_t Fadc250Module::GetTriggerTime() const
{
  // Truncate to 32 bits
  UInt_t shorttime = fadc_data.trig_time;
#ifdef WITH_DEBUG
  if( fDebugFile )
    *fDebugFile << "Fadc250Module::GetTriggerTime = "
                << fadc_data.trig_time << " " << shorttime << endl;
#endif
  return shorttime;
}

//_____________________________________________________________________________
UInt_t Fadc250Module::GetPulseSamplesData( UInt_t chan, UInt_t ievent ) const
{
  vsiz_t nevent = fPulseData[chan].samples.size();
  if( ievent >= nevent ) {
    cout << "ERROR:: Fadc250Module:: GetPulseSamplesData:: invalid event number for slot = " << fSlot << ", channel = "
         << chan << endl;
    return kMaxUInt;
  }
  if( nevent == 0 ) {
    cout << "ERROR:: Fadc250Module:: GetPulseSamplesData:: data vector empty for slot = " << fSlot << ", channel = "
         << chan << endl;
    return kMaxUInt;
  } else {
#ifdef WITH_DEBUG
    if( fDebugFile )
      *fDebugFile << "Fadc250Module::GetPulseSamplesData channel "
                  << chan << ", event " << ievent << " = "
                  << fPulseData[chan].samples[ievent] << endl;
#endif
    return fPulseData[chan].samples[ievent];
  }
}

//_____________________________________________________________________________
vector<uint32_t> Fadc250Module::GetPulseSamplesVector( UInt_t chan ) const
{
  vsiz_t nevent = fPulseData[chan].samples.size();
  if( nevent == 0 ) {
    cout << "ERROR:: Fadc250Module:: GetPulseSamplesVector:: data vector empty for slot = " << fSlot << ", channel = "
         << chan << endl;
    return {};
  } else {
#ifdef WITH_DEBUG
    if( fDebugFile )
      *fDebugFile << "Fadc250Module::GetPulseSamplesVector channel "
                  << chan << " = " << &fPulseData[chan].samples << endl;
#endif
    return fPulseData[chan].samples;
  }
}

//_____________________________________________________________________________
void Fadc250Module::PrintDataType() const
{
#ifdef WITH_DEBUG
  if( !fDebugFile ) return;
  *fDebugFile << "start,  Print Data Type " << endl;
  if( data_type_4 ) *fDebugFile << "data type 4" << endl;
  if( data_type_6 ) *fDebugFile << "data type 6" << endl;
  if( data_type_7 ) *fDebugFile << "data type 7" << endl;
  if( data_type_8 ) *fDebugFile << "data type 8" << endl;
  if( data_type_9 ) *fDebugFile << "data type 9" << endl;
  if( data_type_10 ) *fDebugFile << "data type 10" << endl;
  *fDebugFile << "end,  Print Data Type " << endl;
#endif
}

//_____________________________________________________________________________
Int_t Fadc250Module::GetFadcMode() const
{
  if( fFirmwareVers == 1 ) {  // some "older" firmware
    if( data_type_4 && data_type_6 ) return 8;
    if( data_type_7 ) return 7;
  }
  if( data_type_4 && !(data_type_6) && !(data_type_7) && !(data_type_8) && !(data_type_9) && !(data_type_10) ) return 1;
  else if( !(data_type_4) && data_type_6 && !(data_type_7) && !(data_type_8) && !(data_type_9) &&
           !(data_type_10) )
    return 2;
  else if( !(data_type_4) && !(data_type_6) && data_type_7 && data_type_8 && !(data_type_9) && !(data_type_10) )
    return 3;
  else if( !(data_type_4) && !(data_type_6) && !(data_type_7) && data_type_8 && !(data_type_9) && data_type_10 )
    return 4;
  else if( !(data_type_4) && !(data_type_6) && data_type_7 && data_type_8 && !(data_type_9) && data_type_10 ) return 7;
  else if( data_type_4 && !(data_type_6) && !(data_type_7) && data_type_8 && !(data_type_9) && data_type_10 ) return 8;
  else if( !(data_type_4) && !(data_type_6) && !(data_type_7) && !(data_type_8) && data_type_9 &&
           !(data_type_10) )
    return 9;
  else if( data_type_4 && !(data_type_6) && !(data_type_7) && !(data_type_8) && data_type_9 && !(data_type_10) )
    return 10;
  else {
    //      PrintDataType();
    cout << "ERROR:: Fadc250Module:: GetFadcMode:: FADC is in invalid mode for slot = " << fSlot << endl;
    if( fDebugFile )
      *fDebugFile << "ERROR:: Fadc250Module:: GetFadcMode:: FADC is in invalid mode for slot = " << fSlot << endl;
    return -1;
  }
}

//_____________________________________________________________________________
UInt_t Fadc250Module::GetNumFadcEvents( UInt_t chan ) const
{
  assert(chan < NADCCHAN);
  UInt_t sz = 0;
  Int_t mode = GetFadcMode();
  if( fDebugFile ) PrintDataType();
  // For some "old" firmware version
  if( fFirmwareVers == 1 ) {
    if( mode == 7 &&
        (sz = fPulseData[chan].integral.size()) == fPulseData[chan].time.size() )
      return sz;
    if( mode == 8 )
      return fPulseData[chan].samples.size();
  }
  // The rest for "modern" firmware
  if( mode == 1 )
    return 1;
  else if(
    (mode == 7 &&
     ((sz = fPulseData[chan].integral.size()) == fPulseData[chan].time.size()) &&
     (fPulseData[chan].pedestal.size() == sz) &&
     (fPulseData[chan].peak.size() == sz)) ||
    (mode == 8 &&
     (sz = fPulseData[chan].time.size()) == fPulseData[chan].pedestal.size() &&
     fPulseData[chan].peak.size() == sz) ||
    (mode == 9 && fFirmwareVers == 2 &&
     (sz = fPulseData[chan].integral.size()) == fPulseData[chan].time.size() &&
     fPulseData[chan].pedestal.size() == sz &&
     fPulseData[chan].peak.size() == sz) ||
    (mode == 10 && fFirmwareVers == 2 &&
     (sz = fPulseData[chan].integral.size()) == fPulseData[chan].time.size() &&
     fPulseData[chan].pedestal.size() == sz &&
     fPulseData[chan].peak.size() == sz) ||
    (mode == 9 &&
     (sz = fPulseData[chan].integral.size()) == fPulseData[chan].time.size() &&
     fPulseData[chan].pedestal.size() <= 1 &&
     fPulseData[chan].peak.size() == sz) ||
    (mode == 10 &&
     (sz = fPulseData[chan].integral.size()) == fPulseData[chan].time.size() &&
     fPulseData[chan].pedestal.size() <= 1 &&
     fPulseData[chan].peak.size() == sz)
    ) {
#ifdef WITH_DEBUG
    if( fDebugFile )
      *fDebugFile << "Fadc250Module::GetNumFadcEvents channel "
                  << chan << " = " << sz << endl;
#endif
    return sz;
  } else {
    cout
      << "ERROR:: Fadc250Module:: GetNumFadcEvents:: FADC not in acceptable mode or data vector sizes do not match for for slot = "
      << fSlot << ", channel = " << chan << endl;
    cout << "ERROR:: Fadc250Module:: GetNumFadcEvents:: FADC data potentially corrupt for event " << fadc_data.evt_num
         << ", PROCEED WITH CAUTION!" << endl;
    return kMaxUInt;
  }
}

//_____________________________________________________________________________
UInt_t Fadc250Module::GetNumFadcSamples( UInt_t chan, UInt_t ievent ) const
{
  Int_t mode = GetFadcMode();
  if( (mode == 1) || (mode == 8) || (mode == 10) ) {
    vsiz_t nsamples = fPulseData[chan].samples.size();
    if( ievent >= nsamples ) {
      cout << "ERROR:: Fadc250Module:: GetNumFadcSamples:: invalid event number for slot = " << fSlot << ", channel = "
           << chan << endl;
      return kMaxUInt;
    }
    if( nsamples == 0 ) {
      //cout << "ERROR:: Fadc250Module:: GetNumFadcSamples:: data vector empty for slot = " << fSlot << ", channel = " << chan << endl;
      return kMaxUInt;
    } else {
#ifdef WITH_DEBUG
      if( fDebugFile )
        *fDebugFile << "Fadc250Module::GetNumFadcSamples channel "
                    << chan << ", event " << ievent << " = "
                    << fPulseData[chan].samples.size() << endl;
#endif
      return fPulseData[chan].samples.size();
    }
  } else {
    cout << "ERROR:: Fadc250Module:: GetNumFadcSamples:: FADC is not in Mode 1, 8, or 10 for slot = " << fSlot
         << ", channel = " << chan << endl;
    return kMaxUInt;
  }
}

//_____________________________________________________________________________
inline
void Fadc250Module::DecodeBlockHeader( UInt_t pdat, uint32_t data_type_id )
{
  static const string here{ "Fadc250Module::DecodeBlockHeader" };

  if( data_type_id ) {
    block_header_found = true;                     // Set to true if found
    fadc_data.slot_blk_hdr = (pdat >> 22) & 0x1F;  // Slot number (set by VME64x backplane), mask 5 bits
    // Debug output
#ifdef WITH_DEBUG
    if( fDebugFile )
      *fDebugFile << "Fadc250Module::Decode:: Slot from FADC block header = " << fadc_data.slot_blk_hdr << endl;
#endif
    // Ensure that slots from cratemap and FADC match
    slots_match = ((uint32_t) fSlot == fadc_data.slot_blk_hdr);
    if( !slots_match )
      return;
    fadc_data.mod_id = (pdat >> 18) & 0xF;         // Module ID ('1' for FADC250), mask 4 bits
    fadc_data.iblock_num = (pdat >> 8) & 0x3FF;    // Event block number, mask 10 bits
    fadc_data.nblock_events = (pdat >> 0) & 0xFF;  // Number of events in block, mask 8 bits
    // Debug output
#ifdef WITH_DEBUG
    if( fDebugFile )
      *fDebugFile << "Fadc250Module::Decode:: FADC BLOCK HEADER"
                  << " >> data = " << hex << pdat << dec
                  << " >> slot = " << fadc_data.slot_blk_hdr
                  << " >> module id = " << fadc_data.mod_id << " >> event block number = " << fadc_data.iblock_num
                  << " >> num events in block = " << fadc_data.nblock_events
                  << endl;
#endif
  } else {
    fadc_data.PL =
      (pdat >> 18) & 0x7FF;  // Number of samples before trigger point for processing to begin, mask 11 bits
    fadc_data.NSB =
      (pdat >> 9) & 0x1FF;  // Number of samples before threshold crossing to include processing, mask 9 bits
    fadc_data.NSA =
      (pdat >> 0) & 0x1FF;  // Number of samples after threshold crossing to include processing, mask 9 bits
    // Debug output
#ifdef WITH_DEBUG
    if( fDebugFile )
      *fDebugFile << "Fadc250Module::Decode:: FADC BLOCK HEADER"
                  << " >> data = " << hex << pdat << dec
                  << " >> NSB trigger point (PL) = " << fadc_data.PL
                  << " >> NSB threshold crossing = " << fadc_data.NSA
                  << " >> NSA threshold crossing = " << fadc_data.NSB
                  << endl;
#endif
  }
}

//_____________________________________________________________________________
void Fadc250Module::DecodeBlockTrailer( UInt_t pdat )
{
  block_trailer_found = true;
  fadc_data.slot_blk_trl = (pdat >> 22) & 0x1F;       // Slot number (set by VME64x backplane), mask 5 bits
  fadc_data.nwords_inblock = (pdat >> 0) & 0x3FFFFF;  // Total number of words in block of events, mask 22 bits
  // Debug output
#ifdef WITH_DEBUG
  if( fDebugFile )
    *fDebugFile << "Fadc250Module::Decode:: FADC BLOCK TRAILER"
                << " >> data = " << hex << pdat << dec
                << " >> slot = " << fadc_data.slot_blk_trl
                << " >> nwords in block = " << fadc_data.nwords_inblock
                << endl;
#endif
}

//_____________________________________________________________________________
void Fadc250Module::DecodeEventHeader( UInt_t pdat )
{
  event_header_found = true;
  // For firmware versions pre 0x0C00 (06/09/2016)
  fadc_data.slot_evt_hdr = (pdat >> 22) & 0x1F;  // Slot number (set by VME64x backplane), mask 5 bits
  // fadc_data.evt_num = (pdat >> 0) & 0x3FFFFF;    // Event number, mask 22 bits
  // For firmware versions post 0x0C00 (06/09/2016)
  fadc_data.eh_trig_time = (pdat >> 12) & 0x3FF;  // Event header trigger time
  fadc_data.trig_num = (pdat >> 0) & 0xFFF;       // Trigger number
  // Debug output
  // #ifdef WITH_DEBUG
  //	if (fDebugFile)
  //	  *fDebugFile << "Fadc250Module::Decode:: FADC EVENT HEADER >> data = " << hex
  //		      << data << dec << " >> slot = " << fadc_data.slot_evt_hdr
  //		      << " >> event number = " << fadc_data.evt_num << endl;
  // #endif
#ifdef WITH_DEBUG
  if( fDebugFile )
    *fDebugFile << "Fadc250Module::Decode:: FADC EVENT HEADER"
                << " >> data = " << hex << pdat << dec
                << " >> event header trigger time = " << fadc_data.eh_trig_time
                << " >> trigger number = " << fadc_data.trig_num
                << endl;
#endif
}

//_____________________________________________________________________________
void Fadc250Module::DecodeTriggerTime( UInt_t pdat, uint32_t data_type_id )
{
  if( data_type_id )  // Trigger time word 1
    fadc_data.trig_time_w1 = (pdat >> 0) & 0xFFFFFF;  // Time = T_D T_E T_F
  else  // Trigger time word 2
    fadc_data.trig_time_w2 = (pdat >> 0) & 0xFFFFFF;  // Time = T_A T_B T_C
  // Time = T_A T_B T_C T_D T_E T_F
  fadc_data.trig_time = (fadc_data.trig_time_w2 << 24) | fadc_data.trig_time_w1;
  // Debug output
#ifdef WITH_DEBUG
  if( fDebugFile )
    *fDebugFile << "Fadc250Module::Decode:: FADC TRIGGER TIME"
                << " >> data = " << hex << pdat << dec
                << " >> trigger time word 1 = " << fadc_data.trig_time_w1
                << " >> trigger time word 2 = " << fadc_data.trig_time_w2
                << " >> trigger time = " << fadc_data.trig_time
                << endl;
#endif
}

//_____________________________________________________________________________
void Fadc250Module::DecodeWindowRawData( UInt_t pdat, uint32_t data_type_id )
{
  data_type_4 = true;
  if( data_type_id == 1 ) {
    fadc_data.chan = (pdat >> 23) & 0xF;        // FADC channel number
    fadc_data.win_width = (pdat >> 0) & 0xFFF;  // Window width
    // Debug output
#ifdef WITH_DEBUG
    if( fDebugFile )
      *fDebugFile << "Fadc250Module::Decode:: FADC WINDOW RAW DATA"
                  << " >> data = " << hex << pdat << dec
                  << " >> channel = " << fadc_data.chan
                  << " >> window width  = " << fadc_data.win_width
                  << endl;
#endif
  } else {
    assert(data_type_id == 0);                       // Ensure this is a data continuation word
    Bool_t invalid_1 = (pdat >> 29) & 0x1;             // Check if sample x is invalid
    Bool_t invalid_2 = (pdat >> 13) & 0x1;             // Check if sample x+1 is invalid
    uint32_t sample_1 = 0;                             // ADC sample x (includes overflow bit)
    uint32_t sample_2 = 0;                             // ADC sample x+1 (includes overflow bit)
    if( !invalid_1 ) sample_1 = (pdat >> 16) & 0x1FFF;  // If sample x is valid, assign value
    if( !invalid_2 ) sample_2 = (pdat >> 0) & 0x1FFF;   // If sample x+1 is valid, assign value

    PopulateDataVector(fPulseData[fadc_data.chan].samples, sample_1); // Sample 1
    fadc_data.invalid_samples |= invalid_1;                        // Invalid samples
    fadc_data.overflow = (sample_1 >> 12) & 0x1;                   // Sample 1 overflow bit
    if( invalid_2 ) { // Skip last sample if not expected and flagged as invalid
      if( fPulseData[fadc_data.chan].samples.size() == fadc_data.win_width )
        return;
    }

    PopulateDataVector(fPulseData[fadc_data.chan].samples, sample_2); // Sample 2
    fadc_data.invalid_samples |= invalid_2;                        // Invalid samples
    fadc_data.overflow = (sample_2 >> 12) & 0x1;                   // Sample 2 overflow bit
    // Debug output
#ifdef WITH_DEBUG
    if( fDebugFile )
      *fDebugFile << "Fadc250Module::Decode:: FADC WINDOW RAW DATA"
                  << " >> data = " << hex << pdat << dec
                  << " >> channel = " << fadc_data.chan
                  << " >> sample 1 = " << sample_1
                  << " >> sample 2 = " << sample_2
                  << " >> size of fPulseSamples = "
                  << fPulseData[fadc_data.chan].samples.size()
                  << endl;
#endif
  }  // FADC data sample for window raw data
}

//_____________________________________________________________________________
void Fadc250Module::DecodePulseRawData( UInt_t pdat, uint32_t data_type_id )
{
  data_type_6 = true;
  if( data_type_id == 1 ) {
    fadc_data.chan = (pdat >> 23) & 0xF;            // FADC channel number
    fadc_data.pulse_num = (pdat >> 21) & 0x3;       // FADC pulse number
    fadc_data.sample_num_tc = (pdat >> 0) & 0x3FF;  // FADC sample number of threshold crossing
    // Debug output
#ifdef WITH_DEBUG
    if( fDebugFile )
      *fDebugFile << "Fadc250Module::Decode:: FADC PULSE RAW DATA"
                  << " >> data = " << hex << pdat << dec
                  << " >> channel = " << fadc_data.chan
                  << " >> window width  = " << fadc_data.win_width
                  << endl;
#endif
  } else {
    assert(data_type_id == 0);                       // Ensure this is a data continuation word
    Bool_t invalid_1 = (pdat >> 29) & 0x1;             // Check if sample x is invalid
    Bool_t invalid_2 = (pdat >> 13) & 0x1;             // Check if sample x+1 is invalid
    uint32_t sample_1 = 0;                             // ADC sample x (includes overflow bit)
    uint32_t sample_2 = 0;                             // ADC sample x+1 (includes overflow bit)
    if( !invalid_1 ) sample_1 = (pdat >> 16) & 0x1FFF;  // If sample x is valid, assign value
    if( !invalid_2 ) sample_2 = (pdat >> 0) & 0x1FFF;   // If sample x+1 is valid, assign value

    PopulateDataVector(fPulseData[fadc_data.chan].samples, sample_1);  // Sample 1
    fadc_data.invalid_samples |= invalid_1;                         // Invalid samples
    fadc_data.overflow = (sample_1 >> 12) & 0x1;                    // Sample 1 overflow bit
    if( invalid_2 ) { // Skip last sample if not expected and flagged as invalid
      if( fPulseData[fadc_data.chan].samples.size() == fadc_data.win_width )
        return;
    }

    PopulateDataVector(fPulseData[fadc_data.chan].samples, sample_2);  // Sample 2
    fadc_data.invalid_samples |= invalid_2;                         // Invalid samples
    fadc_data.overflow = (sample_2 >> 12) & 0x1;                    // Sample 2 overflow bit
    // Debug output
#ifdef WITH_DEBUG
    if( fDebugFile )
      *fDebugFile << "Fadc250Module::Decode:: FADC PULSE RAW DATA"
                  << " >> data = " << hex << pdat << dec
                  << " >> sample 1 = " << sample_1
                  << " >> sample 2 = " << sample_2
                  << " >> size of fPulseSamples = "
                  << fPulseData[fadc_data.chan].samples.size()
                  << endl;
#endif
  }  // FADC data sample loop for pulse raw data
}

//_____________________________________________________________________________
void Fadc250Module::DecodePulseIntegral( UInt_t pdat )
{
  data_type_7 = true;
  fadc_data.chan = (pdat >> 23) & 0xF;               // FADC channel number
  fadc_data.pulse_num = (pdat >> 21) & 0x3;          // FADC pulse number
  fadc_data.qual_factor = (pdat >> 19) & 0x3;        // FADC quality factor (0-3)
  fadc_data.pulse_integral = (pdat >> 0) & 0x7FFFF;  // FADC pulse integral
  // Store data in arrays of vectors
  PopulateDataVector(fPulseData[fadc_data.chan].integral, fadc_data.pulse_integral);
  // Debug output
#ifdef WITH_DEBUG
  if( fDebugFile )
    *fDebugFile << "Fadc250Module::Decode:: FADC PULSE INTEGRAL"
                << " >> data = " << hex << pdat << dec
                << " >> chan = " << fadc_data.chan
                << " >> pulse num = " << fadc_data.pulse_num
                << " >> quality factor = " << fadc_data.qual_factor
                << " >> pulse integral = " << fadc_data.pulse_integral
                << endl;
#endif
}

//_____________________________________________________________________________
void Fadc250Module::DecodePulseTime( UInt_t pdat )
{
  data_type_8 = true;
  fadc_data.chan = (pdat >> 23) & 0xF;                // FADC channel number
  fadc_data.pulse_num = (pdat >> 21) & 0x3;           // FADC pulse number
  fadc_data.qual_factor = (pdat >> 19) & 0x3;         // FADC quality factor (0-3)
  fadc_data.coarse_pulse_time = (pdat >> 6) & 0x1FF;  // FADC coarse time (4 ns/count)
  fadc_data.fine_pulse_time = (pdat >> 0) & 0x3F;     // FADC fine time (0.0625 ns/count)
  fadc_data.time = (pdat >> 0) & 0x7FFF;              // FADC time (0.0625 ns/count, bmoffit)
  // Store data in arrays of vectors
  PopulateDataVector(fPulseData[fadc_data.chan].coarse_time, fadc_data.coarse_pulse_time);
  PopulateDataVector(fPulseData[fadc_data.chan].fine_time, fadc_data.fine_pulse_time);
  PopulateDataVector(fPulseData[fadc_data.chan].time, fadc_data.time);
  // Debug output
#ifdef WITH_DEBUG
  if( fDebugFile )
    *fDebugFile << "Fadc250Module::Decode:: FADC PULSE TIME"
                << " >> data = " << hex << pdat << dec
                << " >> chan = " << fadc_data.chan
                << " >> pulse num = " << fadc_data.pulse_num
                << " >> quality factor = " << fadc_data.qual_factor
                << " >> coarse time = " << fadc_data.coarse_pulse_time
                << " >> fine time = " << fadc_data.fine_pulse_time
                << " >> time = " << fadc_data.time
                << endl;
#endif
}

//_____________________________________________________________________________
void Fadc250Module::DecodePulseParameters( UInt_t pdat, uint32_t data_type_id )
{
  data_type_9 = true;
  if( data_type_id == 1 ) {
    fadc_data.evnt_of_blk = (pdat >> 19) & 0xFF;      // Event number within block
    fadc_data.chan = (pdat >> 15) & 0xF;              // FADC channel number
    fadc_data.qual_factor = (pdat >> 14) & 0x1;       // Pedestal quality
    fadc_data.pedestal_sum = (pdat >> 0) & 0x3FFF;    // Pedestal sum
    // Populate data vectors
    PopulateDataVector(fPulseData[fadc_data.chan].pedestal, fadc_data.pedestal_sum);
    PopulateDataVector(fPulseData[fadc_data.chan].pedestal_quality, fadc_data.qual_factor);
    // Debug output
#ifdef WITH_DEBUG
    if( fDebugFile )
      *fDebugFile << "Fadc250Module::Decode:: FADC PULSE PARAMETERS"
                  << " >> data = " << hex << pdat << dec
                  << " >> chan = " << fadc_data.chan
                  << " >> event of block = " << fadc_data.evnt_of_blk
                  << " >> quality factor = " << fadc_data.qual_factor
                  << " >> pedestal sum = " << fadc_data.pedestal_sum
                  << endl;
#endif
  } else {
    assert(data_type_id == 0);                         // Ensure this is a data continuation word
    if( ((pdat >> 30) & 0x1) == 1 ) {                     // Ensure that data is the integral of pulse window word
      fadc_data.sample_sum =
        (pdat >> 12) & 0x3FFFF;     // 18-bit sum of raw samples that constitute the pulse data set
      fadc_data.nsa_ext = (pdat >> 11) & 0x1;            // NSA extended beyond PTW
      fadc_data.samp_overflow = (pdat >> 10) & 0x1;      // One or more samples is overflow
      fadc_data.samp_underflow = (pdat >> 9) & 0x1;      // One or more samples is underflow
      fadc_data.samp_over_thresh =
        (pdat >> 0) & 0x1FF;  // Number of samples within NSA that the pulse is above threshold
      // Populate data vectors
      PopulateDataVector(fPulseData[fadc_data.chan].integral, fadc_data.sample_sum);
      PopulateDataVector(fPulseData[fadc_data.chan].overflow, fadc_data.samp_overflow);
      PopulateDataVector(fPulseData[fadc_data.chan].underflow, fadc_data.samp_underflow);
      // Debug output
#ifdef WITH_DEBUG
      if( fDebugFile )
        *fDebugFile << "Fadc250Module::Decode:: FADC PULSE PARAMETERS"
                    << " >> data = " << hex << pdat << dec
                    << " >> chan = " << fadc_data.chan
                    << " >> integral = " << fadc_data.sample_sum
                    << " >> nsa extended = " << fadc_data.nsa_ext
                    << " >> sample overflow = " << fadc_data.samp_overflow
                    << " >> sample underflow = " << fadc_data.samp_underflow
                    << " >> sample over threshold = " << fadc_data.samp_over_thresh
                    << endl;
#endif
    }
    if( ((pdat >> 30) & 0x1) == 0 ) {                       // Ensure that data is the time of pulse window word
      fadc_data.coarse_pulse_time = (pdat >> 21) & 0x1FF;  // Coarse time (4 ns/count)
      fadc_data.fine_pulse_time = (pdat >> 15) & 0x3F;     // Fine time (0.0625 ns/count)
      fadc_data.time = (pdat >> 15) & 0x7FFF;              // FADC time (0.0625 ns/count)
      fadc_data.pulse_peak = (pdat >> 3) & 0xFFF;          // Pulse peak
      fadc_data.peak_beyond_nsa =
        (pdat >> 2) & 0x1;       // Pulse peak is beyond NSA or could be beyond window end
      fadc_data.peak_not_found = (pdat >> 1) & 0x1;        // Pulse peak cannot be found
      fadc_data.peak_above_maxped =
        (pdat >> 0) & 0x1;     // 1 or more of first four samples is above either MaxPed or TET
      // Populate data vectors
      PopulateDataVector(fPulseData[fadc_data.chan].coarse_time, fadc_data.coarse_pulse_time);
      PopulateDataVector(fPulseData[fadc_data.chan].fine_time, fadc_data.fine_pulse_time);
      PopulateDataVector(fPulseData[fadc_data.chan].time, fadc_data.time);
      PopulateDataVector(fPulseData[fadc_data.chan].peak, fadc_data.pulse_peak);
      // Debug output
#ifdef WITH_DEBUG
      if( fDebugFile )
        *fDebugFile << "Fadc250Module::Decode:: FADC PULSE PARAMETERS"
                    << " >> data = " << hex << pdat << dec
                    << " >> chan = " << fadc_data.chan
                    << " >> coarse time = " << fadc_data.coarse_pulse_time
                    << " >> fine time = " << fadc_data.fine_pulse_time
                    << " >> time = " << fadc_data.time
                    << " >> pulse peak = " << fadc_data.pulse_peak
                    << " >> peak beyond nsa = " << fadc_data.peak_beyond_nsa
                    << " >> peak not found = " << fadc_data.peak_not_found
                    << " >> peak above maxped = " << fadc_data.peak_above_maxped
                    << endl;
#endif
    }
  }
}

//_____________________________________________________________________________
void Fadc250Module::DecodePulsePedestal( UInt_t pdat )
{
  data_type_10 = true;
  fadc_data.chan = (pdat >> 23) & 0xF;          // FADC channel number
  fadc_data.pulse_num = (pdat >> 21) & 0x3;     // FADC pulse number
  fadc_data.pedestal = (pdat >> 12) & 0x1FF;    // FADC pulse pedestal
  fadc_data.pulse_peak = (pdat >> 0) & 0xFFF;   // FADC pulse peak
  // Store data in arrays of vectors
  PopulateDataVector(fPulseData[fadc_data.chan].pedestal, fadc_data.pedestal);
  PopulateDataVector(fPulseData[fadc_data.chan].peak, fadc_data.pulse_peak);
  // Debug output
#ifdef WITH_DEBUG
  if( fDebugFile )
    *fDebugFile << "Fadc250Module::Decode:: FADC PULSE PEDESTAL"
                << " >> data = " << hex << pdat << dec
                << " >> chan = " << fadc_data.chan
                << " >> pulse num = " << fadc_data.pulse_num
                << " >> pedestal = " << fadc_data.pedestal
                << " >> pulse peak = " << fadc_data.pulse_peak
                << endl;
#endif
}

//_____________________________________________________________________________
void Fadc250Module::DecodeScalerHeader( UInt_t pdat )
{
  fadc_data.scaler_words = (pdat >> 0) & 0x3F;  // FADC scaler words
  // Debug output
#ifdef WITH_DEBUG
  if( fDebugFile )
    *fDebugFile << "Fadc250Module::Decode:: FADC SCALER HEADER"
                << " >> data = " << hex << pdat << dec
                << " >> data words = " << hex << fadc_data.scaler_words << dec
                << endl;
#endif
}

//_____________________________________________________________________________
void Fadc250Module::UnsupportedType( UInt_t pdat, uint32_t data_type_id )
{
  // Handle unsupported, invalid, or irrelevant/non-decodable data types
#ifdef WITH_DEBUG
  // Data type descriptions
  static const vector<string> what_text{ "UNDEFINED TYPE",
                                         "DATA NOT VALID",
                                         "FILLER WORD" };
  // Lookup table data_type -> message number
  static const map<uint32_t, uint32_t> what_map = {
    // undefined type
    { 5,  0 },
    { 11, 0 },
    { 13, 0 },
    // data not valid
    { 14, 1 },
    // filler word
    { 15, 2 }
  };
  auto idx = what_map.find(data_type_def);
  assert( idx != what_map.end() );  // else logic error in Decode()
  if( idx == what_map.end() )
    return;
  // Message index. The last message means this function was called when
  // it shouldn't have been called, i.e. coding error in DecodeOneWord
  const string& what = what_text[idx->second];
  ostringstream str;
  str << "Fadc250Module::Decode:: " << what
      << " >> data = " << hex << pdat << dec
      << " >> data type id = " << data_type_id
      << endl;
  if( fDebugFile )
    *fDebugFile << str.str();
  if( fDebug > 2 )
    cerr << str.str();
#endif
}

//_____________________________________________________________________________
Int_t Fadc250Module::Decode( const UInt_t* pdat )
{
  assert(pdat);
  uint32_t data = *pdat;
  uint32_t data_type_id = (data >> 31) & 0x1;  // Data type identification, mask 1 bit
  if( data_type_id == 1 )
    data_type_def = (data >> 27) & 0xF;        // Data type defining words, mask 4 bits

  // Debug output
#ifdef WITH_DEBUG
  if( fDebugFile )
    *fDebugFile << "Fadc250Module::Decode:: FADC DATA TYPES"
                << " >> data = " << hex << data << dec
                << " >> data word id = " << data_type_id
                << " >> data type = " << data_type_def
                << endl;

#endif

  // Ensure that slots match and do not decode if they differ.
  // This should never happen if PipeliningModule::LoadBank selected the
  // correct bank.
  if( !slots_match && data_type_def != 0 ) {
#ifdef WITH_DEBUG
    if( fDebugFile )
      *fDebugFile << "Fadc250Module::Decode:: "
                  << "fSlot & FADC slot do not match AND data type != 0"
                  << endl;
#endif
    return -1;
  }

  // Acquire data objects depending on the data type defining word
  switch( data_type_def ) {
    case 0: // Block header, indicates the beginning of a block of events
      DecodeBlockHeader(data, data_type_id);
      break;
    case 1: // Block trailer, indicates the end of a block of events
      DecodeBlockTrailer(data);
      break;
    case 2: // Event header, indicates start of an event, includes the trigger number
      DecodeEventHeader(data);
      break;
    case 3:  // Trigger time, time of trigger occurrence relative to the most recent global reset
      DecodeTriggerTime(data, data_type_id);
      break;
    case 4:  // Window raw data
      DecodeWindowRawData(data, data_type_id);
      break;
    case 6:  // Pulse raw data
      DecodePulseRawData(data, data_type_id);
      break;
    case 7:  // Pulse integral
      DecodePulseIntegral(data);
      break;
    case 8:  // Pulse time
      DecodePulseTime(data);
      break;
    case 9:  // Pulse Parameters
      DecodePulseParameters(data, data_type_id);
      break;
    case 10: // Pulse Pedestal
      DecodePulsePedestal(data);
      break;
    case 12: // Scaler header
      DecodeScalerHeader(data);
      break;
    case 5:  // Undefined type
    case 11: // Undefined type
    case 13: // Undefined type
    case 14: // Data not valid
    case 15: // Filler Word, should be ignored
#ifdef WITH_DEBUG
      if( fDebugFile || fDebug > 2 )
        UnsupportedType(data, data_type_id);
#endif
      break;
    default:
      throw logic_error("Fadc250Module: incorrect masking of data_type_def");
  }  // data_type_def switch

#ifdef WITH_DEBUG
  if( fDebugFile )
    *fDebugFile << "**********************************************************************"
                << "\n" << endl;
#endif

  return block_trailer_found;
}

//_____________________________________________________________________________
void Fadc250Module::LoadTHaSlotDataObj( THaSlotData* sldat )
{
  // Load THaSlotData
  for( uint32_t chan = 0; chan < NADCCHAN; chan++ ) {
    // Pulse Integral
    for( vsiz_t ievent = 0; ievent < fPulseData[chan].integral.size(); ievent++ )
      sldat->loadData("adc", chan, fPulseData[chan].integral[ievent], fPulseData[chan].integral[ievent]);
    // Pulse Time
    for( vsiz_t ievent = 0; ievent < fPulseData[chan].time.size(); ievent++ )
      sldat->loadData("adc", chan, fPulseData[chan].time[ievent], fPulseData[chan].time[ievent]);
    // Pulse Peak
    for( vsiz_t ievent = 0; ievent < fPulseData[chan].peak.size(); ievent++ )
      sldat->loadData("adc", chan, fPulseData[chan].peak[ievent], fPulseData[chan].peak[ievent]);
    // Pulse Pedestal
    for( vsiz_t ievent = 0; ievent < fPulseData[chan].pedestal.size(); ievent++ )
      sldat->loadData("adc", chan, fPulseData[chan].pedestal[ievent], fPulseData[chan].pedestal[ievent]);
    // Pulse Samples
    for( vsiz_t ievent = 0; ievent < fPulseData[chan].samples.size(); ievent++ )
      sldat->loadData("adc", chan, fPulseData[chan].samples[ievent], fPulseData[chan].samples[ievent]);
  }  // Channel loop
}

//_____________________________________________________________________________
UInt_t Fadc250Module::LoadSlot( THaSlotData* sldat, const UInt_t* evbuffer,
                                const UInt_t* pstop )
{
  // Load from evbuffer between [evbuffer,pstop]

  return LoadSlot(sldat, evbuffer, 0, pstop + 1 - evbuffer);
}

//_____________________________________________________________________________
UInt_t Fadc250Module::LoadSlot( THaSlotData *sldat, const UInt_t* evbuffer,
                                UInt_t pos, UInt_t len)
{
  // Load from bank data in evbuffer between [pos,pos+len)


  const auto* p = evbuffer + pos;
  const auto* q = p + len;
  while( p != q ) {
    if( Decode(p++) == 1 )
      break;  // block trailer found
  }

  LoadTHaSlotDataObj(sldat);

  return fWordsSeen = p - (evbuffer + pos);
}

//_____________________________________________________________________________

} //namespace Decoder

ClassImp(Decoder::Fadc250Module)
