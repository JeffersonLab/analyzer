/////////////////////////////////////////////////////////////////////
//
//   THaCodaDecoder
//   Hall A Event Data from one CODA "Event"
//
//   THaCodaDecoder contains facilities to load an event buffer
//   from a CODA "event" and get information about the event, 
//   such as event type, TDC hits, scalers, prescale factors,
//   etc.  The idea of an "event" is not just a physics trigger,   
//   but more generally any separate record inserted into CODA, 
//   e.g. EPICS insertion.  The user must "LoadEvent" each 
//   event before extracting information about the event.  
//   The "evbuffer" may come from a CODA file or ET connection.  
//   There is no need to create/destroy an THaEvData object each 
//   event because hit counters are cleared each event. 
//   There are various kinds of events including:
//      1. physics triggers
//      2. scaler events (asynch insertion of scalers)
//      3. EPICS "events" 
//      4. prescale factors (one "event" at start of run).
//      5. etc, etc. (see the online DAQ documentation)
//   Public routines allow to fetch these data.
//
//
/////////////////////////////////////////////////////////////////////

#include "THaCodaDecoder.h"
#include "THaFastBusWord.h"
#include "THaCrateMap.h"
#include "THaEpics.h"
#include "THaUsrstrutils.h"
#include "THaBenchmark.h"
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
THaCodaDecoder::THaCodaDecoder() :
  first_scaler(true),
  numscaler_crate(0)
{
  epics = new THaEpics;
  fb = new THaFastBusWord;
  memset(psfact,0,MAX_PSFACT*sizeof(*psfact));

  EnableScalers();
}

//_____________________________________________________________________________
THaCodaDecoder::~THaCodaDecoder()
{
  delete epics;
  delete fb;
}

//_____________________________________________________________________________
Int_t THaCodaDecoder::GetPrescaleFactor(Int_t trigger_type) const
{
  // To get the prescale factors for trigger number "trigger_type"
  // (valid types are 1,2,3...)
  if ( (trigger_type > 0) && (trigger_type <= MAX_PSFACT)) {
    return psfact[trigger_type - 1];
  }
  if (fDebug > 0) {
    cout << "Warning:  Requested prescale factor for ";
    cout << "un-prescalable trigger "<<dec<<trigger_type<<endl;
  }
  return 0;
}

//_____________________________________________________________________________
Int_t THaCodaDecoder::LoadEvent(const Int_t* evbuffer )
{
  // Public interface to decode the event.  Note, LoadEvent()
  // MUST be called once per event BEFORE you can extract 
  // information about the event.
  return gendecode(evbuffer);
}

//_____________________________________________________________________________
Int_t THaCodaDecoder::gendecode(const Int_t* evbuffer )
{
  // Main engine for decoding, called by public LoadEvent() methods
  assert( evbuffer );
  assert( fMap || fNeedInit );
  Int_t ret = HED_OK;
  buffer = evbuffer;
  if(fDebug > 2) dump(evbuffer);
  if (first_decode || fNeedInit) {
    ret = init_cmap();
    if( ret != HED_OK ) return ret;
    ret = init_slotdata(fMap);
    if( ret != HED_OK ) return ret;
    if(first_decode) {
      for (Int_t crate=0; crate<MAXROC; crate++)
	// FIXME: use flag not string
	scalerdef[crate] = "nothing";
      first_decode = false;
    }
  }
  if( fDoBench ) fBench->Begin("clearEvent");
  for( Int_t i=0; i<fNSlotClear; i++ )
    crateslot[fSlotClear[i]]->clearEvent();
  if( fDoBench ) fBench->Stop("clearEvent");
  evscaler = 0;
  //FIXME: Test event header signature
  //  if( (evbuffer[1] & 0xff) != 0xcc ) goto err;
  event_length = evbuffer[0]+1;  // in longwords (4 bytes)
  event_type = evbuffer[1]>>16;
  if(event_type <= 0) return HED_ERR;
  if (event_type <= MAX_PHYS_EVTYPE) {
    event_num = evbuffer[4];
    recent_event = event_num;
    ret = physics_decode(evbuffer);
  } else {
    if( fDoBench && event_type != SCALER_EVTYPE ) 
      fBench->Begin("ctrl_evt_decode");
    event_num = 0;
    switch (event_type) {
    case PRESTART_EVTYPE :
      // Usually prestart is the first 'event'.  Call SetRunTime() to 
      // re-initialize the crate map since we now know the run time.  
      // This won't happen for split files (no prestart). For such files, 
      // the user should call SetRunTime() explicitly.
      SetRunTime(static_cast<ULong64_t>(evbuffer[2]));
      run_num  = evbuffer[3];
      run_type = evbuffer[4];
      evt_time = fRunTime;
      break;
    case EPICS_EVTYPE :
      ret = epics_decode(evbuffer);
      break;
    case PRESCALE_EVTYPE :
    case TS_PRESCALE_EVTYPE :
      ret = prescale_decode(evbuffer);
      break;
    case TRIGGER_FILE :
    case DETMAP_FILE :
      break;
    case SCALER_EVTYPE :
      if( ScalersEnabled() )
	ret = scaler_event_decode(evbuffer);
      break;
    case SYNC_EVTYPE :
    case GO_EVTYPE :
    case PAUSE_EVTYPE :
    case END_EVTYPE :
      evt_time = static_cast<UInt_t>(evbuffer[2]);
      break;
    default:
      // Unknown event type
      break;
    }
    if( fDoBench ) fBench->Stop("ctrl_evt_decode");
  }
  return ret;
}

//_____________________________________________________________________________
void THaCodaDecoder::dump(const Int_t* evbuffer)
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
Int_t THaCodaDecoder::physics_decode(const Int_t* evbuffer )
{
  assert( evbuffer && fMap );
  if( fDoBench ) fBench->Begin("physics_decode");
  Int_t status = HED_OK;
  //FIXME: Check for valid event header info
  //  if( (evbuffer[1]&0xffff) != 0x10cc )      return HED_ERR; // Header sig
  //  if( (evbuffer[1]>>16) > MAX_PHYS_EVTYPE ) return HED_ERR; // Event type
  //  if( (UInt_t)evbuffer[3] != 0xc0000100 )   return HED_ERR; // Event ID sig
  memset(rocdat,0,MAXROC*sizeof(RocDat_t));
  // Set pos to start of first ROC data bank
  Int_t pos = evbuffer[2]+3;  // should be 7
  Int_t nroc = 0;
  Int_t irn[MAXROC];   // Lookup table i-th ROC found -> ROC number
  while( pos+1 < event_length && nroc < MAXROC ) {
    Int_t len  = evbuffer[pos];
    Int_t iroc = (evbuffer[pos+1]&0xff0000)>>16;
    if( iroc>=MAXROC ) {
      if(fDebug > 0) {
	cout << "ERROR in THaCodaDecoder::physics_decode:";
	cout << "  illegal ROC number " <<dec<<iroc<<endl;
      }
      if( fDoBench ) fBench->Stop("physics_decode");
      return HED_ERR;
    }
    // Save position and length of each found ROC data block
    rocdat[iroc].pos  = pos;
    rocdat[iroc].len  = len;
    irn[nroc++] = iroc;
    pos += len+1;
  }
  if( fDoBench ) fBench->Stop("physics_decode");
  // Decode each ROC
  // This is not part of the loop above because it may exit prematurely due 
  // to errors, which would leave the rocdat[] array incomplete.
  for( Int_t i=0; i<nroc; i++ ) {
    Int_t iroc = irn[i];
    const RocDat_t* proc = rocdat+iroc;
    Int_t ipt = proc->pos + 1;
    Int_t iptmax = proc->pos + proc->len;
    if (fMap->isFastBus(iroc)) {
      status = fastbus_decode(iroc,evbuffer,ipt,iptmax);
      if(status == HED_ERR) return HED_ERR;
    } else if (fMap->isVme(iroc)) {
      status = vme_decode(iroc,evbuffer,ipt,iptmax);
      if(status == HED_ERR) return HED_ERR;
    } else if (fMap->isCamac(iroc)) {
      status = camac_decode(iroc,evbuffer,ipt,iptmax);
      if(status == HED_ERR) return HED_ERR;
    }
  }
  return HED_OK;
}

Int_t THaCodaDecoder::epics_decode(const Int_t* evbuffer)
{
  assert( evbuffer );
  if( fDoBench ) fBench->Begin("epics_decode");
  epics->LoadData(evbuffer, recent_event);
  if( fDoBench ) fBench->Stop("epics_decode");
  return HED_OK;
};

//_____________________________________________________________________________
Int_t THaCodaDecoder::prescale_decode(const Int_t* evbuffer)
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
      if (psfact[trig]==0)
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
Int_t THaCodaDecoder::scaler_event_decode( const Int_t* evbuffer )
{
  // Decode scalers

  assert( evbuffer && fMap );
  assert( ScalersEnabled() );
  assert( event_type == SCALER_EVTYPE );
  if( fDoBench ) fBench->Begin("scaler_event_decode");
  if (fDebug > 1) cout << "Scaler decoding"<<endl;
  if (first_scaler) {
    first_scaler = kFALSE;
    numscaler_crate = 0;
    for (Int_t roc=0; roc<MAXROC; roc++) {
      if (fMap->isScalerCrate(roc)) scaler_crate[numscaler_crate++] = roc;
    }
  }
  for(Int_t cra=0; cra<numscaler_crate; cra++) {       
    Int_t roc = scaler_crate[cra];
    rocdat[roc].len  = 0;
  }
  Int_t ret = HED_OK;
  Int_t ipt = -1;
  while (++ipt < event_length) {
    UInt_t headerword = evbuffer[ipt];
    Int_t roc = fMap->getScalerCrate(headerword);
    if (fDebug > 1) {
      cout << "ipt "<<ipt<<" evbuffer "<<hex<<headerword<<dec;
      cout <<" evlen "<<event_length<<" roc "<<roc<<endl;
    }
    if (!roc) continue;               
    const char* location = fMap->getScalerLoc(roc);
    if( scalerdef[roc] == "nothing" ) {
      if      (!strcmp(location,"rscaler")) scalerdef[roc] = "right";
      else if (!strcmp(location,"lscaler")) scalerdef[roc] = "left";
      else if (!strcmp(location,"rcs"))     scalerdef[roc] = "rcs";
    }
    rocdat[roc].len++;
    // If more locations added, put them here.  But scalerdef[] is not
    // needed if you know the roc#, you can call getScaler(roc,...)
    Int_t slot = (headerword&0xf0000)>>16; // 0<=slot<=15
    Int_t numchan = headerword&0xff;
    if (fDebug > 1) cout<<"slot "<<slot<<" numchan "<<numchan<<endl;
    Int_t ics = idx(roc,slot);
    crateslot[ics]->clearEvent();
    for (Int_t chan=0; chan<numchan; chan++) {
      ipt++; 
      rocdat[roc].len++;
      Int_t data = evbuffer[ipt];
      if (fDebug > 1) cout<<"scaler chan "<<chan<<" data "<<data<<endl;
      if (crateslot[ics]->loadData(location,chan,data,data)
	  == SD_ERR) {
	cerr << "THaEvData::scaler_event_decode(): ERROR:";
	cerr << " crateslot loadData for slot "<<slot<<endl;
      }
    }
  }
  if( fDoBench ) fBench->Stop("scaler_event_decode");
  return ret;
}

//_____________________________________________________________________________
Int_t THaCodaDecoder::GetScaler( const TString& spec, Int_t slot,
				 Int_t chan ) const
{
  // Get scaler data by spectrometer, slot, and channel
  // spec = "left" or "right" for the "scaler events" (type 140)
  // and spec = "evleft" and "evright" for scalers
  // that are part of the event (ev) readout.
  for(Int_t cra=0; cra<numscaler_crate; cra++) {       
    Int_t roc = scaler_crate[cra];
    if (spec == scalerdef[roc])
      return GetScaler(roc, slot, chan);
  }
  return 0;
}

//_____________________________________________________________________________
Int_t THaCodaDecoder::GetScaler(Int_t roc, Int_t slot, Int_t chan) const
{
  // Get scaler data by roc, slot, chan.
  assert( ScalersEnabled() );  // Should never request data when not enabled
  assert( GoodIndex(roc,slot) );
  if( scalerdef[roc] != "nothing" )
    return crateslot[idx(roc,slot)]->getData(chan,0);
  return 0;
}

//_____________________________________________________________________________
Bool_t THaCodaDecoder::IsLoadedEpics(const char* tag) const
{
  // Test if the given EPICS variable name has been loaded
  assert( tag );
  return epics->IsLoaded(tag);
}

//_____________________________________________________________________________
double THaCodaDecoder::GetEpicsData(const char* tag, Int_t event) const
{
  // EPICS data which is nearest CODA event# 'event'
  // event == 0 --> get latest data
  // Tag is EPICS variable, e.g. 'IPM1H04B.XPOS'

  assert( IsLoadedEpics(tag) ); // Should never ask for non-existent data
  return epics->GetData(tag, event);
}

//_____________________________________________________________________________
string THaCodaDecoder::GetEpicsString(const char* tag, Int_t event) const
{
  // EPICS string data which is nearest CODA event# 'event'
  // event == 0 --> get latest data

  assert( IsLoadedEpics(tag) ); // Should never ask for non-existent data
  return epics->GetString(tag, event);
}

//_____________________________________________________________________________
double THaCodaDecoder::GetEpicsTime(const char* tag, Int_t event) const
{
  // EPICS time stamp
  // event == 0 --> get latest data
  return epics->GetTimeStamp(tag, event);
}

//_____________________________________________________________________________
Int_t THaCodaDecoder::fastbus_decode( Int_t roc, const Int_t* evbuffer,
				      Int_t istart, Int_t istop)
{
  assert( evbuffer && fMap );
  if( fDoBench ) fBench->Begin("fastbus_decode");
  Int_t slotold = -1;
  const Int_t* p     = evbuffer+istart;
  const Int_t* pstop = evbuffer+istop;
  synchmiss = false;
  synchextra = false;
  buffmode = false;
  Int_t xctr = 0;
  Int_t nspflag = 0;
  if (fDebug > 1) cout << "Fastbus roc  "<<roc<<endl;
  while ( p++ < pstop ) {  
    xctr++;
    if(fDebug > 1) {
      cout << "evbuffer  " <<(p-evbuffer)<<"   ";
      cout << hex << *p << dec << endl;
    }
    Int_t slot = fb->Slot(*p);
    if (!slot) {
      if ((*p & 0xffff0000)==0xfabc0000) {
	xctr = 0;
	nspflag = (*p & 0xff);  // number of special flags.
      }
      loadFlag(p);
      continue;
    }
    if (nspflag > 0 && xctr <= nspflag) continue;  // skip special flags
    Int_t model = fMap->getModel(roc,slot);
    if (model == THaCrateMap::CM_ERR) continue;
    if (!model) {
      if (fDebug > 0) {
	cout << "Warning: Undefined module in data" << endl;
	cout << "roc " << roc << "   slot " << slot << endl;
      }
      continue;
    }
    if (fDebug > 1) {
      cout<<"slot slotold model "<<slot;
      cout<<"  "<<slotold<<"  "<<model<<endl;
    }
    if (slot != slotold) {            
      slotold = slot;
      if (fb->HasHeader(model)) {
	Int_t n = fb->Wdcnt(model,*p);
	if (n == THaFastBusWord::FB_ERR) {
	  if( fDoBench ) fBench->Stop("fastbus_decode");
	  return HED_ERR;
	}
	if (fDebug > 1) cout << "header, wdcnt = "<<n<<endl;
	if (n <= 1) continue;
	p++;
      }
    }
    Int_t chan = fb->Chan(model,*p);
    Int_t data = fb->Data(model,*p);
    if (fDebug > 1) {
      printf("roc %2d  slot %3d  chan %3d  data %5d  ipt %3d"
	     "  raw %8x  device %s\n",
	     roc, slot, chan, data, static_cast<Int_t>(p-evbuffer), 
	     *p, fb->devType(model));
    }
    // At this point, roc and slot ranges have been checked
    Int_t status;
    status = crateslot[idx(roc,slot)]->loadData(fb->devType(model),
						chan,data,*p);
    if( status != SD_OK) {
      if( fDoBench ) fBench->Stop("fastbus_decode");
      return (status == SD_ERR) ? HED_ERR : HED_WARN;
    }
  }
  if( fDoBench ) fBench->Stop("fastbus_decode");
  return HED_OK;
}

//_____________________________________________________________________________
static UInt_t FADCWindowRawDecode( const Int_t* p, const Int_t* pstop,
				   Int_t slot, THaSlotData* crateslot )
{
  // Decode "Window Raw Data" fields in event data from JLab 250 MHz Flash ADC

  Int_t type, type_last = 15, time_last = 0, status = 0;
  Bool_t go = true, new_type = true, valid_1, valid_2;
  //  Int_t slot_id_hd, slot_id_tr, n_evts, blk_num, n_words, evt_num_1, 
  // evt_num_2, time_1, time_2, time_3, time_4, n_samples
  Int_t chan = 0, time_now = 0, adc_1, adc_2;
  UInt_t nwords = 0;

  while( p<=pstop && go ) {
    UInt_t data = static_cast<UInt_t>( *p );
    ++nwords;
    ++p;

    // The following adapted from Dave Abbott's example code 2/16/09 JOH

    if( data & 0x80000000 ) {   // data type defining word
      new_type = true;
      type = (data & 0x78000000) >> 27;
    } else {
      new_type = false;
      type = type_last;
    }
        
    switch( type ) {

      //TODO: ensure nothing gets processed without a block header first

    case 0:		// BLOCK HEADER
      // slot_id_hd = (data & 0x7C00000) >> 22;
      // n_evts = (data & 0x3FF800) >> 11;
      // blk_num = (data & 0x7FF);
      // TODO: check slot
      // TODO: use n_evts, blk_num
      break;
    case 1:		// BLOCK TRAILER
      // slot_id_tr = (data & 0x7C00000) >> 22;
      // n_words = (data & 0x3FFFFF);
      // End of block, quit while loop
      //TODO: check slot, n_words
      go = false;
      break;
    case 2:		// EVENT HEADER
      // if( new_type )
      // 	evt_num_1 = (data & 0x7FFFFFF);
      // else
      // 	evt_num_2 = (data & 0x7FFFFFF);
      break;
    case 3:		// TRIGGER TIME
      if( new_type ) {
	// time_1 = (data & 0xFFFFFF);
	time_now = 1;
	time_last = 1;
      } else if( time_last == 1 ) {
	// time_2 = (data & 0xFFFFFF);
	time_now = 2;
      } else if( time_last == 2 ) {
	// time_3 = (data & 0xFFFFFF);
	time_now = 3;
      } else if( time_last == 3 ) {
	// time_4 = (data & 0xFFFFFF);
	time_now = 4;
      }    
      //else
      //TODO: warn of trigger time error
      time_last = time_now;
      break;
    case 4:		// WINDOW RAW DATA
      if( new_type ) {
	chan = (data & 0x7800000) >> 23;
	// n_samples = (data & 0xFFF);
	//TODO: ensure n_samples really follow
      } else {
	//TODO: ensure this is preceded by the channel/nsamples header
	adc_1 = (data & 0x1FFF0000) >> 16; // NB: 13-bit ADC data
	valid_1 = !( data & 0x20000000 );
	adc_2 = (data & 0x1FFF);
	valid_2 = !( data & 0x2000 );
	if( valid_1 ) {
	  status = crateslot->loadData( "adc", chan, adc_1, adc_1 );
	  if( status != SD_OK ) return (status == SD_ERR) ? 
	    THaEvData::HED_ERR : THaEvData::HED_WARN;
	}
	if( valid_2 ) {
	  status = crateslot->loadData( "adc", chan, adc_2, adc_2 );
	  if( status != SD_OK ) return (status == SD_ERR) ? 
	    THaEvData::HED_ERR : THaEvData::HED_WARN;
	}
	//TODO: warn on invalid data
      }
      break;

      // Not implemented/NOP:
    case 5:		// WINDOW SUM
    case 6:		// PULSE RAW DATA
    case 7:		// PULSE INTEGRAL
    case 8:		// PULSE TIME
    case 9:		// STREAMING RAW DATA
      //TODO: warn on undefined data type (may indicate logic error/
      //data corruption)
    case 10:		// UNDEFINED TYPE
    case 11:		// UNDEFINED TYPE
    case 12:		// UNDEFINED TYPE
    case 13:		// END OF EVENT
    case 14:		// DATA NOT VALID (no data available)
    case 15:		// FILLER WORD
      break;
    }
  
    type_last = type;	// save type of current data word

  }
  return nwords;
}
			   
//_____________________________________________________________________________
Int_t THaCodaDecoder::vme_decode( Int_t roc, const Int_t* evbuffer,
				  Int_t ipt, Int_t istop )
{
  // Decode VME
  assert( evbuffer && fMap );
  if( fDoBench ) fBench->Begin("vme_decode");
  Int_t slot,chan,raw,data,slotprime,ndat,head,mask,nhit;
  Int_t Nslot = fMap->getNslot(roc); //FIXME: use this for crude cross-check
  Int_t retval = HED_OK;
  const Int_t* p      = evbuffer+ipt;    // Points to ROC ID word (1 before data)
  const Int_t* pstop  = evbuffer+istop;  // Points to last word of data
  //FIXME: should never check against event_length since data cannot overrun pstop!
  const Int_t* pevlen = evbuffer+event_length;
  const Int_t* loc    = 0;
  Int_t first_slot_used = 0, n_slots_done = 0;
  Bool_t find_first_used = true;
  Int_t status = SD_ERR;

  if (fDebug > 1) cout << "VME roc "<<dec<<roc<<" nslot "<<Nslot<<endl;
  if (Nslot <= 0) goto err;
  scalerdef[roc] = fMap->getScalerLoc(roc);
  fMap->setSlotDone();
  if (fMap->isScalerCrate(roc) && GetRocLength(roc) >= 16) evscaler = 1;
  while ( p++ < pstop && n_slots_done < Nslot ) {
    if(fDebug > 1) cout << "evbuff "<<(p-evbuffer)<<"  "<<hex<<*p<<dec<<endl;
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
      head = fMap->getHeader(roc,slot);
      mask = fMap->getMask(roc,slot);
      if(fDebug > 1) {
	cout<<"slot head mask "<<slot<<"  ";
	cout<<hex<<head<<"  "<<mask<<dec<<endl;
      }
      if (((*p)&mask) == head) {
	fMap->setSlotDone(slot);

	Int_t model = fMap->getModel(roc,slot);
	if (fDebug > 1) cout<<"model " << model << endl << flush;

	// Process known VME modules
	// FIXME: make this modular!!
	switch(model) {

	case 1182:    // LeCroy 1182 ADC
	  for (chan=0; chan<8; chan++) {
	    if( ++p >= pevlen ) goto SlotDone;  //FIXME: see above, check against pstop
	    if(fDebug > 1) {
	      cout<<"1182 chan data "<<chan<<" 0x"<<hex<<*p<<dec<<endl;
	    }
	    status = crateslot[idx(roc,slot)]->loadData("adc",chan,*p,*p);
	    if( status != SD_OK ) goto err;
	  }
	  break;

	case 7510:    // Struck 7510 ADC
	  nhit = ((*p)&0xfff)/8;
	  if (fDebug > 1) cout << "nhit 7510 " << nhit << endl;
	  for (chan=0; chan<8; chan++) {
	    for (Int_t j=0; j<nhit/2; j++) {  // nhit must be even
	      if( ++p >= pevlen ) goto SlotDone;
	      if(fDebug > 1)  cout<<"7510 raw  0x"<<hex<<*p<<dec<<endl;
	      status = crateslot[idx(roc,slot)]
		->loadData("adc",chan,((*p)&0x0fff0000)>>16,*p);
	      if( status != SD_OK ) goto err;
	      status = crateslot[idx(roc,slot)]
		->loadData("adc",chan,((*p)&0xfff),*p);
	      if( status != SD_OK ) goto err;
	    }
	  }
	  break;

	case 3123:    // VMIC 3123 ADC
	  for (chan=0; chan<16; chan++) {
	    if( ++p >= pevlen ) goto SlotDone;
	    if(fDebug > 1) {
	      cout<<"3123 chan data "<<chan<<"  0x"<<hex<<*p<<dec<<endl;
	    }
	    status = crateslot[idx(roc,slot)]->loadData("adc",chan,*p,*p);
	    if( status != SD_OK ) goto err;
	  }
	  break;

	case 1151:    // LeCroy 1151 scaler
	  // Note, although there may be scalers in physics events, the
	  // "scaler events" dont come here.  See scaler_event_decode().
	  for (chan=0; chan<16; chan++) {
	    if( ++p >= pevlen ) goto SlotDone;
	    if(fDebug > 1) {
	      cout<<"1151 chan data "<<chan<<"  0x"<<hex<<*p<<dec<<endl;
	    }
	    status = crateslot[idx(roc,slot)]->loadData("scaler",chan,*p,*p);
	    if( status != SD_OK ) goto err;
	  }
	  break;

	case 560:     // CAEN 560 scaler
	  // The CAEN 560 is a little tricky; sometimes only 1 channel was read,
	  // so we don't increment ipt. (hmmm... could use time-dep crate map.)
	  loc = p;
	  for (chan=0; chan<16; chan++) {  
	    if( ++loc >= pevlen ) goto SlotDone; 
	    if(fDebug > 1) {
	      cout<<"560 chan data "<<chan<<"  0x"<<hex<<*loc<<dec<<endl;
	    }
	    status = crateslot[idx(roc,slot)]
	      ->loadData("scaler",chan,*loc,*loc);
	    if( status != SD_OK ) goto err;
	  }
	  break;

	case 3801:    // Struck 3801 scaler
	  for (chan=0; chan<32; chan++) {
	    if( ++p >= pevlen) goto SlotDone;
	    if(fDebug > 1) {
	      cout<<"3801 chan data "<<chan<<"  0x"<<hex<<*p<<dec<<endl;
	    }
	    status = crateslot[idx(roc,slot)]->loadData("scaler",chan,*p,*p);
	    if( status != SD_OK ) goto err;
	  }
	  break;

	case 7353:    // Bodo Reitz's hack for the trigger module
	  chan=0;
	  raw = (*p)&0xfff;
	  if(fDebug > 1) {
	    cout<<"7353 chan data "<<chan<<"  0x"<<hex<<raw<<dec<<endl;
	  }
	  status = crateslot[idx(roc,slot)]->loadData("register",chan,raw,raw);
	  if( status != SD_OK ) goto err;
	  break;

	case 550:     // CAEN 550 for RICH 
	  slotprime = 1+(((*p)&0xff0000)>>16);  
	  if (fDebug > 1)
	    cout << "CAEN 550 slot header_slot data = "
		 <<slot<<" "<<slotprime<<" "<<loc<<" "<<hex<<*p<<dec<<endl; 
	  if (slot == slotprime) {    // They should agree, else problem.
	    ndat = (*p)&0xfff;
	    if (fDebug > 1) cout << "CAEN 550 ndat = "<<ndat<<endl;
	    if (p+ndat>pstop) {
	      p=pstop;
	      if( fDebug > 0 )
		cout << "Error: CAEN 550 data overflow roc/slot/ndat = "
		     << roc << "/" << slot << "/" << ndat << endl;
	    } else {
	      loc = p;
	      while( ++loc <= p+ndat ) {
		chan = ((*loc)&0x7ff000)>>12;    
		// channel number
		raw  = (*loc)&0xffffffff;
		data = (*loc)&0x0fff;           
		if (fDebug > 1) cout << "CAEN 550 channel "<<chan
					  <<"  data "<<hex<<data<<dec<<endl;
		status = crateslot[idx(roc,slot)]
		  ->loadData("adc",chan,data,raw);
		if( status != SD_OK ) goto err;
	      }
	      p += ndat;
	    }
	  } else if( fDebug > 0 ) {
	    cout << "Warning: slot != header_slot for CAEN 550 roc/slot = "
		 << roc << "/" << slot << endl;
	  }
	  break;

	case 775:     // CAEN 775 TDC
	case 792:     // CAEN 792 QDC
	  {
	    ++p; 
	    Int_t nword=*p-2;
	    ++p;
	    bool is775 = (model == 775);
	    for (Int_t i=0;i<nword;i++) {
	      ++p;
	      chan=((*p)&0x00ff0000)>>16;
	      raw=((*p)&0x00000fff);	      
	      if (is775) {
		status = crateslot[idx(roc,slot)]
		  ->loadData("tdc",chan,raw,raw);
	      } else {
		//	      if (fMap->getModel(roc,slot) == 792) {
		status = crateslot[idx(roc,slot)]
		  ->loadData("adc",chan,raw,raw);
	      }
	      if( status != SD_OK ) goto err;
	    }
	    ++p;
	  }
	  break;

	case 6401:      // JLab F1 normal resolution mode
	case 3201:      // JLab F1 high resolution mode
	  {
	    // CAUTION: this routine re-numbers the channels
	    // compared to the labelled numbering scheme on the board itself.
	    // According to the labelling and internal numbering scheme,
	    // the F1 module has odd numbered channels on one connector
	    // and even numbered channels on the other.
	    // However we usually put neighboring blocks/wires into the same 
	    // cable, connector etc.
	    // => hana therefore uses a numbering scheme different from the module
	    //
	    // In normal resolution mode, the scheme is:
	    //    connector 1:  ch 0 - 15
	    //    conncetor 2:  ch 16 - 31
	    //    connector 33: ch 32 - 47
	    //    connector 34: ch 48 - 63
	    //
	    // In high-resolution mode, only two connectors are used since
	    // two adjacent channels are internally combined and read out as the
	    // internally-even numbered channels.
	    // this is kind of inconvenient for the rest of the software
	    // => hana therefore uses a numbering scheme different from the module
	    //    connector 1:  unused
	    //    connector 2:  ch 0 - 15
	    //    connector 33: unused
	    //    connector 34: ch 16 - 31
	    //
	    // In both modes:
	    // it is assumed that we only get data from one single trigger
	    // if the F1 is run in multiblock mode (buffered mode) 
	    // this might not be the case anymore - but this will be interesting anyhow
	    // triggertime and eventnumber are not yet read out, they will again
	    // be useful when multiblock mode (buffered mode) is used
	    const Int_t F1_HIT_OFLW = 1<<24; // bad
	    const Int_t F1_OUT_OFLW = 1<<25; // bad
	    const Int_t F1_RES_LOCK = 1<<26; // good
	    const Int_t DATA_CHK = F1_HIT_OFLW | F1_OUT_OFLW | F1_RES_LOCK;
	    const Int_t DATA_MARKER = 1<<23;

	    // look at all the data
	    loc = p;
	    while ((loc <= pevlen) && ((*loc)&0xf8000000)==(head&0xf8000000)) {
	      if ( !( (*loc) & DATA_MARKER ) ) {
		// header/trailer word, to be ignored
		if(fDebug > 1)
		  cout<< "[" << (loc-evbuffer) << "] header/trailer  0x"
		      <<hex<<*loc<<dec<<endl;
	      } else {
		if (fDebug > 1)
		  cout<< "[" << (loc-evbuffer) << "] data            0x"
		      <<hex<<*loc<<dec<<endl;
		Int_t chn = ((*loc)>>16) & 0x3f;  // internal channel number

		chan =0;
		if (model==6401) {        // normal resolution
		  // do the reordering of the channels, for contiguous groups
		  // odd numbered TDC channels from the board -> +16
		  chan = (chn & 0x20) + 16*(chn & 0x01) + ((chn & 0x1e)>>1);
		} else if (model==3201) { // high resolution
		  // drop last bit for channel renumbering
		  chan=(chn >> 1);
		}
		Int_t f1slot = ((*loc)&0xf8000000)>>27;
		//FIXME: cross-check slot number here
		if ( ((*loc) & DATA_CHK) != F1_RES_LOCK ) {
		  cout << event_num << ":";
		  cout << "\tWarning: F1 TDC " << hex << (*loc) << dec;
		  cout << "\tSlot (Ch) = " << f1slot << "(" << chan << ")";
		  if ( (*loc) & F1_HIT_OFLW ) {
		    cout << "\tHit-FIFO overflow";
		  }
		  if ( (*loc) & F1_OUT_OFLW ) {
		    cout << "\tOutput FIFO overflow";
		  }
		  if ( ! ((*loc) & F1_RES_LOCK ) ) {
		    cout << "\tResolution lock failure!";
		  }
		  cout << endl;
		}
		
		raw= (*loc) & 0xffff;
		if(fDebug > 1) {
		  cout<<" int_chn chan data "<<dec<<chn<<"  "<<chan
		      <<"  0x"<<hex<<raw<<dec<<endl;
		}
		status = crateslot[idx(roc,slot)]
		  ->loadData("tdc",chan,raw,raw);
		if( status != SD_OK ) {
		  if (fDebug > 1) {
		    cout<<"Error found loadData tdc roc/slot/chan data"
			<<dec<< roc << "  " << slot << "  " << chan 
			<<"  0x"<<hex<<raw<<endl;
		    goto err;
		  }
		}
	      }
	      loc++;
	    }
	    p=loc-1;  // point to the previous position, so loop goes to current loc
	    if (fDebug > 1) {
	      cout << "Exiting at [" <<dec<<(loc-evbuffer) <<"]  0x"
		   <<hex<<*loc<<dec<<endl;
	    }
	  }
	  break;

	case 767:   // CAEN 767 MultiHit TDC
	  {
	    loc = p;
	    loc++;  // skip first word (header)
	    Int_t nword=0;
	    while ( (loc <= pevlen)&& ((*loc)&0x00600000)==0) {
	      chan=((*loc)&0x7f000000)>>24;
	      raw=((*loc)&0x000fffff);	      
	      status = crateslot[idx(roc,slot)]->loadData("adc",chan,raw,raw);
	      if( status != SD_OK ) goto err;
	      loc++;
	      nword++;
	    }
	    if (((*loc)&0x00600000)!=0x00200000) {
	      return HED_ERR;
	    } else {
	      if (((*loc)&0xffff)!=nword) {
		if (fDebug > 1) cout<<"WC mismatch "<<nword<<" "<<hex<<(*p)<<endl;
		return HED_ERR;
		//    Replace the above line with this to 
		//    disable tossing out the event
		//    when we get a bad thing.
		//	return HED_OK;
	      }
	    }
	    p = loc-1; // so p++ will point to the first word that didn't match
	  }
	  break;
        case 1190:  // CAEN 1190A MultiHit TDC in trigger matching mode·
              //  (written by Simona M.; modified by Brad S.)
          {
            loc = p;
            Int_t nword = 0;      // Num of words in the event including filler words but excluding dummies
            Int_t nword_mod = 0;  // Num of module words from global trailer (includes header & trailer)
            Int_t chip_nr_words    = 0;
            Int_t module_nr_words  = 0;
            Int_t ev_from_gl_hd    = -1;
            Int_t slot_from_gl_hd  = -1;
            Int_t slot_from_gl_tr  = -1;
            Int_t chip_nr_hd       = -1;
            Int_t chip_nr_tr       = -1;
            Int_t bunch_id         = -1;
            Int_t status_err       = -1;

            Bool_t done = false;
            while( loc <= pevlen && !done) {
              switch( (*loc)&0xf8000000 ) {

                case 0xc0000000 :  // buffer-alignment filler word from the 1190; skip it
                  break;

                case 0x40000000 :  // global header; contains: event nr. and slot nr.
                  ev_from_gl_hd = ((*loc)&0x07ffffe0) >> 5; // bits 26-5
                  slot_from_gl_hd = ((*loc)&0x0000001f); // bits 4-0
                  slot = slot_from_gl_hd;
                  /*
                   * cerr << "1190 GL HD: ev_glhd, slot_glhd " << " "
                   *      << ev_from_gl_hd << " " << slot_from_gl_hd << endl;
                   */
                  nword_mod++;  // nr of module words: includes global headers/trailers + what's in between;
                                // will be cross checked against what's stored in the global trailer, bits 20-5
                  break;

                case 0x08000000 :  // chip header; contains: chip nr., ev. nr, bunch ID
                  chip_nr_hd = ((*loc)&0x03000000) >> 24; // bits 25-24
                  bunch_id = ((*loc)&0x00000fff); // bits 11-0
                  nword_mod++;
                  break;

                case 0x00000000 : // measurement data
                  chan=((*loc)&0x03f80000)>>19; // bits 25-19
                  raw=((*loc)&0x0007ffff); // bits 18-0
                  status = crateslot[idx(roc,slot)]->loadData("tdc",chan,raw,raw);
                  if(status != SD_OK) goto err;
                  //cerr << "(evt: " << event_num << ", slot: "<< slot_from_gl_hd << ") ";
                  //cerr << "chan: " << chan << ",  data: " << raw << endl;
                  nword_mod++;
                  break;

                case 0x18000000 : // chip trailer:  contains chip nr. & ev nr & word count
                  chip_nr_tr = ((*loc)&0x03000000) >> 24; // bits 25-24
                  /* If there is a chip trailer we assume there is a header too so we
                   * cross check if chip nr stored in header matches chip nr. stored in
                   * trailer.
                   */
                  if (chip_nr_tr != chip_nr_hd) {
                    cerr  << "mismatch in chip nr between chip header and trailer"
                          << " " << "header says: " << " " << chip_nr_hd
                          << " " << "trailer says: " << " " << chip_nr_tr << endl;
                  };
                  chip_nr_words = ((*loc)&0x00000fff); // bits 11-0
                  nword_mod++;
                  break;

                case 0x80000000 :  // global trailer: contains error status & word count per module & slot nr.
                  status_err = ((*loc)&0x07000000) >> 24; // bits 26-24
                  if(status_err != 0) {
                    cerr << "(evt: " << event_num << ", slot: "<< slot_from_gl_hd << ") ";
                    cerr << "Error in 1190 status word: " << hex << status_err << dec << endl;
                  }
                  module_nr_words = ((*loc)&0x001fffe0) >> 5; // bits 20-5
                  slot_from_gl_tr = ((*loc)&0x0000001f); // bits 4-0
                  if (slot_from_gl_tr != slot_from_gl_hd) {
                    cerr << "(evt: " << event_num << ", slot: "<< slot_from_gl_hd << ") ";
                    cerr  << "mismatch in slot between global header and trailer"
                          << " " << "header says: " << " " << slot_from_gl_hd
                          << " " << "trailer says: " << " " << slot_from_gl_tr << endl;
                  };
                  nword_mod++;
                  done = true;
                  break;


                default : // Unknown word
                  cerr << "unknown word for TDC1190:  0x" << hex << (*loc) << dec << endl;
                  cerr << "according to global header ev. nr. is: " << " " << ev_from_gl_hd << endl;
                  nword_mod++;
                  break;

              }
              loc++;
              nword++;
            } // end of loop over 1190 data blob from one module

            if (nword_mod != module_nr_words) {
              cerr << "(evt: " << event_num << ", slot: "<< slot_from_gl_hd << ") ";
              cerr << " 1190: mismatch between module word count in decoder"
                  << "(" << nword_mod << ")"
                  << " and the global trailer "
                  << "(" << module_nr_words << ")" << endl;
            }

            p = loc-1;
          }
          break;

	case 250:   // JLab 250MHz FADC
	  {
	    // To get started with this complex module, assume the module is
	    // reporting "Window Raw Data" and decode the samples taken into
	    // "hits" on each channel. Ignore anything else it reports.
	    Int_t len = FADCWindowRawDecode( p, pstop, slot,
					     crateslot[idx(roc,slot)] );
	    p += len-1;
	  }
	  break;

	default:
	  if (fDebug > 0)
	    cout << "Warning: unknown VME model " << model << endl;
	  break;
	} //end switch(model)

	goto SlotDone;  

      } //end if(mask==head)
    } //end for(slot)

    if (fDebug > 1) cout<<"skip word"<<endl;
    continue;

  SlotDone:
    if (fDebug > 1) cout<<"slot done"<<endl;
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
Int_t THaCodaDecoder::camac_decode(Int_t roc, const Int_t* evbuffer, 
				   Int_t ipt, Int_t istop)
{
  assert( evbuffer && fMap );
  if (fDebug > 0) {
    cout << "Sorry, no CAMAC decoding yet !!" << endl;
  }
  return HED_ERR;
}

//_____________________________________________________________________________
Int_t THaCodaDecoder::loadFlag(const Int_t* evbuffer)
{
  // Looks for buffer mode and synch problems.  The latter are recoverable
  // but extremely rare, so I haven't bothered to write recovery a code yet, 
  // but at least this warns you. 
  assert( evbuffer );
  UInt_t word   = *evbuffer;
  UInt_t upword = word & 0xffff0000;
  if (fDebug > 1) {
    cout << "Flag data "<<hex<<word<<dec<<endl;
  }
  if( word == 0xdc0000ff) synchmiss = true;
  if( upword == 0xdcfe0000) {
    synchextra = true;
    Int_t slot = (word&0xf800)>>11;
    Int_t nhit = (word&0x7ff);
    if(fDebug > 0) {
      cout << "THaEvData: WARNING: Fastbus slot ";
      cout << slot << "  has extra hits "<<nhit<<endl;
    }
  }
  if( upword == 0xfabc0000) {
    datascan = *(evbuffer+3);
    if(fDebug > 0 && (synchmiss || synchextra)) {
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

//_____________________________________________________________________________
void THaCodaDecoder::SetRunTime( ULong64_t tloc )
{
  // Set run time and re-initialize crate map (and possibly other
  // database parameters for the new time.

  if( fRunTime == tloc ) 
    return;
  fRunTime = tloc;
  fNeedInit = true;  // force re-init
}

//_____________________________________________________________________________
ClassImp(THaCodaDecoder)
