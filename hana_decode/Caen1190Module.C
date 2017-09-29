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
    : VmeModule(crate, slot) {
    memset(&tdc_data, 0, sizeof(tdc_data));
    fDebugFile=0;
    Init();
  }

  Caen1190Module::~Caen1190Module() {
    if(fNumHits) delete [] fNumHits;
    if(fTdcData) delete [] fTdcData;
  }

  void Caen1190Module::Init() {
    Module::Init();
    fNumHits = new Int_t[NTDCCHAN];
    fTdcData = new Int_t[NTDCCHAN*MAXHIT];
    fDebugFile = 0;
    Clear("");
    IsInit = kTRUE;
    fName = "Caen TDC 1190 Module";
  }

  Int_t Caen1190Module::LoadSlot(THaSlotData *sldat, const UInt_t *evbuffer, const UInt_t *pstop) {
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

  Int_t Caen1190Module::LoadSlot(THaSlotData *sldat, const UInt_t *evbuffer, Int_t pos, Int_t len) {
    // Fill data structures of this class, utilizing bank structure
    // Read until out of data or until decode says that the slot is finished
    // len = ndata in event, pos = word number for block header in event
    slot_data = sldat;
    fWordsSeen = 0;
    Int_t index = 0;
    while(fWordsSeen < len) {
      index = pos + fWordsSeen;
      Decode(&evbuffer[index]);
      fWordsSeen++;
    }
    return fWordsSeen;
  }

  Int_t Caen1190Module::Decode(const UInt_t *p) {
    Int_t glbl_trl = 0;
    switch( (*p) & 0xf8000000) {
    case 0xc0000000 :		// buffer alignment filler word; skip
      break;
    case 0x40000000 : 		// Global header
      tdc_data.evno=(*p & 0x07ffffe0) >> 5;
      tdc_data.slot=(*p & 0x0000001f);
      if (tdc_data.slot == static_cast <UInt_t> (fSlot)) {
#ifdef WITH_DEBUG
	if (fDebugFile != 0)
	  *fDebugFile << "Caen1190Module:: 1190 GLOBAL HEADER >> data = " 
		      << hex << *p << " >> event number = " << dec 
		      << tdc_data.evno << " >> slot number = "  
		      << tdc_data.slot << endl;
#endif
      }
      break;
    case 0x08000000 :  // chip header; contains: chip nr., ev. nr, bunch ID
      //chip_nr_hd = ((*p)&0x03000000) >> 24; // bits 25-24
      //bunch_id = ((*p)&0x00000fff); // bits 11-0
      break;
    case  0x00000000 : 		// measurement data
      if (tdc_data.slot == static_cast <UInt_t> (fSlot)) {
	tdc_data.chan=((*p)&0x03f80000)>>19; // bits 25-19
	tdc_data.raw=((*p)&0x0007ffff); // bits 18-0
	tdc_data.status = slot_data->loadData("tdc", tdc_data.chan, tdc_data.raw, tdc_data.raw);
#ifdef WITH_DEBUG
	if (fDebugFile != 0)
	  *fDebugFile << "Caen1190Module:: 1190 MEASURED DATA >> data = " 
		      << hex << *p << " >> channel = " << dec
		      << tdc_data.chan << " >> raw time = "
		      << tdc_data.raw << " >> status = "
		      << tdc_data.status << endl;
#endif
	if(Int_t (tdc_data.chan) < NTDCCHAN) { 
	  if(fNumHits[tdc_data.chan] < MAXHIT) {
	    fTdcData[tdc_data.chan*MAXHIT + fNumHits[tdc_data.chan]++] = tdc_data.raw;
	  }
	}
	if (tdc_data.status != SD_OK ) return -1;
      }
      break;
    case 0x18000000 : // chip trailer:  contains chip nr. & ev nr & word count
      // chip_nr_tr = ((*p)&0x03000000) >> 24; // bits 25-24
      /* If there is a chip trailer we assume there is a header too so we
       * cross check if chip nr stored in header matches chip nr. stored in
       * trailer.
       */
      // if (chip_nr_tr != chip_nr_hd) {
      // cerr  << "mismatch in chip nr between chip header and trailer"
      //      << " " << "header says: " << " " << chip_nr_hd
      //	      << " " << "trailer says: " << " " << chip_nr_tr << endl;
      //};
      //chip_nr_words = ((*p)&0x00000fff); // bits 11-0
      //nword_mod++;
      break;
    case 0x80000000 :  // global trailer: contains error status & word count per module & slot nr.
      //      status_err = ((*p)&0x07000000) >> 24; // bits 26-24
      //      if(status_err != 0) {
      //	cerr << "(evt: " << event_num << ", slot: "<< slot_from_gl_hd << ") ";
      //	cerr << "Error in 1190 status word: " << hex << status_err << dec << endl;
      //      }
      //      module_nr_words = ((*p)&0x001fffe0) >> 5; // bits 20-5
      //      slot_from_gl_tr = ((*p)&0x0000001f); // bits 4-0
      //      if (slot_from_gl_tr != slot_from_gl_hd) {
      //	cerr << "(evt: " << event_num << ", slot: "<< slot_from_gl_hd << ") ";
      //	cerr  << "mismatch in slot between global header and trailer"
      //	      << " " << "header says: " << " " << slot_from_gl_hd
      //	      << " " << "trailer says: " << " " << slot_from_gl_tr << endl;
      //      };
      //      nword_mod++;
      if (tdc_data.slot == static_cast <UInt_t> (fSlot))
	glbl_trl = 1;
      break;
    case 0x88000000:  // Global Trigger Time Tag: trigger arrival time relatively to the Count Reset.
      break;
    case 0x20000000:		// Output Buffer: TDC Error
      if (tdc_data.slot == static_cast <UInt_t> (fSlot)) {
	tdc_data.chip_nr_hd = ((*p)&0x03000000) >> 24; // bits 25-24
	tdc_data.flags = *p&0x7fff;			   // Error flags
	cout << "TDC1190 Error: Slot " << tdc_data.slot << ", Chip " << tdc_data.chip_nr_hd << 
	  ", Flags " << hex << tdc_data.flags << dec << " " << ", Ev #" << tdc_data.evno << endl;
#ifdef WITH_DEBUG
	if (fDebugFile != 0)
	  *fDebugFile << "Caen1190Module:: 1190 TDC ERROR >> data = " 
		      << hex << *p << " >> chip header = " << dec
		      << tdc_data.chip_nr_hd << " >> error flags = " << hex
		      << tdc_data.flags << dec << endl;
#endif
	break;
      default:			// Unknown word
	cout << "unknown word for TDC1190: " << hex << (*p) << dec << endl;
	cout << "according to global header ev. nr. is: " << " " << tdc_data.evno << endl;
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

  void Caen1190Module::Clear(const Option_t*) {
    memset(fNumHits, 0, NTDCCHAN*sizeof(Int_t));
    memset(fTdcData, 0, NTDCCHAN*MAXHIT*sizeof(Int_t));
  }
}

ClassImp(Decoder::Caen1190Module)

