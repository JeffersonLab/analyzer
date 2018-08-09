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

using namespace std;

namespace Decoder {

//   static const Int_t MAX_EVTYPES = 200;
//   static const Int_t MAX_PHYS_EVTYPES = 14;

//_____________________________________________________________________________
CodaDecoder::CodaDecoder() :
  nroc(0), irn(MAXROC,0), fbfound(MAXROC*MAXSLOT,false), psfact(MAX_PSFACT,-1)
{
  fCodaVersion=0;
  evcnt_coda3=0;
  fNeedInit=true;
  first_decode=kFALSE;
  fMultiBlockMode=kFALSE;
  fBlockIsDone=kFALSE;
  // Please leave these 3 lines for me to debug if I need to.  thanks, Bob
#ifdef WANTDEBUG
  fDebugFile = new ofstream();
  fDebugFile->open("bobstuff.txt");
  *fDebugFile<< "Debug my stuff "<<endl<<endl;
#endif
}

//_____________________________________________________________________________
CodaDecoder::~CodaDecoder()
{
}

//_____________________________________________________________________________
Int_t CodaDecoder::GetPrescaleFactor(Int_t trigger_type) const
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
Int_t CodaDecoder::LoadEvent(const UInt_t* evbuffer)
{
  // Main engine for decoding, called by public LoadEvent() methods

  if (fCodaVersion <= 0) {
     cout << "This makes no sense, coda version "<<fCodaVersion<<" ??  Exit !!"<<endl;
     exit(0);
  }
  event_length = evbuffer[0]+1;  // in longwords (4 bytes)
  event_num = 0;
  event_type = 0;
  static Int_t fdfirst=1;
  static Int_t chkfbstat=1;
  if (fDebugFile) *fDebugFile << "CodaDecode:: Loading event  ... "<<endl;
  if (fDebugFile) *fDebugFile << "evbuffer ptr "<<evbuffer<<endl;
  assert( evbuffer );
  assert( fMap || fNeedInit );
  Int_t ret = HED_OK;
  buffer = evbuffer;
  if (first_decode || fNeedInit) {
     ret = init_cmap();
     if (fDebugFile) {
	 *fDebugFile << "\n CodaDecode:: Print of Crate Map"<<endl;
	 fMap->print(fDebugFile);
     } else {
      //      fMap->print();
     }
     if( ret != HED_OK ) return ret;
     ret = init_slotdata();
     if( ret != HED_OK ) return ret;
     FindUsedSlots();
     first_decode=kFALSE;
  }
  if( fDoBench ) fBench->Begin("clearEvent");
  for( Int_t i=0; i<fNSlotClear; i++ ) crateslot[fSlotClear[i]]->clearEvent();
  if( fDoBench ) fBench->Stop("clearEvent");
  if (fCodaVersion == 2) {
    event_type = evbuffer[1]>>16;
    if(event_type < 0) return HED_ERR;
  } else {  // CODA version 3
    interpretCoda3(evbuffer);
  }
  if(fDebugFile) {
      *fDebugFile << "CodaDecode:: dumping "<<endl;
      dump(evbuffer);
  }
 
  if (event_type == PRESTART_EVTYPE) {
     // Usually prestart is the first 'event'.  Call SetRunTime() to
     // re-initialize the crate map since we now know the run time.
     // This won't happen for split files (no prestart). For such files,
     // the user should call SetRunTime() explicitly.
     if (fCodaVersion == 2) { // for CODA3 these are set in interpretCoda3
       SetRunTime(static_cast<ULong64_t>(evbuffer[2]));
       run_num  = evbuffer[3];
       run_type = evbuffer[4];
       evt_time = fRunTime;
     } 
  } else if( event_type == PRESCALE_EVTYPE || event_type == TS_PRESCALE_EVTYPE ) {
    ret = prescale_decode(evbuffer);
    if( ret != HED_OK )
      return ret;
  }
  if (event_type <= MAX_PHYS_EVTYPE) {
     if (fCodaVersion == 3) {
         evcnt_coda3++;
         event_num = evcnt_coda3;
         FindRocsCoda3(evbuffer);
     } else {
         event_num = evbuffer[4];
         FindRocs(evbuffer);
     }
     recent_event = event_num;

     if ((fdfirst==1) & (fDebugFile!=0)) {
       fdfirst=0;
       CompareRocs();
     }

   // Decode each ROC
   // From this point onwards there is no diff between CODA 2.* and CODA 3.*
   // This is not part of the loop above because it may exit prematurely due
   // to errors, which would leave the rocdat[] array incomplete.

    for( Int_t i=0; i<nroc; i++ ) {

      Int_t iroc = irn[i];
      const RocDat_t* proc = rocdat+iroc;
      Int_t ipt = proc->pos + 1;
      Int_t iptmax = proc->pos + proc->len;

      if (fMap->isFastBus(iroc)) {  // checking that slots found = expected
	  if (GetEvNum() > 200 && chkfbstat < 3) chkfbstat=2;
	  if (chkfbstat == 1) ChkFbSlot(iroc, evbuffer, ipt, iptmax);
	  if (chkfbstat == 2) {
	      ChkFbSlots();
	      chkfbstat = 3;
	  }
      }

      Int_t status;

 // If at least one module is in a bank, must split the banks for this roc

      if (fMap->isBankStructure(iroc)) {
	  if (fDebugFile) *fDebugFile << "\nCodaDecode::Calling bank_decode "<<i<<"   "<<iroc<<"  "<<ipt<<"  "<<iptmax<<endl;
	  /*status =*/ bank_decode(iroc,evbuffer,ipt,iptmax);
      }

      if (fDebugFile) *fDebugFile << "\nCodaDecode::Calling roc_decode "<<i<<"   "<<evbuffer<<"  "<<iroc<<"  "<<ipt<<"  "<<iptmax<<endl;

      status = roc_decode(iroc,evbuffer, ipt, iptmax);

      // do something with status
      if (status == -1) break;

    }
  }

  return ret;
}


//_____________________________________________________________________________
Int_t CodaDecoder::interpretCoda3(const UInt_t* evbuffer)
{

  event_type = 0;  
  bank_tag = ((evbuffer[1]&0xffff0000)>>16); 
  data_type = ((evbuffer[1]&0xff00)>>8);
  block_size =  evbuffer[1]&0xff;

  if((bank_tag >= 0xff00)> 0) {/* CODA Reserved bank type */

    switch (bank_tag) {

     case 0xffd1:

        event_type = PRESTART_EVTYPE;
        SetRunTime(static_cast<ULong64_t>(evbuffer[2]));
        run_num  = evbuffer[3];
        run_type = evbuffer[4];
        evt_time = fRunTime;
        if (fDebugFile) {
	  *fDebugFile << "Prestart Event : run_num "<<run_num<<"  run type "<<run_type<<"   event_type "<<event_type<<"  run time "<<fRunTime<<endl;
        }
        break;
     case 0xffd2:
  	    event_type = GO_EVTYPE;
 	    break;
     case 0xffd4:
  	    event_type = END_EVTYPE;
	    break;
     case 0xff50:
     case 0xff70:
  	    event_type=1;  // Physics event type
	    break;
      default:
	    cout << "CodaDecoder:: WARNING:  Undefined CODA 3 event type"<<endl;
    }
  }  else { /* User event type */

      event_type = bank_tag;  // need to check this.

      if(fDebugFile) *fDebugFile<<" User defined event type "<<event_type<<endl;

  }

  tbLen = 0;  
  if (event_type == 1) tbLen = trigBankDecode(&evbuffer[2], block_size);
   
  return HED_OK;
}
         

//_____________________________________________________________________________
Int_t CodaDecoder::trigBankDecode(const UInt_t* evbuffer, int blkSize) {

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
    tbank.evTS = NULL;
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
  fBlockIsDone = kFALSE;

  for( Int_t i=0; i<fNSlotClear; i++ ) {
    if (crateslot[fSlotClear[i]]->GetModule()->IsMultiBlockMode()) crateslot[fSlotClear[i]]->clearEvent();
  }

  for( Int_t i=0; i<nroc; i++ ) {

      Int_t roc = irn[i];
      Int_t minslot = fMap->getMinSlot(roc);
      Int_t maxslot = fMap->getMaxSlot(roc);
      for (Int_t slot = minslot; slot <= maxslot; slot++) {
 // for CODA3, cross-check the block size (found in trigger bank and, separately, in modules)
        if(fDebugFile) *fDebugFile << "cross chk blk size "<<roc<<"  "<<slot<<"  "<<crateslot[idx(roc,slot)]->GetModule()->GetBlockSize()<<"   "<<block_size<<endl;;
        if (fCodaVersion > 2 && (crateslot[idx(roc,slot)]->GetModule()->GetBlockSize() != block_size)) {
	  cout << "ERROR::CodaDecoder:: inconsistent block size between trig. bank and module"<<endl;
	}
	if (fMap->slotUsed(roc,slot) && crateslot[idx(roc,slot)]->GetModule()->IsMultiBlockMode()) {
	  crateslot[idx(roc,slot)]->LoadNextEvBuffer();
	  if (crateslot[idx(roc,slot)]->BlockIsDone()) fBlockIsDone = kTRUE;
	}
      }
  }
  return HED_OK;
}



//_____________________________________________________________________________
Int_t CodaDecoder::roc_decode( Int_t roc, const UInt_t* evbuffer,
				  Int_t ipt, Int_t istop )
{
  // Decode a Readout controller
  assert( evbuffer && fMap );
  if( fDoBench ) fBench->Begin("roc_decode");
  Int_t slot;
  Int_t Nslot = fMap->getNslot(roc);
  Int_t minslot = fMap->getMinSlot(roc);
  Int_t maxslot = fMap->getMaxSlot(roc);
  Int_t retval = HED_OK;
  Int_t nwords;
  synchmiss = false;
  synchextra = false;
  buffmode = false;
  const UInt_t* p      = evbuffer+ipt;    // Points to ROC ID word (1 before data)
  const UInt_t* pstop  =evbuffer+istop;   // Points to last word of data
  fBlockIsDone = kFALSE;

  Int_t firstslot, incrslot;
  Int_t n_slots_checked, n_slots_done;

  Bool_t slotdone;

  Int_t status = SD_ERR;

  n_slots_done = 0;

  if (istop > event_length) {
    cerr << "ERROR:: roc_decode:  stop point exceeds event length (?!)"<<endl;
    goto err;
  }

  if (fMap->isFastBus(roc)) {  // higher slot # appears first in multiblock mode
      firstslot=maxslot;       // the decoding order improves efficiency
      incrslot = -1;
  } else {
      firstslot=minslot;
      incrslot = 1;
  }

  if (fDebugFile) {
    *fDebugFile << "CodaDecode:: roc_decode:: roc#  "<<dec<<roc<<" nslot "<<Nslot<<endl;
    *fDebugFile << "CodaDecode:: roc_decode:: firstslot "<<dec<<firstslot<<"  incrslot "<<incrslot<<endl;
  }

  if (Nslot <= 0) goto err;
  fMap->setSlotDone();      // clears the "done" bits

  while ( p++ < pstop && n_slots_done < Nslot ) {

    if (fDebugFile) {
	 *fDebugFile << "CodaDecode::roc_decode:: evbuff "<<(p-evbuffer)<<"  "<<hex<<*p<<dec<<endl;
	 *fDebugFile << "CodaDecode::roc_decode:: n_slots_done "<<n_slots_done<<"  "<<firstslot<<endl;
    }

    LoadIfFlagData(p);

    n_slots_checked = 0;
    slot = firstslot;
    slotdone = kFALSE;
// bank structure is decoded with bank_decode
    if (fMap->getBank(roc,slot) >= 0) {
      n_slots_done++;
      slotdone=kTRUE;
    }

    while(!slotdone && n_slots_checked < Nslot-n_slots_done && slot >= 0 && slot < MAXSLOT) {


      if (!fMap->slotUsed(roc,slot) || fMap->slotDone(slot)) {
	 slot = slot + incrslot;
	 continue;
      }

      ++n_slots_checked;

     if (fDebugFile) {
       *fDebugFile<< "roc_decode:: slot logic "<<roc<<"  "<<slot<<"  "<<firstslot<<"  "<<n_slots_checked<<"  "<<Nslot-n_slots_done<<endl;
     }

      nwords = crateslot[idx(roc,slot)]->LoadIfSlot(p, pstop);

      if (nwords > 0) {
	   p = p + nwords - 1;
	   fMap->setSlotDone(slot);
	   n_slots_done++;
	   if(fDebugFile) *fDebugFile << "CodaDecode::  slot "<<slot<<"  is DONE    "<<nwords<<endl;
	   slotdone = kTRUE;
      }

      if (crateslot[idx(roc,slot)]->IsMultiBlockMode()) fMultiBlockMode = kTRUE;
      if (crateslot[idx(roc,slot)]->BlockIsDone()) fBlockIsDone = kTRUE;

      if (fDebugFile) {
	  *fDebugFile<< "CodaDecode:: roc_decode:: after LoadIfSlot "<<p << "  "<<pstop<<"  "<<"  "<<hex<<*p<<"  "<<dec<<nwords<<endl;
      }

      slot = slot + incrslot;

    }

  } //end while(p++<pstop)
  goto exit;

 err:
  retval = (status == SD_ERR) ? HED_ERR : HED_WARN;
 exit:
  if( fDoBench ) fBench->Stop("roc_decode");
  return retval;
}

//_____________________________________________________________________________
Int_t CodaDecoder::bank_decode( Int_t roc, const UInt_t* evbuffer,
				  Int_t ipt, Int_t istop )
{
  // Split a roc into banks, if using bank structure
  // Then loop over slots and decode it from a bank if the slot
  // belongs to a bank.
  assert( evbuffer && fMap );
  if( fDoBench ) fBench->Begin("bank_decode");
  Int_t retval = HED_OK;
  if (!fMap->isBankStructure(roc)) return retval;
  fBlockIsDone = kFALSE;

  Int_t pos,len,bank,head;

  if (fDebugFile) *fDebugFile << "CodaDecode:: bank_decode  ... "<<roc<<"   "<<ipt<<"  "<<istop<<endl;

  pos = ipt+1;  // ipt points to ROC ID word

  while (pos < istop) {
    len = evbuffer[pos];
    head = evbuffer[pos+1];
    bank = (head>>16)&0xffff;
    if (fDebugFile) *fDebugFile << "bank 0x"<<hex<<bank<<"  head 0x"<<head<<"    len 0x"<<len<<dec<<endl;

    if (bank >= 0 && bank < MAXBANK) {
      bankdat[MAXBANK*roc+bank].pos=pos+2;
      bankdat[MAXBANK*roc+bank].len=len-1;
    }

    pos += len+1;

  }

  Int_t minslot = fMap->getMinSlot(roc);
  Int_t maxslot = fMap->getMaxSlot(roc);

  for (Int_t slot = minslot; slot <= maxslot; slot++) {
    if (!fMap->slotUsed(roc,slot)) continue;
    bank=fMap->getBank(roc,slot);
    if (bank < 0 || bank >= Decoder::MAXBANK) {
      cerr << "CodaDecoder::ERROR:  bank number out of range "<<endl;
      return 0;
    }
    pos = bankdat[MAXBANK*roc+bank].pos;
    len = bankdat[MAXBANK*roc+bank].len;
    if (fDebugFile) *fDebugFile << "CodaDecode:: loading bank "<<roc<<"  "<<slot<<"   "<<bank<<"  "<<pos<<"   "<<len<<endl;
    crateslot[idx(roc,slot)]->LoadBank(evbuffer,pos,len);
    if (crateslot[idx(roc,slot)]->IsMultiBlockMode()) fMultiBlockMode = kTRUE;
    if (crateslot[idx(roc,slot)]->BlockIsDone()) fBlockIsDone = kTRUE;
  }

  if( fDoBench ) fBench->Stop("bank_decode");
  return retval;
}

//_____________________________________________________________________________
 void CodaDecoder::FillBankData(UInt_t *rdat, Int_t roc, Int_t bank, Int_t offset, Int_t num) const
{
  Int_t ldebug=0;
  Int_t pos,len,i,ilo,ihi,jk;
  jk =  MAXBANK*roc + bank;

  if (ldebug) cout << "Into FillBankData v1 "<<dec<<roc<<"  "<<bank<<"  "<<offset<<"  "<<num<<"   "<<jk<<endl;  
  if(fDebugFile) *fDebugFile << "Check FillBankData "<<roc<<"  "<<bank<<"   "<<jk<<endl;

 if (jk < 0 || jk >= MAXROC*MAXBANK) {
      cerr << "FillBankData::ERROR:  bankdat index out of range "<<endl;
      return;
  }  
  pos = bankdat[jk].pos;
  len = bankdat[jk].len;
  if (ldebug) cout << "FillBankData: pos "<<pos<<"  "<<len<<endl;
  if(fDebugFile) *fDebugFile << "FillBankData  pos, len "<<pos<<"   "<<len<<endl;
  if (offset > len-2) return;
  if (num > len) num = len;
  ilo = pos+offset;
  ihi = pos+offset+num;
  if (ihi >  GetEvLength()) ihi=GetEvLength();
  for (i=ilo; i<ihi; i++) rdat[i-ilo] = GetRawData(i);
 
  return;
}

//_____________________________________________________________________________
Int_t CodaDecoder::LoadIfFlagData(const UInt_t* evbuffer)
{
  // Need to generalize this ...  too Hall A specific
  //
  // Looks for buffer mode and synch problems.  The latter are recoverable
  // but extremely rare, so I haven't bothered to write recovery a code yet,
  // but at least this warns you.
  assert( evbuffer );
  UInt_t word   = *evbuffer;
  UInt_t upword = word & 0xffff0000;
  if (fDebugFile) *fDebugFile << "CodaDecode:: TestBit on :  Flag data ? "<<hex<<word<<dec<<endl;
  if( word == 0xdc0000ff) synchmiss = true;
  if( upword == 0xdcfe0000) {
    synchextra = true;
    Int_t slot = (word&0xf800)>>11;
    Int_t nhit = (word&0x7ff);
    if(fDebug>0) {
      cout << "THaEvData: WARNING: Fastbus slot ";
      cout << slot << "  has extra hits "<<nhit<<endl;
    }
  }
  if( upword == 0xfabc0000) {
    Int_t datascan = *(evbuffer+3);
    if(fDebug>0 && (synchmiss || synchextra)) {
      cout << "THaEvData: WARNING: Synch problems !"<<endl;
      cout << "Data scan word 0x"<<hex<<datascan<<dec<<endl;
    }
  }
  if( upword == 0xfabb0000) buffmode = false;
  if((word&0xffffff00) == 0xfafbbf00) {
    buffmode = true;
    //    synchflag = word&0xff; // not used
  }
  return HED_OK;
}


//_____________________________________________________________________________
Int_t CodaDecoder::FindRocs(const UInt_t *evbuffer) {

// The (old) decoding of CODA 2.* event buffer to find pointers and lengths of ROCs
// ROC = ReadOut Controller, synonymous with "crate".

  assert( evbuffer && fMap );
#ifdef FIXME
  if( fDoBench ) fBench->Begin("physics_decode");
#endif
  Int_t status = HED_OK;

  // The following line is not meaningful for CODA3
  if( (evbuffer[1]&0xffff) != 0x10cc ) std::cout<<"Warning, header error"<<std::endl;
  if( event_type > MAX_PHYS_EVTYPE ) std::cout<<"Warning, Event type makes no sense"<<std::endl;
  memset(rocdat,0,MAXROC*sizeof(RocDat_t));
  // Set pos to start of first ROC data bank
  Int_t pos = evbuffer[2]+3;  // should be 7
  nroc = 0;
  while( pos+1 < event_length && nroc < MAXROC ) {
    Int_t len  = evbuffer[pos];
    Int_t iroc = (evbuffer[pos+1]&0xff0000)>>16;
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
    for (Int_t i=0; i < nroc; i++) {
      Int_t iroc=irn[i];
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
  
  Int_t pos = 2 + tbLen;
  nroc=0;

  while (pos < event_length) {
    Int_t len = (evbuffer[pos]+1);               /* total Length of ROC Bank */
    Int_t iroc = (evbuffer[pos+1]&0xffff0000)>>16;   /* ID of ROC */
    rocdat[iroc].len = len;
    rocdat[iroc].pos = pos;
    irn[nroc] = iroc;
    pos += len;
    nroc++;
  }

  /* Sanity check:  Check if number of ROCs matches */
  if(nroc != tbank.nrocs) {
      printf(" ****ERROR: Trigger and Physics Block sizes do not match (%d != %d)\n",nroc,tbank.nrocs);
// If you are reading a data file orignally written with CODA 2 and then
// processed (written out) with EVIO 4, it will segfault. Do as it says below.
      printf("This might indicate a file written with EVIO 4 that was a CODA 2 file\n");
      printf("Try  analyzer->SetCodaVersion(2)  in the analyzer script.\n");

  }

  if (fDebugFile) {  // debug   

    *fDebugFile << "\n  FindRocsCoda3 :: Starting Event number = "<<dec<<tbank.evtNum<<endl;
    *fDebugFile << "    Trigger Bank Len = "<<tbLen<<" words "<<endl;
    *fDebugFile << "    There are "<<nroc<<"  ROCs"<<endl;
    for(Int_t i=0;i<nroc;i++) {
      *fDebugFile << "     ROC ID = "<<irn[i]<<"  pos = "<<rocdat[irn[i]].pos<<"  Len = "<<rocdat[irn[i]].len<<endl;
    }
    *fDebugFile << "    Trigger BANK INFO,  TAG = "<<hex<<tbank.tag<<dec<<endl;
    *fDebugFile << "    start "<<hex<<tbank.start<<"      blksize "<<dec<<tbank.blksize<<"  len "<<tbank.len<<"   tag "<<tbank.tag<<"   nrocs "<<tbank.nrocs<<"   evtNum "<<tbank.evtNum<<endl;
    *fDebugFile << "         Event #       Time Stamp       Event Type"<<endl;
    for(Int_t i=0; i<tbank.blksize; i++) {
       if(tbank.evTS != NULL) {
          *fDebugFile << "      "<<dec<<tbank.evtNum+i<<"   "<<tbank.evTS[i]<<"   "<<tbank.evType[i]<<endl;
       } else {
	  *fDebugFile << "     "<<tbank.evtNum+i<<"(No Time Stamp)   "<<tbank.evType[i]<<endl;
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
  // Also note that all crateslots and modules will be deleted and recreated
  // right away every time this routine is called, even if there are no
  // changes to the map. This should be fixed.

  if(!fMap) return HED_ERR;

  for (Int_t iroc = 0; iroc<MAXROC; iroc++) {
    if( !fMap->crateUsed(iroc) ) continue;
    for (Int_t islot=0; islot < MAXSLOT; islot++) {
      if( !fMap->slotUsed(iroc,islot) ) continue;
      makeidx(iroc,islot);
      if (fDebugFile)
	*fDebugFile << "CodaDecode::  crate, slot " << iroc << "  " << islot
		    << "   Dev type  = " << crateslot[idx(iroc,islot)]->devType()
		    << endl;
    }
  }

  if (fDebugFile) *fDebugFile << "CodaDecode:: fNSlotUsed "<<fNSlotUsed<<endl;

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
  Int_t ipt = 0;
  for (Int_t j=0; j<(event_length/5); j++) {
    *fDebugFile << dec << "\n evbuffer[" << ipt << "] = ";
    for (Int_t k=j; k<j+5; k++) {
      *fDebugFile << hex << evbuffer[ipt++] << " ";
    }
  }
  if (ipt < event_length) {
    *fDebugFile << dec << "\n evbuffer[" << ipt << "] = ";
    for (Int_t k=ipt; k<event_length; k++) {
      *fDebugFile << hex << evbuffer[ipt++] << " ";
    }
    *fDebugFile << endl;
  }
  *fDebugFile<<dec<<endl;
}

//_____________________________________________________________________________
void CodaDecoder::CompareRocs(  )
{
  if (!fMap || !fDebugFile) return;
  *fDebugFile<< "Comparing cratemap rocs with found rocs"<<endl;
  for (Int_t i=0; i<nroc; i++) {
    Int_t iroc = irn[i];
    if (!fMap->crateUsed(iroc)) {
	  *fDebugFile << "ERROR  CompareRocs:: roc "<<iroc<<"  in data but not in map"<<endl;
    }
  }
  for (Int_t iroc1 = 0; iroc1<MAXROC; iroc1++) {
    if (  !fMap->crateUsed(iroc1)  ) continue;
    Int_t ifound=0;
    for( Int_t i=0; i<nroc; i++ ) {
      Int_t iroc2 = irn[i];
      if (iroc1 == iroc2) {
	ifound=1;
	break;
      }
    }
    if (!ifound) *fDebugFile << "ERROR: CompareRocs:  roc "<<iroc1<<" in cratemap but not found data"<<endl;
  }
}

//_____________________________________________________________________________
void CodaDecoder::ChkFbSlot( Int_t roc, const UInt_t* evbuffer,
				  Int_t ipt, Int_t istop )
{
  const UInt_t* p      = evbuffer+ipt;    // Points to ROC ID word (1 before data)
  const UInt_t* pstop  =evbuffer+istop;   // Points to last word of data in roc
  while (p++ < pstop) {
    Int_t slot = (UInt_t(*p))>>27;  // A "self-reported" slot.
    Int_t index = MAXSLOT*roc + slot;
    if ((slot > 0) && (index >=0 && index < MAXROC*MAXSLOT)) fbfound[index]=true;
  }
}

//_____________________________________________________________________________
void CodaDecoder::ChkFbSlots()
{
  // This checks the fastbus slots to see if slots are appearing in both the
  // data and the cratemap.  If they appear in one but not the other, a warning
  // is issued, which usually means the cratemap is wrong.
  Int_t slotstat[MAXROC*MAXSLOT];
  for (Int_t iroc=0; iroc<MAXROC; iroc++) {
    if ( !fMap->isFastBus(iroc) ) continue;
    for (Int_t islot=0; islot<MAXSLOT; islot++) {
      Int_t index = MAXSLOT*iroc + islot;
      slotstat[index]=0;
      if (fbfound[index] && fMap->slotUsed(iroc, islot)) {
	  if (fDebugFile) *fDebugFile << "FB slot in cratemap and in data.  (good!).  roc = "<<iroc<<"   slot = "<<islot<<endl;
	  slotstat[index]=1;
      }
      if ( !fbfound[index] && fMap->slotUsed(iroc, islot)) {
	if (fDebugFile) *fDebugFile << "FB slot NOT in data, but in cratemap  (bad!).  roc = "<<iroc<<"   slot = "<<islot<<endl;
	slotstat[index]=2;
      }
      if ( fbfound[index] && !fMap->slotUsed(iroc, islot)) {
	if (fDebugFile) *fDebugFile << "FB slot in data, but NOT in cratemap  (bad!).  roc = "<<iroc<<"   slot = "<<islot<<endl;
	slotstat[index]=3;
      }
    }
  }
  // Why do we care? If the cratemap has info about additional hardware
  // that just wasn't read out by the DAQ, so what?
  // for (Int_t iroc=0; iroc<MAXROC; iroc++) {
  //   if ( !fMap->isFastBus(iroc) ) continue;
  //   for (Int_t islot=0; islot<MAXSLOT; islot++) {
  //     Int_t index = MAXSLOT*iroc + islot;
  //     if (slotstat[index]==2) cout << "Decoder:: WARNING:  Fastbus module in (roc,slot) = ("<<iroc<<","<<islot<<")  found in cratemap but NOT in data !"<<endl;
  //   }
  // }
  for (Int_t iroc=0; iroc<MAXROC; iroc++) {
    if ( !fMap->isFastBus(iroc) ) continue;
    for (Int_t islot=0; islot<MAXSLOT; islot++) {
      Int_t index = MAXSLOT*iroc + islot;
      if (slotstat[index]==3) cout << "Decoder:: WARNING:  Fastbus module in (roc,slot) = ("<<iroc<<","<<islot<<")  found in data but NOT in cratemap !"<<endl;
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
  // read from Trig. Super. registors (since 11/03)
  //   - or -
  // PRESCALE_EVTYPE = PS factors from traditional
  //     "prescale.dat" file.

  assert( evbuffer );
  assert( event_type == TS_PRESCALE_EVTYPE ||
	  event_type == PRESCALE_EVTYPE );
  const Int_t HEAD_OFF1 = 2;
  const Int_t HEAD_OFF2 = 4;
  static const char* const pstr[] = { "ps1", "ps2", "ps3", "ps4",
				      "ps5", "ps6", "ps7", "ps8",
				      "ps9", "ps10", "ps11", "ps12" };
  // TS registers -->
  if( event_type == TS_PRESCALE_EVTYPE) {
    // this is more authoritative
    for (Int_t j = 0; j < 8; j++) {
      Int_t k = j + HEAD_OFF1;
      Int_t ps = 0;
      if (k < event_length) {
	ps = evbuffer[k];
	if (psfact[j]!=0 && ps != psfact[j]) {
	  Warning("prescale_decode","Mismatch in prescale factor: "
		  "Trig %d  oldps %d   TS_PRESCALE %d. Setting to TS_PRESCALE",
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
      Int_t ps =  sut.getint(pstr[trig]);
      Int_t psmax = 65536; // 2^16 for trig > 3
      if (trig < 4) psmax = 16777216;  // 2^24 for 1st 4 trigs
      if (trig > 7) ps = 1;  // cannot prescale trig 9-12
      ps = ps % psmax;
      if (psfact[trig]==-1) // not read before
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

} // namespace Decoder

ClassImp(Decoder::CodaDecoder)
