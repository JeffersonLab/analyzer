#ifndef THaEvData_
#define THaEvData_

/////////////////////////////////////////////////////////////////////
//
//   THaEvData
//
/////////////////////////////////////////////////////////////////////

class THaHelicity;

#include "Rtypes.h"
#include "THaFastBusWord.h"
#include "THaEpicsStack.h"
#include "THaSlotData.h"
#include "THaCrateMap.h"
#include "THaUsrstrutils.h"
#include "THaCodaData.h"

class THaEvData {

public:
     THaEvData();              
     virtual ~THaEvData();
     int GetEvType()   const { return event_type; };
     int GetEvLength() const { return event_length; };
     int GetEvNum()    const { return event_num; };
     int GetRunNum()   const { return run_num; };
     UInt_t GetRunTime()  const { return run_time; };
     int GetRunType()  const { return run_type; };
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
     TString DevType(int crate, int slot) const;
     int AddEpicsTag(const TString& tag);
// User can GetScaler, alternativly to GetSlotData for scalers
// spec = "left", "right", "rcs" for event type 140 scaler "events"
// spec = "evleft" or "evright" for L,R scalers injected into datastream.
     int GetScaler(const TString& spec, int slot, int chan) const;   
     int GetScaler(int roc, int slot, int chan) const;         
     int GetHelicity() const;         // Returns Beam Helicity (-1,0,+1)  '0' is 'unknown'
     int GetHelicity(const TString& spec) const;  // Beam Helicity for spec="left","right"
// EPICS data which is nearest CODA event# 'event'.  Tag is EPICS variable, e.g. 'IPM1H04B.XPOS'
     double GetEpicsData(const char* tag, int event) const;
     double GetEpicsData(const char* tag) const;    // get nearest present event.
// Loads CODA data evbuffer using THaCrateMap passed as 2nd arg
     int LoadEvent(const int* evbuffer, THaCrateMap& usermap);    
// Loads CODA data evbuffer using private crate map "cmap" (recommended)
     int LoadEvent(const int* evbuffer);          
     void PrintSlotData(int crate, int slot);
     static const int HED_OK;
     static const int HED_ERR;

private:

     static const int DEBUG = 0;  // set =0 normally
     static const int MAXROC = 20;  
     static const int MAXSLOT = 27;  
     int n1roc[MAXROC],lenroc[MAXROC],irn[MAXROC];
     THaCrateMap cmap;
     THaFastBusWord fb;
     THaSlotData* crateslot;  
     THaSlotData emptyd;
     THaHelicity* helicity;
     THaEpicsStack* epics;
     bool first_load,first_scaler,first_decode;
     TString scalerdef[MAXROC];
     int numscaler_crate;
     int scaler_crate[MAXROC];        // stored from cratemap for fast ref.
     static const int VERBOSE =  1;   // 1 = verbose warnings, recommended.
     static const int MAX_PSFACT = 12;
     int psfact[MAX_PSFACT];
// Hall A Trigger Types
     static const int MAX_PHYS_EVTYPE  = 14;  // Types up to this are physics
     static const int PRESTART_EVTYPE  = 17;
     static const int GO_EVTYPE        = 18;
     static const int PAUSE_EVTYPE     = 19;
     static const int END_EVTYPE       = 20;
     static const int EPICS_EVTYPE     = 131;
     static const int PRESCALE_EVTYPE  = 133;
     static const int DETMAP_FILE      = 135;
     static const int TRIGGER_FILE     = 136;
     static const int SCALER_EVTYPE    = 140;
     const Int_t *buffer;
     Int_t event_type,event_length,event_num,run_num,evscaler;
     Int_t run_type;     // CODA run type from prestart event
     UInt_t run_time;     // CODA run time (Unix time) from prestart event
     Int_t recent_event,synchflag,datascan;
     Double_t dhel,dtimestamp;
     bool buffmode,synchmiss,synchextra;
     void dump(const int* evbuffer);                
     int gendecode(const int* evbuffer, THaCrateMap& map);
     int loadFlag(int ipt, const int* evbuffer);
     int epics_decode(const int* evbuffer);
     int prescale_decode(const int* evbuffer);
     int physics_decode(const int* evbuffer, THaCrateMap& map);
     int fastbus_decode(int roc, THaCrateMap& map, const int* evbuffer, int p1, int p2);
     int vme_decode(int roc, THaCrateMap& map, const int* evbuffer, int p1, int p2);
     int camac_decode(int roc, THaCrateMap& map, const int* evbuffer, int p1, int p2);
     int scaler_event_decode(const int* evbuffer, THaCrateMap& map);
     int init_cmap();      
     int init_slotdata(const THaCrateMap& map);
     int idx(int crate, int slot) const;
     bool GoodIndex(int crate, int slot) const;

     ClassDef(THaEvData,0)  // Decoder for CODA event buffer

};

//=============== inline functions ================================

//A utility function to index into the crateslot array
inline int THaEvData::idx( int crate, int slot) const {
  return slot+MAXSLOT*crate;
}

inline bool THaEvData::GoodIndex( int crate, int slot ) const {
  return ( (crate >= 0) && (crate < MAXROC) && 
	   (slot >= 0) && (slot < MAXSLOT) );
}

inline int THaEvData::GetNumHits(int crate, int slot, int chan) const {
// Number hits in crate, slot, channel
  if( GoodIndex(crate,slot))
    return crateslot[idx(crate,slot)].getNumHits(chan);
  return 0;
};

inline int THaEvData::GetData(int crate, int slot, int chan, int hit) const {
// Return the data in crate, slot, channel #chan and hit# hit
  if( GoodIndex(crate,slot))
    return crateslot[idx(crate,slot)].getData(chan,hit);
  return 0;
};

inline int THaEvData::GetNumRaw(int crate, int slot) const {
// Number of raw words in crate, slot
  if( GoodIndex(crate,slot))
    return crateslot[idx(crate,slot)].getNumRaw();
  return 0;
};

inline int THaEvData::GetRawData(int crate, int slot, int hit) const {
// Raw words in crate, slot
  if( GoodIndex(crate,slot))
    return crateslot[idx(crate,slot)].getRawData(hit);
  return 0;
};

inline int THaEvData::GetRawData(int crate, int slot, int chan, int hit) const {
// Return the Rawdata in crate, slot, channel #chan and hit# hit
  if( GoodIndex(crate,slot))
    return crateslot[idx(crate,slot)].getRawData(chan,hit);
  return 0;
};

inline int THaEvData::GetRawData(int i) const {
// Raw words in evbuffer at location #i.
  if ( !buffer ) return 0;   // oops, you didn't LoadEvent
  if (i >= 0 || i < GetEvLength()) return buffer[i];
  return 0;
};

inline int THaEvData::GetRawData(int crate, int i) const {
// Raw words in evbuffer within crate #crate.
  if (crate < 0 || crate > MAXROC) return 0;
  int index = n1roc[crate] + i;
  if (index >= 0 || index < GetEvLength()) return buffer[index];
  return 0;
};

inline Bool_t THaEvData::InCrate(int crate, int i) const {
// To tell if the index "i" points to a word inside crate #crate.
  if (crate == 0) return kTRUE;     // Used for crawling through whole event.
  if (crate < 0 || crate > MAXROC) return kFALSE;
  if (n1roc[crate] == 0 || lenroc[crate] == 0) return kFALSE;
  return (i >= n1roc[crate] && i < n1roc[crate]+lenroc[crate]);
};

inline int THaEvData::GetNumChan(int crate, int slot) const {
// Get number of unique channels hit
  if( GoodIndex(crate,slot))
    return crateslot[idx(crate,slot)].getNumChan();
  return 0;
};

inline int THaEvData::GetNextChan(int crate, int slot, int index) const {
// Get list of unique channels hit (indexed by index=0,getNumChan())
  if( GoodIndex(crate,slot))
    return crateslot[idx(crate,slot)].getNextChan(index);
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
int THaEvData::LoadEvent(const int* evbuffer, THaCrateMap& cratemap) {
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
      if (init_cmap() == cmap.CM_ERR) return HED_ERR;
  }
  return gendecode(evbuffer, cmap);
};

#endif 
