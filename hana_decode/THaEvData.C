/////////////////////////////////////////////////////////////////////
//
//   THaEvData
//   Hall A Event Data from One "Event"
//
//   This is a pure virtual base class.  You should not
//   instantiate this directly (and actually CAN not), but
//   rather use THaCodaDecoder (the former THaEvData), or (more 
//   unlikely) a sim class like THaVDCSimDecoder.
//   
//   This class is intended to provide a crate/slot structure
//   for derived classes to use.  This class does NOT provide
//   Epics, fastbus, or any fancy stuff that I don't understand
//   (I'm just a lowly student).  Helicity is provided but defaults
//   to NULL and zero results.  Derived classes should implement
//   this if they can. Also, all derived class must define and 
//   implement LoadEvent(const int*, THaCrateMap*).  See the header.
//
//   
//   original author  Robert Michaels (rom@jlab.org)
//
//   modified for abstraction by Ken Rossato (rossato@jlab.org)
//   This blurb also by Ken Rossato, comments labeled KCR
//
//
/////////////////////////////////////////////////////////////////////

#include "THaEvData.h"
#include "THaHelicity.h"
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

static const int VERBOSE = 1;
static const int DEBUG   = 0;
static const int BENCH   = 0;

// Instances of this object
TBits THaEvData::fgInstances;

//_____________________________________________________________________________

THaEvData::THaEvData() :
  helicity(0), first_load(true), first_decode(true), fgTrigSupPS(true),
  buffer(0), run_num(0), run_type(0), run_time(0), 
  recent_event(0),
  dhel(0.0), dtimestamp(0.0), fNSlotUsed(0), fNSlotClear(0), fMap(0)
{
  fInstance = fgInstances.FirstNullBit();
  fgInstances.SetBitNumber(fInstance);
  fInstance++;
  //  epics = new THaEpics;
  crateslot = new THaSlotData*[MAXROC*MAXSLOT];
  cmap = new THaCrateMap;
  //fb = new THaFastBusWord;
  fSlotUsed  = new UShort_t[MAXROC*MAXSLOT];
  fSlotClear = new UShort_t[MAXROC*MAXSLOT];
  //memset(psfact,0,MAX_PSFACT*sizeof(int));
  memset(crateslot,0,MAXROC*MAXSLOT*sizeof(THaSlotData*));
  run_time = time(0); // default run_time is NOW
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
    TString prefix("g");
    // Allow global variable clash if there are several instances of us
    if( fInstance > 1 )
      prefix.Append(Form("%d",fInstance));
    prefix.Append(".");
#ifndef STANDALONE
    gHaVars->DefineVariables( vars, prefix, "THaEvData::THaEvData" );
#endif
  } else
    Warning("THaEvData::THaEvData","No global variable list found. "
	    "Variables not registered.");

#endif
  fDoBench = BENCH;
  if( fDoBench ) {
    fBench = new THaBenchmark;
  } else
    fBench = NULL;

  // Enable scalers by default
  //EnableScalers();
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
  //  delete epics;
  delete helicity;
  delete cmap;
  //delete fb;
  delete [] fSlotUsed;
  delete [] fSlotClear;
  fInstance--;
  fgInstances.SetBitNumber(fInstance,kFALSE);
}

int THaEvData::GetPrescaleFactor(int trigger_type) const {
// To get the prescale factors for trigger number "trigger_type"
// (valid types are 1,2,3...)
  if (VERBOSE) {
    cout << "THaEvData: Warning:  Requested prescale factor ";
    cout << dec << trigger_type;
    cout << " from non-Coda class" << endl;
  }
  return 0;
}

const char* THaEvData::DevType(int crate, int slot) const {
// Device type in crate, slot
  return ( GoodIndex(crate,slot) ) ?
    crateslot[idx(crate,slot)]->devType() : " ";
}

Double_t THaEvData::GetEvTime() const {
  // I'm gonna let helicity related functions get away with it,
  // because they properly check for NULL helicity -KCR

  return helicity ? helicity->GetTime() : 0.0;
}

int THaEvData::GetScaler(const TString& spec, int slot, int chan) const {
  // Can't do this, because we dont read in a crate map
  return GetScaler(0,0,0);
}

int THaEvData::GetScaler(int roc, int slot, int chan) const {
  // Can't do this, because we dont read in a crate map
  if (VERBOSE) {
    cout << "THaEvData: Warning: GetScaler called ";
    cout << "for non-Coda class" << endl;
  }
  return 0;
}

int THaEvData::GetHelicity() const {
  return helicity ? (int)helicity->GetHelicity() : 0;
}

int THaEvData::GetHelicity(const TString& spec) const {
  return helicity ? (int)helicity->GetHelicity(spec) : 0;
}

Bool_t THaEvData::IsLoadedEpics(const char* tag) const {
  // Oops, no Epics in base class, sorry

  if (VERBOSE) {
    cout << "THaEvData: Warning: IsLoadedEpics called ";
    cout << "for non-Coda class" << endl;
  }
  return false;
}

double THaEvData::GetEpicsData(const char* tag, int event) const {
  if (VERBOSE) {
    cout << "THaEvData: Warning: GetEpicsData called ";
    cout << "for non-Coda class" << endl;
  }
  return 0;
}

std::string THaEvData::GetEpicsString(const char* tag, int event) const {
  if (VERBOSE) {
    cout << "THaEvData: Warning: GetEpicsString called ";
    cout << "for non-Coda class" << endl;
  }
  return 0;
}

double THaEvData::GetEpicsTime(const char* tag, int event) const {
  if (VERBOSE) {
    cout << "THaEvData: Warning: GetEpicsTime called ";
    cout << "for non-Coda class" << endl;
  }
  return 0;
}

void THaEvData::SetRunTime( UInt_t tloc )
{
  // Set run time and re-initialize crate map (and possibly other
  // database parameters for the new time.
  if( run_time == tloc ) 
    return;
  run_time = tloc;

  init_cmap();
  // can't initialize what doesn't exist.  Go see THaCodaDecoder
}

void THaEvData::EnableHelicity( Bool_t enable ) 
{
  // Enable/disable helicity decoding
  if( enable )
    SetBit(kHelicityEnabled);
  else
    ResetBit(kHelicityEnabled);
}

Bool_t THaEvData::HelicityEnabled() const
{
  // Test if helicity decoding enabled
  return TestBit(kHelicityEnabled);
}

void THaEvData::EnableScalers( Bool_t enable ) 
{
  // Enable/disable scaler decoding
  if( enable )
    SetBit(kScalersEnabled);
  else
    ResetBit(kScalersEnabled);
}

Bool_t THaEvData::ScalersEnabled() const
{
  // Test if scalar decoding enabled
  return TestBit(kScalersEnabled);
}

void THaEvData::SetOrigPS(Int_t evtyp)
{
  fgTrigSupPS = true;  // default after Nov 2003
  if (evtyp == PRESCALE_EVTYPE) {
    fgTrigSupPS = false;
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
  if (fgTrigSupPS) {
    answer += " Trig Sup evtype ";
    answer.Append(Form("%d",TS_PRESCALE_EVTYPE));
  } else {
    answer += " PS evtype ";
    answer.Append(Form("%d",PRESCALE_EVTYPE));
  }
  return answer;
}

// KCR: wow this function looks cool.  I want one =P
void THaEvData::hexdump(const char* cbuff, size_t nlen)
{
  // Hexdump buffer 'cbuff' of length 'nlen'
  const int NW = 16; const char* p = cbuff;
  while( p<cbuff+nlen ) {
    cout << dec << setw(4) << setfill('0') << (size_t)(p-cbuff) << " ";
    int nelem = TMath::Min(NW,cbuff+nlen-p);
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

// To initialize the crate map member if it is to be used.
int THaEvData::init_cmap()  {
  if (DEBUG) cout << "Init crate map " << endl;
  if( cmap->init(GetRunTime()) == THaCrateMap::CM_ERR )
    return HED_ERR;
  return HED_OK;
}

void THaEvData::makeidx(int crate, int slot)
{
  // Activate crate/slot
  int idx = slot+MAXSLOT*crate;
  delete crateslot[idx];  // just in case
  crateslot[idx] = new THaSlotData(crate,slot);
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
  // KCR: I need to write this function at some point
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

ClassImp(THaEvData)

