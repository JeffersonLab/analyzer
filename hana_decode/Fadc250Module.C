////////////////////////////////////////////////////////////////////
//
//   Fadc250Module
//   JLab FADC 250 Module
//
//   Author: Eric Pooser, pooser@jlab.org
//
/////////////////////////////////////////////////////////////////////

#include "Fadc250Module.h"
#include "VmeModule.h"
#include "THaEvData.h"
#include "THaSlotData.h"
#include "TMath.h"

#include <unistd.h>
#include <stdint.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>  // for memset
#include <iostream>
#include <string>
#include <sstream>
#include <cmath>
#include <iomanip>
#include <numeric>

using namespace std;

#define DEBUG

namespace Decoder {

  Module::TypeIter_t Fadc250Module::fgThisType =
    DoRegister( ModuleType( "Decoder::Fadc250Module" , 250 ));

  Fadc250Module::Fadc250Module()
    : VmeModule()
  { memset(&fadc_data, 0, sizeof(fadc_data)); }

  Fadc250Module::Fadc250Module(Int_t crate, Int_t slot)
    : VmeModule(crate, slot)
  {
    memset(&fadc_data, 0, sizeof(fadc_data));
    IsInit = kFALSE;
    Init();
  }

  Fadc250Module::~Fadc250Module() {
#if defined DEBUG && defined WITH_DEBUG
    delete fDebugFile; fDebugFile = 0;
#endif
  }

  Bool_t Fadc250Module::IsMultiFunction() { 
     return kTRUE; 
  }

  Bool_t Fadc250Module::HasCapability(Decoder::EModuleType type) { 
    if (type == kSampleADC || type == kPulseIntegral || type == kPulseTime 
        || type == kPulsePeak || type == kPulsePedestal) {
      return kTRUE;
    } else {
      return kFALSE; 
    }
  }

  // Clear all data vectors
  void Fadc250Module::ClearDataVectors() {
    // Clear all data objects
    for (uint32_t i = 0; i < NADCCHAN; i++) {
      fPulseIntegral[i].clear(); fPulseTime[i].clear();
      fPulsePeak[i].clear(); fPulsePedestal[i].clear();
      fPulseSamples[i].clear();
    }
  }

  // Require that slot from base class and slot from 
  //   data match before populating data vectors
  void Fadc250Module::PopulateDataVector(vector<uint32_t> data_vector[NADCCHAN], uint32_t chan, uint32_t data) {
    if (static_cast <uint32_t> (fSlot) == fadc_data.slot_blk_hdr)
      data_vector[chan].push_back(data);
  }

  // Sum elements contained in data vector
  Int_t Fadc250Module::SumVectorElements(vector<uint32_t> data_vector) const {
    Int_t sum_of_elements = 0;
    sum_of_elements = accumulate(data_vector.begin(), data_vector.end(), 0);
    return sum_of_elements;
  }

  void Fadc250Module::Clear( const Option_t* opt) {
    // Clear event-by-event data
    VmeModule::Clear(opt);

    ClearDataVectors();

    // Initialize data types to false
    data_type_4 = data_type_6 = data_type_7 = data_type_8 = data_type_10 = false;
    block_header_found = block_trailer_found = event_header_found = slots_match = false;
  }

  void Fadc250Module::Init() {
#if defined DEBUG && defined WITH_DEBUG
    // This will make a HUGE output
    delete fDebugFile; fDebugFile = 0;
    fDebugFile = new ofstream;
    fDebugFile->open("fadcdebug.dat");
#endif

    Clear();
    
    // Initialize data types to false
    data_type_4 = false; data_type_6 = false; 
    data_type_7 = false; data_type_8 = false; 
    data_type_10 = false;
    block_header_found  = false; 
    block_trailer_found = false;  
    event_header_found  = false;  
    slots_match = false;

    IsInit = kTRUE;
    fName = "FADC250Devel Module (example)"; 
  }

  void Fadc250Module::CheckDecoderStatus() const {
    cout << "FADC250 Decoder has been called" << endl;
  }

  void Fadc250Module::CheckDecoderStatus(Int_t crate, Int_t slot) const {
    cout << "FADC250 Decoder has been called for crate " << crate << ", slot " << slot << endl;
  }

  Int_t Fadc250Module::GetPulseIntegralData(Int_t chan, Int_t ievent) const {
    Int_t nevent = 0;
    nevent = fPulseIntegral[chan].size();
    if (ievent < 0 || ievent > nevent) {
      cout << "ERROR:: Fadc250Module:: GetPulseIntegralData:: invalid event number for slot = " << fSlot << ", channel = " << chan << endl;
      return -1;
    }
    if (nevent == 0) {
      cout << "ERROR:: Fadc250Module:: GetPulseIntegralData:: data vector empty for slot = " << fSlot << ", channel = " << chan << endl;
      return -1;
    }
    else {
      return fPulseIntegral[chan][ievent];
#ifdef WITH_DEBUG
      if (fDebugFile != 0)
	*fDebugFile << "Fadc250Module::GetPulseIntegralData channel " 
		    << chan << ", event " << ievent << " = " 
		    <<  fPulseIntegral[chan][ievent] << endl;
#endif
    }
  }
  
  Int_t Fadc250Module::GetEmulatedPulseIntegralData(Int_t chan) const {
    Int_t nevent = 0;
    nevent = fPulseSamples[chan].size();
    if (nevent == 0) {
      cout << "ERROR:: Fadc250Module:: GetEmulatedPulseIntegralData:: data vector empty  for slot = " << fSlot << ", channel = " << chan << "\n" 
	   << "Ensure that FADC is operating in mode 1 OR 8" << endl;
      return -1;
    }
    else {
    return SumVectorElements(fPulseSamples[chan]);
#ifdef WITH_DEBUG
      if (fDebugFile != 0)
	*fDebugFile << "Fadc250Module::GetEmulatedPulseIntegralData channel " 
		    << chan << " = " <<  SumVectorElements(fPulseSamples[chan]) << endl;
#endif
    }
  }

  Int_t Fadc250Module::GetPulseTimeData(Int_t chan, Int_t ievent) const {
    Int_t nevent = 0;
    nevent = fPulseTime[chan].size();
    if (ievent < 0 || ievent > nevent) {
      cout << "ERROR:: Fadc250Module:: GetPulseTimeData:: invalid event number for slot = " << fSlot << ", channel = " << chan << endl;
      return -1;
    }
    if (nevent == 0) {
      cout << "ERROR:: Fadc250Module:: GetPulseTimeData:: data vector empty for slot = " << fSlot << ", channel = " << chan << endl;
      return -1;
    }
    else {
      return fPulseTime[chan][ievent];
#ifdef WITH_DEBUG
      if (fDebugFile != 0)
	*fDebugFile << "Fadc250Module::GetPulseTimeData channel " 
		    << chan << ", event " << ievent << " = " 
		    <<  fPulseTime[chan][ievent] << endl;
#endif
    }
  }

  Int_t Fadc250Module::GetPulsePeakData(Int_t chan, Int_t ievent) const {
    Int_t nevent = 0;
    nevent = fPulsePeak[chan].size();
    if (ievent < 0 || ievent > nevent) {
      cout << "ERROR:: Fadc250Module:: GetPulsePeakData:: invalid event number for slot = " << fSlot << ", channel = " << chan << endl;
      return -1;
    }
    if (nevent == 0) {
      cout << "ERROR:: Fadc250Module:: GetPulsePeakData:: data vector empty for slot = " << fSlot << ", channel = " << chan << endl;
      return -1;
    }
    else {
      return fPulsePeak[chan][ievent];
#ifdef WITH_DEBUG
      if (fDebugFile != 0)
	*fDebugFile << "Fadc250Module::GetPulsePeakData channel " 
		    << chan << ", event " << ievent << " = " 
		    <<  fPulsePeak[chan][ievent] << endl;
#endif
    }
  }
      
  Int_t Fadc250Module::GetPulsePedestalData(Int_t chan, Int_t ievent) const {
    Int_t nevent = 0;
    nevent = fPulsePedestal[chan].size();
    if (ievent < 0 || ievent > nevent) {
      cout << "ERROR:: Fadc250Module:: GetPulsePedestalData:: invalid event number for slot = " << fSlot << ", channel = " << chan << endl;
      return -1;
    }
    if (nevent == 0) {
      cout << "ERROR:: Fadc250Module:: GetPulsePedestalData:: data vector empty for slot = " << fSlot << ", channel = " << chan << endl;
      return -1;
    }
    else {
      return fPulsePedestal[chan][ievent];
#ifdef WITH_DEBUG
      if (fDebugFile != 0)
	*fDebugFile << "Fadc250Module::GetPulsePedestalData channel " 
		    << chan << ", event " << ievent << " = " 
		    <<  fPulsePedestal[chan][ievent] << endl;
#endif
    }
  }

  Int_t Fadc250Module::GetPulseSamplesData(Int_t chan, Int_t ievent) const {
    Int_t nevent = 0;
    nevent = fPulseSamples[chan].size();
    if (ievent < 0 || ievent > nevent) {
      cout << "ERROR:: Fadc250Module:: GetPulseSamplesData:: invalid event number for slot = " << fSlot << ", channel = " << chan << endl;
      return -1;
    }
    if (nevent == 0) {
      cout << "ERROR:: Fadc250Module:: GetPulseSamplesData:: data vector empty for slot = " << fSlot << ", channel = " << chan << endl;
      return -1;
    }
    else {
      return fPulseSamples[chan][ievent];
#ifdef WITH_DEBUG
      if (fDebugFile != 0)
	*fDebugFile << "Fadc250Module::GetPulseSamplesData channel " 
		    << chan << ", event " << ievent << " = " 
		    <<  fPulseSamples[chan][ievent] << endl;
#endif
    }
  }

vector<uint32_t> Fadc250Module::GetPulseSamplesVector(Int_t chan) const {
    Int_t nevent = 0;
    nevent = fPulseSamples[chan].size();
    if (nevent == 0) {
      cout << "ERROR:: Fadc250Module:: GetPulseSamplesVector:: data vector empty for slot = " << fSlot << ", channel = " << chan << endl;
      return vector<uint32_t>();
    }
    else {
      return fPulseSamples[chan];
#ifdef WITH_DEBUG
      if (fDebugFile != 0)
	*fDebugFile << "Fadc250Module::GetPulseSamplesVector channel " 
		    << chan << " = " <<  &fPulseSamples[chan] << endl;
#endif
  }
}

Int_t Fadc250Module::GetNumFadcEvents(Int_t chan) const {
    if (data_type_4 && !data_type_8 && !data_type_10)  // Mode 1 data
      return 1;
    else if ((data_type_7 && data_type_8 && data_type_10) &&  // Mode 7 data
	     (fPulseIntegral[chan].size() == fPulseTime[chan].size()) &&
	     (fPulseTime[chan].size() == fPulsePedestal[chan].size()) &&
	     (fPulsePedestal[chan].size() == fPulsePeak[chan].size())) {
      return fPulseIntegral[chan].size();
#ifdef WITH_DEBUG
      if (fDebugFile != 0)
	*fDebugFile << "Fadc250Module::GetNumFadcEvents channel " 
		    << chan << " = " <<  fPulseIntegral[chan].size() << endl;
#endif
    }
    else if ((data_type_4 && data_type_8 && data_type_10) &&  // Mode 8 data
	     (fPulseTime[chan].size() == fPulsePedestal[chan].size()) &&
	     (fPulsePedestal[chan].size() == fPulsePeak[chan].size())) {
      return fPulseTime[chan].size();
#ifdef WITH_DEBUG
      if (fDebugFile != 0)
	*fDebugFile << "Fadc250Module::GetNumFadcEvents channel " 
		    << chan << " = " <<  fPulseTime[chan].size() << endl;
#endif
    }
    else {
      cout << "ERROR:: Fadc250Module:: GetNumFadcEvents:: FADC not in acceptable mode or data vector sizes do not match for for slot = " << fSlot << ", channel = " << chan << endl;
      cout << "ERROR:: Fadc250Module:: GetNumFadcEvents:: FADC data potentially corrupt for event " << fadc_data.evt_num << ", PROCEED WITH CAUTION!" << endl;
      return -1;
    }
  }

  Int_t Fadc250Module::GetNumFadcSamples(Int_t chan, Int_t ievent) const {
    if (data_type_4 && !data_type_8 && !data_type_10) { // Ensure fadc is in mode 1
      Int_t nsamples = 0;
      nsamples = fPulseSamples[chan].size();
      if (ievent < 0) {
	cout << "ERROR:: Fadc250Module:: GetNumFadcSamples:: invalid event number for slot = " << fSlot << ", channel = " << chan << endl;
	return -1;
      }
      if (nsamples == 0) {
	cout << "ERROR:: Fadc250Module:: GetNumFadcSamples:: data vector empty for slot = " << fSlot << ", channel = " << chan << endl;
	return -1;
      }
      else  {
	return fPulseSamples[chan].size();
#ifdef WITH_DEBUG
	if (fDebugFile != 0)
	  *fDebugFile << "Fadc250Module::GetNumFadcSamples channel " 
		      << chan << ", event " << ievent << " = " 
		      <<  fPulseSamples[chan].size() << endl;
#endif
      }
    }  // Mode 1 condition
    else if (data_type_4 && data_type_8 && data_type_10) {  // Ensure fadc is in mode 8
      Int_t nsamples = 0;
      nsamples = fPulseSamples[chan].size();
      if (ievent < 0) {
	cout << "ERROR:: Fadc250Module:: GetNumFadcSamples:: invalid event number for slot = " << fSlot << ", channel = " << chan << endl;
	return -1;
      }
      if (nsamples == 0) {
	cout << "ERROR:: Fadc250Module:: GetNumFadcSamples:: data vector empty for slot = " << fSlot << ", channel = " << chan << endl;
	return -1;
      }
      else {
	return fPulseSamples[chan].size();
#ifdef WITH_DEBUG
	if (fDebugFile != 0)
	  *fDebugFile << "Fadc250Module::GetNumFadcSamples channel " 
		      << chan << ", event " << ievent << " = " 
		      <<  fPulseSamples[chan].size() << endl;
#endif
      }
    }  // Mode 8 condition
    else {
      cout << "ERROR:: Fadc250Module:: GetNumFadcSamples:: FADC is not in Mode 1 or 8 for slot = " << fSlot << ", channel = " << chan << endl;
      return -1;
    }
  }

  Int_t Fadc250Module::GetFadcMode(Int_t chan) const {
    if      (data_type_4    && !(data_type_6) && !(data_type_7) && !(data_type_8) && !(data_type_10)) return 1;
    else if (!(data_type_4) && data_type_6    && !(data_type_7) && !(data_type_8) && !(data_type_10)) return 2;
    else if (!(data_type_4) && !(data_type_6) && data_type_7    && data_type_8    && !(data_type_10)) return 3;
    else if (!(data_type_4) && !(data_type_6) && !(data_type_7) && data_type_8    && data_type_10)    return 4;
    else if (!(data_type_4) && !(data_type_6) && data_type_7    && data_type_8    && data_type_10)    return 7;
    else if (data_type_4    && !(data_type_6) && !(data_type_7) && data_type_8    && data_type_10)    return 8;
    else {
      cout << "ERROR:: Fadc250Module:: GetFadcMode:: FADC is in invalid mode for slot = " << fSlot << ", channel = " << chan << endl;
      return -1;
    }
  }

  Int_t Fadc250Module::LoadSlot(THaSlotData *sldat, const UInt_t *evbuffer, const UInt_t *pstop) {
        
    // Fill data structures of this class
    // Read until out of data or until Decode says that the slot is finished
    Clear();
    Int_t blk_trailer = 0;
    
    while (evbuffer < pstop && blk_trailer == 0) {
      blk_trailer = Decode(evbuffer);
      fWordsSeen++;
      evbuffer++;
    }

    // Load THaSlotData
    for (uint32_t chan = 0; chan < NADCCHAN; chan++) {
      // Pulse Integral
      for (uint32_t ievent = 0; ievent < fPulseIntegral[chan].size(); ievent++)
	sldat->loadData("adc", chan, fPulseIntegral[chan][ievent], fPulseIntegral[chan][ievent]);
      // Pulse Time
      for (uint32_t ievent = 0; ievent < fPulseTime[chan].size(); ievent++)
	sldat->loadData("adc", chan, fPulseTime[chan][ievent], fPulseTime[chan][ievent]);
      // Pulse Peak
      for (uint32_t ievent = 0; ievent < fPulsePeak[chan].size(); ievent++)
	sldat->loadData("adc", chan, fPulsePeak[chan][ievent], fPulsePeak[chan][ievent]);
      // Pulse Pedestal
      for (uint32_t ievent = 0; ievent < fPulsePedestal[chan].size(); ievent++)
	sldat->loadData("adc", chan, fPulsePedestal[chan][ievent], fPulsePedestal[chan][ievent]);
      // Pulse Samples
      for (uint32_t ievent = 0; ievent < fPulseSamples[chan].size(); ievent++)
	sldat->loadData("adc", chan, fPulseSamples[chan][ievent], fPulseSamples[chan][ievent]);  
    }  // Channel loop
	      
    return fWordsSeen;
  }

  Int_t Fadc250Module::LoadSlot(THaSlotData *sldat, const UInt_t *evbuffer, Int_t pos, Int_t len) {
    
    // Fill data structures of this class
    // Read until out of data or until decode says that the slot is finished
    // len = ndata in event, pos = word number for block header in event
    fWordsSeen = 0;
    Clear();
    Int_t index = 0;

    while (fWordsSeen < len) {   
      index = pos + fWordsSeen;
      Decode(&evbuffer[index]);
      fWordsSeen++;
#ifdef WITH_DEBUG
      if (fDebugFile != 0)
	*fDebugFile << "\n" << "Fadc250Module::LoadSlot >> evbuffer[" << index 
		    << "] = " << hex << evbuffer[index] << dec << " >> crate = "
		    << fCrate << " >> slot = " << fSlot << " >> pos = " 
		    << pos << " >> fWordsSeen = " << fWordsSeen 
		    << " >> len = " << len << " >> index = " << index << "\n" << endl;
#endif
    }

    // Load THaSlotData
    for (uint32_t chan = 0; chan < NADCCHAN; chan++) {
      // Pulse Integral
      for (uint32_t ievent = 0; ievent < fPulseIntegral[chan].size(); ievent++)
	sldat->loadData("adc", chan, fPulseIntegral[chan][ievent], fPulseIntegral[chan][ievent]);
      // Pulse Time
      for (uint32_t ievent = 0; ievent < fPulseTime[chan].size(); ievent++)
	sldat->loadData("adc", chan, fPulseTime[chan][ievent], fPulseTime[chan][ievent]);
      // Pulse Peak
      for (uint32_t ievent = 0; ievent < fPulsePeak[chan].size(); ievent++)
	sldat->loadData("adc", chan, fPulsePeak[chan][ievent], fPulsePeak[chan][ievent]);
      // Pulse Pedestal
      for (uint32_t ievent = 0; ievent < fPulsePedestal[chan].size(); ievent++)
	sldat->loadData("adc", chan, fPulsePedestal[chan][ievent], fPulsePedestal[chan][ievent]);
      // Pulse Samples
      for (uint32_t ievent = 0; ievent < fPulseSamples[chan].size(); ievent++)
	sldat->loadData("adc", chan, fPulseSamples[chan][ievent], fPulseSamples[chan][ievent]);
    }  // Channel loop
        
    return fWordsSeen;  
  }
  
  Int_t Fadc250Module::Decode(const UInt_t *pdat)
  //Int_t Fadc250Module::Decode(const UInt_t *pdat, Bool_t *do_slots_match)
  {
    UInt_t data = *pdat;  // Pointer to event buffer  
    uint32_t data_type_id = (data >> 31) & 0x1;  // Data type identification, mask 1 bit
    static uint32_t data_type_def;               // Data type defining words, mask 4 bits
    if (data_type_id == 1)
      data_type_def = (data >> 27) & 0xF;
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
	fadc_data.slot_evt_hdr = (data >> 22) & 0x1F;  // Slot number (set by VME64x backplane), mask 5 bits
	fadc_data.evt_num = (data >> 0) & 0x3FFFFF;    // Event number, mask 22 bits
	// Debug output
#ifdef WITH_DEBUG
	if (fDebugFile != 0)
	  *fDebugFile << "Fadc250Module::Decode:: FADC EVENT HEADER >> data = " << hex 
		      << data << dec << " >> slot = " << fadc_data.slot_evt_hdr 
		      << " >> event number = " << fadc_data.evt_num << endl;
#endif
	break;	  
      case 3:  // Trigger time, time of trigger occurence relative to the most recent global reset
	if (data_type_id)  // Trigger time word 1
	  fadc_data.trig_time = (data >> 0) & 0xFFFFFF;  // Time = T_D T_E T_F
	else  // Trigger time word 2
	  fadc_data.trig_time = (data >> 0) & 0xFFFFFF;  // Time = T_A T_B T_C
	// Debug output
#ifdef WITH_DEBUG
	if (fDebugFile != 0)
	  *fDebugFile << "Fadc250Module::Decode:: FADC TRIGGER TIME >> data = " << hex 
		      << data << dec << " word number = " << hex << data_type_id 
		      << " time = " << hex << fadc_data.trig_time << dec << endl;
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
		  
	  PopulateDataVector(fPulseSamples, fadc_data.chan, sample_1);   // Sample 1
	  fadc_data.invalid_samples |= invalid_1;                        // Invalid samples
	  fadc_data.overflow = (sample_1 >> 12) & 0x1;                   // Sample 1 overflow bit
	  if((sample_1 + 2) == fadc_data.win_width && invalid_2) break;  // Skip last sample if flagged as invalid
	  
	  PopulateDataVector(fPulseSamples, fadc_data.chan, sample_2);   // Sample 2 
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
			<< fPulseSamples[fadc_data.chan].size() << endl;
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

	  PopulateDataVector(fPulseSamples, fadc_data.chan, sample_1);    // Sample 1
	  fadc_data.invalid_samples |= invalid_1;                         // Invalid samples
	  fadc_data.overflow = (sample_1 >> 12) & 0x1;                    // Sample 1 overflow bit
	  if ((sample_1 + 2) == fadc_data.win_width && invalid_2) break;  // Skip last sample if flagged as invalid
	  
	  PopulateDataVector(fPulseSamples, fadc_data.chan, sample_2);    // Sample 2
	  fadc_data.invalid_samples |= invalid_2;                         // Invalid samples
	  fadc_data.overflow = (sample_2 >> 12) & 0x1;                    // Sample 2 overflow bit
	  // Debug output
#ifdef WITH_DEBUG
	  if (fDebugFile != 0)
	    *fDebugFile << "Fadc250Module::Decode:: FADC PULSE RAW DATA >> data = " << hex 
			<< data << dec << " >> sample 1 = " << sample_1 
			<< " >> sample 2 = " << sample_2
			<< " >> size of fPulseSamples = " 
			<< fPulseSamples[fadc_data.chan].size() << endl;
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
	PopulateDataVector(fPulseIntegral, fadc_data.chan, fadc_data.pulse_integral);
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
	PopulateDataVector(fPulseTime, fadc_data.chan, fadc_data.time);
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
      case 9:  // Undefined type
	{
#ifdef WITH_DEBUG
	  if (fDebugFile != 0)
	    *fDebugFile << "Fadc250Module::Decode:: UNDEFINED TYPE >> data = " << hex << data 
			<< dec << " >> data type id = " << data_type_id << endl;
#endif
	}
      case 10:  // Pulse Pedestal
	data_type_10 = true;
	fadc_data.chan = (data >> 23) & 0xF;          // FADC channel number
	fadc_data.pulse_num = (data >> 21) & 0x3;     // FADC pulse number
	fadc_data.pedestal = (data >> 12) & 0x1FF;    // FADC pulse pedestal
	fadc_data.pulse_peak = (data >> 0) & 0xFFF;   // FADC pulse peak
	// Store data in arrays of vectors 
	PopulateDataVector(fPulsePedestal, fadc_data.chan, fadc_data.pedestal);
	PopulateDataVector(fPulsePeak, fadc_data.chan, fadc_data.pulse_peak);
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
      case 13:  // Undefined type
	{
	  // Debug output
#ifdef WITH_DEBUG
	  if (fDebugFile != 0)
	    *fDebugFile << "Fadc250Module::Decode:: UNDEFINED TYPE >> data = " << hex << data 
			<< dec << " >> data type id = " << data_type_id << endl;
#endif
	}
      case 14:  // Data not valid
	{
	  // Debug output
#ifdef WITH_DEBUG
	  if (fDebugFile != 0)
	    *fDebugFile << "Fadc250Module::Decode:: DATA NOT VALID >> data = " << hex << data 
			<< dec << " >> data type id = " << data_type_id << endl;
#endif
	}
      case 15:  // Filler Word, should be ignored
	{
	  // Debug output
#ifdef WITH_DEBUG
	  if (fDebugFile != 0)
	    *fDebugFile << "Fadc250Module::Decode:: FILLER WORD >> data = " << hex << data 
			<< dec << " >> data type id = " << data_type_id << endl;
#endif
	}

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
