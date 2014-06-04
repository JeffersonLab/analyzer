////////////////////////////////////////////////////////////////////
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
}

//_____________________________________________________________________________
ToyCodaDecoder::~ToyCodaDecoder()
{
  delete [] irn;
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
Int_t ToyCodaDecoder::LoadEvent(const Int_t* evbuffer, THaCrateMap* map)
{//PUBLIC
  // Main engine for decoding, called by public LoadEvent() methods
  // The crate map argument is ignored. Use SetCrateMapName instead
  assert( evbuffer );
   assert( fMap || fgNeedInit );
  Int_t ret = HED_OK;
  buffer = evbuffer;
  //  if(TestBit(kDebug)) dump(evbuffer);
  if (first_decode || fgNeedInit) {
    ret = init_cmap();
    if( ret != HED_OK ) return ret;
    ret = init_slotdata(fMap);
    if( ret != HED_OK ) return ret;
    if(first_decode) {
      first_decode=kFALSE;
    }
  }
  if( fDoBench ) fBench->Begin("clearEvent");
  for( Int_t i=0; i<fNSlotClear; i++ )
    crateslot[fSlotClear[i]]->clearEvent();
  if( fDoBench ) fBench->Stop("clearEvent");
  event_length = evbuffer[0]+1;  // in longwords (4 bytes)
  event_type = evbuffer[1]>>16;
  if(event_type <= 0) return HED_ERR;
 
 if (event_type <= MAX_PHYS_EVTYPE) {
    event_num = evbuffer[4];
    recent_event = event_num;
    FindRocs(evbuffer);
  }

  // Decode each ROC
  // This is not part of the loop above because it may exit prematurely due 
  // to errors, which would leave the rocdat[] array incomplete.

  for( Int_t i=0; i<nroc; i++ ) {
 
    Int_t iroc = irn[i];
    const RocDat_t* proc = rocdat+iroc;
    Int_t ipt = proc->pos + 1;
    Int_t iptmax = proc->pos + proc->len;
    Int_t status = roc_decode(iroc,evbuffer, ipt, iptmax);

    // do something with status
    if (status == -1) break;

  }


  return ret;
}

//_____________________________________________________________________________
Int_t ToyCodaDecoder::roc_decode( Int_t roc, const Int_t* evbuffer,
				  Int_t ipt, Int_t istop )
{
  // Decode VME
  assert( evbuffer && fMap );
  if( fDoBench ) fBench->Begin("vme_decode");
  Int_t slot;
  Int_t Nslot = fMap->getNslot(roc); //FIXME: use this for crude cross-check
  Int_t retval = HED_OK;
  const Int_t* p      = evbuffer+ipt;    // Points to ROC ID word (1 before data)
  const Int_t* pstop  = evbuffer+istop;  // Points to last word of data
  //FIXME: should never check against event_length since data cannot overrun pstop!
  const Int_t* pevlen = evbuffer+event_length;
  Int_t first_slot_used = 0, n_slots_done = 0;
  Bool_t find_first_used = true;
  Int_t status = SD_ERR;

  if (TestBit(kDebug)) cout << "Decode roc#  "<<dec<<roc<<" nslot "<<Nslot<<endl;
  if (Nslot <= 0) goto err;
  fMap->setSlotDone();
  while ( p++ < pstop && n_slots_done < Nslot ) {
    if(TestBit(kDebug)) cout << "evbuff "<<(p-evbuffer)<<"  "<<hex<<*p<<dec<<endl;
    LoadIfFlagData(p); // this can increment p

    // look through all slots, since Nslot only gives number of occupied slots,
    // not the highest-numbered occupied slot.
    // Note- the way the DAQ is set up, each module's VME header word contains 
    // the slot number, so we can find the right module unambiguously with the
    // ((data&mask)==head) test below
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

      status = crateslot[idx(roc,slot)]->LoadIfSlot(p);
      if (status == kTRUE) fMap->setSlotDone(slot);

      if (p >= pevlen) goto SlotDone;

    }

    
  SlotDone:
    if (TestBit(kDebug)) cout<<"slot done"<<endl;
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
  if( fDoBench ) fBench->Stop("vme_decode");
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
    cout << "Flag data "<<hex<<word<<dec<<endl;
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

  return status;

}


// To initialize the THaSlotData member on first call to decoder
int ToyCodaDecoder::init_slotdata(const THaCrateMap* map)
{
  // Update lists of used/clearable slots in case crate map changed
  cout << "into TOY DECODER init slotdata "<<map<<"   "<<fNSlotUsed<<endl;
  if(!map) return HED_ERR;

  for (Int_t iroc = 0; iroc<MAXROC; iroc++) {
    if (  !map->crateUsed(iroc)  ) continue;

    for (Int_t islot=0; islot < MAXSLOT; islot++) {

      if ( !map->slotUsed(iroc,islot) ) continue;

        makeidx(iroc,islot);
        cout << "Num slots defined "<<GetNslots()<<endl;

    }
  }

  for( int i=0; i<fNSlotUsed; i++ ) {
    THaSlotData* crslot = crateslot[fSlotUsed[i]];
    int crate = crslot->getCrate();
    int slot  = crslot->getSlot();
    cout << "print of crate slot ---   crate "<<crate<<"   slot "<<slot<<endl;
    crslot->print();
    crslot->loadModule(map);
    cout << "Dev type xxx  = "<<crslot->devType()<<endl;
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


// We probably want to throw out these, to use the Event Type Handlers instead
//_____________________________________________________________________________
Int_t ToyCodaDecoder::GetScaler( const TString& spec, Int_t slot,
				 Int_t chan ) const
{// PUBLIC -- get a scaler from a spectromter.  This is per event
  // FIXME !

  Int_t roc=0;
  //  roc = scalerdef[spec];
  if (roc >= 0) return GetScaler(roc, slot, chan);
  return 0;

}

//_____________________________________________________________________________
Int_t ToyCodaDecoder::GetScaler(Int_t roc, Int_t slot, Int_t chan) const
{// PUBLIC
  // Get scaler data by roc, slot, chan.

  assert( ScalersEnabled() );  // Should never request data when not enabled
  assert( GoodIndex(roc,slot) );

  //  if (fgScalEvt) return fgScalEvt->GetScaler(roc, slot, chan);
  return crateslot[idx(roc,slot)]->getData(chan,0);

}

// This, and similar routines, use global event type handlers.
//_____________________________________________________________________________
Bool_t ToyCodaDecoder::IsLoadedEpics(const char* tag) const
{// PUBLIC
  // Test if the given EPICS variable name has been loaded
  assert( tag );
  //  if (fgEpicsEvt) return fgEpicsEvt->IsLoaded(tag);
  return kFALSE;
}

//_____________________________________________________________________________
double ToyCodaDecoder::GetEpicsData(const char* tag, Int_t event) const
{//PUBLIC
  // EPICS data which is nearest CODA event# 'event'
  // event == 0 --> get latest data
  // Tag is EPICS variable, e.g. 'IPM1H04B.XPOS'

  assert( IsLoadedEpics(tag) ); // Should never ask for non-existent data
  //  if (fgEpicsEvt) return fgEpics->GetData(tag, event);
  return 0;
}

//_____________________________________________________________________________
string ToyCodaDecoder::GetEpicsString(const char* tag, Int_t event) const
{ // PUBLIC
  // EPICS string data which is nearest CODA event# 'event'
  // event == 0 --> get latest data

  assert( IsLoadedEpics(tag) ); // Should never ask for non-existent data
  //  if (fgEpicsEvt) return fgEpicsEvt->GetString(tag, event);
  return "";
}

//_____________________________________________________________________________
double ToyCodaDecoder::GetEpicsTime(const char* tag, Int_t event) const
{//PUBLIC
  //  if (fgEpicsEvt) return fgEpicsEvt->GetTimeStamp(tag, event);
  return 0; 
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
