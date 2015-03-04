#ifndef THaEvData_
#define THaEvData_

/////////////////////////////////////////////////////////////////////
//
//   THaEvData
//
/////////////////////////////////////////////////////////////////////


#include "Decoder.h"
#include "TObject.h"
#include "TString.h"
#include "THaSlotData.h"
#include "TBits.h"
#include <cassert>
#include <iostream>

class THaBenchmark;

class THaEvData : public TObject {

public:
  THaEvData();
  virtual ~THaEvData();

  // Return codes for LoadEvent
  enum { HED_OK = 0, HED_WARN = -63, HED_ERR = -127, HED_FATAL = -255 };

  // Load CODA data evbuffer. Derived classes MUST implement this function.
  virtual Int_t LoadEvent(const UInt_t* evbuffer) = 0;

  // Basic access to the decoded data
  Int_t     GetEvType()   const { return event_type; }
  Int_t     GetEvLength() const { return event_length; }
  Int_t     GetEvNum()    const { return event_num; }
  Int_t     GetRunNum()   const { return run_num; }
  // Run time/date. Time of prestart event (UNIX time).
  ULong64_t GetRunTime()  const { return fRunTime; }
  Int_t     GetRunType()  const { return run_type; }
  Int_t     GetRocLength(Int_t crate) const;   // Get the ROC length

  Bool_t    IsPhysicsTrigger() const;  // physics trigger (event types 1-14)
  Bool_t    IsScalerEvent() const;     // scalers from datastream
  Bool_t    IsPrestartEvent() const;   // prestart event
  Bool_t    IsEpicsEvent() const;      // epics data inserted in datastream
  Bool_t    IsPrescaleEvent() const;   // prescale factors
  Bool_t    IsSpecialEvent() const;    // e.g. detmap or trigger file insertion
  // number of raw words in crate, slot
  Int_t     GetNumRaw(Int_t crate, Int_t slot) const;
  // raw words for hit 0,1,2.. on crate, slot
  Int_t     GetRawData(Int_t crate, Int_t slot, Int_t hit) const;
  // To retrieve data by crate, slot, channel, and hit# (hit=0,1,2,..)
  Int_t     GetRawData(Int_t crate, Int_t slot, Int_t chan, Int_t hit) const;
  // To get element #i of the raw evbuffer
  Int_t     GetRawData(Int_t i) const;
  // Get raw element i within crate
  Int_t     GetRawData(Int_t crate, Int_t i) const;
  // Get raw data buffer for crate
  const UInt_t* GetRawDataBuffer(Int_t crate) const;
  Int_t     GetNumHits(Int_t crate, Int_t slot, Int_t chan) const;
  Int_t     GetData(Int_t crate, Int_t slot, Int_t chan, Int_t hit) const;
  Bool_t    InCrate(Int_t crate, Int_t i) const;
  // Num unique channels hit
  Int_t     GetNumChan(Int_t crate, Int_t slot) const;
  // List unique chan
  Int_t     GetNextChan(Int_t crate, Int_t slot, Int_t index) const;
  const char* DevType(Int_t crate, Int_t slot) const;

  // Optional functionality that may be implemented by derived classes
  virtual ULong64_t GetEvTime() const { return evt_time; }
   // Returns Beam Helicity (-1,0,+1)  '0' is 'unknown'
  virtual Int_t GetHelicity() const   { return 0; }
  // Beam Helicity for spec="left","right"
  virtual Int_t GetHelicity(const TString& /*spec*/) const
  { return GetHelicity(); }
  virtual Int_t GetPrescaleFactor(Int_t /*trigger*/ ) const
  { assert(fgAllowUnimpl); return -1; }
  // User can GetScaler, alternativly to GetSlotData for scalers
  // spec = "left", "right", "rcs" for event type 140 scaler "events"
  // spec = "evleft" or "evright" for L,R scalers injected into datastream.
  virtual Int_t GetScaler(Int_t /*roc*/, Int_t /*slot*/, Int_t /*chan*/) const
  { assert(ScalersEnabled() && fgAllowUnimpl); return kMaxInt; };
  virtual Int_t GetScaler(const TString& /*spec*/,
			  Int_t /*slot*/, Int_t /*chan*/) const
  { return GetScaler(0,0,0); }
  virtual void SetDebugFile( std::ofstream *file ) { fDebugFile = file; };
  virtual Decoder::Module* GetModule(Int_t roc, Int_t slot);

  // Access functions for EPICS (slow control) data
  virtual double GetEpicsData(const char* tag, Int_t event=0) const;
  virtual double GetEpicsTime(const char* tag, Int_t event=0) const;
  virtual TString GetEpicsString(const char* tag, Int_t event=0) const;
  virtual Bool_t IsLoadedEpics(const char* /*tag*/ ) const
  { return false; }

  virtual void PrintSlotData(Int_t crate, Int_t slot) const;
  virtual void PrintOut() const;
  virtual void SetRunTime( ULong64_t tloc );

  // Status control
  void    EnableBenchmarks( Bool_t enable=true );
  void    EnableHelicity( Bool_t enable=true );
  Bool_t  HelicityEnabled() const;
  void    EnableScalers( Bool_t enable=true );
  Bool_t  ScalersEnabled() const;
  void    SetOrigPS( Int_t event_type );
  TString GetOrigPS() const;

  UInt_t  GetInstance() const { return fInstance; }
  static UInt_t GetInstances() { return fgInstances.CountBits(); }

  Decoder::THaCrateMap* fMap;      // Pointer to active crate map

  // Reporting level
  void SetVerbose( UInt_t level );
  void SetDebug( UInt_t level );

  // Utility function for hexdumping any sort of data
  static void hexdump(const char* cbuff, size_t len);

  void SetCrateMapName( const char* name );
  static void SetDefaultCrateMapName( const char* name );

  enum { MAX_PSFACT = 12 };

protected:
  // Control bits in TObject::fBits used by decoders
  enum {
    kHelicityEnabled = BIT(14),
    kScalersEnabled  = BIT(15),
  };

  // static const Int_t MAXROC = 32;
  // static const Int_t MAXSLOT = 27;

  // // Hall A Trigger Types
  // static const Int_t MAX_PHYS_EVTYPE  = 14;  // Types up to this are physics
  // static const Int_t SYNC_EVTYPE      = 16;
  // static const Int_t PRESTART_EVTYPE  = 17;
  // static const Int_t GO_EVTYPE        = 18;
  // static const Int_t PAUSE_EVTYPE     = 19;
  // static const Int_t END_EVTYPE       = 20;
  // static const Int_t TS_PRESCALE_EVTYPE  = 120;
  // static const Int_t EPICS_EVTYPE     = 131;
  // static const Int_t PRESCALE_EVTYPE  = 133;
  // static const Int_t DETMAP_FILE      = 135;
  // static const Int_t TRIGGER_FILE     = 136;
  // static const Int_t SCALER_EVTYPE    = 140;

  struct RocDat_t {           // ROC raw data descriptor
    Int_t pos;                // position in evbuffer[]
    Int_t len;                // length of data
  } rocdat[Decoder::MAXROC];
  Decoder::THaSlotData** crateslot;

  Bool_t first_decode;
  Bool_t fTrigSupPS;

  const UInt_t *buffer;

  std::ofstream *fDebugFile;  // debug output

  Int_t  event_type,event_length,event_num,run_num,evscaler;
  Int_t  run_type;    // CODA run type from prestart event
  ULong64_t fRunTime; // CODA run time (Unix time) from prestart event
  ULong64_t evt_time; // Event time. Not directly supported by CODA
  Int_t  recent_event;

  Bool_t buffmode,synchmiss,synchextra;
  Int_t  idx(Int_t crate, Int_t slot) const;
  Int_t  idx(Int_t crate, Int_t slot);
  Bool_t GoodCrateSlot(Int_t crate, Int_t slot) const;
  Bool_t GoodIndex(Int_t crate, Int_t slot) const;

  Int_t init_cmap();
  Int_t init_slotdata(const Decoder::THaCrateMap* map);
  void  makeidx(Int_t crate, Int_t slot);

  Int_t     fNSlotUsed;   // Number of elements of crateslot[] actually used
  Int_t     fNSlotClear;  // Number of elements of crateslot[] to clear
  UShort_t* fSlotUsed;    // [fNSlotUsed] Indices of crateslot[] used
  UShort_t* fSlotClear;   // [fNSlotClear] Indices of crateslot[] to clear

  Bool_t fDoBench;
  THaBenchmark *fBench;

  UInt_t fInstance;            // My instance
  static TBits fgInstances;    // Number of instances of this object

  static const Double_t kBig;  // default value for invalid data
  static Bool_t fgAllowUnimpl; // If true, allow unimplemented functions

  static TString fgDefaultCrateMapName; // Default crate map name
  TString fCrateMapName; // Crate map database file name to use
  Bool_t fNeedInit;  // Crate map needs to be (re-)initialized

  Int_t  fDebug;     // Debug/verbosity level

  ClassDef(THaEvData,0)  // Decoder for CODA event buffer

};

//=============== inline functions ================================

//Utility function to index into the crateslot array
inline Int_t THaEvData::idx( Int_t crate, Int_t slot) const {
  return slot+Decoder::MAXSLOT*crate;
}
//Like idx() const, but initializes empty slots
inline Int_t THaEvData::idx( Int_t crate, Int_t slot) {
  Int_t ix = slot+Decoder::MAXSLOT*crate;
  if( !crateslot[ix] ) makeidx(crate,slot);
  return ix;
}

inline Bool_t THaEvData::GoodCrateSlot( Int_t crate, Int_t slot ) const {
  return ( crate >= 0 && crate < Decoder::MAXROC &&
	   slot >= 0 && slot < Decoder::MAXSLOT );
}

inline Bool_t THaEvData::GoodIndex( Int_t crate, Int_t slot ) const {
  return ( GoodCrateSlot(crate,slot) && crateslot[idx(crate,slot)] != 0);
}

inline Int_t THaEvData::GetRocLength(Int_t crate) const {
  assert(crate >= 0 && crate < Decoder::MAXROC);
  return rocdat[crate].len;
}

inline Int_t THaEvData::GetNumHits(Int_t crate, Int_t slot, Int_t chan) const {
  // Number hits in crate, slot, channel
  assert( GoodCrateSlot(crate,slot) );
  if( crateslot[idx(crate,slot)] != 0 )
    return crateslot[idx(crate,slot)]->getNumHits(chan);
  return 0;
};

inline Int_t THaEvData::GetData(Int_t crate, Int_t slot, Int_t chan,
				Int_t hit) const {
  // Return the data in crate, slot, channel #chan and hit# hit
  assert( GoodIndex(crate,slot) );
  return crateslot[idx(crate,slot)]->getData(chan,hit);
};

inline Int_t THaEvData::GetNumRaw(Int_t crate, Int_t slot) const {
  // Number of raw words in crate, slot
  assert( GoodCrateSlot(crate,slot) );
  if( crateslot[idx(crate,slot)] != 0 )
    return crateslot[idx(crate,slot)]->getNumRaw();
  return 0;
};

inline Int_t THaEvData::GetRawData(Int_t crate, Int_t slot, Int_t hit) const {
  // Raw words in crate, slot
  assert( GoodIndex(crate,slot) );
  return crateslot[idx(crate,slot)]->getRawData(hit);
};

inline Int_t THaEvData::GetRawData(Int_t crate, Int_t slot, Int_t chan,
				   Int_t hit) const {
  // Return the Rawdata in crate, slot, channel #chan and hit# hit
  assert( GoodIndex(crate,slot) );
  return crateslot[idx(crate,slot)]->getRawData(chan,hit);
};

inline Int_t THaEvData::GetRawData(Int_t i) const {
  // Raw words in evbuffer at location #i.
  assert( buffer && i >= 0 && i < GetEvLength() );
  return buffer[i];
};

inline Int_t THaEvData::GetRawData(Int_t crate, Int_t i) const {
  // Raw words in evbuffer within crate #crate.
  assert( crate >= 0 && crate < Decoder::MAXROC );
  Int_t index = rocdat[crate].pos + i;
  return GetRawData(index);
};

inline const UInt_t* THaEvData::GetRawDataBuffer(Int_t crate) const {
  // Direct access to the event buffer for the given crate,
  // e.g. for fast header word searches
  assert( crate >= 0 && crate < Decoder::MAXROC );
  Int_t index = rocdat[crate].pos;
  return buffer+index;
};

inline Bool_t THaEvData::InCrate(Int_t crate, Int_t i) const {
  // To tell if the index "i" poInt_ts to a word inside crate #crate.
  assert( crate >= 0 && crate < Decoder::MAXROC );
  // Used for crawling through whole event
  if (crate == 0) return (i >= 0 && i < GetEvLength());
  if (rocdat[crate].pos == 0 || rocdat[crate].len == 0) return false;
  return (i >= rocdat[crate].pos &&
	  i <= rocdat[crate].pos+rocdat[crate].len);
};

inline Int_t THaEvData::GetNumChan(Int_t crate, Int_t slot) const {
  // Get number of unique channels hit
  assert( GoodCrateSlot(crate,slot) );
  if( crateslot[idx(crate,slot)] != 0 )
    return crateslot[idx(crate,slot)]->getNumChan();
  return 0;
};

inline Int_t THaEvData::GetNextChan(Int_t crate, Int_t slot,
				    Int_t index) const {
  // Get list of unique channels hit (indexed by index=0,getNumChan()-1)
  assert( GoodIndex(crate,slot) );
  assert( index >= 0 && index < GetNumChan(crate,slot) );
  return crateslot[idx(crate,slot)]->getNextChan(index);
};

inline
Bool_t THaEvData::IsPhysicsTrigger() const {
  return ((event_type > 0) && (event_type <= Decoder::MAX_PHYS_EVTYPE));
};

inline
Bool_t THaEvData::IsScalerEvent() const {
  // Either 'event type 140' or events with the synchronous readout
  // of scalers (roc11, etc).
  // Important: A scaler event can also be a physics event.
  return (event_type == Decoder::SCALER_EVTYPE || evscaler == 1);
};

inline
Bool_t THaEvData::IsPrestartEvent() const {
  return (event_type == Decoder::PRESTART_EVTYPE);
};

inline
Bool_t THaEvData::IsEpicsEvent() const {
  return (event_type == Decoder::EPICS_EVTYPE);
};

inline
Bool_t THaEvData::IsPrescaleEvent() const {
  return (event_type == Decoder::TS_PRESCALE_EVTYPE ||
	  event_type == Decoder::PRESCALE_EVTYPE);
};

inline
Bool_t THaEvData::IsSpecialEvent() const {
  return ( (event_type == Decoder::DETMAP_FILE) ||
	   (event_type == Decoder::TRIGGER_FILE) );
};

inline
Bool_t THaEvData::HelicityEnabled() const
{
  // Test if helicity decoding enabled
  return TestBit(kHelicityEnabled);
}

inline
Bool_t THaEvData::ScalersEnabled() const
{
  // Test if scaler decoding enabled
  return TestBit(kScalersEnabled);
}

// Dummy versions of EPICS data access functions. These will always fail
// in debug mode unless IsLoadedEpics is changed. This is by design -
// clients should never try to retrieve data that are not loaded.
inline
double THaEvData::GetEpicsData(const char* /*tag*/, Int_t /*event*/ ) const
{
  assert(IsLoadedEpics("") && fgAllowUnimpl);
  return kBig;
}

inline
double THaEvData::GetEpicsTime(const char* /*tag*/, Int_t /*event*/ ) const
{
  assert(IsLoadedEpics("") && fgAllowUnimpl);
  return kBig;
}

inline
TString THaEvData::GetEpicsString(const char* /*tag*/,
				  Int_t /*event*/ ) const
{
  assert(IsLoadedEpics("") && fgAllowUnimpl);
  return TString("");
}

#endif
