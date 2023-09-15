/** \class Caen1190 Module
    \author Stephen Wood
    \author Simona Malace
    \author Brad Sawatzky
    \author Eric Pooser
    \author Ole Hansen

    Decoder module to retrieve Caen 1190 TDCs.  Based on CAEN 1190 decoding in
    THaCodaDecoder.C in podd 1.5.   (Written by S. Malace, modified by B. Sawatzky)
*/

#include "Caen1190Module.h"
#include "THaSlotData.h"
#include <iostream>
#include <sstream>
#include <cassert>

using namespace std;

//#define DEBUG
//#define WITH_DEBUG

namespace Decoder {

const UInt_t NTDCCHAN = 128;
const UInt_t MAXHIT = 100;

Module::TypeIter_t Caen1190Module::fgThisType =
  DoRegister(ModuleType("Decoder::Caen1190Module", 1190));

//_____________________________________________________________________________
Caen1190Module::Caen1190Module( Int_t crate, Int_t slot )
  : PipeliningModule(crate, slot)
  , fNumHits(NTDCCHAN)
  , fTdcData(NTDCCHAN * MAXHIT)
  , fTdcOpt(NTDCCHAN * MAXHIT)
  , fSlotData(nullptr)
  , fEvBuf(nullptr)
  , fNfill(0)
{
  Caen1190Module::Init();
}

//_____________________________________________________________________________
void Caen1190Module::Init()
{
  PipeliningModule::Init();
  fNumHits.resize(NTDCCHAN);
  fTdcData.resize(NTDCCHAN * MAXHIT);
  fTdcOpt.resize(NTDCCHAN * MAXHIT);
  Caen1190Module::Clear();
  IsInit = true;
  fName = "Caen TDC 1190 Module";
}

//_____________________________________________________________________________
UInt_t Caen1190Module::LoadSlot( THaSlotData* sldat, const UInt_t* evbuffer,
                                 const UInt_t* pstop )
{
  // Load from evbuffer between [evbuffer,pstop]

  return LoadSlot(sldat, evbuffer, 0, pstop + 1 - evbuffer);
}

//_____________________________________________________________________________
UInt_t Caen1190Module::LoadSlot( THaSlotData* sldat, const UInt_t* evbuffer,
                                 UInt_t pos, UInt_t len )
{
  // Fill data structures of this class.
  // Read until out of data or until Decode() says that the slot is finished.
  // len = ndata in event, pos = word number for block header in event

  fSlotData = sldat;    // Used in Decode()
  const auto* p = evbuffer + pos;
  const auto* const q = p + len;
  while( p != q ) {
    if( Decode(p++) != 0 )
      break;  // global trailer found
  }
  return fWordsSeen = p - (evbuffer + pos);
}

//_____________________________________________________________________________
Int_t Caen1190Module::Decode( const UInt_t* p )
{
  // Interpret the data word pointed to by 'p'

  Int_t glbl_trl = 0;
  switch( *p >> 27 ) {
    case kFiller : // buffer alignment filler word; skip
      break;
    case kGlobalHeader : // global header
      tdc_data.glb_hdr_evno = (*p & 0x07ffffe0) >> 5; // bits 26-5
      tdc_data.glb_hdr_slno = *p & 0x0000001f;       // bits 4-0
      if( tdc_data.glb_hdr_slno == fSlot ) {
#ifdef WITH_DEBUG
        if( fDebugFile )
          *fDebugFile << "Caen1190Module:: 1190 GLOBAL HEADER >> data = "
                      << hex << *p << " >> event number = " << dec
                      << tdc_data.glb_hdr_evno << " >> slot number = "
                      << tdc_data.glb_hdr_slno << endl;
#endif
      }
      break;
    case kTDCHeader : // tdc header
      if( tdc_data.glb_hdr_slno == fSlot ) {
        tdc_data.hdr_chip_id = (*p & 0x03000000) >> 24; // bits 25-24
        tdc_data.hdr_event_id = (*p & 0x00fff000) >> 12; // bits 23-12
        tdc_data.hdr_bunch_id = *p & 0x00000fff;        // bits 11-0
#ifdef WITH_DEBUG
        if( fDebugFile )
          *fDebugFile << "Caen1190Module:: 1190 TDC HEADER >> data = "
                      << hex << *p << " >> chip id = " << dec
                      << tdc_data.hdr_chip_id << " >> event id = "
                      << tdc_data.hdr_event_id << " >> bunch_id = "
                      << tdc_data.hdr_bunch_id << endl;
#endif
      }
      break;
    case kTDCData : // tdc measurement
      if( tdc_data.glb_hdr_slno == fSlot ) {
        tdc_data.chan = (*p & 0x03f80000) >> 19; // bits 25-19
        tdc_data.raw = *p & 0x0007ffff;      // bits 18-0
        tdc_data.opt = (*p & 0x04000000) >> 26;      // bit 26
        tdc_data.status = fSlotData->loadData("tdc", tdc_data.chan, tdc_data.raw, tdc_data.opt);
#ifdef WITH_DEBUG
        if( fDebugFile )
          *fDebugFile << "Caen1190Module:: 1190 MEASURED DATA >> data = "
                      << hex << *p << " >> channel = " << dec
                      << tdc_data.chan << " >> edge = "
                      << tdc_data.opt << " >> raw time = "
                      << tdc_data.raw << " >> status = "
                      << tdc_data.status << endl;
#endif
        if( tdc_data.chan < NTDCCHAN &&
            fNumHits[tdc_data.chan] < MAXHIT ) {
          fTdcData[tdc_data.chan * MAXHIT + fNumHits[tdc_data.chan]] = tdc_data.raw;
          fTdcOpt[tdc_data.chan * MAXHIT + fNumHits[tdc_data.chan]++] = tdc_data.opt;
        }
        if( tdc_data.status != SD_OK ) return -1;
      }
      break;
    case kTDCTrailer : // tdc trailer
      if( tdc_data.glb_hdr_slno == fSlot ) {
        tdc_data.trl_chip_id = (*p & 0x03000000) >> 24; // bits 25-24
        tdc_data.trl_event_id = (*p & 0x00fff000) >> 12; // bits 23-12
        tdc_data.trl_word_cnt = *p & 0x00000fff;        // bits 11-0
#ifdef WITH_DEBUG
        if( fDebugFile )
          *fDebugFile << "Caen1190Module:: 1190 TDC TRAILER >> data = "
                      << hex << *p << " >> chip id = " << dec
                      << tdc_data.trl_chip_id << " >> event id = "
                      << tdc_data.trl_event_id << " >> word count = "
                      << tdc_data.trl_word_cnt << endl;
#endif
      }
      break;
    case kTDCError : // tdc error
      if( tdc_data.glb_hdr_slno == fSlot ) {
        tdc_data.chip_nr_hd = (*p & 0x03000000) >> 24; // bits 25-24
        tdc_data.flags = *p & 0x00007fff;               // bits 14-0
        cout << "TDC1190 Error: Slot " << tdc_data.glb_hdr_slno << ", Chip " << tdc_data.chip_nr_hd <<
             ", Flags " << hex << tdc_data.flags << dec << " " << ", Ev #" << tdc_data.glb_hdr_evno << endl;
#ifdef WITH_DEBUG
        if( fDebugFile )
          *fDebugFile << "Caen1190Module:: 1190 TDC ERROR >> data = "
                      << hex << *p << " >> chip header = " << dec
                      << tdc_data.chip_nr_hd << " >> error flags = " << hex
                      << tdc_data.flags << dec << endl;
#endif
      }
      break;
    case kTriggerTime :  // extended trigger time tag
      if( tdc_data.glb_hdr_slno == fSlot ) {
        tdc_data.trig_time = *p & 0x7ffffff; // bits 27-0
#ifdef WITH_DEBUG
        if( fDebugFile )
          *fDebugFile << "Caen1190Module:: 1190 GLOBAL TRIGGER TIME >> data = "
                      << hex << *p << " >> trigger time = " << dec
                      << tdc_data.trig_time << endl;
#endif
      }
      break;
    case kGlobalTrailer : // global trailer
      if( tdc_data.glb_hdr_slno == fSlot ) {
        tdc_data.glb_trl_status = (*p & 0x07000000) >> 24; // bits 24-26
        tdc_data.glb_trl_wrd_cnt = (*p & 0x001fffe0) >> 5;  // bits 20-5
        tdc_data.glb_trl_slno = *p & 0x0000001f;        // bits 4-0
#ifdef WITH_DEBUG
        if( fDebugFile )
          *fDebugFile << "Caen1190Module:: 1190 GLOBAL TRAILER >> data = "
                      << hex << *p << " >> status = "
                      << tdc_data.glb_trl_status << " >> word count = " << dec
                      << tdc_data.glb_trl_wrd_cnt << " >> slot number = "
                      << tdc_data.glb_trl_slno << endl;
#endif
        glbl_trl = 1;
      }
      break;
    default:        // unknown word
      cout << "unknown word for TDC1190: " << hex << (*p) << dec << endl;
      cout << "according to global header ev. nr. is: " << " " << tdc_data.glb_hdr_evno << endl;
      break;
  }
  return glbl_trl;
}

//_____________________________________________________________________________
UInt_t Caen1190Module::GetData( UInt_t chan, UInt_t hit ) const
{
  if( hit >= fNumHits[chan] ) return 0;
  UInt_t idx = chan * MAXHIT + hit;
  if( idx > MAXHIT * NTDCCHAN ) return 0;
  return fTdcData[idx];
}

//_____________________________________________________________________________
UInt_t Caen1190Module::GetOpt( UInt_t chan, UInt_t hit ) const
{
  if( hit >= fNumHits[chan] ) return 0;
  UInt_t idx = chan * MAXHIT + hit;
  if( idx > MAXHIT * NTDCCHAN ) return 0;
  return fTdcOpt[idx];
}

//_____________________________________________________________________________
void Caen1190Module::Clear( Option_t* )
{
  PipeliningModule::Clear();
  fNumHits.assign(NTDCCHAN, 0);
  fTdcData.assign(NTDCCHAN * MAXHIT, 0);
  fTdcOpt.assign(NTDCCHAN * MAXHIT, 0);
  fNfill = 0;
}

//_____________________________________________________________________________
UInt_t Caen1190Module::LoadBank( THaSlotData* sldat, const UInt_t* evbuffer,
                                 UInt_t pos, UInt_t len )
{
  // Load event block. If multi-block data, split the buffer into individual
  // events. This routine is called for buffers with bank structure.

  const char* const here = "LoadBank";

  // Note: Clear() has been called at this point
  assert(evtblk.empty());

  const UInt_t MINLEN = 10;
  if( len < MINLEN ) {
    cerr << Here(here) << "FATAL ERROR: Bank too short. Expected >= "
         << MINLEN << ", got " << len << " words. Skipping event." << endl;
    // Caller neither checks error codes nor handles exceptions. Hopeless.
    return 0;
  }
  // Find all the global headers for this module's slot in this bank.
  // Unfortunately, the current data format lacks a block header/trailer
  // and does not tell us anything about the module's idea of the block size,
  // so we have to discover it from the data.
  evtblk.reserve(16);
  UInt_t cur = pos;
  UInt_t evtnum = 0;
  Long64_t ihdr;
  Long64_t itrl = 0;
  while( cur < pos+len &&
         (ihdr = Find1190Word(evbuffer, cur, len+pos-cur,
                              kGlobalHeader, fSlot)) != -1 ) {
    const UInt_t* p = evbuffer + ihdr;
    UInt_t next_evtnum = (*p & 0x07FFFFE0) >> 5;
    if( !evtblk.empty() ) {
      if( next_evtnum != evtnum + 1 ) {
        cerr << Here(here) << "WARNING: event numbers in block not "
                              "sequential, prev/next = "
             << evtnum << "/" << next_evtnum << endl;
      }
    }
    evtnum = next_evtnum;
    evtblk.push_back(ihdr);
    cur = ihdr + 1;
    itrl = Find1190Word(evbuffer, cur, len+pos-cur, kGlobalTrailer, fSlot);
    if( itrl == -1 ) {
      cerr << Here(here)
           << "FATAL ERROR: No global trailer found for slot " << fSlot
           << " event " << evtnum << ", skipping block" << endl;
      return 0;
    }
    p = evbuffer + itrl;
    UInt_t nwords_trl = (*p & 0x1FFFE0) >> 5;
    if( nwords_trl != itrl-ihdr+1 ) {
      cerr << Here(here) << "Warning: global trailer word count mismatch, "
           << "got " << nwords_trl << ", expected " << itrl-ihdr+1 << endl;
    }
    cur = itrl + 1;
  }
  if( evtblk.empty() ) {
    // No global header found at all. Should not happen.
    cerr << Here(here)
         << "FATAL ERROR: No global header found in bank. Call expert." << endl;
    return 0;
  }
  block_size = evtblk.size();
  // TODO: cross check block size with trigger bank info

  UInt_t ibeg = evtblk[0];
  // Put position one past last global trailer at end of evtblk array
  UInt_t iend = itrl+1;
  evtblk.push_back(iend);

  // Consume filler words, if any
  const UInt_t* p = evbuffer + iend - 1;
  while( ++p-evbuffer < pos+len && (*p >> 27) == kFiller ) {
    ++fNfill;
  }

  // Multi-block event?
  fMultiBlockMode = ( block_size > 1 );

  if( fMultiBlockMode ) {
    // Save pointer to event buffer for use in LoadNextEvBuffer
    fEvBuf = evbuffer;
    // Multi-block: decode first event, using event positions from evtblk[]
    index_buffer = 0;
    return LoadNextEvBuffer(sldat);

  } else {
    // Single block: decode, starting at global header
    return LoadSlot(sldat, evbuffer, ibeg, iend-ibeg)
           + fNfill;
  }
  // not reached
}

//_____________________________________________________________________________
UInt_t Caen1190Module::LoadNextEvBuffer( THaSlotData* sldat )
{
  // In multi-block mode, load the next event from the current block.
  // Returns number of words consumed

  UInt_t ii = index_buffer;
  assert( ii+1 < evtblk.size() );

  // ibeg = event header, iend = one past last word of this event ( = next
  // event header if more events pending)
  auto ibeg = evtblk[ii], iend = evtblk[ii+1];
  assert(ibeg > 0 && iend > ibeg);  // else bug in LoadBank

  assert(fEvBuf);  // We should never get here without a prior call to LoadBank
  // or after fBlockIsDone is set

  // Load slot starting with event header at ibeg
  ii = LoadSlot(sldat, fEvBuf, ibeg, iend-ibeg);

  // Next cached buffer. Set flag if we've exhausted the cache.
  ++index_buffer;
  if( index_buffer+1 >= evtblk.size() ) {
    fBlockIsDone = true;
    fEvBuf = nullptr;
  }

  if( fBlockIsDone )
    ii += fNfill; // filler words
  return ii;
}

//_____________________________________________________________________________
string Caen1190Module::Here( const char* function )
{
  // Convenience function for error messages
  ostringstream os;
  os << "Caen1190Module::" << function << ": slot/crate "
     << fSlot << "/" << fCrate << ": ";
  return os.str();
}

} // namespace Decoder

ClassImp(Decoder::Caen1190Module)
