////////////////////////////////////////////////////////////////////
//
//   CodaDecoder
//
//    Object Oriented version of decoder
//    Sept, 2014    R. Michaels
//
/////////////////////////////////////////////////////////////////////

#include "CodaDecoder.h"
#include "THaCrateMap.h"
#include "THaBenchmark.h"
#include "THaUsrstrutils.h"
#include "TError.h"
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <utility>

using namespace std;

#define ALL(c) (c).begin(), (c).end()

namespace Decoder {

//_____________________________________________________________________________
CodaDecoder::CodaDecoder() :
  nroc(0), irn(MAXROC, 0), fbfound(MAXROC*MAXSLOT, false),
  psfact(MAX_PSFACT, kMaxUInt), fdfirst(true), chkfbstat(1), evcnt_coda3(0)
{
  fNeedInit=true;
  first_decode=true;
  fMultiBlockMode=false;
  fBlockIsDone=false;
  // Please leave these 3 lines for me to debug if I need to.  thanks, Bob
#ifdef WANTDEBUG
  fDebugFile = new ofstream();
  fDebugFile->open("bobstuff.txt");
  *fDebugFile<< "Debug my stuff "<<endl<<endl;
#endif
}

//_____________________________________________________________________________
#ifndef WANTDEBUG
CodaDecoder::~CodaDecoder() = default;
#else
CodaDecoder::~CodaDecoder()
{
  delete fDebugFile; fDebugFile = nullptr;
}
#endif

//_____________________________________________________________________________
UInt_t CodaDecoder::GetPrescaleFactor( UInt_t trigger_type) const
{
  // To get the prescale factors for trigger number "trigger_type"
  // (valid types are 1,2,3...)
  if ( (trigger_type > 0) && (trigger_type <= MAX_PSFACT)) {
    return psfact[trigger_type - 1];
  }
  if (fDebug > 0) {
    Warning( "CodaDecoder::GetPrescaleFactor", "Requested prescale factor for "
	     "undefined trigger type %d", trigger_type );
  }
  return 0;
}

//_____________________________________________________________________________
Int_t CodaDecoder::Init()
{
  Int_t ret = THaEvData::Init();
  if( ret != HED_OK ) return ret;
  FindUsedSlots();
  return ret;
}

//_____________________________________________________________________________
Int_t CodaDecoder::LoadEvent( const UInt_t* evbuffer )
{
  // Main engine for decoding, called by public LoadEvent() methods

  assert(evbuffer);

  if (fDebugFile) {
    *fDebugFile << "CodaDecode:: Loading event  ... " << endl
                << "evbuffer ptr " << hex << evbuffer << dec << endl;
  }

  if (fDataVersion <= 0) {
    cerr << "CODA version not set (=" << fDataVersion << "). "
         << "Do SetCodaVersion(codaData->getCodaVersion()) first" << endl;
    return HED_FATAL;
  }
  if (fDataVersion != 2 && fDataVersion != 3) {
    cerr << "Unsupported CODA version = " << fDataVersion << "). Need 2 or 3."
         << "Cannot analyze these data. Twonk."<<endl;
    return HED_FATAL;
  }

  buffer = evbuffer;
  event_length = evbuffer[0]+1;  // in longwords (4 bytes)
  event_num = 0;
  event_type = 0;

  // Determine event type
  if (fDataVersion == 2) {
    event_type = evbuffer[1]>>16;
  } else {  // CODA version 3
    interpretCoda3(evbuffer);
  }
  if(fDebugFile) {
      *fDebugFile << "CodaDecode:: dumping "<<endl;
      dump(evbuffer);
  }

  Int_t ret = HED_OK;
  if (event_type == PRESTART_EVTYPE ) {
    // Usually prestart is the first 'event'.  Call SetRunTime() to
    // re-initialize the crate map since we now know the run time.
    // This won't happen for split files (no prestart). For such files,
    // the user should call SetRunTime() explicitly.
    SetRunTime(evbuffer[2]);
    run_num  = evbuffer[3];
    run_type = evbuffer[4];
    evt_time = fRunTime;
    if (fDebugFile) {
      *fDebugFile << "Prestart Event : run_num " << run_num
                  << "  run type "   << run_type
                  << "  event_type " << event_type
                  << "  run time "   << fRunTime
                  << endl;
    }
  }

  else if( event_type == PRESCALE_EVTYPE || event_type == TS_PRESCALE_EVTYPE ) {
    if( (ret = prescale_decode(evbuffer)) != HED_OK )
      return ret;
  }

  else if( event_type <= MAX_PHYS_EVTYPE ) {
    assert(fMap || fNeedInit);
    if( first_decode || fNeedInit ) {
      if( (ret = Init()) != HED_OK )
        return ret;
    }
    assert(fMap);
    if( fDoBench ) fBench->Begin("clearEvent");
    for( auto i : fSlotClear )
      crateslot[i]->clearEvent();
    if( fDoBench ) fBench->Stop("clearEvent");

    if( fDataVersion == 3 ) {
      event_num = ++evcnt_coda3;
      FindRocsCoda3(evbuffer);
    } else {
      event_num = evbuffer[4];
      FindRocs(evbuffer);
    }
    recent_event = event_num;

    if( fdfirst && fDebugFile ) {
      fdfirst = false;
      CompareRocs();
    }

   // Decode each ROC
   // From this point onwards there is no diff between CODA 2.* and CODA 3.*

    for( UInt_t i = 0; i < nroc; i++ ) {

      UInt_t iroc = irn[i];
      const RocDat_t& ROC = rocdat[iroc];
      UInt_t ipt = ROC.pos + 1;
      UInt_t iptmax = ROC.pos + ROC.len;

      if( fMap->isFastBus(iroc) ) {  // checking that slots found = expected
        if( GetEvNum() > 200 && chkfbstat < 3 ) chkfbstat = 2;
        if( chkfbstat == 1 ) ChkFbSlot(iroc, evbuffer, ipt, iptmax);
        if( chkfbstat == 2 ) {
          ChkFbSlots();
          chkfbstat = 3;
        }
      }

      // If at least one module is in a bank, must split the banks for this roc

      if( fMap->isBankStructure(iroc) ) {
        if( fDebugFile )
          *fDebugFile << "\nCodaDecode::Calling bank_decode "
                      << i << "   " << iroc << "  " << ipt << "  " << iptmax
                      << endl;
        /*status =*/
        bank_decode(iroc, evbuffer, ipt, iptmax);
      }

      if( fDebugFile )
        *fDebugFile << "\nCodaDecode::Calling roc_decode " << i << "   "
                    << evbuffer << "  " << iroc << "  " << ipt
                    << "  " << iptmax << endl;

      Int_t status = roc_decode(iroc, evbuffer, ipt, iptmax);

      // do something with status
      if( status )
        break;

    }
  }

  return ret;
}


//_____________________________________________________________________________
Int_t CodaDecoder::interpretCoda3(const UInt_t* evbuffer) {

  event_type = 0;
  bank_tag   = (evbuffer[1] & 0xffff0000) >> 16;
  data_type  = (evbuffer[1] & 0xff00) >> 8;
  block_size = evbuffer[1] & 0xff;

  if( bank_tag >= 0xff00 ) { /* CODA Reserved bank type */

    switch( bank_tag ) {

    case 0xffd1:
      event_type = PRESTART_EVTYPE;
      break;
    case 0xffd2:
      event_type = GO_EVTYPE;
      break;
    case 0xffd4:
      event_type = END_EVTYPE;
      break;
    case 0xff50:
    case 0xff58: // Physics event with sync bit
    case 0xff70:
      event_type = 1;  // Physics event type
      break;
    default:
      cout << "CodaDecoder:: WARNING:  Undefined CODA 3 event type" << endl;
    }
  } else { /* User event type */

    event_type = bank_tag;  // need to check this.

    if( fDebugFile )
      *fDebugFile << " User defined event type " << event_type << endl;

  }

  tbLen = 0;
  if( event_type == 1 ) tbLen = trigBankDecode(&evbuffer[2], block_size);

  return HED_OK;
}


//_____________________________________________________________________________
UInt_t CodaDecoder::trigBankDecode( const UInt_t* evbuffer, UInt_t blkSize) {

// Decode the "Trigger Bank" which is a CODA3 structure that appears
// near the top of the event buffer.  

  memset((void *)&tbank, 0, sizeof(TBOBJ));

  tbank.start = (uint32_t *)evbuffer;
  tbank.blksize = blkSize;
  tbank.len = evbuffer[0] + 1;
  tbank.tag = (evbuffer[1]&0xffff0000)>>16;
  tbank.nrocs = (evbuffer[1]&0xff);
  tbank.evtNum = evbuffer[3];

  if((tbank.tag)&1)
    tbank.withTimeStamp = 1;
  if((tbank.tag)&2)
    tbank.withRunInfo = 1;

  if(tbank.withTimeStamp) {
    tbank.evTS = (uint64_t *)&evbuffer[5];
    if(tbank.withRunInfo) {
      tbank.evType = (uint16_t *)&evbuffer[5 + 2*blkSize + 3];
    }else{
      tbank.evType = (uint16_t *)&evbuffer[5 + 2*blkSize + 1];
    }
  }else{
    tbank.evTS = nullptr;
    if(tbank.withRunInfo) {
      tbank.evType = (uint16_t *)&evbuffer[5 + 3];
    }else{
      tbank.evType = (uint16_t *)&evbuffer[5 + 1];
    }
  }

  return(tbank.len);

}

//_____________________________________________________________________________
Int_t CodaDecoder::LoadFromMultiBlock()
{
  // LoadFromMultiBlock : This assumes the data are in multiblock mode.

  // For modules that are in multiblock mode, the next event is loaded.
  // For other modules not in multiblock mode (e.g. scalers) or other data (e.g. flags)
  // the data remain "stale" until the next block of events.

  if (!fMultiBlockMode) return HED_ERR;
  fBlockIsDone = false;

  if( first_decode || fNeedInit ) {
    Int_t ret = Init();
    if( ret != HED_OK )
      return ret;
  }

  for( auto i : fSlotClear ) {
    if( crateslot[i]->GetModule()->IsMultiBlockMode() )
      crateslot[i]->clearEvent();
  }

  for( UInt_t i = 0; i < nroc; i++ ) {
    UInt_t roc = irn[i];
    for( auto slot : fMap->GetUsedSlots(roc) ) {
      assert(fMap->slotUsed(roc, slot));
      auto* sd = crateslot[idx(roc, slot)].get();
      auto* mod = sd->GetModule();
      // for CODA3, cross-check the block size (found in trigger bank and, separately, in modules)
      if( fDebugFile )
        *fDebugFile << "cross chk blk size " << roc << "  " << slot << "  "
                    << mod->GetBlockSize() << "   " << block_size << endl;
      if( fDataVersion > 2 && mod->GetBlockSize() != block_size ) {
        cerr << "ERROR::CodaDecoder:: inconsistent block size between trig. bank and module" << endl;
      }
      if( mod->IsMultiBlockMode() ) {
        sd->LoadNextEvBuffer();
        if( sd->BlockIsDone() )
          fBlockIsDone = true;
      }
    }
  }
  return HED_OK;
}

//_____________________________________________________________________________
Int_t CodaDecoder::roc_decode( UInt_t roc, const UInt_t* evbuffer,
                               UInt_t ipt, UInt_t istop )
{
  // Decode a Readout controller
  assert( evbuffer && fMap );
  if( ipt+1 >= istop )
    return HED_OK;
  if( fDoBench ) fBench->Begin("roc_decode");
  Int_t retval = HED_OK;
  try {
    UInt_t Nslot = fMap->getNslot(roc);
    if( Nslot == 0 || Nslot == kMaxUInt ) {
      // Found data for a ROC which is not defined in the crate map.
      // Just ignore it. Do debug log occurrences and warn user once about it.
      if( fDebugFile ) {
        *fDebugFile << "CodaDecode:: roc_decode:: WARNING: Undefined ROC # "
                    << dec << roc << ", event " << event_num << endl;
      }
      UInt_t ibit = roc;
      if( !fMsgPrinted.TestBitNumber(ibit) ) {
        Warning("roc_decode", "ROC %d found in data but NOT in cratemap. "
                           "Ignoring it.", roc);
        fMsgPrinted.SetBitNumber(ibit);
      }
      return HED_OK;
    }
    if( istop > event_length )
      throw invalid_argument("ERROR:: roc_decode:  stop point exceeds event length (?!)");

    if( fDebugFile )
      *fDebugFile << "CodaDecode:: roc_decode:: roc#  " << dec << roc
                  << " nslot " << Nslot << endl;

    synchmiss = false;
    synchextra = false;
    buffmode = false;
    fBlockIsDone = false;
    const UInt_t* p = evbuffer + ipt;    // Points to ROC ID word (1 before data)
    const UInt_t* pstop = evbuffer + istop;   // Points to last word of data

    assert(fMap->GetUsedSlots(roc).size() == Nslot); // else bug in THaCrateMap

    // Build the to-do list of slots based on the contents of the crate map
    vector<pair<UInt_t,THaSlotData*>> slots_todo; slots_todo.reserve(Nslot);
    for( auto slot : fMap->GetUsedSlots(roc) ) {
      assert(fMap->slotUsed(roc, slot));   // else bug in THaCrateMap
      // ignore bank structure slots; they are decoded with bank_decode
      if( fMap->getBank(roc, slot) >= 0 )
        continue;
      slots_todo.emplace_back(slot,crateslot[idx(roc,slot)].get());
    }

    // higher slot # appears first in multiblock mode
    // the decoding order improves efficiency
    bool is_fastbus = fMap->isFastBus(roc);
    if( is_fastbus )
      std::reverse( ALL(slots_todo) );

    // Crawl through this ROC's data block. Each word is tested against all the
    // defined modules (slots) in the crate for a match with the expected slot
    // header. If a match is found, this word is the slot header. Zero or more
    // words following the slot header represent the data for the slot. These
    // data are loaded into the module's internal storage, and the corresponding
    // slot is removed from the search list. The search for the remaining slots
    // then resumes at the first word after the data.
    UInt_t nextidx = 0;
    while( p++ < pstop ) {
      if( fDebugFile )
        *fDebugFile << "CodaDecode::roc_decode:: evbuff " << (p - evbuffer)
                    << "  " << hex << *p << dec << endl;

      if( is_fastbus && LoadIfFlagData(p) )
        continue;

      bool update_nextidx = true;
      for( UInt_t i = nextidx; i < slots_todo.size(); ++i ) {
        UInt_t slot = slots_todo[i].first;
        // ignore bank structure slots; they are decoded with bank_decode
        if( slot == kMaxUInt )
          continue;
        auto* sd = slots_todo[i].second;

        if( fDebugFile )
          *fDebugFile << "roc_decode:: slot logic " << roc << "  " << slot;

        // Check if data word at p belongs to the module at the current slot
        UInt_t nwords = sd->LoadIfSlot(p, pstop);

        if( sd->IsMultiBlockMode() ) fMultiBlockMode = true;
        if( sd->BlockIsDone() ) fBlockIsDone = true;

        if( fDebugFile )
          *fDebugFile << "CodaDecode:: roc_decode:: after LoadIfSlot "
                      << p + ((nwords > 0) ? nwords - 1 : 0) << "  " << pstop
                      << "    " << hex << *p << "  " << dec << nwords << endl;

        if( nwords > 0 ) {
          if( fDebugFile )
            *fDebugFile << "CodaDecode::  slot " << slot << "  is DONE    "
                        << nwords << endl;
          // Data for this slot found and loaded. Advance to next data block.
          p += nwords-1;
          slots_todo[i].first = kMaxUInt;  // Mark slot as done
          if( update_nextidx )
            nextidx = i+1;
          break;
        } else if( update_nextidx ) {
          nextidx = i;
          update_nextidx = false;
        }
      }
    } //end while(p++<pstop)
  }
  catch( const exception& e ) {
    cerr << e.what() << endl;
    retval = HED_ERR;
  }

  if( fDoBench ) fBench->Stop("roc_decode");
  return retval;
}

//_____________________________________________________________________________
Int_t CodaDecoder::bank_decode( UInt_t roc, const UInt_t* evbuffer,
                                UInt_t ipt, UInt_t istop )
{
  // Split a roc into banks, if using bank structure
  // Then loop over slots and decode it from a bank if the slot
  // belongs to a bank.
  assert( evbuffer && fMap );
  if (!fMap->isBankStructure(roc))
    return HED_OK;

  if( fDoBench ) fBench->Begin("bank_decode");
  if (fDebugFile)
    *fDebugFile << "CodaDecode:: bank_decode  ... " << roc << "   " << ipt
                << "  " << istop << endl;

  fBlockIsDone = false;
  UInt_t pos = ipt+1;  // ipt points to ROC ID word
  while (pos < istop) {
    UInt_t len = evbuffer[pos];
    UInt_t head = evbuffer[pos+1];
    UInt_t bank = (head >> 16) & 0xffff;
    if( fDebugFile )
      *fDebugFile << "bank 0x" << hex << bank << "  head 0x" << head
                  << "    len 0x" << len << dec << endl;

    if( bank < MAXBANK ) {
      bankdat[MAXBANK*roc+bank].pos = pos+2;
      bankdat[MAXBANK*roc+bank].len = len-1;
    } else {
      cerr << "CodaDecoder::ERROR:  bank number out of range " << endl;
      return HED_ERR;
    }

    pos += len+1;
  }

  for( auto slot : fMap->GetUsedSlots(roc) ) {
    assert(fMap->slotUsed(roc,slot));
    Int_t bank=fMap->getBank(roc,slot);
    if( bank < 0 ) continue;  // skip non-bank mode modules in mixed-mode crate
    if( bank >= static_cast<Int_t>(Decoder::MAXBANK) ) {
      cerr << "CodaDecoder::ERROR:  bank number out of range "<<endl;
      return HED_ERR;
    }
    pos = bankdat[MAXBANK*roc+bank].pos;
    UInt_t len = bankdat[MAXBANK*roc+bank].len;
    if (fDebugFile)
      *fDebugFile << "CodaDecode:: loading bank " << roc << "  " << slot << "   "
                  << bank << "  " << pos << "   " << len << endl;
    auto* sd = crateslot[idx(roc,slot)].get();
    sd->LoadBank(evbuffer,pos,len);
    if (sd->IsMultiBlockMode()) fMultiBlockMode = true;
    if (sd->BlockIsDone()) fBlockIsDone = true;
  }

  if( fDoBench ) fBench->Stop("bank_decode");
  return HED_OK;
}

//_____________________________________________________________________________
 void CodaDecoder::FillBankData( UInt_t *rdat, UInt_t roc, Int_t bank, UInt_t offset, UInt_t num) const
{
  UInt_t jk = MAXBANK*roc + bank;

  if (fDebug>1) cout << "Into FillBankData v1 "<<dec<<roc<<"  "<<bank<<"  "<<offset<<"  "<<num<<"   "<<jk<<endl;
  if(fDebugFile) *fDebugFile << "Check FillBankData "<<roc<<"  "<<bank<<"   "<<jk<<endl;

  if( jk >= MAXROC*MAXBANK ) {
      cerr << "FillBankData::ERROR:  bankdat index out of range "<<endl;
      return;
  }
  UInt_t pos = bankdat[jk].pos;
  UInt_t len = bankdat[jk].len;
  if (fDebug>1) cout << "FillBankData: pos "<<pos<<"  "<<len<<endl;
  if(fDebugFile) *fDebugFile << "FillBankData  pos, len "<<pos<<"   "<<len<<endl;
  if( offset > len-2 )
    return;
  if( num > len )
    num = len;
  UInt_t ilo = pos + offset;
  UInt_t ihi = pos + offset + num;
  if( ihi > GetEvLength() )
    ihi = GetEvLength();
  for( UInt_t i = ilo; i < ihi; i++ )
    rdat[i-ilo] = GetRawData(i);

}

//_____________________________________________________________________________
Int_t CodaDecoder::LoadIfFlagData(const UInt_t* evbuffer)
{
  // Need to generalize this ...  too Hall A specific
  //
  // Looks for buffer mode and synch problems.  The latter are recoverable
  // but extremely rare, so I haven't bothered to write recovery a code yet,
  // but at least this warns you.
  // Returns 0 if no flag data detected, != 0 otherwise
  assert( evbuffer );
  UInt_t word = *evbuffer;
  if (fDebugFile)
    *fDebugFile << "CodaDecode:: TestBit on :  Flag data ? "
                << hex << word << dec << endl;
  UInt_t stdslot = word >> 27;
  if( stdslot > 0 && stdslot < MAXSLOT_FB )
    return 0; // non-flag data

  Int_t ret = 1;
  UInt_t upword = word >> 16;   // upper 16 bits
  word &= 0xffff;               // lower 16 bits
  switch( upword ) {
  case 0xdc00:
    if( word == 0x00ff )        // word == 0xdc0000ff
      synchmiss = true;
    break;
  case 0xdcfe:
    synchextra = true;
    if( fDebug > 0 ) {
      UInt_t slot = word >> 11;
      UInt_t nhit = word & 0x7ff;
      cout << "CodaDecoder: WARNING: Fastbus slot ";
      cout << slot << "  has extra hits "<<nhit<<endl;
    }
    break;
  case 0xfaaa:
    break;
  case 0xfabc:
    if( fDebug > 0 && (synchmiss || synchextra) ) {
      UInt_t datascan = *(evbuffer+3);
      cout << "CodaDecoder: WARNING: Synch problems !"<<endl;
      cout << "Data scan word 0x"<<hex<<datascan<<dec<<endl;
    }
    break;
  case 0xfabb:
    buffmode = false;
    break;
  case 0xfafb:
    if( (word >> 8) == 0xbf ) { // word & 0xffffff00 == 0xfafbbf00
      buffmode = true;
      //    synchflag = word&0xff; // not used
    }
    break;
  default:
    break;
  }
  return ret;
}


//_____________________________________________________________________________
Int_t CodaDecoder::FindRocs(const UInt_t *evbuffer) {

// The (old) decoding of CODA 2.* event buffer to find pointers and lengths of ROCs
// ROC = ReadOut Controller, synonymous with "crate".

  assert( evbuffer );
#ifdef FIXME
  if( fDoBench ) fBench->Begin("physics_decode");
#endif
  Int_t status = HED_OK;

  // The following line is not meaningful for CODA3
  if( (evbuffer[1]&0xffff) != 0x10cc ) std::cout<<"Warning, header error"<<std::endl;
  if( event_type > MAX_PHYS_EVTYPE ) std::cout<<"Warning, Event type makes no sense"<<std::endl;
  for_each(ALL(rocdat), []( RocDat_t& ROC ) { ROC.clear(); });
  // Set pos to start of first ROC data bank
  UInt_t pos = evbuffer[2]+3;  // should be 7
  nroc = 0;
  while( pos+1 < event_length && nroc < MAXROC ) {
    UInt_t len  = evbuffer[pos];
    UInt_t iroc = (evbuffer[pos+1]&0xff0000)>>16;
    if( iroc>=MAXROC ) {
#ifdef FIXME
      if(fDebug>0) {
	cout << "ERROR in EvtTypeHandler::FindRocs "<<endl;
	cout << "  illegal ROC number " <<dec<<iroc<<endl;
      }
      if( fDoBench ) fBench->Stop("physics_decode");
#endif
      return HED_ERR;
    }
    // Save position and length of each found ROC data block
    rocdat[iroc].pos  = pos;
    rocdat[iroc].len  = len;
    irn[nroc++] = iroc;
    pos += len+1;
  }

  if (fDebugFile) {
    *fDebugFile << "CodaDecode:: num rocs "<<dec<<nroc<<endl;
    for( UInt_t i = 0; i < nroc; i++ ) {
      UInt_t iroc = irn[i];
      *fDebugFile << "   CodaDecode::   roc  num "<<iroc<<"   pos "<<rocdat[iroc].pos<<"     len "<<rocdat[iroc].len<<endl;
    }
  }

  return status;

}

//_____________________________________________________________________________
Int_t CodaDecoder::FindRocsCoda3(const UInt_t *evbuffer) {

// Find the pointers and lengths of ROCs in CODA3
// ROC = ReadOut Controller, synonymous with "crate".
// Earlier we had decoded the Trigger Bank in method trigBankDecode.
// This determined tbLen and the tbank structure. 
// For CODA3, the ROCs start after the Trigger Bank.

  UInt_t pos = 2 + tbLen;
  nroc=0;

  while (pos < event_length) {
    UInt_t len = (evbuffer[pos]+1);               /* total Length of ROC Bank */
    UInt_t iroc = (evbuffer[pos+1]&0x0fff0000)>>16;   /* ID of ROC is 12 bits*/
    rocdat[iroc].len = len;
    rocdat[iroc].pos = pos;
    irn[nroc] = iroc;
    pos += len;
    nroc++;
  }

  /* Sanity check:  Check if number of ROCs matches */
  if(nroc != tbank.nrocs) {
      printf(" ****ERROR: Trigger and Physics Block sizes do not match (%d != %d)\n",nroc,tbank.nrocs);
// If you are reading a data file originally written with CODA 2 and then
// processed (written out) with EVIO 4, it will segfault. Do as it says below.
      printf("This might indicate a file written with EVIO 4 that was a CODA 2 file\n");
      printf("Try  analyzer->SetCodaVersion(2)  in the analyzer script.\n");

  }

  if (fDebugFile) {  // debug

    *fDebugFile << endl << "  FindRocsCoda3 :: Starting Event number = " << dec << tbank.evtNum;
    *fDebugFile << endl;
    *fDebugFile << "    Trigger Bank Len = "<<tbLen<<" words "<<endl;
    *fDebugFile << "    There are "<<nroc<<"  ROCs"<<endl;
    for( UInt_t i = 0; i < nroc; i++ ) {
      *fDebugFile << "     ROC ID = "<<irn[i]<<"  pos = "<<rocdat[irn[i]].pos
          <<"  Len = "<<rocdat[irn[i]].len<<endl;
    }
    *fDebugFile << "    Trigger BANK INFO,  TAG = "<<hex<<tbank.tag<<dec<<endl;
    *fDebugFile << "    start "<<hex<<tbank.start<<"      blksize "<<dec<<tbank.blksize
        <<"  len "<<tbank.len<<"   tag "<<tbank.tag<<"   nrocs "<<tbank.nrocs<<"   evtNum "<<tbank.evtNum;
    *fDebugFile << endl;
    *fDebugFile << "         Event #       Time Stamp       Event Type"<<endl;
    for( UInt_t i = 0; i < tbank.blksize; i++ ) {
      if( tbank.evTS ) {
          *fDebugFile << "      "<<dec<<tbank.evtNum+i<<"   "<<tbank.evTS[i]<<"   "<<tbank.evType[i];
          *fDebugFile << endl;
       } else {
	  *fDebugFile << "     "<<tbank.evtNum+i<<"(No Time Stamp)   "<<tbank.evType[i];
	  *fDebugFile << endl;
       }
       *fDebugFile << endl<<endl;
    }
  }

  return 1;

}

//_____________________________________________________________________________
// To initialize the THaSlotData member on first call to decoder
int CodaDecoder::init_slotdata()
{
  // Initialize all slots defined in the crate map as "used".
  //
  // This defeats the clever on-demand mechanism of the original decoder,
  // which would only allocate resources for a crate/slot if data for that
  // slot were actually encountered in the data stream. Now, resources
  // are allocated for everything, whether actually active or not.
  // The advantage is that one can configure the module objects associated
  // with each crate/slot. Of course, that could be done via database
  // parameters as well. Presumably the resource waste is minor on today's
  // computers, so fine.

  if(!fMap) return HED_ERR;

  for( auto iroc : fMap->GetUsedCrates() ) {
    assert(fMap->crateUsed(iroc));
    for( auto islot : fMap->GetUsedSlots(iroc) ) {
      assert(fMap->slotUsed(iroc, islot));
      makeidx(iroc,islot);
      if (fDebugFile)
	*fDebugFile << "CodaDecode::  crate, slot " << iroc << "  " << islot
		    << "   Dev type  = " << crateslot[idx(iroc,islot)]->devType()
		    << endl;
    }
  }

  if (fDebugFile)
    *fDebugFile << "CodaDecode:: fNSlotUsed "<<fSlotUsed.size()<<endl;

  // Update lists of used/clearable slots in case crate map changed
  return THaEvData::init_slotdata();
}


//_____________________________________________________________________________
void CodaDecoder::dump(const UInt_t* evbuffer) const
{
  if( !evbuffer ) return;
  if ( !fDebugFile ) return;
  *fDebugFile << "\n\n Raw Data Dump  " << endl;
  *fDebugFile << "\n Event number  " << dec << event_num;
  *fDebugFile << "  length " << event_length << "  type " << event_type << endl;
  UInt_t ipt = 0;
  for( UInt_t j = 0; j < (event_length/5); j++ ) {
    *fDebugFile << dec << "\n evbuffer[" << ipt << "] = ";
    for( UInt_t k = j; k < j+5; k++ ) {
      *fDebugFile << hex << evbuffer[ipt++] << " ";
    }
  }
  if (ipt < event_length) {
    *fDebugFile << dec << "\n evbuffer[" << ipt << "] = ";
    for( UInt_t k = ipt; k < event_length; k++ ) {
      *fDebugFile << hex << evbuffer[ipt++] << " ";
    }
    *fDebugFile << endl;
  }
  *fDebugFile<<dec<<endl;
}

//_____________________________________________________________________________
void CodaDecoder::CompareRocs()
{
  if (!fMap || !fDebugFile) return;
  *fDebugFile<< "Comparing cratemap rocs with found rocs"<<endl;
  for( UInt_t i = 0; i < nroc; i++ ) {
    UInt_t iroc = irn[i];
    if( !fMap->crateUsed(iroc) ) {
      *fDebugFile << "ERROR  CompareRocs:: roc " << iroc << "  in data but not in map" << endl;
    }
  }
  for( UInt_t iroc1 = 0; iroc1 < MAXROC; iroc1++ ) {
    if( !fMap->crateUsed(iroc1) ) continue;
    Bool_t ifound = false;
    for( UInt_t i=0; i<nroc; i++ ) {
      UInt_t iroc2 = irn[i];
      if (iroc1 == iroc2) {
        ifound = true;
	break;
      }
    }
    if (!ifound) *fDebugFile << "ERROR: CompareRocs:  roc "<<iroc1<<" in cratemap but not found data"<<endl;
  }
}

//_____________________________________________________________________________
void CodaDecoder::ChkFbSlot( UInt_t roc, const UInt_t* evbuffer, UInt_t ipt,
                             UInt_t istop )
{
  const UInt_t* p     = evbuffer+ipt;   // Points to ROC ID word (1 before data)
  const UInt_t* pstop = evbuffer+istop; // Points to last word of data in roc
  while( p++ < pstop ) {
    UInt_t slot = *p >> 27;  // A "self-reported" slot.
    UInt_t index = MAXSLOT*roc + slot;
    if( slot > 0 && index < MAXROC*MAXSLOT )
      fbfound[index] = true;
  }
}

//_____________________________________________________________________________
void CodaDecoder::ChkFbSlots()
{
  // This checks the fastbus slots to see if slots are appearing in both the
  // data and the cratemap.  If they appear in one but not the other, a warning
  // is issued, which usually means the cratemap is wrong.
  for( UInt_t iroc = 0; iroc < MAXROC; iroc++ ) {
    if( !fMap->isFastBus(iroc) ) continue;
    for( UInt_t islot = 0; islot < MAXSLOT; islot++ ) {
      UInt_t index = MAXSLOT * iroc + islot;
      bool inEvent = fbfound[index], inMap = fMap->slotUsed(iroc, islot);
      if( inEvent ) {
        if( inMap ) {
          if (fDebugFile)
            *fDebugFile << "FB slot in cratemap and in data.  (good!).  "
                        << "roc = "<<iroc<<"   slot = "<<islot<<endl;
        } else {
          if (fDebugFile)
            *fDebugFile << "FB slot in data, but NOT in cratemap  (bad!).  "
                        << "roc = "<<iroc<<"   slot = "<<islot<<endl;
          Warning("ChkFbSlots", "Fastbus module in (roc,slot) = (%d,%d)  "
                                "found in data but NOT in cratemap !", iroc, islot);
        }
      } else if( inMap ) {
        if (fDebugFile)
          *fDebugFile << "FB slot NOT in data, but in cratemap  (bad!).  "
                      << "roc = "<<iroc<<"   slot = "<<islot<<endl;
        // Why do we care? If the cratemap has info about additional hardware
        // that just wasn't read out by the DAQ, so what?
        //Warning("ChkFbSlots", "Fastbus module in (roc,slot) = = (%d,%d)  "
        //               "found in cratemap but NOT in data !", iroc, islot);
      }
    }
  }
}

//_____________________________________________________________________________
void CodaDecoder::SetRunTime( ULong64_t tloc )
{
  // Set run time and re-initialize crate map (and possibly other
  // database parameters for the new time.

  if( fRunTime == tloc )
    return;
  fRunTime = tloc;
  fNeedInit = true;  // force re-init
}

//_____________________________________________________________________________
Int_t CodaDecoder::prescale_decode(const UInt_t* evbuffer)
{
  // Decodes prescale factors from either
  // TS_PRESCALE_EVTYPE(default) = PS factors
  // read from Trig. Super. registers (since 11/03)
  //   - or -
  // PRESCALE_EVTYPE = PS factors from traditional
  //     "prescale.dat" file.

  assert( evbuffer );
  assert( event_type == TS_PRESCALE_EVTYPE ||
	  event_type == PRESCALE_EVTYPE );
  const UInt_t HEAD_OFF1 = 2;
  const UInt_t HEAD_OFF2 = 4;
  static const char* const pstr[] = { "ps1", "ps2", "ps3", "ps4",
				      "ps5", "ps6", "ps7", "ps8",
				      "ps9", "ps10", "ps11", "ps12" };
  // TS registers -->
  if( event_type == TS_PRESCALE_EVTYPE) {
    // this is more authoritative
    for( UInt_t j = 0; j < 8; j++ ) {
      UInt_t k = j + HEAD_OFF1;
      UInt_t ps = 0;
      if( k < event_length ) {
	ps = evbuffer[k];
        if( psfact[j] != 0 && ps != psfact[j] ) {
	  Warning("prescale_decode","Mismatch in prescale factor: "
		  "Trig %u  oldps %u   TS_PRESCALE %d. Setting to TS_PRESCALE",
		  j+1,psfact[j],ps);
	}
      }
      psfact[j]=ps;
      if (fDebug > 1)
	cout << "%% TS psfact "<<dec<<j<<"  "<<psfact[j]<<endl;
    }
  }
  // "prescale.dat" -->
  else if( event_type == PRESCALE_EVTYPE ) {
    if( event_length <= HEAD_OFF2 )
      return HED_ERR;  //oops, event too short?
    THaUsrstrutils sut;
    sut.string_from_evbuffer(evbuffer+HEAD_OFF2, event_length-HEAD_OFF2);
    for(Int_t trig=0; trig<MAX_PSFACT; trig++) {
      UInt_t ps =  sut.getint(pstr[trig]);
      UInt_t psmax = 65536; // 2^16 for trig > 3
      if (trig < 4) psmax = 16777216;  // 2^24 for 1st 4 trigs
      if (trig > 7) ps = 1;  // cannot prescale trig 9-12
      ps = ps % psmax;
      if (psfact[trig]==kMaxUInt) // not read before
	psfact[trig] = ps;
      else if (ps != psfact[trig]) {
	Warning("prescale_decode","Mismatch in prescale factor: "
		"Trig %d  oldps %d   prescale.dat %d, Keeping old value",
		trig+1,psfact[trig],ps);
      }
      if (fDebug > 1)
	cout << "** psfact[ "<<trig+1<< " ] = "<<psfact[trig]<<endl;
    }
  }
  // Ok in any case
  return HED_OK;
}

//_____________________________________________________________________________
Int_t CodaDecoder::SetCodaVersion( Int_t version )
{
  if (version != 2 && version != 3) {
    cout << "ERROR::CodaDecoder::SetCodaVersion version " << version;
    cout << "  must be 2 or 3 only." << endl;
    cout << "Not setting CODA version ! " << endl;
    return -1;
  }
  return (fDataVersion = version);
}

//_____________________________________________________________________________

} // namespace Decoder

ClassImp(Decoder::CodaDecoder)
