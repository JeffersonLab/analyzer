#ifndef THaEvData_
#define THaEvData_

/////////////////////////////////////////////////////////////////////
//
//   THaEvData
//
/////////////////////////////////////////////////////////////////////


#include "TObject.h"
#include "TString.h"
#include "THaSlotData.h"
#include "TBits.h"
#include "evio.h"

class THaBenchmark;
class THaEpics;
class THaCrateMap;
class THaFastBusWord;
class THaHelicity;

class THaEvData : public TObject {

public:
     THaEvData();
     virtual ~THaEvData();
     int GetEvType()   const { return event_type; }
     int GetEvLength() const { return event_length; }
     int GetEvNum()    const { return event_num; }
     int GetRunNum()   const { return run_num; }
     // Run time/date. Time of prestart event (UNIX time).
     UInt_t GetRunTime() const { return run_time; }
     // Event time from 100 kHz helicity clock.
     Double_t GetEvTime() const;
     int GetRunType()  const { return run_type; }
     int GetRocLength(int crate) const;   // Get the ROC length
     int GetPrescaleFactor(int trigger) const;  // Obtain prescale factor
     bool IsPhysicsTrigger() const;    // physics trigger (event types 1-14)
     bool IsScalerEvent() const;       // scalers from datastream
     bool IsPrestartEvent() const;     // prestart event
     bool IsEpicsEvent() const;        // epics data inserted in datastream
     bool IsPrescaleEvent() const;     // prescale factors
     bool IsSpecialEvent() const;      // e.g. detmap or trigger file insertion
// number of raw words in crate, slot
     int GetNumRaw(int crate, int slot) const;
// raw words for hit 0,1,2.. on crate, slot
     int GetRawData(int crate, int slot, int hit) const;            
// To retrieve data by crate, slot, channel, and hit# (hit=0,1,2,..)
     int GetRawData(int crate, int slot, int chan, int hit) const;
// To get element #i of the raw evbuffer
     int GetRawData(int i) const;
     int GetRawData(int crate, int i) const;   // Get raw element i within crate
     int GetNumHits(int crate, int slot, int chan) const;
     int GetData(int crate, int slot, int chan, int hit) const;
     Bool_t InCrate(int crate, int i) const;
     int GetNumChan(int crate, int slot) const;    // Num unique channels hit
     int GetNextChan(int crate, int slot, int index) const; // List unique chan
     const char* DevType(int crate, int slot) const;
// User can GetScaler, alternativly to GetSlotData for scalers
// spec = "left", "right", "rcs" for event type 140 scaler "events"
// spec = "evleft" or "evright" for L,R scalers injected into datastream.
     int GetScaler(const TString& spec, int slot, int chan) const;   
     int GetScaler(int roc, int slot, int chan) const;         
     int GetHelicity() const;         // Returns Beam Helicity (-1,0,+1)  '0' is 'unknown'
     int GetHelicity(const TString& spec) const;  // Beam Helicity for spec="left","right"
// EPICS data which is nearest CODA event# 'event'.  Tag is EPICS variable, e.g. 'IPM1H04B.XPOS'
     double GetEpicsData(const char* tag, int event=0) const;
     double GetEpicsTime(const char* tag, int event=0) const;
     Bool_t IsLoadedEpics(const char* tag) const;
// Loads CODA data evbuffer using THaCrateMap passed as 2nd arg
     int LoadEvent(const int* evbuffer, THaCrateMap* usermap);    
// Loads CODA data evbuffer using private crate map "cmap" (recommended)
     int LoadEvent(const int* evbuffer);          
     void PrintSlotData(int crate, int slot) const;
     void PrintOut() const { dump(buffer); }
     void SetRunTime( UInt_t tloc );
     void EnableHelicity( Bool_t enable=kTRUE );
     Bool_t HelicityEnabled() const;
     void EnableScalers( Bool_t enable=kTRUE );
     Bool_t ScalersEnabled() const;

     UInt_t GetInstance() const { return fInstance; }
     static UInt_t GetInstances() { return fgInstances.CountBits(); }

// Utility function for hexdumping any sort of data
     static void hexdump(const char* cbuff, size_t len);

     enum { HED_OK = 0, HED_ERR = -127};
     enum { MAX_PSFACT = 12 };

protected:
  // Control bits in TObject::fBits used by decoders
     enum { kHelicityEnabled = BIT(14),
	    kScalersEnabled  = BIT(15)
     };

private:
     static const int MAXROC = 20;  
     static const int MAXSLOT = 27;  

     static const int MAX_PHYS_EVTYPE  = 14;  // Types up to this are physics
     static const int SYNC_EVTYPE      = 16;
     static const int PRESTART_EVTYPE  = 17;
     static const int GO_EVTYPE        = 18;
     static const int PAUSE_EVTYPE     = 19;
     static const int END_EVTYPE       = 20;
     static const int EPICS_EVTYPE     = 131;
     static const int PRESCALE_EVTYPE  = 133;
     static const int DETMAP_FILE      = 135;
     static const int TRIGGER_FILE     = 136;
     static const int SCALER_EVTYPE    = 140;

     struct RocDat_t {           // ROC raw data descriptor
       int pos;                  // position in evbuffer[]
       int len;                  // length of data
     } rocdat[MAXROC];
     THaCrateMap* cmap;          // default crate map
     THaFastBusWord* fb;
     THaSlotData** crateslot;  
     THaHelicity* helicity;
     THaEpics* epics;
     bool first_load,first_scaler,first_decode;
     TString scalerdef[MAXROC];
     int numscaler_crate;
     int scaler_crate[MAXROC];        // stored from cratemap for fast ref.
     int psfact[MAX_PSFACT];
// Hall A Trigger Types
     const Int_t *buffer;
     Int_t event_type,event_length,event_num,run_num,evscaler;
     Int_t run_type;     // CODA run type from prestart event
     UInt_t run_time;    // CODA run time (Unix time) from prestart event
     UInt_t evt_time;    // Event time (Unix time). Not supported by CODA.
     Int_t recent_event,synchflag,datascan;
     Double_t dhel,dtimestamp;
     bool buffmode,synchmiss,synchextra;
     void dump(const int* evbuffer) const;
     int gendecode(const int* evbuffer, THaCrateMap* map);
     int loadFlag(const int* evbuffer);
     int epics_decode(const int* evbuffer);
     int prescale_decode(const int* evbuffer);
     int physics_decode(const int* evbuffer, THaCrateMap* map);
     int fastbus_decode(int roc, THaCrateMap* map, const int* evbuffer, 
			int p1, int p2);
     int vme_decode(int roc, THaCrateMap* map, const int* evbuffer, 
		    int p1, int p2);
     int camac_decode(int roc, THaCrateMap* map, const int* evbuffer, 
		      int p1, int p2);
     int scaler_event_decode(const int* evbuffer, THaCrateMap* map);
     int init_cmap();      
     int init_slotdata(const THaCrateMap* map);
     int idx(int crate, int slot) const;
     int idx(int crate, int slot);
     void makeidx(int crate, int slot);
     bool GoodIndex(int crate, int slot) const;

     int       fNSlotUsed;   // Number of elements of crateslot[] actually used
     int       fNSlotClear;  // Number of elements of crateslot[] to clear
     UShort_t* fSlotUsed;    // [fNSlotUsed] Indices of crateslot[] used
     UShort_t* fSlotClear;   // [fNSlotClear] Indices of crateslot[] to clear

     THaCrateMap* fMap;      // Pointer to active crate map

     bool fDoBench;
     THaBenchmark *fBench;

     UInt_t fInstance;          //My instance
     static TBits fgInstances;  //number of instances of this object

     ClassDef(THaEvData,0)  // Decoder for CODA event buffer

};

//=============== inline functions ================================

//A utility function to index into the crateslot array
inline int THaEvData::idx( int crate, int slot) const {
  return slot+MAXSLOT*crate;
}
//Another utility function, but it initializes empty slots
inline int THaEvData::idx( int crate, int slot) {
  int idx = slot+MAXSLOT*crate;
  if( !crateslot[idx] ) makeidx(crate,slot);
  return idx;
}

inline bool THaEvData::GoodIndex( int crate, int slot ) const {
  return ( crate >= 0 && crate < MAXROC && 
	   slot >= 0 && slot < MAXSLOT &&
	   crateslot[idx(crate,slot)] != 0);
}

inline int THaEvData::GetRocLength(int crate) const {
  if (crate >= 0 && crate < MAXROC) return rocdat[crate].len;
  return 0;
}

inline int THaEvData::GetNumHits(int crate, int slot, int chan) const {
// Number hits in crate, slot, channel
  if( GoodIndex(crate,slot))
    return crateslot[idx(crate,slot)]->getNumHits(chan);
  return 0;
};

inline int THaEvData::GetData(int crate, int slot, int chan, int hit) const {
// Return the data in crate, slot, channel #chan and hit# hit
  if( GoodIndex(crate,slot))
    return crateslot[idx(crate,slot)]->getData(chan,hit);
  return 0;
};

inline int THaEvData::GetNumRaw(int crate, int slot) const {
// Number of raw words in crate, slot
  if( GoodIndex(crate,slot))
    return crateslot[idx(crate,slot)]->getNumRaw();
  return 0;
};

inline int THaEvData::GetRawData(int crate, int slot, int hit) const {
// Raw words in crate, slot
  if( GoodIndex(crate,slot))
    return crateslot[idx(crate,slot)]->getRawData(hit);
  return 0;
};

inline int THaEvData::GetRawData(int crate, int slot, int chan, int hit) const {
// Return the Rawdata in crate, slot, channel #chan and hit# hit
  if( GoodIndex(crate,slot))
    return crateslot[idx(crate,slot)]->getRawData(chan,hit);
  return 0;
};

inline int THaEvData::GetRawData(int i) const {
// Raw words in evbuffer at location #i.
  if ( !buffer ) return 0;   // oops, you didn't LoadEvent
  if (i >= 0 && i < GetEvLength()) return buffer[i];
  return 0;
};

inline int THaEvData::GetRawData(int crate, int i) const {
// Raw words in evbuffer within crate #crate.
  if (crate < 0 || crate > MAXROC) return 0;
  int index = rocdat[crate].pos + i;
  if (index >= 0 && index < GetEvLength()) return buffer[index];
  return 0;
};

inline Bool_t THaEvData::InCrate(int crate, int i) const {
// To tell if the index "i" points to a word inside crate #crate.
  if (crate == 0) return kTRUE;     // Used for crawling through whole event.
  if (crate < 0 || crate > MAXROC) return kFALSE;
  if (rocdat[crate].pos == 0 || rocdat[crate].len == 0) return kFALSE;
  return (i >= rocdat[crate].pos && i <= rocdat[crate].pos+rocdat[crate].len);
};

inline int THaEvData::GetNumChan(int crate, int slot) const {
// Get number of unique channels hit
  if( GoodIndex(crate,slot))
    return crateslot[idx(crate,slot)]->getNumChan();
  return 0;
};

inline int THaEvData::GetNextChan(int crate, int slot, int index) const {
// Get list of unique channels hit (indexed by index=0,getNumChan())
  if( GoodIndex(crate,slot))
    return crateslot[idx(crate,slot)]->getNextChan(index);
  return 0;
};

inline
bool THaEvData::IsPhysicsTrigger() const {
  return ((event_type > 0) && (event_type <= MAX_PHYS_EVTYPE));
};

inline
bool THaEvData::IsScalerEvent() const {
// Either 'event type 140' or events with the synchronous readout of scalers (roc11, etc)
// A scaler event can also be a physics event.
  return (event_type == SCALER_EVTYPE || evscaler == 1);
};

inline
bool THaEvData::IsPrestartEvent() const {
  return (event_type == PRESTART_EVTYPE);
};

inline
bool THaEvData::IsEpicsEvent() const {
  return (event_type == EPICS_EVTYPE);
};

inline
bool THaEvData::IsPrescaleEvent() const {
  return (event_type == PRESCALE_EVTYPE);
};

inline
bool THaEvData::IsSpecialEvent() const {
  return ( (event_type == DETMAP_FILE) ||
	   (event_type == TRIGGER_FILE) );
};

inline
int THaEvData::LoadEvent(const int* evbuffer, THaCrateMap* cratemap) {
// Public interface to decode the event.  Note, LoadEvent()
// MUST be called once per event BEFORE you can extract 
// information about the event.
// This version of LoadEvent() uses externally provided THaCrateMap
    return gendecode(evbuffer, cratemap);
};
                     
inline
int THaEvData::LoadEvent(const int* evbuffer) {
// This version of LoadEvent() uses private THaCrateMap cmap (recommended)
  if (first_load) {
      first_load = false;
      if (init_cmap() == HED_ERR) return HED_ERR;
  }
  return gendecode(evbuffer, cmap);
};

#endif 
