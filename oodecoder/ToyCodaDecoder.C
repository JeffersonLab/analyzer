//////////////////////////////////////////////////////////////////
//
//   ToyCodaDecoder
//
//   A toy model of an improved THaCodaDecoder.  
//   Work in progress towards OO decoder upgrade.
//   Dec 2013.  Bob Michaels
//
/////////////////////////////////////////////////////////////////////

#include "ToyCodaDecoder.h"
#include "THaEvData.h"
#include "THaCrateMap.h"
#include "THaEpics.h"
#include "THaUsrstrutils.h"
#include "THaBenchmark.h"
#include "ToyModule.h"
#include "TError.h"
#include <cstring>
#include <cstdio>
#include <cctype>
#include <iostream>
#include <iomanip>
#include <ctime>

#ifndef STANDALONE
#include "THaVarList.h"
#include "THaGlobals.h"
#endif

using namespace std;

//_____________________________________________________________________________
ToyCodaDecoder::ToyCodaDecoder() 
{
  irn = new Int_t[MAXROC];  
  fbfound = new Int_t[MAXROC*MAXSLOT];
  memset(irn, 0, MAXROC*sizeof(Int_t));
  memset(fbfound, 0, MAXROC*MAXSLOT*sizeof(Int_t));
}

//_____________________________________________________________________________
ToyCodaDecoder::~ToyCodaDecoder()
{
  delete [] irn;
  delete [] fbfound;
}

//_____________________________________________________________________________
Int_t ToyCodaDecoder::GetPrescaleFactor(Int_t trigger_type) const
{// PUBLIC

  // To get the prescale factors for trigger number "trigger_type"
  // (valid types are 1,2,3...)
  //  if (fgPsFact) return fgPsFact->GetPrescaleFactor(trigger_type);
  return 0;
}

//_____________________________________________________________________________
Int_t ToyCodaDecoder::LoadEvent(const Int_t* evbuffer)
{//PUBLIC
  // Main engine for decoding, called by public LoadEvent() methods
  // The crate map argument is ignored. Use SetCrateMapName instead
 static Int_t fdfirst=1; 
 static Int_t chkfbstat=1;
 if (fDebugFile) *fDebugFile << "Loading event in ToyCodaDecoder ... "<<endl;
  assert( evbuffer );
  assert( fMap || fgNeedInit );
  Int_t ret = HED_OK;
  buffer = evbuffer;
  //  if(TestBit(kDebug)) dump(evbuffer);
  if (first_decode || fgNeedInit) {
    ret = init_cmap();
    if (fDebugFile) {
         *fDebugFile << "Print of Crate Map"<<endl;
         fMap->print(fDebugFile);
    } else {
      fMap->print();
    }
    if( ret != HED_OK ) return ret;
    ret = init_slotdata(fMap);
    if( ret != HED_OK ) return ret;
    if(first_decode) {
      first_decode=kFALSE;
    }
  }
  if( fDoBench ) fBench->Begin("clearEvent");
  for( Int_t i=0; i<fNSlotClear; i++ ) crateslot[fSlotClear[i]]->clearEvent();
  if( fDoBench ) fBench->Stop("clearEvent");
  event_length = evbuffer[0]+1;  // in longwords (4 bytes)
  event_type = evbuffer[1]>>16;
  if(event_type <= 0) return HED_ERR;
 
  if (event_type <= MAX_PHYS_EVTYPE) {
     event_num = evbuffer[4];
     recent_event = event_num;
     FindRocs(evbuffer);
     if ((fdfirst==1) & (fDebugFile>0)) {
       fdfirst=0;
       CompareRocs();
     }

   // Decode each ROC
   // This is not part of the loop above because it may exit prematurely due 
   // to errors, which would leave the rocdat[] array incomplete.

    for( Int_t i=0; i<nroc; i++ ) {
 
      Int_t iroc = irn[i];
      const RocDat_t* proc = rocdat+iroc;
      Int_t ipt = proc->pos + 1;
      Int_t iptmax = proc->pos + proc->len;
      if (fDebugFile) *fDebugFile << "\nCalling roc_decode "<<i<<"   "<<iroc<<"  "<<ipt<<"  "<<iptmax<<endl;
      if (fMap->isFastBus(iroc)) {  // checking that slots found = expected
          if (GetEvNum() > 200 && chkfbstat < 3) chkfbstat=2;
          if (chkfbstat == 1) ChkFbSlot(iroc, evbuffer, ipt, iptmax);
          if (chkfbstat == 2) {
              ChkFbSlots();
              chkfbstat = 3;
          }
      }

      Int_t status = roc_decode(iroc,evbuffer, ipt, iptmax);

      // do something with status
      if (status == -1) break;

    }
  }

  return ret;
}

//_____________________________________________________________________________
Int_t ToyCodaDecoder::roc_decode( Int_t roc, const Int_t* evbuffer,
				  Int_t ipt, Int_t istop )
{
  // Decode a Readout controller
  assert( evbuffer && fMap );
  if( fDoBench ) fBench->Begin("roc_decode");
  Int_t slot;
  Int_t Nslot = fMap->getNslot(roc); //FIXME: use this for crude cross-check
  Int_t retval = HED_OK;
  const Int_t* p      = evbuffer+ipt;    // Points to ROC ID word (1 before data)
  const Int_t* pstop  =evbuffer+istop;  // Points to last word of data
  //FIXME: should never check against event_length since data cannot overrun pstop!
  const Int_t* pevlen = evbuffer+event_length;
  Int_t first_slot_used = 0, n_slots_done = 0;
  Bool_t find_first_used = true;
  Int_t status = SD_ERR;

  if (TestBit(kDebug)) {
    if (fDebugFile) *fDebugFile << "Decode roc#  "<<dec<<roc<<" nslot "<<Nslot<<endl;
  }

  if (Nslot <= 0) goto err;
  fMap->setSlotDone();

  while ( p++ < pstop && n_slots_done < Nslot ) {

    if(TestBit(kDebug)) {
      if (fDebugFile) *fDebugFile << "evbuff "<<(p-evbuffer)<<"  "<<hex<<*p<<dec<<endl;
    }
    LoadIfFlagData(p); 

    // look through all slots, since Nslot only gives number of occupied slots,
    // not the highest-numbered occupied slot.
    // Note- the way the DAQ is set up, each module's VME header word contains 
    // the slot number, so we can find the right module unambiguously 

    Int_t n_slots_checked = 0;
    for (slot=first_slot_used; n_slots_checked<Nslot-n_slots_done 
	   && slot<MAXSLOT; slot++) {
      if (!fMap->slotUsed(roc,slot)) continue; 
      if (fMap->slotDone(slot)) continue;
      if (find_first_used) {
	first_slot_used = slot;
	find_first_used = false;
      }
      ++n_slots_checked;

      if (fDebugFile) {
        *fDebugFile<< "slot logic "<<first_slot_used << "  "<<n_slots_checked <<  "   "<<slot<<endl;
      }

      status = crateslot[idx(roc,slot)]->LoadIfSlot(p, pstop); 
      if (status == kTRUE) fMap->setSlotDone(slot);

      if (fDebugFile) {
        *fDebugFile<< "after LoadIfSlot "<<p << "  "<<pstop<<endl;
      }

      if (p >= pstop || p >= pevlen) goto SlotDone;

    }

    
  SlotDone:
    if (TestBit(kDebug)) {
        if (fDebugFile) *fDebugFile <<"slot done"<<endl;
    }
    if( slot == first_slot_used ) {
      ++first_slot_used;
      find_first_used = true;
    }
    ++n_slots_done;
  } //end while(p++<pstop)
  goto exit;

 err:
  retval = (status == SD_ERR) ? HED_ERR : HED_WARN;
 exit:
  if( fDoBench ) fBench->Stop("roc_decode");
  return retval;
}


//_____________________________________________________________________________
Int_t ToyCodaDecoder::LoadIfFlagData(const Int_t* evbuffer)
{
  // Need to generalize this ...  too Hall A specific
  //
  // Looks for buffer mode and synch problems.  The latter are recoverable
  // but extremely rare, so I haven't bothered to write recovery a code yet, 
  // but at least this warns you. 
  assert( evbuffer );
  UInt_t word   = *evbuffer;
  UInt_t upword = word & 0xffff0000;
  if (TestBit(kDebug)) {
    if (fDebugFile) *fDebugFile << "TestBit on :  Flag data ? "<<hex<<word<<dec<<endl;
  }
  if( word == 0xdc0000ff) synchmiss = true;
  if( upword == 0xdcfe0000) {
    synchextra = true;
    Int_t slot = (word&0xf800)>>11;
    Int_t nhit = (word&0x7ff);
    if(TestBit(kVerbose)) {
      cout << "THaEvData: WARNING: Fastbus slot ";
      cout << slot << "  has extra hits "<<nhit<<endl;
    }
  }
  if( upword == 0xfabc0000) {
    datascan = *(evbuffer+3);
    if(TestBit(kVerbose) && (synchmiss || synchextra)) {
      cout << "THaEvData: WARNING: Synch problems !"<<endl;
      cout << "Data scan word 0x"<<hex<<datascan<<dec<<endl;
    }
  }
  if( upword == 0xfabb0000) buffmode = false;
  if((word&0xffffff00) == 0xfafbbf00) {
    buffmode = true;
    synchflag = word&0xff;
  }
  return HED_OK;
}



Int_t ToyCodaDecoder::FindRocs(const Int_t *evbuffer) {
  
  assert( evbuffer && fMap );
#ifdef FIXME
  if( fDoBench ) fBench->Begin("physics_decode");
#endif
  Int_t status = HED_OK;

  if( (evbuffer[1]&0xffff) != 0x10cc ) std::cout<<"Warning, header error"<<std::endl;
  if( (evbuffer[1]>>16) > MAX_PHYS_EVTYPE ) std::cout<<"Warning, Event type makes no sense"<<std::endl;
  memset(rocdat,0,MAXROC*sizeof(RocDat_t));
  // Set pos to start of first ROC data bank
  Int_t pos = evbuffer[2]+3;  // should be 7
  nroc = 0;
  while( pos+1 < evbuffer[0]+1 && nroc < MAXROC ) {
    Int_t len  = evbuffer[pos];
    Int_t iroc = (evbuffer[pos+1]&0xff0000)>>16;
    if( iroc>=MAXROC ) {
#ifdef FIXME
      if(TestBit(kVerbose)) { 
	cout << "ERROR in EvtTypeHandler::FindRocs "<<endl;
	cout << "  illegal ROC number " <<dec<<iroc<<endl;
      }
      if( fDoBench ) fBench->Stop("physics_decode");
#endif
      return HED_ERR;
    }
    // Save position and length of each found ROC data block
    // Save position and length of each found ROC data block
    rocdat[iroc].pos  = pos;
    rocdat[iroc].len  = len;
    irn[nroc++] = iroc;
    pos += len+1;
  }

  if (fDebugFile) {
    *fDebugFile << "num rocs "<<dec<<nroc<<endl;
    for (Int_t i=0; i < nroc; i++) {
      Int_t iroc=irn[i];
      *fDebugFile << "   roc  num "<<iroc<<"   pos "<<rocdat[iroc].pos<<"     len "<<rocdat[iroc].len<<endl;
    }
  }

  return status;

}


// To initialize the THaSlotData member on first call to decoder
int ToyCodaDecoder::init_slotdata(const THaCrateMap* map)
{
  // Update lists of used/clearable slots in case crate map changed
  if (fDebugFile) {
    *fDebugFile << "into TOY DECODER init slotdata "<<map<<"   "<<fNSlotUsed<<endl;
  }
  if(!map) return HED_ERR;

  for (Int_t iroc = 0; iroc<MAXROC; iroc++) {
    if (  !map->crateUsed(iroc)  ) continue;

    for (Int_t islot=0; islot < MAXSLOT; islot++) {

      if ( !map->slotUsed(iroc,islot) ) continue;

        makeidx(iroc,islot);
        if (fDebugFile) {
           *fDebugFile << "Num slots defined "<<GetNslots()<<endl;
	}
    }
  }

  if (fDebugFile) *fDebugFile << "fNSlotUsed "<<fNSlotUsed<<endl;

  for( int i=0; i<fNSlotUsed; i++ ) {
    THaSlotData* crslot = crateslot[fSlotUsed[i]];
    int crate = crslot->getCrate();
    int slot  = crslot->getSlot();
    if (fDebugFile) crslot->print();
    crslot->loadModule(map);
    if (fDebugFile) *fDebugFile << "Dev type  = "<<crslot->devType()<<endl;
    if( !map->crateUsed(crate) || !map->slotUsed(crate,slot) ||
	!map->slotClear(crate,slot)) {
      for( int k=0; k<fNSlotClear; k++ ) {
	if( crslot == crateslot[fSlotClear[k]] ) {
	  for( int j=k+1; j<fNSlotClear; j++ )
	    fSlotClear[j-1] = fSlotClear[j];
	  fNSlotClear--;
	  break;
	}
      }
    }
    if( !map->crateUsed(crate) || !map->slotUsed(crate,slot)) {
      for( int j=i+1; j<fNSlotUsed; j++ )
	fSlotUsed[j-1] = fSlotUsed[j];
      fNSlotUsed--;
    }
  }


  return HED_OK;

}


//_____________________________________________________________________________
void ToyCodaDecoder::dump(const Int_t* evbuffer)
{
  if( !evbuffer ) return;
  Int_t len = evbuffer[0]+1;  
  Int_t type = evbuffer[1]>>16;
  Int_t num = evbuffer[4];
  cout << "\n\n Raw Data Dump  " << hex << endl;
  cout << "\n Event number  " << dec << num;
  cout << "  length " << len << "  type " << type << endl;
  Int_t ipt = 0;
  for (Int_t j=0; j<(len/5); j++) {
    cout << dec << "\n evbuffer[" << ipt << "] = ";
    for (Int_t k=j; k<j+5; k++) {
      cout << hex << evbuffer[ipt++] << " ";
    }
  }
  if (ipt < len) {
    cout << dec << "\n evbuffer[" << ipt << "] = ";
    for (Int_t k=ipt; k<len; k++) {
      cout << hex << evbuffer[ipt++] << " ";
    }
    cout << endl;
  }
  cout<<dec<<endl;
}

//_____________________________________________________________________________
void ToyCodaDecoder::CompareRocs(  )
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
void ToyCodaDecoder::ChkFbSlot( Int_t roc, const Int_t* evbuffer,
				  Int_t ipt, Int_t istop )
{
  const Int_t* p      = evbuffer+ipt;    // Points to ROC ID word (1 before data)
  const Int_t* pstop  =evbuffer+istop;   // Points to last word of data in roc
  while (p++ < pstop) {
    Int_t slot = (*p)>>27;  // A "self-reported" slot.
    Int_t index = MAXSLOT*roc + slot;
    if ((slot > 0) && (index >=0 && index < MAXROC*MAXSLOT)) fbfound[index]=1;
  }
}
 
//_____________________________________________________________________________
void ToyCodaDecoder::ChkFbSlots()
{
  // This checks the fastbus slots to see if slots are appearing in both the
  // data and the cratemap.  If they appear in one but not the other, an warning
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
  for (Int_t iroc=0; iroc<MAXROC; iroc++) {
    for (Int_t islot=0; islot<MAXSLOT; islot++) {
      Int_t index = MAXSLOT*iroc + islot;
      if (slotstat[index]==2) cout << "Decoder:: WARNING:  Fastbus module in (roc,slot) = ("<<iroc<<","<<islot<<")  found in cratemap but NOT in data !"<<endl;
    }
  }
  for (Int_t iroc=0; iroc<MAXROC; iroc++) {
    for (Int_t islot=0; islot<MAXSLOT; islot++) {
      Int_t index = MAXSLOT*iroc + islot;
      if (slotstat[index]==3) cout << "Decoder:: WARNING:  Fastbus module in (roc,slot) = ("<<iroc<<","<<islot<<")  found in data but NOT in cratemap !"<<endl;
    }
  }
}


//_____________________________________________________________________________
void ToyCodaDecoder::SetRunTime( ULong64_t tloc )
{// PUBLIC
  // Set run time and re-initialize crate map (and possibly other
  // database parameters for the new time.

  if( fRunTime == tloc ) 
    return;
  fRunTime = tloc;
  fgNeedInit = true;  // force re-init
}




//_____________________________________________________________________________
ClassImp(ToyCodaDecoder)
