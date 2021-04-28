/** \class Caen1190 Module
    \author Stephen Wood
    \author Simona Malace
    \author Brad Sawatzky
    \author Eric Pooser

    Decoder module to retrieve Caen 1190 TDCs.  Based on CAEN 1190 decoding in
    THaCodaDecoder.C in podd 1.5.   (Written by S. Malace, modified by B. Sawatzky)
*/

#include "Caen1190Module.h"
#include "THaSlotData.h"
#include <iostream>

using namespace std;

//#define DEBUG
//#define WITH_DEBUG

namespace Decoder {

  const Int_t NTDCCHAN = 128;
  const Int_t MAXHIT = 100;

  Module::TypeIter_t Caen1190Module::fgThisType =
    DoRegister( ModuleType( "Decoder::Caen1190Module" , 1190 ));

  Caen1190Module::Caen1190Module(Int_t crate, Int_t slot)
    : VmeModule(crate, slot), fNumHits(NTDCCHAN), fTdcData(NTDCCHAN*MAXHIT),
      fTdcOpt(NTDCCHAN*MAXHIT), slot_data(nullptr)
  {
    Caen1190Module::Init();
  }

  void Caen1190Module::Init() {
    Module::Init();
    fNumHits.resize(NTDCCHAN);
    fTdcData.resize(NTDCCHAN*MAXHIT);
    fTdcOpt.resize(NTDCCHAN*MAXHIT);
    Clear();
    IsInit = true;
    fName = "Caen TDC 1190 Module";
  }

  Int_t Caen1190Module::LoadSlot( THaSlotData* sldat, const UInt_t* evbuffer,
                                  const UInt_t *pstop) {
    // This is a simple, default method for loading a slot
    const UInt_t *p = evbuffer;
    slot_data = sldat;
    fWordsSeen = 0; 		// Word count including global header  
    Int_t glbl_trl = 0;
    while(p <= pstop && glbl_trl == 0) {
      glbl_trl = Decode(p);
      fWordsSeen++;
      ++p;
    }
    return fWordsSeen;
  }

  Int_t Caen1190Module::LoadSlot( THaSlotData* sldat, const UInt_t* evbuffer,
                                  Int_t pos, Int_t len ) {
    // Fill data structures of this class, utilizing bank structure
    // Read until out of data or until decode says that the slot is finished
    // len = ndata in event, pos = word number for block header in event
    slot_data = sldat;
    fWordsSeen = 0;
    while(fWordsSeen < len) {
      Decode(&evbuffer[pos+fWordsSeen]);
      fWordsSeen++;
    }
    return fWordsSeen;
  }

  Int_t Caen1190Module::Decode(const UInt_t *p) {
    Int_t glbl_trl = 0;
    switch( *p & 0xf8000000) {
    case 0xc0000000 : // buffer alignment filler word; skip
      break;
    case 0x40000000 : // global header
      	tdc_data.glb_hdr_evno = (*p & 0x07ffffe0) >> 5; // bits 26-5
	tdc_data.glb_hdr_slno =  *p & 0x0000001f;       // bits 4-0
	if (tdc_data.glb_hdr_slno == static_cast <UInt_t> (fSlot)) {
#ifdef WITH_DEBUG
	if (fDebugFile)
	  *fDebugFile << "Caen1190Module:: 1190 GLOBAL HEADER >> data = " 
		      << hex << *p << " >> event number = " << dec 
		      << tdc_data.glb_hdr_evno << " >> slot number = "  
		      << tdc_data.glb_hdr_slno << endl;
#endif
      }
      break;
    case 0x08000000 : // tdc header
      if (tdc_data.glb_hdr_slno == static_cast <UInt_t> (fSlot)) {
	tdc_data.hdr_chip_id  = (*p & 0x03000000) >> 24; // bits 25-24
	tdc_data.hdr_event_id = (*p & 0x00fff000) >> 12; // bits 23-12
	tdc_data.hdr_bunch_id =  *p & 0x00000fff;        // bits 11-0
#ifdef WITH_DEBUG
	if (fDebugFile)
	  *fDebugFile << "Caen1190Module:: 1190 TDC HEADER >> data = " 
		      << hex << *p << " >> chip id = " << dec 
		      << tdc_data.hdr_chip_id  << " >> event id = "
		      << tdc_data.hdr_event_id << " >> bunch_id = "
		      << tdc_data.hdr_bunch_id << endl;
#endif
      }
      break;
    case  0x00000000 : // tdc measurement
      if (tdc_data.glb_hdr_slno == static_cast <UInt_t> (fSlot)) {
	tdc_data.chan   = (*p & 0x03f80000)>>19; // bits 25-19
	tdc_data.raw    =  *p & 0x0007ffff;      // bits 18-0
	tdc_data.opt    = (*p & 0x04000000)>>26;      // bit 26
	tdc_data.status = slot_data->loadData("tdc", tdc_data.chan, tdc_data.raw, tdc_data.opt);
#ifdef WITH_DEBUG
	if (fDebugFile)
	  *fDebugFile << "Caen1190Module:: 1190 MEASURED DATA >> data = " 
		      << hex << *p << " >> channel = " << dec
		      << tdc_data.chan << " >> edge = "
		      << tdc_data.opt  << " >> raw time = "
		      << tdc_data.raw << " >> status = "
		      << tdc_data.status << endl;
#endif
        if(Int_t (tdc_data.chan) < NTDCCHAN &&
           fNumHits[tdc_data.chan] < MAXHIT) {
          fTdcData[tdc_data.chan * MAXHIT + fNumHits[tdc_data.chan]] = tdc_data.raw;
          fTdcOpt[tdc_data.chan * MAXHIT + fNumHits[tdc_data.chan]++] = tdc_data.opt;
        }
        if (tdc_data.status != SD_OK ) return -1;
      }
      break;
    case 0x18000000 : // tdc trailer
      if (tdc_data.glb_hdr_slno == static_cast <UInt_t> (fSlot)) {
	tdc_data.trl_chip_id     = (*p & 0x03000000) >> 24; // bits 25-24
	tdc_data.trl_event_id    = (*p & 0x00fff000) >> 12; // bits 23-12
	tdc_data.trl_word_cnt    =  *p & 0x00000fff;        // bits 11-0
#ifdef WITH_DEBUG
	if (fDebugFile)
	  *fDebugFile << "Caen1190Module:: 1190 TDC TRAILER >> data = " 
		      << hex << *p << " >> chip id = " << dec 
		      << tdc_data.trl_chip_id  << " >> event id = "
		      << tdc_data.trl_event_id << " >> word count = "
		      << tdc_data.trl_word_cnt << endl;
#endif
      }
      break;
    case 0x20000000 : // tdc error
      if (tdc_data.glb_hdr_slno == static_cast <UInt_t> (fSlot)) {
	tdc_data.chip_nr_hd = (*p & 0x03000000) >> 24; // bits 25-24
	tdc_data.flags      =  *p & 0x00007fff;	       // bits 14-0
	cout << "TDC1190 Error: Slot " << tdc_data.glb_hdr_slno << ", Chip " << tdc_data.chip_nr_hd << 
	  ", Flags " << hex << tdc_data.flags << dec << " " << ", Ev #" << tdc_data.glb_hdr_evno << endl;
#ifdef WITH_DEBUG
	if (fDebugFile)
	  *fDebugFile << "Caen1190Module:: 1190 TDC ERROR >> data = " 
		      << hex << *p << " >> chip header = " << dec
		      << tdc_data.chip_nr_hd << " >> error flags = " << hex
		      << tdc_data.flags << dec << endl;
#endif
	break;
      default:	// unknown word
	cout << "unknown word for TDC1190: " << hex << (*p) << dec << endl;
	cout << "according to global header ev. nr. is: " << " " << tdc_data.glb_hdr_evno << endl;
      }
      break;
    case 0x88000000 :  // extended trigger time tag
      if (tdc_data.glb_hdr_slno == static_cast <UInt_t> (fSlot)) {
	tdc_data.trig_time = *p & 0x7ffffff; // bits 27-0
#ifdef WITH_DEBUG
	if (fDebugFile)
	  *fDebugFile << "Caen1190Module:: 1190 GLOBAL TRIGGER TIME >> data = " 
		      << hex << *p << " >> trigger time = " << dec
		      << tdc_data.trig_time << endl;
#endif
      }
      break;
    case 0x80000000 : // global trailer
     if (tdc_data.glb_hdr_slno == static_cast <UInt_t> (fSlot)) {
       tdc_data.glb_trl_status  = (*p & 0x07000000) >> 24; // bits 24-26
       tdc_data.glb_trl_wrd_cnt = (*p & 0x001fffe0) >> 5;  // bits 20-5
       tdc_data.glb_trl_slno    =  *p & 0x0000001f;        // bits 4-0   
#ifdef WITH_DEBUG
	if (fDebugFile)
	  *fDebugFile << "Caen1190Module:: 1190 GLOBAL TRAILER >> data = " 
		      << hex << *p << " >> status = "
		      << tdc_data.glb_trl_status << " >> word count = " << dec 
		      << tdc_data.glb_trl_wrd_cnt << " >> slot number = "  
		      << tdc_data.glb_trl_slno << endl;
#endif
	glbl_trl = 1;
      }
      break;
    }
    return glbl_trl;
  }
 
  Int_t Caen1190Module::GetData(Int_t chan, Int_t hit) const {
    if(hit >= fNumHits[chan]) return 0;
    Int_t idx = chan*MAXHIT + hit;
    if (idx < 0 || idx > MAXHIT*NTDCCHAN) return 0;
    return fTdcData[idx];
  }

  Int_t Caen1190Module::GetOpt(Int_t chan, Int_t hit) const {
    if((Int_t)hit >= fNumHits[chan]) return 0;
    Int_t idx = chan*MAXHIT + hit;
    if (idx < 0 || idx > MAXHIT*NTDCCHAN) return 0;
    return fTdcOpt[idx];
  }

  void Caen1190Module::Clear(Option_t*) {
    fNumHits.assign(NTDCCHAN,0);
    fTdcData.assign(NTDCCHAN*MAXHIT, 0);
    fTdcOpt.assign(NTDCCHAN*MAXHIT, 0);
  }
}

ClassImp(Decoder::Caen1190Module)
