#ifndef Podd_THaEvData_h_
#define Podd_THaEvData_h_

/////////////////////////////////////////////////////////////////////
//
//   THaEvData
//
/////////////////////////////////////////////////////////////////////


#include "Decoder.h"
#include "Module.h"
#include "TObject.h"
#include "TString.h"
#include "THaSlotData.h"
#include "TBits.h"
#include <cassert>
#include <iostream>
#include <cstdio>
#include <vector>
#include <array>
#include <memory>
#include <string>

class THaBenchmark;

class THaEvData : public TObject {

public:
  THaEvData();
  virtual ~THaEvData();

  // Return codes for LoadEvent
  enum { HED_OK = 0, HED_WARN = -63, HED_ERR = -127, HED_FATAL = -255 };

  // Parse raw data in 'evbuffer'. Actual decoding/unpacking takes place here.
  // Derived classes MUST implement this function.
  virtual Int_t LoadEvent( const UInt_t* evbuffer ) = 0;

  // return a pointer to a full event
  const UInt_t*  GetRawDataBuffer() const { return buffer;}

  virtual Bool_t DataCached() { return false; }

  virtual Int_t Init();

  // Set the EPICS event type
  void      SetEpicsEvtType( UInt_t itype) { fEpicsEvtType = itype; };

  void      SetEvTime( ULong64_t evtime ) { evt_time = evtime; }

  // Basic access to the decoded data
  UInt_t    GetEvType()      const { return event_type; }
  UInt_t    GetEvLength()    const { return event_length; }
  UInt_t    GetTrigBits()    const { return trigger_bits; }
  UInt_t    GetEvNum()       const { return event_num; }
  UInt_t    GetRunNum()      const { return run_num; }
  Int_t     GetDataVersion() const { return fDataVersion; }
  // Run time/date. Time of prestart event (UNIX time).
  ULong64_t GetRunTime()     const { return fRunTime; }
  UInt_t    GetRunType()     const { return run_type; }
  UInt_t    GetRocLength( UInt_t crate ) const;   // Get the ROC length

  Bool_t    IsPhysicsTrigger() const;  // physics trigger (event types 1-14)
  Bool_t    IsScalerEvent()    const;  // scalers from data stream
  Bool_t    IsPrestartEvent()  const;  // prestart event
  Bool_t    IsEpicsEvent()     const;  // slow control data
  Bool_t    IsPrescaleEvent()  const;  // prescale factors
  Bool_t    IsSpecialEvent()   const;  // e.g. detmap or trigger file insertion
  // number of raw words in crate, slot
  UInt_t    GetNumRaw( UInt_t crate, UInt_t slot ) const;
  // raw words for hit 0,1,2.. on crate, slot
  UInt_t    GetRawData( UInt_t crate, UInt_t slot, UInt_t hit ) const;
  // To retrieve data by crate, slot, channel, and hit# (hit=0,1,2,..)
  UInt_t    GetRawData( UInt_t crate, UInt_t slot, UInt_t chan, UInt_t hit ) const;
  // To get element #i of the raw evbuffer
  UInt_t    GetRawData( UInt_t i ) const;
  // Get raw element i within crate
  UInt_t    GetRawData( UInt_t crate, UInt_t i ) const;
  // Get raw data buffer for crate
  const UInt_t* GetRawDataBuffer( UInt_t crate ) const;
  UInt_t    GetNumHits( UInt_t crate, UInt_t slot, UInt_t chan ) const;
  UInt_t    GetData( UInt_t crate, UInt_t slot, UInt_t chan, UInt_t hit ) const;
  Bool_t    InCrate( UInt_t crate, UInt_t i ) const;
  // Num unique channels hit
  UInt_t    GetNumChan( UInt_t crate, UInt_t slot ) const;
  // List unique chan
  UInt_t    GetNextChan( UInt_t crate, UInt_t slot, UInt_t index ) const;
  const char* DevType( UInt_t crate, UInt_t slot ) const;

  Bool_t    HasCapability( Decoder::EModuleType type, UInt_t crate, UInt_t slot ) const;
  Bool_t    IsMultifunction( UInt_t crate, UInt_t slot ) const;
  UInt_t    GetNumEvents( Decoder::EModuleType type, UInt_t crate, UInt_t slot, UInt_t chan ) const;
  UInt_t    GetData( Decoder::EModuleType type, UInt_t crate, UInt_t slot, UInt_t chan, UInt_t hit ) const;
  UInt_t    GetLEbit( UInt_t crate, UInt_t slot, UInt_t chan, UInt_t hit ) const;
  UInt_t    GetOpt( UInt_t crate, UInt_t slot, UInt_t chan, UInt_t hit ) const;

  // Optional functionality that may be implemented by derived classes
  virtual ULong64_t GetEvTime() const { return evt_time; }
   // Returns Beam Helicity (-1,0,+1)  '0' is 'unknown'
  virtual Int_t GetHelicity() const   { return 0; }
  // Beam Helicity for spec="left","right"
  virtual Int_t GetHelicity(const TString& /*spec*/) const
  { return GetHelicity(); }
  virtual UInt_t GetPrescaleFactor( UInt_t /*trigger*/ ) const
  { assert(fgAllowUnimpl); return kMaxUInt; }
  // User can GetScaler, alternatively to GetSlotData for scalers
  // spec = "left", "right", "rcs" for event type 140 scaler "events"
  // spec = "evleft" or "evright" for L,R scalers injected into data stream.
  virtual UInt_t GetScaler( UInt_t /*roc*/, UInt_t /*slot*/, UInt_t /*chan*/ ) const
  { assert(ScalersEnabled() && fgAllowUnimpl); return kMaxUInt; };
  virtual UInt_t GetScaler( const TString& /*spec*/,
                            UInt_t /*slot*/, UInt_t /*chan*/ ) const
  { return GetScaler(0,0,0); }
  virtual void SetDebugFile( std::ofstream *file ) { fDebugFile = file; };
  virtual Decoder::Module* GetModule( UInt_t roc, UInt_t slot ) const;

  // Access functions for EPICS (slow control) data
  virtual double GetEpicsData( const char* tag, UInt_t event= 0 ) const;
  virtual double GetEpicsTime( const char* tag, UInt_t event= 0 ) const;
  virtual TString GetEpicsString( const char* tag, UInt_t event= 0 ) const;
  virtual Bool_t IsLoadedEpics(const char* /*tag*/ ) const
  { return false; }

  UInt_t GetNslots() const { return fSlotUsed.size(); };
  virtual void PrintSlotData( UInt_t crate, UInt_t slot ) const;
  virtual void PrintOut() const;
  virtual void SetRunTime( ULong64_t tloc );
  virtual Int_t SetDataVersion( Int_t version );

  // Status control
  void    EnableBenchmarks( Bool_t enable=true );
  void    EnableHelicity( Bool_t enable=true );
  Bool_t  HelicityEnabled() const;
  void    EnableScalers( Bool_t enable=true );
  Bool_t  ScalersEnabled() const;
  void    EnablePrescanMode( Bool_t enable=true );
  Bool_t  PrescanModeEnabled() const;
  void    SetOrigPS( Int_t event_type );
  TString GetOrigPS() const;

  UInt_t  GetInstance() const { return fInstance; }
  static UInt_t GetInstances() { return fgInstances.CountBits(); }

  Decoder::THaCrateMap* GetCrateMap() const { return fMap.get(); }

  TObject* GetExtra() const { return fExtra; }

  // Reporting level
  void SetVerbose( Int_t level );
  void SetDebug( Int_t level );

  // Utility function for hexdumping any sort of data
  static void hexdump(const char* cbuff, size_t len);

  void SetCrateMapName( const char* name );
  static void SetDefaultCrateMapName( const char* name );

  // For THaRun to set info found during prescan FIXME BCI make virtual?
  void SetRunInfo( UInt_t num, UInt_t type, ULong64_t tloc );

protected:
  // Control bits in TObject::fBits used by decoders
  enum {
    kHelicityEnabled = BIT(14),
    kScalersEnabled  = BIT(15),
    kPrescanMode     = BIT(16)
  };

  // Initialization routines
  virtual Int_t init_cmap();
  virtual Int_t init_slotdata();
  virtual void  makeidx( UInt_t crate, UInt_t slot );
  virtual void  FindUsedSlots();

  // Helper functions
  UInt_t idx( UInt_t crate, UInt_t slot ) const;
  UInt_t idx( UInt_t crate, UInt_t slot );
  static Bool_t GoodCrateSlot( UInt_t crate, UInt_t slot );
  Bool_t GoodIndex( UInt_t crate, UInt_t slot ) const;

  // Data
  std::unique_ptr<Decoder::THaCrateMap> fMap;  // Pointer to active crate map

  class RocDat_t {            // Coordinates of ROC data in raw event
  public:
    RocDat_t() : pos(0), len(0) {}
    void clear() { pos = len = 0; }
    UInt_t pos;   // position of ROC length word in evbuffer[], so pos+1 = first word of data
    UInt_t len;   // length of data after pos, so pos+len = last word of data
  };
  std::array<RocDat_t, Decoder::MAXROC> rocdat;

  // Per-event, per-module hit data extracted from raw event
  std::vector<std::unique_ptr<Decoder::THaSlotData>> crateslot;

  Bool_t first_decode;
  Bool_t fTrigSupPS;
  Int_t  fDataVersion;    // Data format version (implementation-defined)
  UInt_t fEpicsEvtType;

  const UInt_t *buffer;

  std::ofstream *fDebugFile;  // debug output

  UInt_t event_type, event_length, event_num, run_num;
  UInt_t run_type, data_type, trigger_bits;
  Int_t  evscaler;
  ULong64_t fRunTime; // Run start time (Unix time)
  ULong64_t evt_time; // Event time (for CODA 3.* this is a 250 Mhz clock)

  std::vector<UShort_t> fSlotUsed;    // Indices of crateslot[] used
  std::vector<UShort_t> fSlotClear;   // Indices of crateslot[] to clear

  Bool_t fDoBench;
  std::unique_ptr<THaBenchmark> fBench;

  UInt_t fInstance;            // My instance
  static TBits fgInstances;    // Number of instances of this object

  static const Double_t kBig;  // default value for invalid data
  static Bool_t fgAllowUnimpl; // If true, allow unimplemented functions

  static TString fgDefaultCrateMapName; // Default crate map name
  TString fCrateMapName; // Crate map database file name to use
  Bool_t fNeedInit;  // Crate map needs to be (re-)initialized

  Int_t  fDebug;     // Debug/verbosity level

  TBits fMsgPrinted; // Flags indicating one-time warnings printed

  TObject* fExtra;   // additional member data, for binary compatibility

  ClassDef(THaEvData,0)  // Base class for raw data decoders

};

//=============== inline functions ================================

//Utility function to index into the crateslot array
inline UInt_t THaEvData::idx( UInt_t crate, UInt_t slot ) const { // NOLINT(readability-convert-member-functions-to-static)
  return slot+Decoder::MAXSLOT*crate;
}
//Like idx() const, but initializes empty slots
inline UInt_t THaEvData::idx( UInt_t crate, UInt_t slot ) {
  UInt_t ix = slot+Decoder::MAXSLOT*crate;
  if( !crateslot[ix] ) makeidx(crate,slot);
  return ix;
}

inline Bool_t THaEvData::GoodCrateSlot( UInt_t crate, UInt_t slot ) {
  return (crate < Decoder::MAXROC && slot < Decoder::MAXSLOT);
}

inline Bool_t THaEvData::GoodIndex( UInt_t crate, UInt_t slot ) const {
  return (GoodCrateSlot(crate,slot) && crateslot[idx(crate,slot)] );
}

inline UInt_t THaEvData::GetRocLength( UInt_t crate ) const {
  assert( crate < rocdat.size() );
  return rocdat[crate].len;
}

inline UInt_t THaEvData::GetNumHits( UInt_t crate, UInt_t slot,
                                     UInt_t chan ) const {
  // Number hits in crate, slot, channel
  assert( GoodCrateSlot(crate,slot) );
  if( crateslot[idx(crate,slot)] )
    return crateslot[idx(crate,slot)]->getNumHits(chan);
  return 0;
}

inline UInt_t THaEvData::GetData( UInt_t crate, UInt_t slot, UInt_t chan,
                                  UInt_t hit ) const {
  // Return the data in crate, slot, channel #chan and hit# hit
  assert( GoodIndex(crate,slot) );
  return crateslot[idx(crate,slot)]->getData(chan,hit);
}

inline UInt_t THaEvData::GetNumRaw( UInt_t crate, UInt_t slot ) const {
  // Number of raw words in crate, slot
  assert( GoodCrateSlot(crate,slot) );
  if( crateslot[idx(crate,slot)] )
    return crateslot[idx(crate,slot)]->getNumRaw();
  return 0;
}

inline UInt_t THaEvData::GetRawData( UInt_t crate, UInt_t slot,
                                     UInt_t hit ) const {
  // Raw words in crate, slot
  assert( GoodIndex(crate,slot) );
  return crateslot[idx(crate,slot)]->getRawData(hit);
}

inline UInt_t THaEvData::GetRawData( UInt_t crate, UInt_t slot, UInt_t chan,
                                     UInt_t hit ) const {
  // Return the Rawdata in crate, slot, channel #chan and hit# hit
  assert( GoodIndex(crate,slot) );
  return crateslot[idx(crate,slot)]->getRawData(chan,hit);
}

inline UInt_t THaEvData::GetRawData( UInt_t i ) const {
  // Raw words in evbuffer at location #i.
  assert( buffer && i < GetEvLength() );
  return buffer[i];
}

inline UInt_t THaEvData::GetRawData( UInt_t crate, UInt_t i ) const {
  // Raw words in evbuffer within crate #crate.
  assert( crate < rocdat.size() );
  return GetRawData(rocdat[crate].pos + i);
}

inline const UInt_t* THaEvData::GetRawDataBuffer( UInt_t crate ) const {
  // Direct access to the event buffer for the given crate,
  // e.g. for fast header word searches
  assert( crate < rocdat.size() );
  return buffer+rocdat[crate].pos;
}

inline Bool_t THaEvData::InCrate( UInt_t crate, UInt_t i ) const {
  // To tell if the index "i" poInt_ts to a word inside crate #crate.
  assert( crate < rocdat.size() );
  // Used for crawling through whole event
  if (crate == 0) return (i < GetEvLength());
  if (rocdat[crate].pos == 0 || rocdat[crate].len == 0) return false;
  return (i >= rocdat[crate].pos &&
	  i <= rocdat[crate].pos+rocdat[crate].len);
}

inline UInt_t THaEvData::GetNumChan( UInt_t crate, UInt_t slot ) const {
  // Get number of unique channels hit
  assert( GoodCrateSlot(crate,slot) );
  if( crateslot[idx(crate,slot)] )
    return crateslot[idx(crate,slot)]->getNumChan();
  return 0;
}

inline UInt_t THaEvData::GetNextChan( UInt_t crate, UInt_t slot,
                                      UInt_t index ) const {
  // Get list of unique channels hit (indexed by index=0,getNumChan()-1)
  assert( GoodIndex(crate,slot) );
  assert( index < GetNumChan(crate,slot) );
  return crateslot[idx(crate,slot)]->getNextChan(index);
}

inline
Bool_t THaEvData::IsPhysicsTrigger() const {
  return ((event_type > 0) && (event_type <= Decoder::MAX_PHYS_EVTYPE));
}


inline
Bool_t THaEvData::IsScalerEvent() const {
  // Either 'event type 140' or events with the synchronous readout
  // of scalers (roc11, etc).
  // Important: A scaler event can also be a physics event.
  return (event_type == Decoder::SCALER_EVTYPE || evscaler == 1);
}

inline
Bool_t THaEvData::IsPrestartEvent() const {
  return (event_type == Decoder::PRESTART_EVTYPE);
}

inline
Bool_t THaEvData::IsEpicsEvent() const {
  return (event_type == fEpicsEvtType);
}

inline
Bool_t THaEvData::IsPrescaleEvent() const {
  return (event_type == Decoder::TS_PRESCALE_EVTYPE ||
	  event_type == Decoder::PRESCALE_EVTYPE);
}

inline
Bool_t THaEvData::IsSpecialEvent() const {
  return ( (event_type == Decoder::DETMAP_FILE) ||
	   (event_type == Decoder::TRIGGER_FILE) );
}

inline
Bool_t THaEvData::HelicityEnabled() const
{
  // Test if helicity decoding enabled
  return TestBit(kHelicityEnabled);
}

inline
Bool_t THaEvData::ScalersEnabled() const {
  // Test if scaler decoding enabled
  return TestBit(kScalersEnabled);
}

inline
Bool_t THaEvData::PrescanModeEnabled() const {
  // Test if prescan mode enabled
  return TestBit(kPrescanMode);
}

// Dummy versions of EPICS data access functions. These will always fail
// in debug mode unless IsLoadedEpics is changed. This is by design -
// clients should never try to retrieve data that are not loaded.
inline
Double_t THaEvData::GetEpicsData( const char* /*tag*/, UInt_t /*event*/ ) const
{
  assert(IsLoadedEpics("") && fgAllowUnimpl);
  return kBig;
}

inline
Double_t THaEvData::GetEpicsTime( const char* /*tag*/, UInt_t /*event*/ ) const
{
  assert(IsLoadedEpics("") && fgAllowUnimpl);
  return kBig;
}

inline
TString THaEvData::GetEpicsString( const char* /*tag*/, UInt_t /*event*/ ) const
{
  assert(IsLoadedEpics("") && fgAllowUnimpl);
  return {""};
}

inline
Bool_t THaEvData::HasCapability( Decoder::EModuleType type, UInt_t crate,
                                 UInt_t slot ) const {
  Decoder::Module* module = GetModule(crate, slot);
  if( !module ) {
    std::cerr << "No module at crate " << crate << "   slot " << slot << std::endl;
    return false;
  }
  return module->HasCapability(type);
}

inline
Bool_t THaEvData::IsMultifunction( UInt_t crate, UInt_t slot ) const {
  Decoder::Module* module = GetModule(crate, slot);
  if( !module ) {
    std::cerr << "No module at crate " << crate << "   slot " << slot << std::endl;
    return false;
  }
  return module->IsMultiFunction();
}

inline
UInt_t THaEvData::GetNumEvents( Decoder::EModuleType type, UInt_t crate, UInt_t slot, UInt_t chan ) const {
  Decoder::Module* module = GetModule(crate, slot);
  if( !module ) return 0;
  if( module->HasCapability(type) ) {
    return module->GetNumEvents(type, chan);
  } else {
    return GetNumHits(crate, slot, chan);
  }
}

inline
UInt_t THaEvData::GetData( Decoder::EModuleType type, UInt_t crate, UInt_t slot, UInt_t chan, UInt_t hit ) const {
  Decoder::Module* module = GetModule(crate, slot);
  if( !module ) return 0;
  if( module->HasCapability(type) ) {
    if( hit >= module->GetNumEvents(type, chan) ) return 0;
    return module->GetData(type, chan, hit);
  } else {
    return GetData(crate, slot, chan, hit);
  }
}

inline
UInt_t THaEvData::GetLEbit( UInt_t crate, UInt_t slot, UInt_t chan, UInt_t hit ) const {
  // get the Leading Edge bit (works for fastbus)
  return GetOpt(crate, slot, chan, hit );
}

inline
UInt_t THaEvData::GetOpt( UInt_t crate, UInt_t slot, UInt_t /*chan*/, UInt_t hit ) const {
  // get the "Opt" bit (works for fastbus, is otherwise zero)
  Decoder::Module* module = GetModule(crate, slot);
  if( !module ) {
    std::cerr << "No module at crate " << crate << "   slot " << slot << std::endl;
    return 0;
  }
  return module->GetOpt(GetRawData(crate, slot, hit));
}

#endif
