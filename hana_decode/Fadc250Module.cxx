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
#include "PipeliningModule.h"
#include "THaEvData.h"
#include "THaSlotData.h"
#include "TMath.h"

#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>  // for memset
#include <iostream>
#include <string>
#include <sstream>
#include <cmath>
#include <iomanip>
#include <numeric>
#include <cassert>

using namespace std;

// #define DEBUG
// #define WITH_DEBUG

namespace Decoder {

  Module::TypeIter_t Fadc250Module::fgThisType =
    DoRegister( ModuleType( "Decoder::Fadc250Module" , 250 ));

  Fadc250Module::Fadc250Module()
    : PipeliningModule(), fPulseData(NADCCHAN)
  { memset(&fadc_data, 0, sizeof(fadc_data)); }

  Fadc250Module::Fadc250Module(Int_t crate, Int_t slot)
    : PipeliningModule(crate, slot), fPulseData(NADCCHAN)
  {
    memset(&fadc_data, 0, sizeof(fadc_data));
    IsInit = kFALSE;
    Init();
  }

  Fadc250Module::~Fadc250Module() {
#if defined DEBUG && defined WITH_DEBUG
    // delete fDebugFile; fDebugFile = 0;
#endif
  }

  Bool_t Fadc250Module::IsMultiFunction() {
    return kTRUE;
  }

  Bool_t Fadc250Module::HasCapability(Decoder::EModuleType type) {
    return ( type == kSampleADC || type == kPulseIntegral || type == kPulseTime
	     || type == kPulsePeak || type == kPulsePedestal
	     || type == kCoarseTime || type == kFineTime);
  }

  // Clear all data vectors
  void Fadc250Module::ClearDataVectors() {
    // Clear all data objects
    assert(fPulseData.size() == NADCCHAN);  // Initialization error in constructor
    for (uint32_t i = 0; i < NADCCHAN; i++) {
      fPulseData[i].clear();
    }
  }

  // Require that slot from base class and slot from
  //   data match before populating data vectors
  void Fadc250Module::PopulateDataVector(vector<uint32_t>& data_vector, uint32_t data) {
    if (static_cast <uint32_t> (fSlot) == fadc_data.slot_blk_hdr)
      data_vector.push_back(data);
  }

  // Sum elements contained in data vector
  Int_t Fadc250Module::SumVectorElements(const vector<uint32_t>& data_vector) const {
    Int_t sum_of_elements = 0;
    sum_of_elements = accumulate(data_vector.begin(), data_vector.end(), 0);
    return sum_of_elements;
  }

  void Fadc250Module::Clear( const Option_t* opt) {
    // Clear event-by-event data
    VmeModule::Clear(opt);
    ClearDataVectors();
    // Initialize data_type_def to FILLER and data types to false
    data_type_def = 15;
    // Initialize data types to false
    data_type_4 = data_type_6 = data_type_7 = data_type_8 = data_type_9 = data_type_10 = false;
    block_header_found = block_trailer_found = event_header_found = slots_match = false;
  }

  void Fadc250Module::Init() {
#if defined DEBUG && defined WITH_DEBUG
    // This will make a HUGE output
    // delete fDebugFile; fDebugFile = 0;
    // fDebugFile = new ofstream;
    // fDebugFile->open("fadcdebug.dat");
#endif
    Clear();
    IsInit = kTRUE;
    fName = "FADC250 JLab Flash ADC Module";
  }

  void Fadc250Module::CheckDecoderStatus() const {
    cout << "FADC250 Decoder has been called" << endl;
  }

  Int_t Fadc250Module::GetNumEvents(Decoder::EModuleType emode, Int_t chan) const {
    switch(emode)
      {
      case kSampleADC:
	return fPulseData[chan].samples.size();
      case kPulseIntegral:
	return fPulseData[chan].integral.size();
      case kPulseTime:
	return fPulseData[chan].time.size();
      case kPulsePeak:
	return fPulseData[chan].peak.size();
      case kPulsePedestal:
	if (fFirmwareVers == 2) return fPulseData[chan].pedestal.size();
	else return fPulseData[chan].integral.size();
      case kCoarseTime:
	return fPulseData[chan].coarse_time.size();
      case kFineTime:
	return fPulseData[chan].fine_time.size();
      }
    return 0;
  }

  Int_t Fadc250Module::GetData(Decoder::EModuleType emode, Int_t chan, Int_t ievent) const {
    switch(emode)
      {
      case kSampleADC:
	return GetPulseSamplesData(chan, ievent);
      case kPulseIntegral:
	return  GetPulseIntegralData(chan, ievent);
      case kPulseTime:
	return  GetPulseTimeData(chan, ievent);
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

  Int_t Fadc250Module::GetPulseIntegralData(Int_t chan, Int_t ievent) const {
    Int_t nevent = 0;
    nevent = fPulseData[chan].integral.size();
    if (ievent < 0 || ievent > nevent) {
      cout << "ERROR:: Fadc250Module:: GetPulseIntegralData:: invalid event number for slot = " << fSlot << ", channel = " << chan << endl;
      return -1;
    }
    if (nevent == 0) {
      cout << "ERROR:: Fadc250Module:: GetPulseIntegralData:: data vector empty for slot = " << fSlot << ", channel = " << chan << endl;
      return -1;
    }
    else {
      return fPulseData[chan].integral[ievent];
#ifdef WITH_DEBUG
      if (fDebugFile != 0)
	*fDebugFile << "Fadc250Module::GetPulseIntegralData channel "
		    << chan << ", event " << ievent << " = "
		    <<  fPulseData[chan].integral[ievent] << endl;
#endif
    }
  }

  Int_t Fadc250Module::GetEmulatedPulseIntegralData(Int_t chan) const {
    Int_t nevent = 0;
    nevent = fPulseData[chan].samples.size();
    if (nevent == 0) {
      cout << "ERROR:: Fadc250Module:: GetEmulatedPulseIntegralData:: data vector empty  for slot = " << fSlot << ", channel = " << chan << "\n"
	   << "Ensure that FADC is operating in mode 1 OR 8" << endl;
      return -1;
    }
    else {
      return SumVectorElements(fPulseData[chan].samples);
#ifdef WITH_DEBUG
      if (fDebugFile != 0)
	*fDebugFile << "Fadc250Module::GetEmulatedPulseIntegralData channel "
		    << chan << " = " <<  SumVectorElements(fPulseData[chan].samples) << endl;
#endif
    }
  }

  Int_t Fadc250Module::GetPulseTimeData(Int_t chan, Int_t ievent) const {
    Int_t nevent = 0;
    nevent = fPulseData[chan].time.size();
    if (ievent < 0 || ievent > nevent) {
      cout << "ERROR:: Fadc250Module:: GetPulseTimeData:: invalid event number for slot = " << fSlot << ", channel = " << chan << endl;
      return -1;
    }
    if (nevent == 0) {
      cout << "ERROR:: Fadc250Module:: GetPulseTimeData:: data vector empty for slot = " << fSlot << ", channel = " << chan << endl;
      return -1;
    }
    else {
      return fPulseData[chan].time[ievent];
#ifdef WITH_DEBUG
      if (fDebugFile != 0)
	*fDebugFile << "Fadc250Module::GetPulseTimeData channel "
		    << chan << ", event " << ievent << " = "
		    <<  fPulseData[chan].time[ievent] << endl;
#endif
    }
  }

  Int_t Fadc250Module::GetPulseCoarseTimeData(Int_t chan, Int_t ievent) const {
    Int_t nevent = 0;
    nevent = fPulseData[chan].coarse_time.size();
    if (ievent < 0 || ievent > nevent) {
      cout << "ERROR:: Fadc250Module:: GetPulseCoarseTimeData:: invalid event number for slot = " << fSlot << ", channel = " << chan << endl;
      return -1;
    }
    if (nevent == 0) {
      cout << "ERROR:: Fadc250Module:: GetPulseCoarseTimeData:: data vector empty for slot = " << fSlot << ", channel = " << chan << endl;
      return -1;
    }
    else {
      return fPulseData[chan].coarse_time[ievent];
#ifdef WITH_DEBUG
      if (fDebugFile != 0)
	*fDebugFile << "Fadc250Module::GetPulseCoarseTimeData channel "
		    << chan << ", event " << ievent << " = "
		    <<  fPulseData[chan].coarse_time[ievent] << endl;
#endif
    }
  }

  Int_t Fadc250Module::GetPulseFineTimeData(Int_t chan, Int_t ievent) const {
    Int_t nevent = 0;
    nevent = fPulseData[chan].fine_time.size();
    if (ievent < 0 || ievent > nevent) {
      cout << "ERROR:: Fadc250Module:: GetPulseCoarseTimeData:: invalid event number for slot = " << fSlot << ", channel = " << chan << endl;
      return -1;
    }
    if (nevent == 0) {
      cout << "ERROR:: Fadc250Module:: GetPulseCoarseTimeData:: data vector empty for slot = " << fSlot << ", channel = " << chan << endl;
      return -1;
    }
    else {
      return fPulseData[chan].fine_time[ievent];
#ifdef WITH_DEBUG
      if (fDebugFile != 0)
	*fDebugFile << "Fadc250Module::GetPulseFineTimeData channel "
		    << chan << ", event " << ievent << " = "
		    <<  fPulseData[chan].fine_time[ievent] << endl;
#endif
    }
  }

  Int_t Fadc250Module::GetPulsePeakData(Int_t chan, Int_t ievent) const {
    Int_t nevent = 0;
    nevent = fPulseData[chan].peak.size();
    if (ievent < 0 || ievent > nevent) {
      cout << "ERROR:: Fadc250Module:: GetPulsePeakData:: invalid event number for slot = " << fSlot << ", channel = " << chan << endl;
      return -1;
    }
    if (nevent == 0) {
      cout << "ERROR:: Fadc250Module:: GetPulsePeakData:: data vector empty for slot = " << fSlot << ", channel = " << chan << endl;
      return -1;
    }
    else {
      return fPulseData[chan].peak[ievent];
#ifdef WITH_DEBUG
      if (fDebugFile != 0)
	*fDebugFile << "Fadc250Module::GetPulsePeakData channel "
		    << chan << ", event " << ievent << " = "
		    <<  fPulseData[chan].peak[ievent] << endl;
#endif
    }
  }

  Int_t Fadc250Module::GetPulsePedestalData(Int_t chan, Int_t ievent) const {
    Int_t nevent = 0;
    nevent = fPulseData[chan].pedestal.size();
    if (ievent < 0) {
      cout << "ERROR:: Fadc250Module:: GetPulsePedestalData:: invalid event number for slot = " << fSlot << ", channel = " << chan << endl;
      return -1;
    }
    if (nevent == 0) {
      cout << "ERROR:: Fadc250Module:: GetPulsePedestalData:: data vector empty for slot = " << fSlot << ", channel = " << chan << endl;
      return -1;
    }
    if (fFirmwareVers == 2) {
      if (ievent > nevent) {
	cout << "ERROR:: Fadc250Module:: GetPulsePedestalData:: invalid data vector size = " << fSlot << ", channel = " << chan << endl;
	return -1;
      }
      else {    
	return fPulseData[chan].pedestal[ievent];
#ifdef WITH_DEBUG
	if (fDebugFile != 0)
	  *fDebugFile << "Fadc250Module::GetPulsePedestalData channel "
		      << chan << ", event " << ievent << " = "
		      <<  fPulseData[chan].pedestal[ievent] << endl;
#endif
      }
    }
    if (nevent != 1) {
      cout << "ERROR:: Fadc250Module:: GetPulsePedestalData:: invalid data vector size = " << fSlot << ", channel = " << chan << endl;
      return -1;
    }
    else {    
      return fPulseData[chan].pedestal[0];
#ifdef WITH_DEBUG
      if (fDebugFile != 0)
	*fDebugFile << "Fadc250Module::GetPulsePedestalData channel "
		    << chan << ", event " << ievent << " = "
		    <<  fPulseData[chan].pedestal[0] << endl;
#endif
    }
  }

  Int_t Fadc250Module::GetPedestalQuality(Int_t chan, Int_t ievent) const {
    Int_t nevent = 0;
    nevent = fPulseData[chan].pedestal_quality.size();
    if (ievent < 0) {
      cout << "ERROR:: Fadc250Module:: GetPedestalQuality:: invalid event number for slot = " << fSlot << ", channel = " << chan << endl;
      return -1;
    }
    if (nevent == 0) {
      cout << "ERROR:: Fadc250Module:: GetPedestalQuality:: data vector empty for slot = " << fSlot << ", channel = " << chan << endl;
      return -1;
    }
    if (nevent != 1) {
      cout << "ERROR:: Fadc250Module:: GetPedestalQuality:: invalid data vector size = " << fSlot << ", channel = " << chan << endl;
      return -1;
    }
    else {    
      return fPulseData[chan].pedestal_quality[0];
#ifdef WITH_DEBUG
      if (fDebugFile != 0)
	*fDebugFile << "Fadc250Module::GetPedestalQuality channel "
		    << chan << ", event " << ievent << " = "
		    <<  fPulseData[chan].pedestal_quality[0] << endl;
#endif
    }
  }

  Int_t Fadc250Module::GetOverflowBit(Int_t chan, Int_t ievent) const {
    Int_t nevent = 0;
    nevent = fPulseData[chan].overflow.size();
    if (ievent < 0) {
      cout << "ERROR:: Fadc250Module:: GetOverflowBit:: invalid event number for slot = " << fSlot << ", channel = " << chan << endl;
      return -1;
    }
    if (nevent == 0) {
      cout << "ERROR:: Fadc250Module:: GetOverflowBit:: data vector empty for slot = " << fSlot << ", channel = " << chan << endl;
      return -1;
    }
    else {
      return fPulseData[chan].overflow[ievent];
#ifdef WITH_DEBUG
      if (fDebugFile != 0)
	*fDebugFile << "Fadc250Module::GetOverflowBit channel "
		    << chan << ", event " << ievent << " = "
		    <<  fPulseData[chan].overflow[ievent] << endl;
#endif
    }
  }
  
  Int_t Fadc250Module::GetUnderflowBit(Int_t chan, Int_t ievent) const {
    Int_t nevent = 0;
    nevent = fPulseData[chan].underflow.size();
    if (ievent < 0) {
      cout << "ERROR:: Fadc250Module:: GetUnderflowBit:: invalid event number for slot = " << fSlot << ", channel = " << chan << endl;
      return -1;
    }
    if (nevent == 0) {
      cout << "ERROR:: Fadc250Module:: GetUnderflowBit:: data vector empty for slot = " << fSlot << ", channel = " << chan << endl;
      return -1;
    }
    else {
      return fPulseData[chan].underflow[ievent];
#ifdef WITH_DEBUG
      if (fDebugFile != 0)
	*fDebugFile << "Fadc250Module::GetUnderflowBit channel "
		    << chan << ", event " << ievent << " = "
		    <<  fPulseData[chan].underflow[ievent] << endl;
#endif
    }
  }

  Int_t Fadc250Module::GetTriggerTime() const {
    // Truncate to 32 bits
    Int_t shorttime=fadc_data.trig_time;
#ifdef WITH_DEBUG
    if (fDebugFile != 0)
      *fDebugFile << "Fadc250Module::GetTriggerTime = "
      << fadc_data.trig_time << " " << shorttime << endl;
#endif	
    return shorttime;
  }
  
  Int_t Fadc250Module::GetPulseSamplesData(Int_t chan, Int_t ievent) const {
    Int_t nevent = 0;
    nevent = fPulseData[chan].samples.size();
    if (ievent < 0 || ievent > nevent) {
      cout << "ERROR:: Fadc250Module:: GetPulseSamplesData:: invalid event number for slot = " << fSlot << ", channel = " << chan << endl;
      return -1;
    }
    if (nevent == 0) {
      cout << "ERROR:: Fadc250Module:: GetPulseSamplesData:: data vector empty for slot = " << fSlot << ", channel = " << chan << endl;
      return -1;
    }
    else {
      return fPulseData[chan].samples[ievent];
#ifdef WITH_DEBUG
      if (fDebugFile != 0)
	*fDebugFile << "Fadc250Module::GetPulseSamplesData channel "
		    << chan << ", event " << ievent << " = "
		    <<  fPulseData[chan].samples[ievent] << endl;
#endif
    }
  }

  vector<uint32_t> Fadc250Module::GetPulseSamplesVector(Int_t chan) const {
    Int_t nevent = 0;
    nevent = fPulseData[chan].samples.size();
    if (nevent == 0) {
      cout << "ERROR:: Fadc250Module:: GetPulseSamplesVector:: data vector empty for slot = " << fSlot << ", channel = " << chan << endl;
      return vector<uint32_t>();
    }
    else {
      return fPulseData[chan].samples;
#ifdef WITH_DEBUG
      if (fDebugFile != 0)
	*fDebugFile << "Fadc250Module::GetPulseSamplesVector channel "
		    << chan << " = " <<  &fPulseData[chan].samples << endl;
#endif
    }
  }

  void Fadc250Module::PrintDataType() const {
    if (fDebugFile == 0) return;
    *fDebugFile << "start,  Print Data Type "<<endl;
    if (data_type_4) *fDebugFile << "data type 4"<<endl;
    if (data_type_6) *fDebugFile << "data type 6"<<endl;
    if (data_type_7) *fDebugFile << "data type 7"<<endl;
    if (data_type_8) *fDebugFile << "data type 8"<<endl;
    if (data_type_9) *fDebugFile << "data type 9"<<endl;
    if (data_type_10) *fDebugFile << "data type 10"<<endl;
    *fDebugFile << "end,  Print Data Type "<<endl;
  }

  Int_t Fadc250Module::GetFadcMode() const {
    if (fFirmwareVers==1) {  // some "older" firmware
      if (data_type_4 && data_type_6) return 8;
      if (data_type_7) return 7;
    }
    if      (data_type_4    && !(data_type_6) && !(data_type_7) && !(data_type_8) && !(data_type_9) && !(data_type_10)) return 1;
    else if (!(data_type_4) && data_type_6    && !(data_type_7) && !(data_type_8) && !(data_type_9) && !(data_type_10)) return 2;
    else if (!(data_type_4) && !(data_type_6) && data_type_7    && data_type_8    && !(data_type_9) && !(data_type_10)) return 3;
    else if (!(data_type_4) && !(data_type_6) && !(data_type_7) && data_type_8    && !(data_type_9) && data_type_10)    return 4;
    else if (!(data_type_4) && !(data_type_6) && data_type_7    && data_type_8    && !(data_type_9) && data_type_10)    return 7;
    else if (data_type_4    && !(data_type_6) && !(data_type_7) && data_type_8    && !(data_type_9) && data_type_10)    return 8;
    else if (!(data_type_4) && !(data_type_6) && !(data_type_7) && !(data_type_8) && data_type_9    && !(data_type_10)) return 9;
    else if (data_type_4    && !(data_type_6) && !(data_type_7) && !(data_type_8) && data_type_9    && !(data_type_10)) return 10;
    else {
      //      PrintDataType();
      cout << "ERROR:: Fadc250Module:: GetFadcMode:: FADC is in invalid mode for slot = " << fSlot << endl;
      if (fDebugFile) *fDebugFile << "ERROR:: Fadc250Module:: GetFadcMode:: FADC is in invalid mode for slot = " << fSlot << endl;
      return -1;
    }
  }

  Int_t Fadc250Module::GetNumFadcEvents(Int_t chan) const {
    assert(chan >= 0 && static_cast <size_t> (chan) < NADCCHAN);
    size_t sz = 0;
    if (fDebugFile != 0) PrintDataType();
    // For some "old" firmware version
    if (fFirmwareVers==1) {
      if (GetFadcMode() == 7 && ((sz = fPulseData[chan].integral.size()) == fPulseData[chan].time.size())) return sz;
      if (GetFadcMode() == 8) return fPulseData[chan].samples.size();
    }
    // The rest for "modern" firmware
    if (GetFadcMode() == 1)
      return 1;
    else if ((GetFadcMode() == 7) &&
	     ((sz = fPulseData[chan].integral.size()) == fPulseData[chan].time.size()) &&
	     (fPulseData[chan].pedestal.size() == sz ) &&
	     (fPulseData[chan].peak.size() == sz)) {
#ifdef WITH_DEBUG
      if (fDebugFile != 0)
	*fDebugFile << "Fadc250Module::GetNumFadcEvents channel "
		    << chan << " = " <<  sz << endl;
#endif
      return sz;
    }
    else if ((GetFadcMode() == 8) &&
	     ((sz = fPulseData[chan].time.size()) == fPulseData[chan].pedestal.size()) &&
	     (fPulseData[chan].peak.size() == sz)) {
#ifdef WITH_DEBUG
      if (fDebugFile != 0)
	*fDebugFile << "Fadc250Module::GetNumFadcEvents channel "
		    << chan << " = " <<  sz << endl;
#endif
      return sz;
    }
    else if ((GetFadcMode() == 9 && fFirmwareVers == 2) &&
	     ((sz = fPulseData[chan].integral.size()) == fPulseData[chan].time.size()) &&
	     (fPulseData[chan].pedestal.size() == sz) &&
	     (fPulseData[chan].peak.size() == sz)) {
#ifdef WITH_DEBUG
      if (fDebugFile != 0)
	*fDebugFile << "Fadc250Module::GetNumFadcEvents channel "
		    << chan << " = " <<  sz << endl;
#endif
      return sz;
    }
    else if ((GetFadcMode() == 10 && fFirmwareVers == 2) &&
	     ((sz = fPulseData[chan].integral.size()) == fPulseData[chan].time.size()) &&
	     (fPulseData[chan].pedestal.size() == sz) &&
	     (fPulseData[chan].peak.size() == sz)) {
#ifdef WITH_DEBUG
      if (fDebugFile != 0)
	*fDebugFile << "Fadc250Module::GetNumFadcEvents channel "
		    << chan << " = " <<  sz << endl;
#endif
      return sz;
    }
    else if ((GetFadcMode() == 9) &&
	     ((sz = fPulseData[chan].integral.size()) == fPulseData[chan].time.size()) &&
	     (fPulseData[chan].pedestal.size() == 1 || fPulseData[chan].pedestal.size() == 0) &&
	     (fPulseData[chan].peak.size() == sz)) {
#ifdef WITH_DEBUG
      if (fDebugFile != 0)
	*fDebugFile << "Fadc250Module::GetNumFadcEvents channel "
		    << chan << " = " <<  sz << endl;
#endif
      return sz;
    }
    else if ((GetFadcMode() == 10) &&
	     ((sz = fPulseData[chan].integral.size()) == fPulseData[chan].time.size()) &&
	     (fPulseData[chan].pedestal.size() == 1 || fPulseData[chan].pedestal.size() == 0) &&
	     (fPulseData[chan].peak.size() == sz)) {
#ifdef WITH_DEBUG
      if (fDebugFile != 0)
	*fDebugFile << "Fadc250Module::GetNumFadcEvents channel "
		    << chan << " = " <<  sz << endl;
#endif
      return sz;
    }
    else {
      cout << "ERROR:: Fadc250Module:: GetNumFadcEvents:: FADC not in acceptable mode or data vector sizes do not match for for slot = " << fSlot << ", channel = " << chan << endl;
      cout << "ERROR:: Fadc250Module:: GetNumFadcEvents:: FADC data potentially corrupt for event " << fadc_data.evt_num << ", PROCEED WITH CAUTION!" << endl;
      return -1;
    }
  }

  Int_t Fadc250Module::GetNumFadcSamples(Int_t chan, Int_t ievent) const {
    if ((GetFadcMode() == 1) || (GetFadcMode() == 8) || (GetFadcMode() == 10)) {
      Int_t nsamples = 0;
      nsamples = fPulseData[chan].samples.size();
      if (ievent < 0) {
	cout << "ERROR:: Fadc250Module:: GetNumFadcSamples:: invalid event number for slot = " << fSlot << ", channel = " << chan << endl;
	return -1;
      }
      if (nsamples == 0) {
	//cout << "ERROR:: Fadc250Module:: GetNumFadcSamples:: data vector empty for slot = " << fSlot << ", channel = " << chan << endl;
	return -1;
      }
      else  {
	return fPulseData[chan].samples.size();
#ifdef WITH_DEBUG
	if (fDebugFile != 0)
	  *fDebugFile << "Fadc250Module::GetNumFadcSamples channel "
		      << chan << ", event " << ievent << " = "
		      <<  fPulseData[chan].samples.size() << endl;
#endif
      }
    }
    else {
      cout << "ERROR:: Fadc250Module:: GetNumFadcSamples:: FADC is not in Mode 1, 8, or 10 for slot = " << fSlot << ", channel = " << chan << endl;
      return -1;
    }
  }

  Int_t Fadc250Module::LoadSlot(THaSlotData *sldat, const UInt_t* evbuffer, const UInt_t *pstop) {
    // the 3-arg version of LoadSlot

    std::vector< UInt_t > evb;
    while (evbuffer < pstop) evb.push_back(*evbuffer++);

    // Note, methods SplitBuffer, GetNextBlock  are defined in PipeliningModule

    SplitBuffer(evb);
    return LoadThisBlock(sldat, GetNextBlock());
  }

  Int_t Fadc250Module::LoadSlot(THaSlotData *sldat, const UInt_t *evbuffer, Int_t pos, Int_t len) {
    // the 4-arg version of LoadSlot.  Let it call the 3-arg version.
    // I'm not sure we need both (historical)

    return LoadSlot(sldat, evbuffer+pos, evbuffer+pos+len);
  }

  void Fadc250Module::LoadTHaSlotDataObj(THaSlotData *sldat) {
    // Load THaSlotData
    for (uint32_t chan = 0; chan < NADCCHAN; chan++) {
      // Pulse Integral
      for (uint32_t ievent = 0; ievent < fPulseData[chan].integral.size(); ievent++)
	sldat->loadData("adc", chan, fPulseData[chan].integral[ievent], fPulseData[chan].integral[ievent]);
      // Pulse Time
      for (uint32_t ievent = 0; ievent < fPulseData[chan].time.size(); ievent++)
	sldat->loadData("adc", chan, fPulseData[chan].time[ievent], fPulseData[chan].time[ievent]);
      // Pulse Peak
      for (uint32_t ievent = 0; ievent < fPulseData[chan].peak.size(); ievent++)
	sldat->loadData("adc", chan, fPulseData[chan].peak[ievent], fPulseData[chan].peak[ievent]);
      // Pulse Pedestal
      for (uint32_t ievent = 0; ievent < fPulseData[chan].pedestal.size(); ievent++)
	sldat->loadData("adc", chan, fPulseData[chan].pedestal[ievent], fPulseData[chan].pedestal[ievent]);
      // Pulse Samples
      for (uint32_t ievent = 0; ievent < fPulseData[chan].samples.size(); ievent++)
	sldat->loadData("adc", chan, fPulseData[chan].samples[ievent], fPulseData[chan].samples[ievent]);
    }  // Channel loop
  }

  Int_t Fadc250Module::LoadNextEvBuffer(THaSlotData *sldat) {
    // Note, GetNextBlock belongs to PipeliningModule
    return LoadThisBlock(sldat, GetNextBlock());
  }

  Int_t Fadc250Module::LoadThisBlock(THaSlotData *sldat, std::vector< UInt_t>evbuffer) {

    // Fill data structures of this class using the event buffer of one "event".
    // An "event" is defined in the traditional way -- a scattering from a target, etc.

    Clear();

    Int_t index = 0;
    for (UInt_t i = 0; i<evbuffer.size(); i++)
      DecodeOneWord(evbuffer[index++]);

    LoadTHaSlotDataObj(sldat);

    return index;
  }

  Int_t Fadc250Module::DecodeOneWord(UInt_t data)
  {
    uint32_t data_type_id = (data >> 31) & 0x1;  // Data type identification, mask 1 bit
    if (data_type_id == 1)
      data_type_def = (data >> 27) & 0xF;        // Data type defining words, mask 4 bits
    // Debug output
#ifdef WITH_DEBUG
    if (fDebugFile != 0)
      *fDebugFile << "Fadc250Module::Decode:: FADC DATA TYPES >> data = " << hex
		  << data << dec << " >> data word id = " << data_type_id
		  << " >> data type = " << data_type_def << endl;

#endif

    // Ensure that slots match and do not decode if they differ
    if (!slots_match && data_type_def != 0) {
#ifdef WITH_DEBUG
      if (fDebugFile != 0)
	*fDebugFile << "Fadc250Module::Decode:: fSlot & FADC slot do not match AND data type != 0" << endl;
#endif
      return -1;
    }

    // Acquire data objects depending on the data type defining word
    switch(data_type_def)
      {
      case 0: // Block header, indicates the begining of a block of events
	if (data_type_id) {
	  block_header_found = true;                     // Set to true if found
	  fadc_data.slot_blk_hdr = (data >> 22) & 0x1F;  // Slot number (set by VME64x backplane), mask 5 bits
	  // Debug output
#ifdef WITH_DEBUG
	  if (fDebugFile != 0)
	    *fDebugFile << "Fadc250Module::Decode:: Slot from FADC block header = " << fadc_data.slot_blk_hdr << endl;
#endif
	  // Ensure that slots from cratemap and FADC match
	  if (uint32_t (fSlot) == fadc_data.slot_blk_hdr) slots_match = true;
	  else { slots_match = false; return -1; }
	  fadc_data.mod_id = (data >> 18) & 0xF;         // Module ID ('1' for FADC250), mask 4 bits
	  fadc_data.iblock_num = (data >> 8) & 0x3FF;    // Event block number, mask 10 bits
	  fadc_data.nblock_events = (data >> 0) & 0xFF;  // Number of events in block, mask 8 bits
	  // Debug output
#ifdef WITH_DEBUG
	  if (fDebugFile != 0)
	    *fDebugFile << "Fadc250Module::Decode:: FADC BLOCK HEADER >> data = " << hex
			<< data << dec << " >> slot = " << fadc_data.slot_blk_hdr
			<< " >> module id = " << fadc_data.mod_id << " >> event block number = " << fadc_data.iblock_num
			<< " >> num events in block = " << fadc_data.nblock_events << endl;
#endif
	}
	else {
	  fadc_data.PL = (data >> 18) & 0x7FF;  // Number of samples before trigger point for processing to begin, mask 11 bits
	  fadc_data.NSB = (data >> 9) & 0x1FF;  // Number of samples before threshold crossing to include processing, mask 9 bits
	  fadc_data.NSA = (data >> 0) & 0x1FF;  // Number of samples after threshold crossing to include processing, mask 9 bits
	  // Debug output
#ifdef WITH_DEBUG
	  if (fDebugFile != 0)
	    *fDebugFile << "Fadc250Module::Decode:: FADC BLOCK HEADER >> data = " << hex
			<< data << dec << " >> NSB trigger point (PL) = " <<  fadc_data.PL
			<< " >> NSB threshold crossing = " <<  fadc_data.NSA
			<< " >> NSA threshold crossing = " << fadc_data.NSB << endl;
#endif
	}
	break;
      case 1: // Block trailer, indicates the end of a block of events
	block_trailer_found = true;
	fadc_data.slot_blk_trl = (data >> 22) & 0x1F;       // Slot number (set by VME64x backplane), mask 5 bits
	fadc_data.nwords_inblock = (data >> 0) & 0x3FFFFF;  // Total number of words in block of events, mask 22 bits
	// Debug output
#ifdef WITH_DEBUG
	if (fDebugFile != 0)
	  *fDebugFile << "Fadc250Module::Decode:: FADC BLOCK TRAILER >> data = " << hex
		      << data << dec << " >> slot = " <<  fadc_data.slot_blk_trl
		      << " >> nwords in block = " << fadc_data.nwords_inblock << endl;
#endif
	break;
      case 2: // Event header, indicates start of an event, includes the trigger number
	event_header_found = true;
	// For firware versions pre 0x0C00 (06/09/2016)
	// fadc_data.slot_evt_hdr = (data >> 22) & 0x1F;  // Slot number (set by VME64x backplane), mask 5 bits
	// fadc_data.evt_num = (data >> 0) & 0x3FFFFF;    // Event number, mask 22 bits
	// For firmware versions post 0x0C00 (06/09/2016)
	fadc_data.eh_trig_time = (data >> 12) & 0x3FF;  // Event header trigger time
	fadc_data.trig_num = (data >> 0) & 0xFFF;       // Trigger number
	// Debug output
	// #ifdef WITH_DEBUG
	//	if (fDebugFile != 0)
	//	  *fDebugFile << "Fadc250Module::Decode:: FADC EVENT HEADER >> data = " << hex
	//		      << data << dec << " >> slot = " << fadc_data.slot_evt_hdr
	//		      << " >> event number = " << fadc_data.evt_num << endl;
	// #endif
#ifdef WITH_DEBUG
	if (fDebugFile != 0)
	  *fDebugFile << "Fadc250Module::Decode:: FADC EVENT HEADER >> data = " << hex
		      << data << dec << " >> event header trigger time = " << fadc_data.eh_trig_time
		      << " >> trigger number = " << fadc_data.trig_num << endl;
#endif
	break;
      case 3:  // Trigger time, time of trigger occurence relative to the most recent global reset
	if (data_type_id)  // Trigger time word 1
	  fadc_data.trig_time_w1 = (data >> 0) & 0xFFFFFF;  // Time = T_D T_E T_F
	else  // Trigger time word 2
	  fadc_data.trig_time_w2 = (data >> 0) & 0xFFFFFF;  // Time = T_A T_B T_C
	// Time = T_A T_B T_C T_D T_E T_F
	fadc_data.trig_time = (fadc_data.trig_time_w2 << 24) | fadc_data.trig_time_w1;
	// Debug output
#ifdef WITH_DEBUG
	if (fDebugFile != 0)
	  *fDebugFile << "Fadc250Module::Decode:: FADC TRIGGER TIME >> data = " << hex
		      << data << dec << " >> trigger time word 1 = " << fadc_data.trig_time_w1
		      << " >> trigger time word 2 = " << fadc_data.trig_time_w2
		      << " >> trigger time = " << fadc_data.trig_time << endl;
#endif
	break;
      case 4: // Window raw data
	data_type_4 = true;
	if (data_type_id == 1) {
	  fadc_data.chan = (data >> 23) & 0xF;        // FADC channel number
	  fadc_data.win_width = (data >> 0) & 0xFFF;  // Window width
	  // Debug output
#ifdef WITH_DEBUG
	  if (fDebugFile != 0)
	    *fDebugFile << "Fadc250Module::Decode:: FADC WINDOW RAW DATA >> data = " << hex
			<< data << dec << " >> channel = " << fadc_data.chan
			<< " >> window width  = " << fadc_data.win_width << endl;
#endif
	}
	else {
	  if (data_type_id != 0) break;                      // Ensure this is a data continuation word
	  Bool_t invalid_1 = (data >> 29) & 0x1;             // Check if sample x is invalid
	  Bool_t invalid_2 = (data >> 13) & 0x1;             // Check if sample x+1 is invalid
	  uint32_t sample_1 = 0;                             // ADC sample x (includes overflow bit)
	  uint32_t sample_2 = 0;                             // ADC sample x+1 (includes overflow bit)
	  if (!invalid_1) sample_1 = (data >> 16) & 0x1FFF;  // If sample x is valid, assign value
	  if (!invalid_2) sample_2 = (data >> 0) & 0x1FFF;   // If sample x+1 is valid, assign value

	  PopulateDataVector(fPulseData[fadc_data.chan].samples, sample_1); // Sample 1
	  fadc_data.invalid_samples |= invalid_1;                        // Invalid samples
	  fadc_data.overflow = (sample_1 >> 12) & 0x1;                   // Sample 1 overflow bit
	  if((sample_1 + 2) == fadc_data.win_width && invalid_2) break;  // Skip last sample if flagged as invalid

	  PopulateDataVector(fPulseData[fadc_data.chan].samples, sample_2); // Sample 2
	  fadc_data.invalid_samples |= invalid_2;                        // Invalid samples
	  fadc_data.overflow = (sample_2 >> 12) & 0x1;                   // Sample 2 overflow bit
	  // Debug output
#ifdef WITH_DEBUG
	  if (fDebugFile != 0)
	    *fDebugFile << "Fadc250Module::Decode:: FADC WINDOW RAW DATA >> data = " << hex
			<< data << dec << " >> channel = " << fadc_data.chan
			<< " >> sample 1 = " << sample_1
			<< " >> sample 2 = " << sample_2
			<< " >> size of fPulseSamples = "
			<< fPulseData[fadc_data.chan].samples.size() << endl;
#endif
	}  // FADC data sample for window raw data
	break;
      case 5:  // Undefined type
	{
#ifdef WITH_DEBUG
	  if (fDebugFile != 0)
	    *fDebugFile << "Fadc250Module::Decode:: UNDEFINED TYPE >> data = " << hex << data
			<< dec << " >> data type id = " << data_type_id << endl;
#endif
	}
        break;
      case 6:  // Pulse raw data
	data_type_6 = true;
	if (data_type_id == 1) {
	  fadc_data.chan = (data >> 23) & 0xF;            // FADC channel number
	  fadc_data.pulse_num = (data >> 21) & 0x3;       // FADC pulse number
	  fadc_data.sample_num_tc = (data >> 0) & 0x3FF;  // FADC sample number of threshold crossing
	  // Debug output
#ifdef WITH_DEBUG
	  if (fDebugFile != 0)
	    *fDebugFile << "Fadc250Module::Decode:: FADC PULSE RAW DATA >> data = " << hex
			<< data << dec << " >> channel = " << fadc_data.chan
			<< " >> window width  = " << fadc_data.win_width << endl;
#endif
	}
	else {
	  if (data_type_id != 0) break;                      // Ensure this is a data continuation word
	  Bool_t invalid_1 = (data >> 29) & 0x1;             // Check if sample x is invalid
	  Bool_t invalid_2 = (data >> 13) & 0x1;             // Check if sample x+1 is invalid
	  uint32_t sample_1 = 0;                             // ADC sample x (includes overflow bit)
	  uint32_t sample_2 = 0;                             // ADC sample x+1 (includes overflow bit)
	  if (!invalid_1) sample_1 = (data >> 16) & 0x1FFF;  // If sample x is valid, assign value
	  if (!invalid_2) sample_2 = (data >> 0) & 0x1FFF;   // If sample x+1 is valid, assign value

	  PopulateDataVector(fPulseData[fadc_data.chan].samples, sample_1);  // Sample 1
	  fadc_data.invalid_samples |= invalid_1;                         // Invalid samples
	  fadc_data.overflow = (sample_1 >> 12) & 0x1;                    // Sample 1 overflow bit
	  if ((sample_1 + 2) == fadc_data.win_width && invalid_2) break;  // Skip last sample if flagged as invalid

	  PopulateDataVector(fPulseData[fadc_data.chan].samples, sample_2);  // Sample 2
	  fadc_data.invalid_samples |= invalid_2;                         // Invalid samples
	  fadc_data.overflow = (sample_2 >> 12) & 0x1;                    // Sample 2 overflow bit
	  // Debug output
#ifdef WITH_DEBUG
	  if (fDebugFile != 0)
	    *fDebugFile << "Fadc250Module::Decode:: FADC PULSE RAW DATA >> data = " << hex
			<< data << dec << " >> sample 1 = " << sample_1
			<< " >> sample 2 = " << sample_2
			<< " >> size of fPulseSamples = "
			<< fPulseData[fadc_data.chan].samples.size() << endl;
#endif
	}  // FADC data sample loop for pulse raw data
	break;
      case 7:  // Pulse integral
	data_type_7 = true;
	fadc_data.chan = (data >> 23) & 0xF;               // FADC channel number
	fadc_data.pulse_num = (data >> 21) & 0x3;          // FADC pulse number
	fadc_data.qual_factor = (data >> 19) & 0x3;        // FADC qulatity factor (0-3)
	fadc_data.pulse_integral = (data >> 0) & 0x7FFFF;  // FADC pulse integral
	// Store data in arrays of vectors
	PopulateDataVector(fPulseData[fadc_data.chan].integral, fadc_data.pulse_integral);
	// Debug output
#ifdef WITH_DEBUG
	if (fDebugFile != 0)
	  *fDebugFile << "Fadc250Module::Decode:: FADC PULSE INTEGRAL >> data = " << hex
		      << data << dec << " >> chan = " << fadc_data.chan
		      << " >> pulse num = " << fadc_data.pulse_num
		      << " >> quality factor = " << fadc_data.qual_factor
		      << " >> pulse integral = " << fadc_data.pulse_integral << endl;
#endif
	break;
      case 8: // Pulse time
	data_type_8 = true;
	fadc_data.chan = (data >> 23) & 0xF;                // FADC channel number
	fadc_data.pulse_num = (data >> 21) & 0x3;           // FADC pulse number
	fadc_data.qual_factor = (data >> 19) & 0x3;         // FADC qulatity factor (0-3)
	fadc_data.coarse_pulse_time = (data >> 6) & 0x1FF;  // FADC coarse time (4 ns/count)
	fadc_data.fine_pulse_time = (data >> 0) & 0x3F;     // FADC fine time (0.0625 ns/count)
	fadc_data.time = (data >> 0) & 0x7FFF;              // FADC time (0.0625 ns/count, bmoffit)
	// Store data in arrays of vectors
	PopulateDataVector(fPulseData[fadc_data.chan].coarse_time, fadc_data.coarse_pulse_time);
	PopulateDataVector(fPulseData[fadc_data.chan].fine_time, fadc_data.fine_pulse_time);
	PopulateDataVector(fPulseData[fadc_data.chan].time, fadc_data.time);
	// Debug output
#ifdef WITH_DEBUG
	if (fDebugFile != 0)
	  *fDebugFile << "Fadc250Module::Decode:: FADC PULSE TIME >> data = " << hex
		      << data << dec << " >> chan = " << fadc_data.chan
		      << " >> pulse num = " << fadc_data.pulse_num
		      << " >> quality factor = " << fadc_data.qual_factor
		      << " >> coarse time = " << fadc_data.coarse_pulse_time
		      << " >> fine time = " << fadc_data.fine_pulse_time
		      << " >> time = " << fadc_data.time << endl;
#endif
	break;
      case 9:  // Pulse Parameters
	data_type_9 = true;
	if (data_type_id == 1) {
	  fadc_data.evnt_of_blk = (data >> 19) & 0xFF;      // Event number within block
	  fadc_data.chan = (data >> 15) & 0xF;              // FADC channel number
	  fadc_data.qual_factor = (data >> 14) & 0x1;       // Pedestal quality
	  fadc_data.pedestal_sum = (data >> 0) & 0x3FFF;    // Pedestal sum
	  // Populate data vectors
	  PopulateDataVector(fPulseData[fadc_data.chan].pedestal, fadc_data.pedestal_sum);
	  PopulateDataVector(fPulseData[fadc_data.chan].pedestal_quality, fadc_data.qual_factor);
	  // Debug output
#ifdef WITH_DEBUG
	  if (fDebugFile != 0)
	    *fDebugFile << "Fadc250Module::Decode:: FADC PULSE PARAMETERS >> data = " << hex
			<< data << dec << " >> chan = " << fadc_data.chan
			<< " >> event of block = " << fadc_data.evnt_of_blk
			<< " >> quality factor = " << fadc_data.qual_factor
			<< " >> pedestal sum = " << fadc_data.pedestal_sum << endl;
#endif
	}
	else {
	  if (data_type_id != 0) break;                        // Ensure this is a data continuation word
	  if (((data >> 30) & 0x1) == 1) {                     // Ensure that data is the integral of pulse window word
	    fadc_data.sample_sum = (data >> 12) & 0x3FFFF;     // 18-bit sum of raw samples that constitute the pulse data set
	    fadc_data.nsa_ext = (data >> 11) & 0x1;            // NSA extended beyond PTW
	    fadc_data.samp_overflow = (data >> 10) & 0x1;      // One or more samples is overflow
	    fadc_data.samp_underflow = (data >> 9) & 0x1;      // One or more samples is underflow
	    fadc_data.samp_over_thresh = (data >> 0) & 0x1FF;  // Number of samples within NSA that the pulse is above threshold
	    // Populate data vectors
	    PopulateDataVector(fPulseData[fadc_data.chan].integral, fadc_data.sample_sum);
	    PopulateDataVector(fPulseData[fadc_data.chan].overflow, fadc_data.samp_overflow);
	    PopulateDataVector(fPulseData[fadc_data.chan].underflow, fadc_data.samp_underflow);
	    // Debug output
#ifdef WITH_DEBUG
	    if (fDebugFile != 0)
	      *fDebugFile << "Fadc250Module::Decode:: FADC PULSE PARAMETERS >> data = " << hex
			  << data << dec << " >> chan = " << fadc_data.chan
			  << " >> integral = " << fadc_data.sample_sum
			  << " >> nsa extended = " << fadc_data.nsa_ext
			  << " >> sample overflow = " << fadc_data.samp_overflow
			  << " >> sample underflow = " << fadc_data.samp_underflow
			  << " >> sample over threshold = " << fadc_data.samp_over_thresh << endl;
#endif
	  }
	  if (((data >> 30) & 0x1) == 0) {                       // Ensure that data is the time of pulse window word
	    fadc_data.coarse_pulse_time = (data >> 21) & 0x1FF;  // Coarse time (4 ns/count)
	    fadc_data.fine_pulse_time = (data >> 15) & 0x3F;     // Fine time (0.0625 ns/count)
	    fadc_data.time = (data >> 15) & 0x7FFF;              // FADC time (0.0625 ns/count)
	    fadc_data.pulse_peak = (data >> 3) & 0xFFF;          // Pulse peak
	    fadc_data.peak_beyond_nsa = (data >> 2) & 0x1;       // Pulse peak is beyond NSA or could be beyond window end
	    fadc_data.peak_not_found = (data >> 1) & 0x1;        // Pulse peak cannot be found
	    fadc_data.peak_above_maxped = (data >> 0) & 0x1;     // 1 or more of first four samples is above either MaxPed or TET
	    // Populate data vectors
	    PopulateDataVector(fPulseData[fadc_data.chan].coarse_time, fadc_data.coarse_pulse_time);
	    PopulateDataVector(fPulseData[fadc_data.chan].fine_time, fadc_data.fine_pulse_time);
	    PopulateDataVector(fPulseData[fadc_data.chan].time, fadc_data.time);
	    PopulateDataVector(fPulseData[fadc_data.chan].peak, fadc_data.pulse_peak);
	    // Debug output
#ifdef WITH_DEBUG
	    if (fDebugFile != 0)
	      *fDebugFile << "Fadc250Module::Decode:: FADC PULSE PARAMETERS >> data = " << hex
			  << data << dec << " >> chan = " << fadc_data.chan
			  << " >> coarse time = " << fadc_data.coarse_pulse_time
			  << " >> fine time = " << fadc_data.fine_pulse_time
			  << " >> time = " << fadc_data.time
			  << " >> pulse peak = " << fadc_data.pulse_peak
			  << " >> peak beyond nsa = " << fadc_data.peak_beyond_nsa
			  << " >> peak not found = " << fadc_data.peak_not_found
			  << " >> peak above maxped = " << fadc_data.peak_above_maxped << endl;
#endif
	  }
	}
	break;
      case 10:  // Pulse Pedestal
	data_type_10 = true;
	fadc_data.chan = (data >> 23) & 0xF;          // FADC channel number
	fadc_data.pulse_num = (data >> 21) & 0x3;     // FADC pulse number
	fadc_data.pedestal = (data >> 12) & 0x1FF;    // FADC pulse pedestal
	fadc_data.pulse_peak = (data >> 0) & 0xFFF;   // FADC pulse peak
	// Store data in arrays of vectors
	PopulateDataVector(fPulseData[fadc_data.chan].pedestal, fadc_data.pedestal);
	PopulateDataVector(fPulseData[fadc_data.chan].peak, fadc_data.pulse_peak);
	// Debug output
#ifdef WITH_DEBUG
	if (fDebugFile != 0)
	  *fDebugFile << "Fadc250Module::Decode:: FADC PULSE PEDESTAL >> data = " << hex
		      << data << dec << " >> chan = " << fadc_data.chan
		      << " >> pulse num = " << fadc_data.pulse_num
		      << " >> pedestal = " << fadc_data.pedestal
		      << " >> pulse peak = " << fadc_data.pulse_peak << endl;
#endif
	break;
      case 11:  // Undefined type
	{
	  // Debug output
#ifdef WITH_DEBUG
	  if (fDebugFile != 0)
	    *fDebugFile << "Fadc250Module::Decode:: UNDEFINED TYPE >> data = " << hex << data
			<< dec << " >> data type id = " << data_type_id << endl;
#endif
	}
        break;
      case 12:  // Scaler header
	{
	  fadc_data.scaler_words = (data >> 0) & 0x3F;  // FADC scaler words
	  // Debug output
#ifdef WITH_DEBUG
	  if (fDebugFile != 0)
	    *fDebugFile << "Fadc250Module::Decode:: FADC SCALER HEADER >> data = " << hex
			<< data << dec << " >> data words = "
			<< hex << fadc_data.scaler_words << dec << endl;
#endif
	}
        break;
      case 13:  // Undefined type
	{
	  // Debug output
#ifdef WITH_DEBUG
	  if (fDebugFile != 0)
	    *fDebugFile << "Fadc250Module::Decode:: UNDEFINED TYPE >> data = " << hex << data
			<< dec << " >> data type id = " << data_type_id << endl;
#endif
	}
        break;
      case 14:  // Data not valid
	{
	  // Debug output
#ifdef WITH_DEBUG
	  if (fDebugFile != 0)
	    *fDebugFile << "Fadc250Module::Decode:: DATA NOT VALID >> data = " << hex << data
			<< dec << " >> data type id = " << data_type_id << endl;
#endif
	}
        break;
      case 15:  // Filler Word, should be ignored
	{
	  // Debug output
#ifdef WITH_DEBUG
	  if (fDebugFile != 0)
	    *fDebugFile << "Fadc250Module::Decode:: FILLER WORD >> data = " << hex << data
			<< dec << " >> data type id = " << data_type_id << endl;
#endif
	}
        break;
      }  // data_type_def switch

    return block_trailer_found;

#ifdef WITH_DEBUG
    if (fDebugFile != 0)
      *fDebugFile << "**********************************************************************"
		  << "\n" << endl;
#endif

  }  // Fadc250Module::Decode method

}


ClassImp(Decoder::Fadc250Module)
