////////////////////////////////////////////////////////////////////
//
//   PipeliningModule
//   A JLab PipeLining Module.  See header file for more comments.
//   R. Michaels, Oct 2016
//
/////////////////////////////////////////////////////////////////////

#include "PipeliningModule.h"
#include "THaSlotData.h"
#include <iostream>
#include <cstdlib>  // for exit()

using namespace std;

namespace Decoder {

PipeliningModule::PipeliningModule(Int_t crate, Int_t slot)
  : VmeModule(crate,slot),
    fNWarnings(0), fBlockHeader(0),
    data_type_def(15),  // initialize to FILLER WORD
    fFirstTime(kTRUE), index_buffer(0)
{
  fMultiBlockMode = kFALSE;
  fBlockIsDone = kFALSE;
  ReStart();
}

PipeliningModule::~PipeliningModule() {
}

Int_t PipeliningModule::SplitBuffer(const std::vector< UInt_t >& codabuffer) {

// Split a CODA buffer into blocks.   A block is data from a traditional physics event.
// In MultiBlock Mode, a pipelining module can have several events in each CODA buffer.
// If block level is 1, then the buffer is a traditional physics event.
// If finding >1 block, this will set fMultiBlockMode = kTRUE

  std::vector<UInt_t > oneEventBuffer;
  eventblock.clear();
  fBlockIsDone = kFALSE;
  Int_t eventnum = 1;
  Int_t evt_num_modblock;

  if ((fFirstTime == kFALSE) && (IsMultiBlockMode() == kFALSE)) {
     eventblock.push_back(codabuffer);
     index_buffer=1;
     return 1;
  }

  int debug=1;

  Int_t slot_blk_hdr = 0, slot_evt_hdr = 0;
  Int_t BlockStart=0;

  block_size = 0;   // member of the base class Module.h

  for (UInt_t i=0;  i < codabuffer.size(); i++) {

    UInt_t data=codabuffer[i];

    if (debug >= 1) {
      if (fDebugFile != 0) *fDebugFile << hex <<"SplitBuffer, data = "<<hex<<data<<dec<<endl;
    }

    UInt_t data_type_id = (data >> 31) & 0x1;  // Data type identification, mask 1 bit

    if (data_type_id == 1)
      data_type_def = (data >> 27) & 0xF;

    if (debug == 1) {
      if (fDebugFile != 0) *fDebugFile << "SplitBuffer: data types: data_type_id = " << data_type_id
			<< " data_type_def = " << data_type_def << endl;
    }

    switch(data_type_def)
      {
      case 0: // Block header, indicates the begining of a block of events
        if (data_type_id) {
          fBlockHeader = data;
          slot_blk_hdr = (data >> 22) & 0x1F;  // Slot number (set by VME64x backplane), mask 5 bits
          block_size = (data >> 0) & 0xFF;  // Number of events in block, mask 8 bits
          if (block_size > 1) fMultiBlockMode = kTRUE;
          if ((fMultiBlockMode==kTRUE) && (slot_blk_hdr==fSlot)) BlockStart=1;
          // Debug output
          if (debug >= 1 && fDebugFile != 0) {
            Int_t iblock_num = (data >> 8) & 0x3FF;  // Event block number, mask 10 bits
            *fDebugFile << "SplitBuffer:  %% data BLOCK header: slot_blk_hdr = " << dec<<slot_blk_hdr
                << " iblock_num = " << iblock_num << " block_size = " << block_size << endl;
          }
        }
        break;
      case 1: // Block trailer, indicates the end of a block of events
        {
          Int_t slot_blk_trl = (data >> 22) & 0x1F;  // Slot number (set by VME64x backplane), mask 5 bits
          if ((fMultiBlockMode==kTRUE) && (slot_blk_trl==fSlot)) {
            BlockStart++;
            oneEventBuffer.push_back(data);
            // There is no "event trailer", but a block trailer indicates the last event in a block.
            eventblock.push_back(oneEventBuffer);
            oneEventBuffer.clear();
          }

          // Debug output
          if (debug >= 1 && fDebugFile != 0) {
            Int_t nwords_inblock = (data >> 0) & 0x3FFFFF;  // Total number of words in block of events, mask 22 bits
            *fDebugFile << "SplitBuffer: %% data BLOCK trailer: slot_blk_trl = " <<  slot_blk_trl
                << " nwords_inblock = " << nwords_inblock << endl;
          }
        }
        break;
      case 2: // Event header, indicates start of an event, includes the trigger number
        {
          slot_evt_hdr = (data >> 22) & 0x1F;  // Slot number (set by VME64x backplane), mask 5 bits
          Int_t evt_num = (data & 0x3FFFFF);   // Total number of words in block of events, mask 22 bits
          if (block_size==0) {
            evt_num_modblock=0;
          } else {
            evt_num_modblock = (evt_num%block_size);
          }
          if (slot_blk_hdr==fSlot) {
            BlockStart++;
            if (fDebugFile != 0) *fDebugFile << "evt_num logic "<< evt_num<<"  "<<block_size<<"  "<<evt_num_modblock<<"   "<<eventnum<<endl;
          }
          // for some older firmware, slot_evt_hdr is zero, so use slot_blk_hdr
          if ((fMultiBlockMode==kTRUE) && (slot_blk_hdr==fSlot)) {
// There is no "event trailer", so we use the change to next event to recognize the end of an event.
// One could look for the (evt_num_modblock != eventnum) but I find that for some data files the
// evt_num makes no sense and is a random number.  Instead, the following logic works.
            if (BlockStart != 2) {
              eventblock.push_back(oneEventBuffer);
              oneEventBuffer.clear();
            }
            eventnum = evt_num_modblock;
            oneEventBuffer.push_back(fBlockHeader);  // put block header with each event, e.g. FADC250 needs it.
            oneEventBuffer.push_back(data);
          }

          // Debug output
          if (debug >= 1 && fDebugFile != 0) {
            *fDebugFile << "SplitBuffer:  %% data EVENT header: slot_evt_hdr = " << slot_evt_hdr
                << " evt_num = " << evt_num << "  "
                << oneEventBuffer.size() <<"   "<<eventblock.size()<<endl;
          }
        }
        break;
      default:
        if (slot_blk_hdr != slot_evt_hdr) {
          // for some older firmware, slot_evt_hdr is zero
          if ((fNWarnings++ % 100)==0)
            cerr << "PipeliningModule::WARNING : inconsistent slot num  "<<endl;
        }
        // all other data goes here
        if ((fMultiBlockMode==kTRUE) && (slot_blk_hdr==fSlot)) oneEventBuffer.push_back(data);

      }

  }

  fFirstTime = kFALSE;

  if (IsMultiBlockMode() == kFALSE) {
    eventblock.push_back(codabuffer);
    index_buffer=1;
    return 1;
  }
  if (IsMultiBlockMode() == kTRUE) {
    if (static_cast<size_t>(block_size) != eventblock.size()) {
      cerr << "PipeliningModule::ERROR:  num events in block inconsistent"<<endl;
      if (fDebugFile != 0) *fDebugFile << "block_size = "<<dec<<block_size<<"   "<<eventblock.size()<<endl;
    }
    if (debug >= 1) PrintBlocks();  // debug
  }


  return 0;

}

void PipeliningModule::PrintBlocks() {
// Print all the blocks(events) if in mulitblock mode.
// Note, the first event buffer will have the block header
// the last buffer will have the block trailer
// and all buffers will have an event header
  static Int_t maxloops=5000000;
  if (IsMultiBlockMode() == kFALSE) {
     if (fDebugFile != 0) *fDebugFile << "PipelingModule:  Not in multiblock mode.  Bye."<<endl;
     return;
  }
  ReStart();
  if (fDebugFile != 0) {
      *fDebugFile << "PipeliningModule :: Number of events in block = "<<eventblock.size()<<endl;
      *fDebugFile << "fSlot = "<<fSlot<<endl;
  }
  Int_t iblk=1;
  Int_t icnt=0;
  while (!BlockIsDone()) {
    if (icnt++ > maxloops) {
       cerr << "PipeliningModule:: ERROR: infinite loop PrintBlocks "<<endl;
       exit(0);  //  should never happen
    }
    std::vector<UInt_t> evbuffer = GetNextBlock();
    if (fDebugFile != 0) *fDebugFile << "Block number " << iblk++ <<endl;
    for (UInt_t j = 0; j < evbuffer.size(); j++) {
      if (fDebugFile != 0) *fDebugFile << "            evbuffer["<<j<<"] =   0x"<<hex<<evbuffer[j]<<dec<<endl;
    }
  }
  ReStart();
}

void PipeliningModule::ReStart() {
   index_buffer = 0;
   fBlockIsDone = kFALSE;
}

std::vector< UInt_t > PipeliningModule::GetNextBlock() {
  std::vector< UInt_t > vnothing;  vnothing.clear();
  if (eventblock.size()==0) {
      cerr << "ERROR:  No event buffers ! "<<endl;   // Should never happen
      return vnothing;
  }
  if (IsMultiBlockMode() == kFALSE ) return eventblock[0];
  if (index_buffer == (eventblock.size()-1)) fBlockIsDone=kTRUE;
  index_buffer++;
  return eventblock[GetIndex()];
}

UInt_t PipeliningModule::GetIndex() {
  UInt_t idx = index_buffer - 1;
  if (index_buffer > 0 && idx < eventblock.size())
    return idx;
  cerr << "Warning:  index problem in PipeliningModule "
       << idx << "  " << eventblock.size() << endl;
  return 0;
}


}


ClassImp(Decoder::PipeliningModule)
