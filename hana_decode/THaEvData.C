/////////////////////////////////////////////////////////////////////
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
//   author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////


#include <iostream>
#include "THaEvData.h"
#include "THaHelicity.h"
#ifndef STANDALONE
#include "THaVarList.h"
#include "THaInterface.h"
#endif
#include "TError.h"
#include <string>

ClassImp(THaEvData)

const int THaEvData::HED_OK  =  1;
const int THaEvData::HED_ERR = -1;

THaEvData::THaEvData() :
  first_load(true), first_scaler(true), first_decode(true),
  numscaler_crate(0), run_num(0), run_type(0), run_time(0)
{
  buffer = 0;
  epics = new THaEpicsStack;
  epics->setupDefaultList();
  for (int i=0; i<MAX_PSFACT; i++) psfact[i] = 0;
  crateslot = new THaSlotData[MAXROC*MAXSLOT];
  helicity = new THaHelicity;

#ifndef STANDALONE
// Register global variables. 
  if( gHaVars ) {
    VarDef vars[] = {
      { "runnum",    "Run number",     kInt,    0, &run_num },
      { "runtype",   "CODA run type",  kInt,    0, &run_type },
      { "runtime",   "CODA run time",  kUInt,   0, &run_time },
      { "evnum",     "Event number",   kInt,    0, &event_num },
      { "evtyp",     "Event type",     kInt,    0, &event_type },
      { "evlen",     "Event Length",   kInt,    0, &event_length },
      { "helicity",  "Beam helicity",  kDouble, 0, &dhel },
      { "timestamp", "Timestamp",      kDouble, 0, &dtimestamp },
      { 0 }
    };
    gHaVars->DefineVariables( vars, "g.", "THaEvData::THaEvData" );
  } else
    Warning("THaEvData::THaEvData","No global variable list found. "
	    "Variables not registered.");
#endif

};


THaEvData::~THaEvData() {
  delete [] crateslot;  
  delete epics;
  delete helicity;
#ifndef STANDALONE
  if( gHaVars )
    gHaVars->RemoveRegexp( "g.*" );
#endif
};

void THaEvData::PrintSlotData(int crate, int slot) {
// Print the contents of (crate, slot).
  if ( (crate >= 0) && (crate < MAXROC) && 
       (slot >= 0) && (slot < MAXSLOT) ) {
       crateslot[idx(crate,slot)].print();
  } else {
      cout << "THaEvData: Warning: Crate, slot combination";
      cout << "\nexceeds limits.  Cannot print"<<endl;
  }
  return;
};     

TString THaEvData::DevType(int crate, int slot) const {
// Device type in crate, slot
  if ( (crate >= 0) && (crate < MAXROC) && 
       (slot >= 0) && (slot < MAXSLOT) ) {
      return crateslot[idx(crate,slot)].devType();
  }
  return " ";
};

int THaEvData::GetRocLength(int crate) const {
  if (crate >= 0 && crate < MAXROC) return lenroc[crate];
  return 0;
};

int THaEvData::GetPrescaleFactor(int trigger_type) const {
// To get the prescale factors for trigger number "trigger_type"
// (valid types are 1,2,3...)
  if ( (trigger_type > 0) && (trigger_type <= MAX_PSFACT)) {
        return psfact[trigger_type - 1];
  };
  if (VERBOSE) {
       cout << "Warning:  Requested prescale factor for ";
       cout << "un-prescalable trigger "<<dec<<trigger_type<<endl;
  }
  return 0;
};

int THaEvData::gendecode(const int* evbuffer, THaCrateMap& map) {
// Main engine for decoding, called by public LoadEvent() methods
     buffer = evbuffer;
     if(DEBUG) dump(evbuffer);    
     if (first_decode) {
       if (init_slotdata(map) == HED_ERR) return HED_ERR;
       first_decode = false;
     }
     for (int roc=0; roc<MAXROC; roc++) {
       lenroc[roc] = 0;
       if (!map.crateUsed(roc)) continue;
       for (int slot=0; slot<MAXSLOT; slot++) {
          if (!map.slotUsed(roc,slot)) continue;
 	  if (!map.slotClear(roc,slot)) continue;
          crateslot[idx(roc,slot)].clearEvent();  // clear hit counters
       }
     }
     for (int roc = 0; roc < MAXROC; roc++) {
          n1roc[roc] = 0;
          lenroc[roc] = 0;
          irn[roc]=0;
     }
     evscaler = 0;
     event_length = evbuffer[0]+1;  
     event_type = evbuffer[1]>>16;
     if(event_type <= 0) return HED_ERR;
     if (event_type <= MAX_PHYS_EVTYPE) {
       event_num = evbuffer[4];
       recent_event = event_num;
       return physics_decode(evbuffer, map);
     } else {
       event_num = 0;
       switch (event_type) {
	 case PRESTART_EVTYPE :
           run_time = static_cast<UInt_t>(evbuffer[2]);
           run_num  = evbuffer[3];
           run_type = evbuffer[4];
// Usually prestart is the first 'event'.  Re-initialize crate map since we
// now know the run time.  This won't happen for split files (no prestart).
           init_cmap();     
           init_slotdata(cmap);
           return HED_OK;
	 case GO_EVTYPE :
           return HED_OK;
	 case PAUSE_EVTYPE :
           return HED_OK;
	 case END_EVTYPE :
           return HED_OK;
         case EPICS_EVTYPE :
           return epics_decode(evbuffer);
         case PRESCALE_EVTYPE :
           return prescale_decode(evbuffer);
         case TRIGGER_FILE :
 	   return HED_OK;
         case DETMAP_FILE :
 	   return HED_OK;
         case SCALER_EVTYPE :
           return scaler_event_decode(evbuffer,map);
         default:
           return HED_ERR;
       }
     }
};
     

void THaEvData::dump(const int* evbuffer) {
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
};

int THaEvData::physics_decode(const int* evbuffer, THaCrateMap& map) {
     int status = HED_OK;
     int i,roc,ipt,n1,numroc,lentot;
     numroc = 0;
// This decoding is for physics triggers only
     if (evbuffer[3] > MAX_PHYS_EVTYPE) return HED_ERR; 
// Split up the ROCs
     lentot = 0;
     for (i=0; i<MAXROC; i++) {
       // n1 = pointer to first word of ROC
       if(i==0) {
         n1 = evbuffer[2]+3;
       } else {
         n1 = n1roc[irn[i-1]]+lenroc[irn[i-1]]+1;
       }
       if( (n1+1) >= event_length ) break;
       // irn = ROC number 
       irn[i]=(evbuffer[n1+1]&0xff0000)>>16;
       if(irn[i]<0 || irn[i]>=MAXROC) {
         if(VERBOSE) { 
  	   cout << "ERROR in THaEvData::physics_decode:";
	   cout << "  illegal ROC number " <<dec<<irn[i]<<endl;
	 }
         return HED_ERR;
       }
       if (i == 0) {
         n1roc[irn[i]] = n1;
         lenroc[irn[i]] = evbuffer[n1];
         lentot = n1 + evbuffer[n1];
       } else {
         n1roc[irn[i]]=
            n1roc[irn[i-1]]+lenroc[irn[i-1]]+1;
         lenroc[irn[i]] = evbuffer[n1roc[irn[i]]];
         lentot  = lentot + lenroc[irn[i]] + 1;
       } 
       numroc++;
       if(DEBUG) {
           cout << "Roc ptr " <<dec<<numroc<<"  "<<i+1<<"  ";
           cout <<irn[i]<<"  "<<n1roc[irn[i]]<<"  "<<lenroc[irn[i]];
           cout <<"  "<<lentot<<"  "<<event_length<<endl;
       }
       if (lentot >= event_length) break;
     }
// Decode each ROC
     for (roc=0; roc<numroc; roc++) {
       ipt = n1roc[irn[roc]]+1;
       int iptmax = n1roc[irn[roc]]+lenroc[irn[roc]];
       if (map.isFastBus(irn[roc])) {
            status = fastbus_decode(irn[roc],map,evbuffer,ipt,iptmax);
            if(status == HED_ERR) return HED_ERR;
         } else if (map.isVme(irn[roc])) {
            status = vme_decode(irn[roc],map,evbuffer,ipt,iptmax);
            if(status == HED_ERR) return HED_ERR;
         } else if (map.isCamac(irn[roc])) {
            status = camac_decode(irn[roc],map,evbuffer,ipt,iptmax);
            if(status == HED_ERR) return HED_ERR;
	 }
     }
     if(helicity->Decode(*this) != 1) return HED_ERR;
     dhel = (Double_t)helicity->GetHelicity();
     dtimestamp = (Double_t)helicity->GetTime();
     return HED_OK;
};

int THaEvData::AddEpicsTag(const TString& tag) {
// Add EPICS variable to the list you want decoded
// (though a reasonable list already exists by default).
  return epics->addEpicsTag(tag);
};

int THaEvData::epics_decode(const int* evbuffer) {
     static const int DEBUGL = 0;
     static const int MAX  = 5000;     
     static const int MAXEPV = 40;   
     int j,m,pos1,pos2,neword;
     bool foundVar = false;
     bool startNum = false;
     bool stopNum = false;
     union cidata {
        int ibuff[MAX];
        char cbuff[MAX];
     } evbuff; 
     char wtag[MAXEPV],wval[MAXEPV];
     int len = (sizeof(int)/sizeof(char))*(evbuffer[0]+1);  
     int nlen = (len < MAX) ? len : MAX;   
     for (j=0; j < nlen; j++) evbuff.ibuff[j] = evbuffer[j];
     if(DEBUGL) cout << "epics_decode  --- length " <<dec<<nlen<<endl;
     neword = 0;
     pos1 = 0;
     for(j = 0; j < nlen; j++) {
       if(DEBUGL) cout << "cbuff["<<j<<"]= "<<evbuff.cbuff[j]<<endl;
       pos2 = j;
       if (!foundVar) {
	  if (evbuff.cbuff[j] == ' ' || evbuff.cbuff[j] == '\n') {
             pos1 = j+1;
             continue;
	  }
          if (pos2 >= pos1 && pos2-pos1 < MAXEPV-1) {
             for (m = 0; m <= pos2-pos1; m++) wtag[m] = evbuff.cbuff[pos1+m];
             wtag[m+1] = '\0'; 
          } else {
             cout << "THaEvData:epics_decode: WARNING: truncation of epics tag"<<endl;
          }
          if(DEBUGL) cout << "checking tag " << m << " " << wtag << endl;
          if (epics->findVar(wtag) >= 0) {
	     if(DEBUGL) cout << "Valid epics variable found ! " << endl;
             pos1 = j+1;
             foundVar = true;   
             startNum = false;
             stopNum = false;
          }
       } else {
          if (evbuff.cbuff[j] == ' ' || evbuff.cbuff[j] == '\n') {
             if ( startNum ) stopNum = true;
	  } else {
             startNum = true;
             if (pos2 >= pos1 && pos2-pos1 < MAXEPV-1) {
               for (m = 0; m <= pos2-pos1; m++) wval[m] = evbuff.cbuff[pos1+m];
               wval[m+1] = '\0'; 
             } else {
                cout << "THaEvData:epics_decode: WARNING: truncation of epics val"<<endl;
            }
	  }
          if (stopNum) {
             if (DEBUGL) cout << "loading data "<<wtag<<"  val = "<<wval<<endl;
             epics->loadData(wtag,wval,recent_event);
  	     pos1 = j+1;      // found end of number
             foundVar = false;
	  }
       } 
     }
     epics->bumpStack();
     if(DEBUGL) epics->print();
     return HED_OK;
};

int THaEvData::prescale_decode(const int* evbuffer) {
     const int MAX = 5000;
     const int HEAD_OFF = 4;
     char temp1[20];
     int j,trig,data[MAX];
     string pstr[] = { "ps1", "ps2", "ps3", "ps4",
                       "ps5", "ps6", "ps7", "ps8",
                       "ps9", "ps10", "ps11", "ps12" };
     int type = evbuffer[1]>>16;
     if (type != PRESCALE_EVTYPE) return HED_ERR;
     int len = (sizeof(int)/sizeof(char))*(evbuffer[0]+1);  
     int nlen = (len < MAX) ? len : MAX;   
     for (j=HEAD_OFF; j<nlen; j++) data[j-HEAD_OFF] = evbuffer[j];  
     THaUsrstrutils sut;
     sut.string_from_evbuffer(data);
     for(trig=0; trig<MAX_PSFACT; trig++) {
        strcpy(temp1,pstr[trig].c_str());        
        psfact[trig] = sut.getint(temp1);
        int psmax = 65536; // 2^16 for trig > 3
	if (trig < 4) psmax = 16777216;  // 2^24 for 1st 4 trigs
        if (trig > 7) psfact[trig] = 1;  // cannot prescale trig 9-12
        psfact[trig] = psfact[trig] % psmax;
        if (psfact[trig] == 0) psfact[trig] = psmax;
        if (DEBUG) cout << "psfact[ "<<trig+1<< " ] = "<<psfact[trig]<<endl;
     }
     return HED_OK;
}

int THaEvData::scaler_event_decode(const int* evbuffer, THaCrateMap& map) {
      int roc;
      int type = evbuffer[1]>>16;
      if (type != SCALER_EVTYPE) return HED_ERR;
      if (DEBUG) cout << "Scaler decoding"<<endl;
      if (first_scaler) {
        first_scaler = kFALSE;
        numscaler_crate = 0;
	for (roc=0; roc<MAXROC; roc++) {
          if (map.isScalerCrate(roc)) scaler_crate[numscaler_crate++] = roc;
	}
      }
      int ipt = -1;
      while (ipt++ < event_length) {
        if (ipt >= event_length) break;
        roc = map.getScalerCrate(evbuffer[ipt]);
        if (DEBUG) {
	  cout << "ipt "<<dec<<ipt<<" evbuffer "<<hex<<evbuffer[ipt];
  	  cout <<" evlen "<<dec<<event_length<<" roc "<<roc<<endl;
	}
        if (!roc) continue;               
        TString location = map.getScalerLoc(roc);
        string temp = location.Data();
        if (temp == "rscaler") scalerdef[roc] = "right";
        if (temp == "lscaler") scalerdef[roc] = "left";
        if (temp == "rcs") scalerdef[roc] = "rcs";
// If more locations added, put them here.  But scalerdef[] is not
// needed if you know the roc#, you can call getScaler(roc,...)
        int slot = (evbuffer[ipt]&0xf0000)>>16;
        crateslot[idx(roc,slot)].clearEvent();
        int numchan = evbuffer[ipt]&0xff;
        if (DEBUG) cout<<"slot "<<slot<<" numchan "<<numchan<<endl;
          for (int chan=0; chan<numchan; chan++) {
            ipt++; 
            if ((slot >=0 ) && (slot < MAXSLOT)) {
              int data = evbuffer[ipt];
	      if (DEBUG) cout<<"scaler chan "<<chan<<" data "<<data<<endl;
              if (crateslot[idx(roc,slot)].loadData(location.Data(),chan,data,data)
                 == SD_ERR) return HED_ERR;
	    }
	}
      }
      return HED_OK;
};


int THaEvData::GetScaler(const TString& spec, int slot, int chan) const {
// Get scaler data by spectrometer, slot, and channel
// spec = "left" or "right" for the "scaler events" (type 140)
// and spec = "evleft" and "evright" for scalers
// that are part of the event (ev) readout.
    for(int cra=0; cra<numscaler_crate; cra++) {       
      int roc = scaler_crate[cra];
      if (spec == scalerdef[roc]) return GetScaler(roc, slot, chan);
    }
    return 0;
};

int THaEvData::GetScaler(int roc, int slot, int chan) const {
// Get scaler data by roc, slot, chan.
     if ((roc >= 0) && (roc < MAXROC)) {
        if (scalerdef[roc] == "nothing") return 0;
        if ((slot >= 0) && (slot < MAXSLOT)) {
           return crateslot[idx(roc,slot)].getData(chan,0);
	}
    }
    return 0;
};

int THaEvData::GetHelicity() const {
    return (int)helicity->GetHelicity();
};

int THaEvData::GetHelicity(const TString& spec) const {
    return (int)helicity->GetHelicity(spec);
};

double THaEvData::GetEpicsData(const char* tag, int event) const {
// EPICS data which is nearest CODA event# 'event'
     return epics->getData(tag, event);
};

double THaEvData::GetEpicsData(const char* tag) const {
// EPICS data, nearest to present CODA event.
     return GetEpicsData(tag, recent_event);
};

int THaEvData::fastbus_decode(int roc, THaCrateMap& map,
          const int* evbuffer, int istart, int istop) {
    int slotold = -1;
    int ipt = istart;
    synchmiss = false;
    synchextra = false;
    buffmode = false;
    if (DEBUG) cout << "Fastbus roc  "<<dec<<roc<<endl;
    while ( ipt++ < istop ) {
       if(DEBUG) {
          cout << "evbuffer  " <<dec<<ipt<<"   ";
          cout << hex << evbuffer[ipt] << endl;
       }
       int slot = fb.Slot(evbuffer[ipt]);
       if (!slot) {
         loadFlag(ipt,evbuffer);
         continue;
       }
       int model = map.getModel(roc,slot);
       if (model == map.CM_ERR) continue;
       if (!model) {
	 if (VERBOSE) cout << "Warning: Undefined module in data" << endl;
         continue;
       }
       if (DEBUG) {
         cout<<"slot slotold model "<<dec<<slot;
         cout<<"  "<<slotold<<"  "<<model<<endl;
       }
       if (slot != slotold) {            
          slotold = slot;
          if (fb.headExist(model)) {
             int n = fb.Wdcnt(model,evbuffer[ipt]);
             if (n == fb.FB_ERR) return HED_ERR;
	     if (DEBUG) cout << "header, wdcnt = "<<dec<<n<<endl;
             if (n <= 1) continue;
             ipt++;
          }
       }
       int chan = fb.Chan(model,evbuffer[ipt]);
       int data = fb.Data(model,evbuffer[ipt]);
       int raw = evbuffer[ipt];
       if (DEBUG) {
	 cout << "chan "<<dec<<chan<<"  data "<<data<<"  ipt "<<ipt;
         cout << "  raw  "<<hex<<raw<<"  device "<<fb.devType(model)<<endl;
       }
       if ((roc >= 0) && (roc < MAXROC)) {
         if ((slot >=0) && (slot < MAXSLOT)) {
            if(crateslot[idx(roc,slot)].loadData(fb.devType(model).Data(),chan,data,raw)
              == SD_ERR) return HED_ERR;
	 }
       }
    }
    return HED_OK;
};

int THaEvData::vme_decode(int roc, THaCrateMap& map, const int* evbuffer,
          int ipt, int istop)  {
    int Nslot = map.getNslot(roc);
    if (DEBUG) cout << "VME roc "<<dec<<roc<<" nslot "<<Nslot<<endl;
    if (Nslot <= 0) return HED_ERR;
    int slot,chan,raw,data,loc,slotprime,ndat;
    map.setSlotDone();
    TString location = map.getScalerLoc(roc);
    scalerdef[roc] = location.Data();
    if (map.isScalerCrate(roc) && GetRocLength(roc) >= 16) evscaler = 1;
    while ( ipt++ < istop ) {
      if(DEBUG) cout << "evbuff "<<dec<<ipt<<"  "<<hex<<evbuffer[ipt]<<endl;  
      for (slot=1; slot<=Nslot; slot++) {
       if (!map.slotUsed(roc,slot)) continue;
       if (map.slotDone(slot)) continue;
       int head = map.getHeader(roc,slot);
       int mask = map.getMask(roc,slot);
       if(DEBUG) {
         cout<<"slot head mask "<<dec<<slot<<"  ";
         cout<<hex<<head<<"  "<<mask<<endl;
       }
       if ((evbuffer[ipt]&mask) == head) {
          map.setSlotDone(slot);
          if (map.getModel(roc,slot) == 1182) {   // LeCroy 1182 ADC
	    for (chan=0; chan<8; chan++) {
              if (ipt++ >= event_length) goto SlotDone;
              raw = evbuffer[ipt];
              if(DEBUG) {
                cout<<"1182 chan data "<<dec<<chan<<" 0x"<<hex<<raw<<endl;
              }
              if (crateslot[idx(roc,slot)].loadData("adc",chan,raw,raw)
                  == SD_ERR) return HED_ERR;
 	    }
	  }
          if (map.getModel(roc,slot) == 7510) {   // Struck 7510 ADC
           int nhit = (evbuffer[ipt]&0xfff)/8;
           if (DEBUG) cout << "nhit 7510 " << nhit << endl;
	   for (chan=0; chan<8; chan++) {
             for (int j=0; j<nhit/2; j++) {  // nhit must be even
               if (ipt++ >= event_length) goto SlotDone;
               raw = evbuffer[ipt];
               if(DEBUG)  cout<<"7510 raw  0x"<<hex<<raw<<endl;
               if(crateslot[idx(roc,slot)].loadData
                  ("adc",chan,(raw&0x0fff0000)>>16,raw) == 
                     SD_ERR) return HED_ERR;
               if (crateslot[idx(roc,slot)].loadData("adc",chan,(raw&0xfff),raw)
                    == SD_ERR) return HED_ERR;
             }
	   }
	  }
          if (map.getModel(roc,slot) == 3123) {  // VMIC 3123 ADC
	    for (chan=0; chan<16; chan++) {
              if (ipt++ >= event_length) goto SlotDone;
              raw = evbuffer[ipt];
              if(DEBUG) {
                cout<<"3123 chan data "<<dec<<chan<<"  0x"<<hex<<raw<<endl;
              }
              if (crateslot[idx(roc,slot)].loadData("adc",chan,raw,raw)
		   == SD_ERR) return HED_ERR;  
	    }
	  }
// Note, although there may be scalers in physics events, the
// "scaler events" dont come here.  See scaler_event_decode().
          if (map.getModel(roc,slot) == 1151) {  // LeCroy 1151 scaler
	    for (chan=0; chan<16; chan++) {
              if (ipt++ >= event_length) goto SlotDone;
              raw = evbuffer[ipt];
              if(DEBUG) {
                cout<<"1151 chan data "<<dec<<chan<<"  0x"<<hex<<raw<<endl;
              }
              if (crateslot[idx(roc,slot)].loadData("scaler",chan,raw,raw)
	           == SD_ERR) return HED_ERR;  
	    }
	  }
// The CAEN 560 is a little tricky; sometimes only 1 channel was read,
// so we don't increment ipt. (hmmm... could use time-dep crate map.)
          if (map.getModel(roc,slot) == 560) {   // CAEN 560 scaler
            loc = ipt;
	    for (chan=0; chan<16; chan++) {  
              if (loc++ >= event_length) goto SlotDone;
              raw = evbuffer[loc];
              if(DEBUG) {
                cout<<"560 chan data "<<dec<<chan<<"  0x"<<hex<<raw<<endl;
              }
              if (crateslot[idx(roc,slot)].loadData("scaler",chan,raw,raw)
		  == SD_ERR) return HED_ERR;  
	    }
	  }
          if (map.getModel(roc,slot) == 3801) {  // Struck 3801 scaler
	    for (chan=0; chan<32; chan++) {
              if (ipt++ >= event_length) goto SlotDone;
              raw = evbuffer[ipt];
              if(DEBUG) {
                cout<<"3801 chan data "<<dec<<chan<<"  0x"<<hex<<raw<<endl;
              }
              if (crateslot[idx(roc,slot)].loadData("scaler",chan,raw,raw)
	          == SD_ERR) return HED_ERR;  
	    }
	  }
          if (map.getModel(roc,slot) == 7353) {  // BRs hack for the trigger module
	    chan=0;
            raw = evbuffer[ipt]&0xfff;
	    if(DEBUG) {
	      cout<<"7353 chan data "<<dec<<chan<<"  0x"<<hex<<raw<<endl;
	    }
	    if (crateslot[idx(roc,slot)].loadData("register",chan,raw,raw)
		== SD_ERR) return HED_ERR;  
	  }
          if (map.getModel(roc,slot) == 550) {  // CAEN 550 for RICH ('slot' was called 'chan' in Fortran)
            loc = ipt; 
            slotprime = 1+((evbuffer[loc]&0xff0000)>>16);  
            if (DEBUG) cout << "Rich slot "<<slot<<" "<<slotprime<<" "<<loc<<" "<<hex<<evbuffer[loc]<<dec<<endl; 
            if (slot == slotprime) {    // They should agree, else problem.
              ndat = (evbuffer[ipt]&0xfff);
              if (DEBUG) cout << "Rich ndat = "<<ndat<<endl;
	      if (ipt+ndat>istop) {
		ipt=istop;
		// FIX ME : in this case a warning should apear
		//          the buffer/data must be corrupted
	      }
	      else {
		for (loc = ipt+1; loc <= ipt+ndat; loc++) {
		  chan = (evbuffer[loc]&0x7ff000)>>12;    
		  // channel number (was called 'wire' in Fortran)
		  raw = (evbuffer[loc]&0xffffffff);
		  data =  (evbuffer[loc]&0x0fff);           
		  if (DEBUG) cout << "channel "<<chan<<"  raw data "<<raw<<endl;
		  if (crateslot[idx(roc,slot)].loadData("adc",chan,data,raw) 
		      == SD_ERR) {
		    return HED_ERR;
		  }
		}
		ipt=ipt+ndat;
	      }
	    }
	  }
          goto SlotDone;  
       }
      }
SlotDone:
      if (DEBUG) cout<<"slot done, or skip word"<<endl;
    }
    return HED_OK;
};

int THaEvData::camac_decode(int roc, THaCrateMap& map, const int* evbuffer,
          int ipt, int istop) {
   if (VERBOSE) {
     cout << "Sorry, no CAMAC decoding yet !!" << endl;
   }
   return HED_OK;
};

int THaEvData::loadFlag(int ipt, const int* evbuffer) {
// Looks for buffer mode and synch problems.  The latter are recoverable
// but extremely rare, so I haven't bothered to write recovery a code yet, 
// but at least this warns you. 
  if (DEBUG) {
    cout << "Flag data "<<dec<<ipt<<hex<<"  "<<evbuffer[ipt]<<endl;
  }
  if((unsigned long)evbuffer[ipt] == 0xdc0000ff) synchmiss = true;
  if((evbuffer[ipt]&0xffff0000) == 0xdcfe0000) {
    synchextra = true;
    int slot = (evbuffer[ipt]&0xf800)>>11;
    int nhit = (evbuffer[ipt]&0x7ff);
    if(VERBOSE) {
      cout << "THaEvData: WARNING: Fastbus slot ";
      cout << dec << slot << "  has extra hits "<<nhit<<endl;
    }
  }
  if((evbuffer[ipt]&0xffff0000) == 0xfabc0000) {
      datascan = evbuffer[ipt+3];
      if(VERBOSE && (synchmiss || synchextra)) {
        cout << "THaEvData: WARNING: Synch problems !"<<endl;
	cout << "Data scan word 0x"<<hex<<datascan<<endl;
      }
  }
  if((evbuffer[ipt]&0xffff0000) == 0xfabb0000) buffmode = false;
  if((evbuffer[ipt]&0xffffff00) == 0xfafbbf00) {
     buffmode = true;
     synchflag = evbuffer[ipt]&0xff;
  }
  return HED_OK;
};

// To initialize the crate map member if it is to be used.
int THaEvData::init_cmap()  {
  if (DEBUG) cout << "Init crate map " << endl;
  return cmap.init(GetRunTime());  
};

// To initialize the THaSlotData member on first call to decoder
int THaEvData::init_slotdata(const THaCrateMap& map)  {
  if (DEBUG) cout << "Init slot data " << endl;
  for (int crate=0; crate<MAXROC; crate++) {
        scalerdef[crate] = "nothing";
        if (!map.crateUsed(crate)) continue;
   for (int slot=0; slot<MAXSLOT; slot++) {
      if (!map.slotUsed(crate,slot)) continue;
      crateslot[idx(crate,slot)].define(crate,slot);
   }
  }
  return HED_OK;
};

