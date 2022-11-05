////////////////////////////////////////////////////////////////////
//
//   THaEvData
//   Hall A Event Data from One "Event"
//
//   This is a pure virtual base class.  You should not
//   instantiate this directly (and actually CAN not), but
//   rather use THaCodaDecoder or (less likely) a sim class like
//   THaVDCSimDecoder.
//
//   This class is intended to provide a crate/slot structure
//   for derived classes to use.  All derived class must define and
//   implement LoadEvent(const int*).  See the header.
//
//   original author  Robert Michaels (rom@jlab.org)
//
//   modified for abstraction by Ken Rossato (rossato@jlab.org)
//
//
/////////////////////////////////////////////////////////////////////

#include "THaEvData.h"
#include "Module.h"
#include "Helper.h"
#include "THaSlotData.h"
#include "THaCrateMap.h"
#include "THaBenchmark.h"
#include "TError.h"
#include <cctype>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <algorithm>
#include <cassert>
#include <utility>
#include <stdexcept>
#include <sstream>

using namespace std;
using namespace Decoder;

// Instances of this object
TBits THaEvData::fgInstances;

const Double_t THaEvData::kBig = 1e38;
static constexpr auto MAXROCSLOT = MAXROC * MAXSLOT;

// If false, signal attempted use of unimplemented features
#ifndef NDEBUG
Bool_t THaEvData::fgAllowUnimpl = false;
#else
Bool_t THaEvData::fgAllowUnimpl = true;
#endif

TString THaEvData::fgDefaultCrateMapName = "cratemap";

//_____________________________________________________________________________
THaEvData::THaEvData() :
  fMap{nullptr},
  // FIXME: allocate dynamically?
  crateslot(MAXROCSLOT),
  first_decode{true},
  fTrigSupPS{true},
  fDataVersion{0},
  fEpicsEvtType{Decoder::EPICS_EVTYPE},  // default for Hall A
  buffer{nullptr},
  fDebugFile{nullptr},
  event_type{0},
  event_length{0},
  event_num{0},
  run_num{0},
  run_type{0},
  data_type{0},
  trigger_bits{0},
  evscaler{0},
  fRunTime(time(nullptr)),    // default fRunTime is NOW
  evt_time{0},
  fDoBench{false},
  fInstance{fgInstances.FirstNullBit()},
  fNeedInit{true},
  fDebug{0},
  fExtra{nullptr}
{
  fSlotUsed.reserve(MAXROCSLOT/4);  // Generous space for a typical setup
  fSlotClear.reserve(MAXROCSLOT/4);
  fgInstances.SetBitNumber(fInstance);
  fInstance++;
}

//_____________________________________________________________________________
THaEvData::~THaEvData() {
  if( fDoBench ) {
    Float_t a,b;
    fBench->Summary(a,b);
  }
  delete fExtra;
  fInstance--;
  fgInstances.ResetBitNumber(fInstance);
}

//_____________________________________________________________________________
const char* THaEvData::DevType( UInt_t crate, UInt_t slot) const {
// Device type in crate, slot
  return ( GoodIndex(crate,slot) ) ?
    crateslot[idx(crate,slot)]->devType() : " ";
}

//_____________________________________________________________________________
Int_t THaEvData::Init()
{
  Int_t ret = init_cmap();
  if( ret != HED_OK ) {
    ::Error("THaEvData::init", "Error initializing crate map. "
                               "Cannot continue. Fix database.");
    return ret;
  }
  if( fDebugFile ) {
    *fDebugFile << endl << " THaEvData:: Print of Crate Map" << endl;
    fMap->print(*fDebugFile);
  }
  //  if (fMap) fMap->print();
  ret = init_slotdata();
  if( ret != HED_OK ) return ret;
  first_decode = false;
  fNeedInit = false;
  fMsgPrinted.ResetAllBits();
  return ret;
}

//_____________________________________________________________________________
void THaEvData::SetRunTime( ULong64_t tloc )
{
  // Set run time and re-initialize crate map (and possibly other
  // database parameters for the new time.
  if( fRunTime == tloc )
    return;
  fRunTime = tloc;

  init_cmap();
}

//_____________________________________________________________________________
void THaEvData::EnableBenchmarks( Bool_t enable )
{
  // Enable/disable run time reporting
  fDoBench = enable;
  if( fDoBench ) {
    if( !fBench ) {
#if __cplusplus >= 201402L
      fBench = make_unique<THaBenchmark>();
#else
      fBench.reset(new THaBenchmark);
#endif
    }
  } else {
    fBench = nullptr;
  }
}

//_____________________________________________________________________________
void THaEvData::EnableHelicity( Bool_t enable )
{
  // Enable/disable helicity decoding
  SetBit(kHelicityEnabled, enable);
}

//_____________________________________________________________________________
void THaEvData::EnableScalers( Bool_t enable )
{
  // Enable/disable scaler decoding
  SetBit(kScalersEnabled, enable);
}

//_____________________________________________________________________________
void THaEvData::EnablePrescanMode( Bool_t enable )
{
  // Enable/disable prescan mode
  SetBit(kPrescanMode, enable);
}

//_____________________________________________________________________________
void THaEvData::SetVerbose( Int_t level )
{
  // Set verbosity level. Identical to SetDebug(). Kept for compatibility.

  SetDebug(level);
}

//_____________________________________________________________________________
void THaEvData::SetDebug( Int_t level )
{
  // Set debug level

  fDebug = level;
}

//_____________________________________________________________________________
void THaEvData::SetOrigPS(Int_t evtyp)
{
  switch(evtyp) {
  case TS_PRESCALE_EVTYPE: // default after Nov 2003
    fTrigSupPS = true;
    break;
  case PRESCALE_EVTYPE:
    fTrigSupPS = false;
    break;
  default:
    cerr << "SetOrigPS::Warn: PS factors";
    cerr << " originate only from evtype ";
    cerr << PRESCALE_EVTYPE << "  or ";
    cerr << TS_PRESCALE_EVTYPE << endl;
    break;
  }
}

//_____________________________________________________________________________
TString THaEvData::GetOrigPS() const
{
  TString answer = "PS from ";
  if (fTrigSupPS) {
    answer += " Trig Sup evtype ";
    answer.Append(Form("%d",TS_PRESCALE_EVTYPE));
  } else {
    answer += " PS evtype ";
    answer.Append(Form("%d",PRESCALE_EVTYPE));
  }
  return answer;
}

//_____________________________________________________________________________
void THaEvData::hexdump(const char* cbuff, size_t nlen)
{
  // Hexdump buffer 'cbuff' of length 'nlen'
  const int NW = 16; const char* p = cbuff;
  while( p<cbuff+nlen ) {
    cout << dec << setw(4) << setfill('0') << (size_t)(p-cbuff) << " ";
    Long_t nelem = TMath::Min((Long_t)NW,(Long_t)(cbuff+nlen-p));
    for(int i=0; i<NW; i++) {
      UInt_t c = (i<nelem) ? *(const unsigned char*)(p+i) : 0;
      cout << " " << hex << setfill('0') << setw(2) << c << dec;
    } cout << setfill(' ') << "  ";
    for(int i=0; i<NW; i++) {
      char c = (i<nelem) ? *(p+i) : (char)0;
      if(isgraph(c)||c==' ') cout << c; else cout << ".";
    } cout << endl;
    p += NW;
  }
}

//_____________________________________________________________________________
void THaEvData::SetDefaultCrateMapName( const char* name )
{
  // Static function to set fgDefaultCrateMapName. Call this function to set a
  // global default name for all decoder instances before initialization. This
  // is usually what you want to do for a given replay.

  if( name && *name ) {
    fgDefaultCrateMapName = name;
  }
  else {
    ::Error( "THaEvData::SetDefaultCrateMapName", "Default crate map name "
	     "must not be empty" );
  }
}

//_____________________________________________________________________________
void THaEvData::SetCrateMapName( const char* name )
{
  // Set fCrateMapName for this decoder instance only

  if( name && *name ) {
    if( fCrateMapName != name ) {
      fCrateMapName = name;
      fNeedInit = true;
    }
  } else if( fCrateMapName != fgDefaultCrateMapName ) {
    fCrateMapName = fgDefaultCrateMapName;
    fNeedInit = true;
  }
}

//_____________________________________________________________________________
// Set up and initialize the crate map
int THaEvData::init_cmap()  {
  if( fCrateMapName.IsNull() )
    fCrateMapName = fgDefaultCrateMapName;
  // Make a new crate map object unless we already have one
  if( !fMap || fCrateMapName != fMap->GetName() ) {
#if __cplusplus >= 201402L
    fMap = make_unique<THaCrateMap>(fCrateMapName);
#else
    fMap.reset(new THaCrateMap(fCrateMapName));
#endif
  }
  // Initialize the newly created crate map
  if( fDebug>0 )
    cout << "Initializing crate map " << endl;
  if( fMap->init(GetRunTime()) != THaCrateMap::CM_OK )
    return HED_FATAL; // Can't continue w/o cratemap
  fNeedInit = false;
  return HED_OK;
}

//_____________________________________________________________________________
void THaEvData::makeidx( UInt_t crate, UInt_t slot )
{
  // Activate crate/slot
  UShort_t idx = slot + MAXSLOT*crate;
  if( !crateslot[idx] ) {
#if __cplusplus >= 201402L
    crateslot[idx] = make_unique<THaSlotData>(crate, slot);
#else
    crateslot[idx].reset(new THaSlotData(crate, slot));
#endif
  }
#ifndef NDEBUG
  else {
    // Slot already defined
    assert(crateslot[idx]->getCrate() == crate &&
           crateslot[idx]->getSlot() == slot);
  }
#endif
  if (fDebugFile) crateslot[idx]->SetDebugFile(fDebugFile);
  if( !fMap ) return;
  if( fMap->crateUsed(crate) && fMap->slotUsed(crate,slot)) {
    crateslot[idx]
      ->define( crate, slot, fMap->getNchan(crate,slot) );
// ndata no longer used
//    ->define( crate, slot, fMap->getNchan(crate,slot),
//		fMap->getNdata(crate,slot) );
    // Prevent duplicates in fSlotUsed and fSlotClear
    if( find(ALL(fSlotUsed), idx) == fSlotUsed.end() )
      fSlotUsed.push_back(idx);
    if( fMap->slotClear(crate,slot) &&
        find(ALL(fSlotClear), idx) == fSlotClear.end() )
      fSlotClear.push_back(idx);
    if( crateslot[idx]->loadModule(fMap.get()) != SD_OK ) {
      ostringstream ostr;
      ostr << "Failed to initialize decoder for crate " << crate << " "
           << "slot " << slot << ". Fix database or call expert.";
      throw runtime_error(ostr.str());
    }
  }
}

//_____________________________________________________________________________
void THaEvData::PrintOut() const {
  //TODO
  cout << "THaEvData::PrintOut() called" << endl;
}

//_____________________________________________________________________________
void THaEvData::PrintSlotData( UInt_t crate, UInt_t slot) const {
  // Print the contents of (crate, slot).
  if( GoodIndex(crate,slot)) {
    crateslot[idx(crate,slot)]->print();
  } else {
      cout << "THaEvData: Warning: Crate, slot combination";
      cout << "\nexceeds limits.  Cannot print"<<endl;
  }
}

//_____________________________________________________________________________
// To initialize the THaSlotData member on first call to decoder
int THaEvData::init_slotdata()
{
  // Update lists of used/clearable slots in case crate map changed
  if(!fMap) return HED_ERR;
  for( auto it = fSlotUsed.begin(); it != fSlotUsed.end(); ) {
    THaSlotData* pSlotData = crateslot[*it].get();
    UInt_t crate = pSlotData->getCrate();
    UInt_t slot  = pSlotData->getSlot();
    if( !fMap->crateUsed(crate) || !fMap->slotUsed(crate, slot) ||
        !fMap->slotClear(crate, slot) ) {
      auto jt = find(ALL(fSlotClear), *it);
      if( jt != fSlotClear.end() )
        fSlotClear.erase(jt);
    }
    if( !fMap->crateUsed(crate) || !fMap->slotUsed(crate, slot) ) {
      assert( find(ALL(fSlotClear), *it) == fSlotClear.end() );
      crateslot[*it].reset();  // Release unused resources
      it = fSlotUsed.erase(it);
    } else
      ++it;
  }
  return HED_OK;
}

//_____________________________________________________________________________
void THaEvData::FindUsedSlots() {
  // Disable slots for which no module is defined.
  // This speeds up the decoder.
  vector< pair<UInt_t,UInt_t> > to_unset;
  for( auto roc : fMap->GetUsedCrates() ) {
    for( auto slot : fMap->GetUsedSlots(roc) ) {
      assert( fMap->slotUsed(roc,slot) );
      if ( !crateslot[idx(roc,slot)]->GetModule() ) {
	cout << "WARNING:  No module defined for crate "<<roc<<"   slot "<<slot<<endl;
	cout << "Check db_cratemap.dat for module that is undefined"<<endl;
	cout << "This crate, slot will be ignored"<<endl;
        // To avoid undefined behavior, save the roc/slot numbers here so we
        // can call setUnused outside of these loops. setUnused modifies the
        // vectors underlying the loops.
        to_unset.emplace_back(roc, slot);
      }
    }
  }
  for( const auto& cs : to_unset )
    fMap->setUnused(cs.first, cs.second);
}

//_____________________________________________________________________________
Module* THaEvData::GetModule( UInt_t roc, UInt_t slot) const
{
  if( crateslot[idx(roc,slot)] )
    return crateslot[idx(roc,slot)]->GetModule();
  return nullptr;
}

//_____________________________________________________________________________
Int_t THaEvData::SetDataVersion( Int_t version )
{
  return (fDataVersion = version);
}

//_____________________________________________________________________________
void THaEvData::SetRunInfo( UInt_t num, UInt_t type, ULong64_t tloc )
{
  run_num = num;
  run_type = type;
  SetRunTime(tloc);
}

//_____________________________________________________________________________
ClassImp(THaEvData)
ClassImp(THaBenchmark)
