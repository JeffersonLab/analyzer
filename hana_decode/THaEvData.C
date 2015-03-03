/////////////////////////////////////////////////////////////////////
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
#include "THaSlotData.h"
#include "THaFastBusWord.h"
#include "THaCrateMap.h"
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
using namespace Decoder;

// Instances of this object
TBits THaEvData::fgInstances;

const Double_t THaEvData::kBig = 1e38;

// If false, signal attempted use of unimplemented features
#ifndef NDEBUG
Bool_t THaEvData::fgAllowUnimpl = false;
#else
Bool_t THaEvData::fgAllowUnimpl = true;
#endif

TString THaEvData::fgDefaultCrateMapName = "cratemap";

//_____________________________________________________________________________

THaEvData::THaEvData() :
  fMap(0), first_decode(true), fTrigSupPS(true),
  buffer(0), fDebugFile(0), run_num(0), run_type(0), fRunTime(0),
  evt_time(0), recent_event(0), fNSlotUsed(0), fNSlotClear(0),
  fDoBench(kFALSE), fBench(0), fNeedInit(true), fDebug(0)
{
  fInstance = fgInstances.FirstNullBit();
  fgInstances.SetBitNumber(fInstance);
  fInstance++;
  // FIXME: dynamic allocation
  crateslot = new THaSlotData*[MAXROC*MAXSLOT];
  fSlotUsed  = new UShort_t[MAXROC*MAXSLOT];
  fSlotClear = new UShort_t[MAXROC*MAXSLOT];
  //memset(psfact,0,MAX_PSFACT*sizeof(int));
  memset(crateslot,0,MAXROC*MAXSLOT*sizeof(THaSlotData*));
  fRunTime = time(0); // default fRunTime is NOW
#ifndef STANDALONE
// Register global variables.
  if( gHaVars ) {
    VarDef vars[] = {
      { "runnum",    "Run number",     kInt,    0, &run_num },
      { "runtype",   "CODA run type",  kInt,    0, &run_type },
      { "runtime",   "CODA run time",  kULong,  0, &fRunTime },
      { "evnum",     "Event number",   kInt,    0, &event_num },
      { "evtyp",     "Event type",     kInt,    0, &event_type },
      { "evlen",     "Event Length",   kInt,    0, &event_length },
      { "evtime",    "Event time",     kULong,  0, &evt_time },
      { 0 }
    };
    TString prefix("g");
    // Prevent global variable clash if there are several instances of us
    if( fInstance > 1 )
      prefix.Append(Form("%d",fInstance));
    prefix.Append(".");
    gHaVars->DefineVariables( vars, prefix, "THaEvData::THaEvData" );
  } else
    Warning("THaEvData::THaEvData","No global variable list found. "
	    "Variables not registered.");
#endif
}


THaEvData::~THaEvData() {
  if( fDoBench ) {
    Float_t a,b;
    fBench->Summary(a,b);
  }
  delete fBench;
#ifndef STANDALONE
  if( gHaVars ) {
    TString prefix("g");
    if( fInstance > 1 )
      prefix.Append(Form("%d",fInstance));
    prefix.Append(".*");
    gHaVars->RemoveRegexp( prefix );
  }
#endif
  // We must delete every array element since not all may be in fSlotUsed.
  for( int i=0; i<MAXROC*MAXSLOT; i++ )
    delete crateslot[i];
  delete [] crateslot;
  delete [] fSlotUsed;
  delete [] fSlotClear;
  fInstance--;
  fgInstances.ResetBitNumber(fInstance);
}

const char* THaEvData::DevType(int crate, int slot) const {
// Device type in crate, slot
  return ( GoodIndex(crate,slot) ) ?
    crateslot[idx(crate,slot)]->devType() : " ";
}

void THaEvData::SetRunTime( ULong64_t tloc )
{
  // Set run time and re-initialize crate map (and possibly other
  // database parameters for the new time.
  if( fRunTime == tloc )
    return;
  fRunTime = tloc;

  init_cmap();
}

void THaEvData::EnableBenchmarks( Bool_t enable )
{
  // Enable/disable run time reporting
  fDoBench = enable;
  if( fDoBench ) {
    if( !fBench )
      fBench = new THaBenchmark;
  } else {
    delete fBench;
    fBench = 0;
  }
}

void THaEvData::EnableHelicity( Bool_t enable )
{
  // Enable/disable helicity decoding
  SetBit(kHelicityEnabled, enable);
}

void THaEvData::EnableScalers( Bool_t enable )
{
  // Enable/disable scaler decoding
  SetBit(kScalersEnabled, enable);
}

void THaEvData::SetVerbose( UInt_t level )
{
  // Set verbosity level. Identical to SetDebug(). Kept for compatibility.

  SetDebug(level);
}

void THaEvData::SetDebug( UInt_t level )
{
  // Set debug level

  fDebug = level;
}

void THaEvData::SetOrigPS(Int_t evtyp)
{
  fTrigSupPS = true;  // default after Nov 2003
  if (evtyp == PRESCALE_EVTYPE) {
    fTrigSupPS = false;
    return;
  } else if (evtyp != TS_PRESCALE_EVTYPE) {
    cout << "SetOrigPS::Warn: PS factors";
    cout << " originate only from evtype ";
    cout << PRESCALE_EVTYPE << "  or ";
    cout << TS_PRESCALE_EVTYPE << endl;
  }
}

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

void THaEvData::hexdump(const char* cbuff, size_t nlen)
{
  // Hexdump buffer 'cbuff' of length 'nlen'
  const int NW = 16; const char* p = cbuff;
  while( p<cbuff+nlen ) {
    cout << dec << setw(4) << setfill('0') << (size_t)(p-cbuff) << " ";
    int nelem = TMath::Min((Long_t)NW,(Long_t)(cbuff+nlen-p));
    for(int i=0; i<NW; i++) {
      UInt_t c = (i<nelem) ? *(const unsigned char*)(p+i) : 0;
      cout << " " << hex << setfill('0') << setw(2) << c << dec;
    } cout << setfill(' ') << "  ";
    for(int i=0; i<NW; i++) {
      char c = (i<nelem) ? *(p+i) : 0;
      if(isgraph(c)||c==' ') cout << c; else cout << ".";
    } cout << endl;
    p += NW;
  }
}

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

// Set up and initialize the crate map
int THaEvData::init_cmap()  {
  if( fCrateMapName.IsNull() )
    fCrateMapName = fgDefaultCrateMapName;
  if( !fMap || fNeedInit || fCrateMapName != fMap->GetName() ) {
    delete fMap;
    fMap = new THaCrateMap( fCrateMapName );
  }
  if( fDebug>0 ) cout << "Init crate map " << endl;
  if( fMap->init(GetRunTime()) == THaCrateMap::CM_ERR )
    return HED_FATAL; // Can't continue w/o cratemap
  fNeedInit = false;
  return HED_OK;
}

void THaEvData::makeidx(int crate, int slot)
{
  // Activate crate/slot
  int idx = slot+MAXSLOT*crate;
  delete crateslot[idx];  // just in case
  crateslot[idx] = new THaSlotData(crate,slot);
  if (fDebugFile) crateslot[idx]->SetDebugFile(fDebugFile);
  if( !fMap ) return;
  if( fMap->crateUsed(crate) && fMap->slotUsed(crate,slot)) {
    crateslot[idx]
      ->define( crate, slot, fMap->getNchan(crate,slot),
		fMap->getNdata(crate,slot) );
    fSlotUsed[fNSlotUsed++] = idx;
    if( fMap->slotClear(crate,slot))
      fSlotClear[fNSlotClear++] = idx;
  }
}

void THaEvData::PrintOut() const {
  //TODO
  cout << "THaEvData::PrintOut() called" << endl;
}

void THaEvData::PrintSlotData(int crate, int slot) const {
  // Print the contents of (crate, slot).
  if( GoodIndex(crate,slot)) {
    crateslot[idx(crate,slot)]->print();
  } else {
      cout << "THaEvData: Warning: Crate, slot combination";
      cout << "\nexceeds limits.  Cannot print"<<endl;
  }
  return;
}

// To initialize the THaSlotData member on first call to decoder
int THaEvData::init_slotdata(const THaCrateMap* map)
{
  // Update lists of used/clearable slots in case crate map changed
  if(!map) return HED_ERR;
  for( int i=0; i<fNSlotUsed; i++ ) {
    THaSlotData* module = crateslot[fSlotUsed[i]];
    int crate = module->getCrate();
    int slot  = module->getSlot();
    if( !map->crateUsed(crate) || !map->slotUsed(crate,slot) ||
	!map->slotClear(crate,slot)) {
      for( int k=0; k<fNSlotClear; k++ ) {
	if( module == crateslot[fSlotClear[k]] ) {
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
Module* THaEvData::GetModule(Int_t roc, Int_t slot)
{
  THaSlotData *sldat = crateslot[idx(roc,slot)];
  if (sldat) return sldat->GetModule();
  return NULL;
}

ClassImp(THaEvData)
ClassImp(THaBenchmark)
