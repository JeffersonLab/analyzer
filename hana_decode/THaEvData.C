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
//   implement LoadEvent(const int*, THaCrateMap*).  See the header.
//
//   
//   original author  Robert Michaels (rom@jlab.org)
//
//   modified for abstraction by Ken Rossato (rossato@jlab.org)
//
//
/////////////////////////////////////////////////////////////////////

#include "THaEvData.h"
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
static const int DEBUG   = 1;
static const int BENCH   = 0;

// Instances of this object
TBits THaEvData::fgInstances;

const Double_t THaEvData::kBig = 1e38;

// If false, signal attempted use of unimplemented features
#ifndef NDEBUG
Bool_t THaEvData::fgAllowUnimpl = false;
#else
Bool_t THaEvData::fgAllowUnimpl = true;
#endif

//_____________________________________________________________________________

THaEvData::THaEvData() :
  first_load(true), first_decode(true), fTrigSupPS(true),
  buffer(0), run_num(0), run_type(0), fRunTime(0), evt_time(0),
  recent_event(0), fNSlotUsed(0), fNSlotClear(0), fMap(0)
{
  fInstance = fgInstances.FirstNullBit();
  fgInstances.SetBitNumber(fInstance);
  fInstance++;
  crateslot = new THaSlotData*[MAXROC*MAXSLOT];
  cmap = new THaCrateMap;
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
  delete cmap;
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
  // can't initialize what doesn't exist.  Go see THaCodaDecoder
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

// KCR: wow this function looks cool.  I want one =P
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

ClassImp(THaEvData)
ClassImp(THaBenchmark)

