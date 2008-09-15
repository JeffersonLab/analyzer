/////////////////////////////////////////////////////////////////////
//
//   Documentation may need revision due to class restructuring
//   (THaEvData is now a base class inherited by THaCodaDecoder)
//
//   -Ken Rossato (rossato@jlab.org)
//
//   THaEvData
//   Hall A Event Data from One CODA "Event"
//
//   THaEvData contains facilities to load an event buffer
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
#include "THaHelicity.h"
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

static const int VERBOSE = 1;
static const int DEBUG   = 0;
static const int BENCH   = 0;

THaCodaDecoder::THaCodaDecoder() :
  first_scaler(true),
  numscaler_crate(0)
{
  epics = new THaEpics;
  fb = new THaFastBusWord;
  memset(psfact,0,MAX_PSFACT*sizeof(*psfact));

  EnableScalers();
}

THaCodaDecoder::~THaCodaDecoder() {
  delete epics;
  delete fb;
}

int THaCodaDecoder::GetPrescaleFactor(int trigger_type) const {
// To get the prescale factors for trigger number "trigger_type"
// (valid types are 1,2,3...)
  if ( (trigger_type > 0) && (trigger_type <= MAX_PSFACT)) {
    return psfact[trigger_type - 1];
  }
  if (VERBOSE) {
    cout << "Warning:  Requested prescale factor for ";
    cout << "un-prescalable trigger "<<dec<<trigger_type<<endl;
  }
  return 0;
}

int THaCodaDecoder::LoadEvent(const int* evbuffer, THaCrateMap* cratemap) {
// Public interface to decode the event.  Note, LoadEvent()
// MUST be called once per event BEFORE you can extract 
// information about the event.
// This version of LoadEvent() uses externally provided THaCrateMap
    return gendecode(evbuffer, cratemap);
};

int THaCodaDecoder::gendecode(const int* evbuffer, THaCrateMap* map) {
// Main engine for decoding, called by public LoadEvent() methods
  int ret = HED_OK;
  fMap = map;
     buffer = evbuffer;
     if(DEBUG) dump(evbuffer);    
     if (first_decode) {
       for (int crate=0; crate<MAXROC; crate++)
	 scalerdef[crate] = "nothing";
       init_cmap();     
       if (init_slotdata(map) == HED_ERR) goto err;
       first_decode = false;
     }
     if( fDoBench ) fBench->Begin("clearEvent");
     for( int i=0; i<fNSlotClear; i++ )
       crateslot[fSlotClear[i]]->clearEvent();
     if( fDoBench ) fBench->Stop("clearEvent");
     evscaler = 0;
     event_length = evbuffer[0]+1;  
     event_type = evbuffer[1]>>16;
     if(event_type <= 0) goto err;
     if (event_type <= MAX_PHYS_EVTYPE) {
       event_num = evbuffer[4];
       recent_event = event_num;
       ret = physics_decode(evbuffer, map);
     } else {
       if( fDoBench && event_type != SCALER_EVTYPE ) 
	 fBench->Begin("ctrl_evt_decode");
       event_num = 0;
       switch (event_type) {
	 case PRESTART_EVTYPE :
// Usually prestart is the first 'event'.  Call SetRunTime() to re-initialize 
// the crate map since we now know the run time.  
// This won't happen for split files (no prestart). For such files, 
// the user should call SetRunTime() explicitly.
           SetRunTime(static_cast<UInt_t>(evbuffer[2]));
           run_num  = evbuffer[3];
           run_type = evbuffer[4];
	   evt_time = run_time;
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
	     ret = scaler_event_decode(evbuffer,map);
	   goto exit;
	 case SYNC_EVTYPE :
	 case GO_EVTYPE :
	 case PAUSE_EVTYPE :
	 case END_EVTYPE :
           evt_time = static_cast<UInt_t>(evbuffer[2]);
	   break;
         default:
	   // Ignore unknown event types. Clients will only analyze
	   // known event types anyway.
           break;
       }
       if( fDoBench ) fBench->Stop("ctrl_evt_decode");
     }
     goto exit;
 err:
     ret = HED_ERR;
 exit:
     return ret;
}

void THaCodaDecoder::dump(const int* evbuffer) const {
   int len = evbuffer[0]+1;  
   int type = evbuffer[1]>>16;
   int num = evbuffer[4];
   cout << "\n\n Raw Data Dump  " << hex << endl;
   cout << "\n Event number  " << dec << num;
   cout << "  length " << len << "  type " << type << endl;
   int ipt = 0;
   for (int j=0; j<(len/5); j++) {
     cout << dec << "\n evbuffer[" << ipt << "] = ";
       for (int k=j; k<j+5; k++) {
	   cout << hex << evbuffer[ipt++] << " ";
       }
   }
   if (ipt < len) {
      cout << dec << "\n evbuffer[" << ipt << "] = ";
      for (int k=ipt; k<len; k++) {
	   cout << hex << evbuffer[ipt++] << " ";
      }
      cout << endl;
   }
   cout<<dec<<endl;
}

int THaCodaDecoder::physics_decode(const int* evbuffer, THaCrateMap* map) {
     if( fDoBench ) fBench->Begin("physics_decode");
     int status = HED_OK;
// This decoding is for physics triggers only
     if (evbuffer[3] > MAX_PHYS_EVTYPE) return HED_ERR; 
     memset(rocdat,0,MAXROC*sizeof(RocDat_t));
     // n1 = pointer to first word of ROC
     int pos = evbuffer[2]+3;
     int nroc = 0;
     int irn[MAXROC];   // Lookup table i-th ROC found -> ROC number
     while( pos+1 < event_length && nroc < MAXROC ) {
       int len  = evbuffer[pos];
       int iroc = (evbuffer[pos+1]&0xff0000)>>16;
       if(iroc>=MAXROC) {
         if(VERBOSE) { 
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
     for( int i=0; i<nroc; i++ ) {
       int iroc = irn[i];
       const RocDat_t* proc = rocdat+iroc;
       int ipt = proc->pos + 1;
       int iptmax = proc->pos + proc->len;
       if (map->isFastBus(iroc)) {
	 status = fastbus_decode(iroc,map,evbuffer,ipt,iptmax);
	 if(status == HED_ERR) return HED_ERR;
       } else if (map->isVme(iroc)) {
	 status = vme_decode(iroc,map,evbuffer,ipt,iptmax);
	 if(status == HED_ERR) return HED_ERR;
       } else if (map->isCamac(iroc)) {
	 status = camac_decode(iroc,map,evbuffer,ipt,iptmax);
	 if(status == HED_ERR) return HED_ERR;
       }
     }
     if( HelicityEnabled()) {
       if( !helicity ) {
	 helicity = new THaHelicity;
	 if( !helicity ) return HED_ERR;
       }
       if(helicity->Decode(*this) != 1) return HED_ERR;
       dhel = (Double_t)helicity->GetHelicity();
       dtimestamp = (Double_t)helicity->GetTime();
     }
     return HED_OK;
}

int THaCodaDecoder::epics_decode(const int* evbuffer) {
     if( fDoBench ) fBench->Begin("epics_decode");
     epics->LoadData(evbuffer, recent_event);
     if( fDoBench ) fBench->Stop("epics_decode");
     return HED_OK;
};

int THaCodaDecoder::prescale_decode(const int* evbuffer) {
// Decodes prescale factors from either
// TS_PRESCALE_EVTYPE(default) = PS factors 
// read from Trig. Super. registors (since 11/03)
//   - or -
// PRESCALE_EVTYPE = PS factors from traditional
//     "prescale.dat" file.
// To cause THaEvData select one or the other,
// use SetOrigPS() method.

  const int HEAD_OFF1 = 2;
  const int HEAD_OFF2 = 4;
  int type = evbuffer[1]>>16;
  int len  = evbuffer[0]+1; // in longwords (4 bytes)
  // TS registers -->
  if (/* fgTrigSupPS && */ type == TS_PRESCALE_EVTYPE) {
    // this is more authoritative
    for (int j = 0; j < 8; j++) {
      int k = j + HEAD_OFF1;
      int ps = 0;
      if (k < len) {
	ps = evbuffer[k];
	if (psfact[j]!=0 && ps != psfact[j]) {
	  Warning("prescale_decode","Mismatch in prescale factor: Trig %d  oldps %d   TS_PRESCALE %d. Setting to TS_PRESCALE",
		  j+1,psfact[j],ps);
	}
      }
      psfact[j]=ps;
      if (DEBUG) cout << "%% TS psfact "<<dec<<j<<"  "<<psfact[j]<<endl;
    }        
  } 
  // "prescale.dat" -->
  static const char* const pstr[] = { "ps1", "ps2", "ps3", "ps4",
				      "ps5", "ps6", "ps7", "ps8",
				      "ps9", "ps10", "ps11", "ps12" };
  if ( /* !fgTrigSupPS && */ type == PRESCALE_EVTYPE) {  
    if( len <= HEAD_OFF2 )
      return HED_ERR;  //oops, event too short?
    THaUsrstrutils sut;
    sut.string_from_evbuffer(evbuffer+HEAD_OFF2, len-HEAD_OFF2);
    for(int trig=0; trig<MAX_PSFACT; trig++) {
      int ps =  sut.getint(pstr[trig]);
      int psmax = 65536; // 2^16 for trig > 3
      if (trig < 4) psmax = 16777216;  // 2^24 for 1st 4 trigs
      if (trig > 7) ps = 1;  // cannot prescale trig 9-12
      ps = ps % psmax;
      if (psfact[trig]==0)
	psfact[trig] = ps;
      else if (ps != psfact[trig]) {
	Warning("prescale_decode","Mismatch in prescale factor: Trig %d  oldps %d   prescale.dat %d, Keeping old value",
		trig+1,psfact[trig],ps);
      }
      if (DEBUG) cout << "** psfact[ "<<trig+1<< " ] = "<<psfact[trig]<<endl;
    }
  }
  // Ok in any case
  return HED_OK;  
}

int THaCodaDecoder::scaler_event_decode(const int* evbuffer, THaCrateMap* map) 
{
  // Decode scalers
      int type = evbuffer[1]>>16;
      if (type != SCALER_EVTYPE) return HED_ERR;
      if( fDoBench ) fBench->Begin("scaler_event_decode");
      if (DEBUG) cout << "Scaler decoding"<<endl;
      if (first_scaler) {
        first_scaler = kFALSE;
        numscaler_crate = 0;
	for (int roc=0; roc<MAXROC; roc++) {
          if (map->isScalerCrate(roc)) scaler_crate[numscaler_crate++] = roc;
	}
      }
      for(int cra=0; cra<numscaler_crate; cra++) {       
        int roc = scaler_crate[cra];
        rocdat[roc].len  = 0;
      }
      int ret = HED_OK;
      int ipt = -1;
      while (++ipt < event_length) {
	UInt_t headerword = evbuffer[ipt];
        int roc = map->getScalerCrate(headerword);
        if (DEBUG) {
	  cout << "ipt "<<ipt<<" evbuffer "<<hex<<headerword<<dec;
  	  cout <<" evlen "<<event_length<<" roc "<<roc<<endl;
	}
        if (!roc) continue;               
	const char* location = map->getScalerLoc(roc);
	if( scalerdef[roc] == "nothing" ) {
	  if      (!strcmp(location,"rscaler")) scalerdef[roc] = "right";
	  else if (!strcmp(location,"lscaler")) scalerdef[roc] = "left";
	  else if (!strcmp(location,"rcs"))     scalerdef[roc] = "rcs";
	}
        rocdat[roc].len++;
// If more locations added, put them here.  But scalerdef[] is not
// needed if you know the roc#, you can call getScaler(roc,...)
        int slot = (headerword&0xf0000)>>16; // 0<=slot<=15
        int numchan = headerword&0xff;
        if (DEBUG) cout<<"slot "<<slot<<" numchan "<<numchan<<endl;
	int ics = idx(roc,slot);
	crateslot[ics]->clearEvent();
	for (int chan=0; chan<numchan; chan++) {
	  ipt++; 
          rocdat[roc].len++;
	  int data = evbuffer[ipt];
	  if (DEBUG) cout<<"scaler chan "<<chan<<" data "<<data<<endl;
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

int THaCodaDecoder::GetScaler(const TString& spec, int slot, int chan) const {
// Get scaler data by spectrometer, slot, and channel
// spec = "left" or "right" for the "scaler events" (type 140)
// and spec = "evleft" and "evright" for scalers
// that are part of the event (ev) readout.
    for(int cra=0; cra<numscaler_crate; cra++) {       
      int roc = scaler_crate[cra];
      if (spec == scalerdef[roc]) return GetScaler(roc, slot, chan);
    }
    return 0;
}

int THaCodaDecoder::GetScaler(int roc, int slot, int chan) const {
// Get scaler data by roc, slot, chan.
  if( GoodIndex(roc,slot) && scalerdef[roc] != "nothing")
    return crateslot[idx(roc,slot)]->getData(chan,0);
  return 0;
}

Bool_t THaCodaDecoder::IsLoadedEpics(const char* tag) const {
  return epics->IsLoaded(tag);
}

double THaCodaDecoder::GetEpicsData(const char* tag, int event) const {
// EPICS data which is nearest CODA event# 'event'
// event == 0 --> get latest data
  return epics->GetData(tag, event);
}

string THaCodaDecoder::GetEpicsString(const char* tag, int event) const {
// EPICS string data which is nearest CODA event# 'event'
// event == 0 --> get latest data
  return epics->GetString(tag, event);
}

double THaCodaDecoder::GetEpicsTime(const char* tag, int event) const {
// EPICS time stamp
// event == 0 --> get latest data
  return epics->GetTimeStamp(tag, event);
}

int THaCodaDecoder::fastbus_decode(int roc, THaCrateMap* map,
          const int* evbuffer, int istart, int istop) {
    if( fDoBench ) fBench->Begin("fastbus_decode");
    int slotold = -1;
    const int* p     = evbuffer+istart;
    const int* pstop = evbuffer+istop;
    synchmiss = false;
    synchextra = false;
    buffmode = false;
    int xctr = 0;
    int nspflag = 0;
    if (DEBUG) cout << "Fastbus roc  "<<roc<<endl;
    while ( p++ < pstop ) {  
       xctr++;
       if(DEBUG) {
          cout << "evbuffer  " <<(p-evbuffer)<<"   ";
          cout << hex << *p << dec << endl;
       }
       int slot = fb->Slot(*p);
       if (!slot) {
         if ((*p & 0xffff0000)==0xfabc0000) {
	   xctr = 0;
           nspflag = (*p & 0xff);  // number of special flags.
	 }
         loadFlag(p);
         continue;
       }
       if (nspflag > 0 && xctr <= nspflag) continue;  // skip special flags
       int model = map->getModel(roc,slot);
       if (model == THaCrateMap::CM_ERR) continue;
       if (!model) {
	 if (VERBOSE) {
           cout << "Warning: Undefined module in data" << endl;
           cout << "roc " << roc << "   slot " << slot << endl;
	 }
         continue;
       }
       if (DEBUG) {
         cout<<"slot slotold model "<<slot;
         cout<<"  "<<slotold<<"  "<<model<<endl;
       }
       if (slot != slotold) {            
          slotold = slot;
          if (fb->HasHeader(model)) {
             int n = fb->Wdcnt(model,*p);
             if (n == THaFastBusWord::FB_ERR) {
	       if( fDoBench ) fBench->Stop("fastbus_decode");
	       return HED_ERR;
	     }
	     if (DEBUG) cout << "header, wdcnt = "<<n<<endl;
             if (n <= 1) continue;
	     p++;
          }
       }
       int chan = fb->Chan(model,*p);
       int data = fb->Data(model,*p);
       if (DEBUG) {
	 printf("roc %2d  slot %3d  chan %3d  data %5d  ipt %3d"
		"  raw %8x  device %s\n",
		roc, slot, chan, data, static_cast<int>(p-evbuffer), 
		*p, fb->devType(model));
       }
       // At this point, roc and slot ranges have been checked
       if( crateslot[idx(roc,slot)]->loadData(fb->devType(model),chan,data,*p) 
	  == SD_ERR) {
	 if( fDoBench ) fBench->Stop("fastbus_decode");
	 return HED_ERR;
       }
    }
    if( fDoBench ) fBench->Stop("fastbus_decode");
    return HED_OK;
}

int THaCodaDecoder::vme_decode(int roc, THaCrateMap* map, const int* evbuffer,
          int ipt, int istop)  {
    if( fDoBench ) fBench->Begin("vme_decode");
    int slot,chan,raw,data,slotprime,ndat,head,mask,nhit;
    int Nslot = map->getNslot(roc);
    int retval = HED_OK;
    const int* p      = evbuffer+ipt;
    const int* pstop  = evbuffer+istop;
    const int* pevlen = evbuffer+event_length;
    const int* loc;

    if (DEBUG) cout << "VME roc "<<dec<<roc<<" nslot "<<Nslot<<endl;
    if (Nslot <= 0) goto err;
    scalerdef[roc] = map->getScalerLoc(roc);
    map->setSlotDone();
    if (map->isScalerCrate(roc) && GetRocLength(roc) >= 16) evscaler = 1;
    while ( p++ < pstop ) {
      if(DEBUG) cout << "evbuff "<<(p-evbuffer)<<"  "<<hex<<*p<<dec<<endl;
      // look through all slots, since Nslot only gives number of occupied slots,
      // not the highest-numbered occupied slot.
      for (slot=1; slot<=MAXSLOT; slot++) {
	if (!map->slotUsed(roc,slot)) continue; 
	if (map->slotDone(slot)) continue;
	head = map->getHeader(roc,slot);
	mask = map->getMask(roc,slot);
	if(DEBUG) {
	  cout<<"slot head mask "<<slot<<"  ";
	  cout<<hex<<head<<"  "<<mask<<dec<<endl;
	}
	if (((*p)&mask) == head) {
          map->setSlotDone(slot);

	  int model = map->getModel(roc,slot);
	  if (DEBUG) cout<<"model " << model << flush;
	  switch(model) {
	  case 1182:    // LeCroy 1182 ADC
	    for (chan=0; chan<8; chan++) {
              if( ++p >= pevlen ) goto SlotDone;  
              if(DEBUG) {
                cout<<"1182 chan data "<<chan<<" 0x"<<hex<<*p<<dec<<endl;
              }
              if( crateslot[idx(roc,slot)]->loadData("adc",chan,*p,*p)
                  == SD_ERR) goto err;
 	    }
	    break;
	  case 7510:    // Struck 7510 ADC
	    nhit = ((*p)&0xfff)/8;
	    if (DEBUG) cout << "nhit 7510 " << nhit << endl;
	    for (chan=0; chan<8; chan++) {
	      for (int j=0; j<nhit/2; j++) {  // nhit must be even
		if( ++p >= pevlen ) goto SlotDone;
		if(DEBUG)  cout<<"7510 raw  0x"<<hex<<*p<<dec<<endl;
		if( crateslot[idx(roc,slot)]
		    ->loadData("adc",chan,((*p)&0x0fff0000)>>16,*p)
		    ==  SD_ERR) goto err;
		if( crateslot[idx(roc,slot)]
		    ->loadData("adc",chan,((*p)&0xfff),*p)
		    == SD_ERR) goto err;
	      }
	    }
	    break;
	  case 3123:    // VMIC 3123 ADC
	    for (chan=0; chan<16; chan++) {
              if( ++p >= pevlen ) goto SlotDone;
              if(DEBUG) {
                cout<<"3123 chan data "<<chan<<"  0x"<<hex<<*p<<dec<<endl;
              }
              if( crateslot[idx(roc,slot)]->loadData("adc",chan,*p,*p) 
		  == SD_ERR) goto err;
	    }
	    break;
// Note, although there may be scalers in physics events, the
// "scaler events" dont come here.  See scaler_event_decode().
	  case 1151:    // LeCroy 1151 scaler
	    for (chan=0; chan<16; chan++) {
              if( ++p >= pevlen ) goto SlotDone;
              if(DEBUG) {
                cout<<"1151 chan data "<<chan<<"  0x"<<hex<<*p<<dec<<endl;
              }
              if( crateslot[idx(roc,slot)]
		  ->loadData("scaler",chan,*p,*p) == SD_ERR) goto err;
	    }
	    break;
// The CAEN 560 is a little tricky; sometimes only 1 channel was read,
// so we don't increment ipt. (hmmm... could use time-dep crate map.)
	  case 560:     // CAEN 560 scaler
            loc = p;
	    for (chan=0; chan<16; chan++) {  
              if( ++loc >= pevlen ) goto SlotDone; 
              if(DEBUG) {
                cout<<"560 chan data "<<chan<<"  0x"<<hex<<*loc<<dec<<endl;
              }
              if( crateslot[idx(roc,slot)]
		  ->loadData("scaler",chan,*loc,*loc) == SD_ERR) goto err;
	    }
	    break;
	  case 3801:    // Struck 3801 scaler
	    for (chan=0; chan<32; chan++) {
              if( ++p >= pevlen) goto SlotDone;
              if(DEBUG) {
                cout<<"3801 chan data "<<chan<<"  0x"<<hex<<*p<<dec<<endl;
              }
              if( crateslot[idx(roc,slot)]->loadData("scaler",chan,*p,*p)
	          == SD_ERR) goto err;
	    }
	    break;
	  case 7353:    // BRs hack for the trigger module
	    chan=0;
            raw = (*p)&0xfff;
	    if(DEBUG) {
	      cout<<"7353 chan data "<<chan<<"  0x"<<hex<<raw<<dec<<endl;
	    }
	    if( crateslot[idx(roc,slot)]->loadData("register",chan,raw,raw)
		== SD_ERR) goto err;
	    break;
	  case 550:     // CAEN 550 for RICH 
	    // ('slot' was called 'chan' in Fortran)
            slotprime = 1+(((*p)&0xff0000)>>16);  
            if (DEBUG) cout << "Rich slot "<<slot<<" "
			    <<slotprime<<" "<<loc<<" "<<hex<<*p<<dec<<endl; 
            if (slot == slotprime) {    // They should agree, else problem.
              ndat = (*p)&0xfff;
              if (DEBUG) cout << "Rich ndat = "<<ndat<<endl;
	      if (p+ndat>pstop) {
		p=pstop;
		// FIXME : in this case a warning should apear
		//          the buffer/data must be corrupted
	      }
	      else {
		loc = p;
		while( ++loc <= p+ndat ) {
		  chan = ((*loc)&0x7ff000)>>12;    
		  // channel number (was called 'wire' in Fortran)
		  raw  = (*loc)&0xffffffff;
		  data = (*loc)&0x0fff;           
		  if (DEBUG) cout << "channel "<<chan<<"  raw data "<<raw<<endl;
		  if( crateslot[idx(roc,slot)]->loadData("adc",chan,data,raw) 
		      == SD_ERR) goto err;
		}
		p += ndat;
	      }
	    }
	    break;
	  case 775:     // CAEN 775 TDC
	  case 792:     // CAEN 792 QDC
	    {
	      ++p; 
	      int nword=*p-2;
	      ++p;
	      bool is775 = (model == 775);
	      for (int i=0;i<nword;i++) {
		++p;
		chan=((*p)&0x00ff0000)>>16;
		raw=((*p)&0x00000fff);	      
		if (is775) {
		  if (crateslot[idx(roc,slot)]->loadData("tdc",chan,raw,raw)
		      == SD_ERR) return HED_ERR;
		} else {
		  //	      if (map->getModel(roc,slot) == 792) {
		  if (crateslot[idx(roc,slot)]->loadData("adc",chan,raw,raw)
		      == SD_ERR) return HED_ERR;
		}
	      }
	      ++p;
	    }
	    break;
	  case 6401:      // JLab F1 normal resolution mode
	  case 3201:      // JLab F1 high resolution mode
	    {
	      // CAUTION: this routine does re-number the channels
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
              // be usefull when multiblock mode (buffered mode) is used
	      const int F1_HIT_OFLW = 1<<24; // bad
	      const int F1_OUT_OFLW = 1<<25; // bad
	      const int F1_RES_LOCK = 1<<26; // good
	      const int DATA_CHK = F1_HIT_OFLW | F1_OUT_OFLW | F1_RES_LOCK;
	      const int DATA_MARKER = 1<<23;

	      // look at all the data
	      loc = p;
	      while ((loc <= pevlen) && ((*loc)&0xf8000000)==(head&0xf8000000)) {
		if ( ((*loc) & DATA_CHK) != F1_RES_LOCK ) {
		  cout << "Warning: F1 TDC " << hex << (*loc) << dec;
		  if ( (*loc) & F1_HIT_OFLW ) {
		    cout << " Hit-FIFO overflow";
		  }
		  if ( (*loc) & F1_OUT_OFLW ) {
		    cout << " Output FIFO overflow";
		  }
		  if ( ! ((*loc) & F1_RES_LOCK ) ) {
		    cout << " Resolution lock failure!";
		  }
		  cout << endl;
		}
		
		if ( !( (*loc) & DATA_MARKER ) ) {
		  // header/trailer word, to be ignored
		  if(DEBUG)
		    cout<< "[" << (loc-evbuffer) << "] header/trailer  0x"<<hex<<*loc<<dec<<endl;
		} else {
		  if (DEBUG)
		    cout<< "[" << (loc-evbuffer) << "] data            0x"<<hex<<*loc<<dec<<endl;
		  int chn = ((*loc)>>16) & 0x3f;  // internal channel number

		  chan =0;
		  if (model==6401) {        // normal resolution
		    // do the reordering of the channels, for contiguous groups
		    // odd numbered TDC channels from the board -> +16
		    chan = (chn & 0x20) + 16*(chn & 0x01) + ((chn & 0x1e)>>1);
		  } else if (model==3201) { // high resolution
		    // drop last bit for channel renumbering
		    chan=(chn >> 1);
		  }
		  raw= (*loc) & 0xffff;
		  if(DEBUG) {
		    cout<<" int_chn chan data "<<dec<<chn<<"  "<<chan
			<<"  0x"<<hex<<raw<<dec<<endl;
		  }
		  if (crateslot[idx(roc,slot)]->loadData("tdc",chan,raw,raw)
		      == SD_ERR) {
		    if (DEBUG) {
		      cout<<"Error found loadData tdc roc/slot/chan data"
			  <<dec<< roc << "  " << slot << "  " << chan 
			  <<"  0x"<<hex<<raw<<endl;
		      return HED_ERR;
		    }
		  }
		}
		loc++;
	      }
	      p=loc-1;  // point to the previous position, so loop goes to current loc
	      if (DEBUG) {
		cout << "Exiting at [" <<dec<<(loc-evbuffer) <<"]  0x"<<hex<<*loc<<dec<<endl;
	      }
	    }
	    break;
	  case 767:   // CAEN 767 MultiHit TDC
	    {
	      loc = p;
	      loc++;  // skip first word (header)
	      int nword=0;
	      while ( (loc <= pevlen)&& ((*loc)&0x00600000)==0) {
		chan=((*loc)&0x7f000000)>>24;
		raw=((*loc)&0x000fffff);	      
		if (crateslot[idx(roc,slot)]->loadData("adc",chan,raw,raw)
		    == SD_ERR) return HED_ERR;
		loc++;
		nword++;
	      }
	      if (((*loc)&0x00600000)!=0x00200000) {
		return HED_ERR;
	      } else {
		if (((*loc)&0xffff)!=nword) {
		  if (DEBUG) cout<<"WC mismatch "<<nword<<" "<<hex<<(*p)<<endl;
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
	  default:
	    break;
	  } //end switch(model)

          goto SlotDone;  

	} //end if(mask==head)
      } //end for(slot)

SlotDone:
      if (DEBUG) cout<<"slot done, or skip word"<<endl;
    } //end while(p++<pstop)
    goto exit;

err:
    retval = HED_ERR;
exit:
    if( fDoBench ) fBench->Stop("vme_decode");
    return retval;
}

int THaCodaDecoder::camac_decode(int roc, THaCrateMap* map, const int* evbuffer, 
			    int ipt, int istop) {
   if (VERBOSE) {
     cout << "Sorry, no CAMAC decoding yet !!" << endl;
   }
   return HED_OK;
}

int THaCodaDecoder::loadFlag(const int* evbuffer) {
// Looks for buffer mode and synch problems.  The latter are recoverable
// but extremely rare, so I haven't bothered to write recovery a code yet, 
// but at least this warns you. 
  UInt_t word   = *evbuffer;
  UInt_t upword = word & 0xffff0000;
  if (DEBUG) {
    cout << "Flag data "<<hex<<word<<dec<<endl;
  }
  if( word == 0xdc0000ff) synchmiss = true;
  if( upword == 0xdcfe0000) {
    synchextra = true;
    int slot = (word&0xf800)>>11;
    int nhit = (word&0x7ff);
    if(VERBOSE) {
      cout << "THaEvData: WARNING: Fastbus slot ";
      cout << slot << "  has extra hits "<<nhit<<endl;
    }
  }
  if( upword == 0xfabc0000) {
      datascan = *(evbuffer+3);
      if(VERBOSE && (synchmiss || synchextra)) {
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

void THaCodaDecoder::SetRunTime( UInt_t tloc )
{
  // Set run time and re-initialize crate map (and possibly other
  // database parameters for the new time.

  if( run_time == tloc ) 
    return;
  run_time = tloc;
  init_cmap();     
  init_slotdata(fMap);
}

ClassImp(THaCodaDecoder)
