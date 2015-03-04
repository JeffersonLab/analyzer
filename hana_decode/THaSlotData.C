/////////////////////////////////////////////////////////////////////
//
//   THaSlotData
//   Data in one slot of one crate from DAQ.
//
//   THaSlotData contains data from one slot of one crate
//   from a CODA event.  Public methods allow to obtain
//   raw data for this crate & slot, or to obtain TDC, ADC,
//   or scaler data for each channel in the slot.  Methods
//   clearEvent() and loadData() should only be used by
//   the decoder.  WARNING: For efficiency, only the
//   hit counters are zero'd each event, not the data
//   arrays, see below.
//
//   author  Robert Michaels (rom@jlab.org)
//
/////////////////////////////////////////////////////////////////////

#include "Decoder.h"
#include "Module.h"
#include "THaSlotData.h"
#include "THaCrateMap.h"
#include "TClass.h"
#include "TMath.h"
#include <iostream>
#include <cstring>

using namespace std;

namespace Decoder {

static const bool VERBOSE = true;
const int THaSlotData::DEFNCHAN = 128;  // Default number of channels
const int THaSlotData::DEFNDATA = 1024; // Default number of data words
const int THaSlotData::DEFNHITCHAN = 1; // Default number of hits per channel

THaSlotData::THaSlotData() :
  crate(-1), slot(-1), fModule(0), numhitperchan(0), numraw(0), numchanhit(0), firstfreedataidx(0),
  numholesdataidx(0), numHits(0), xnumHits(0), chanlist(0), idxlist (0), chanindex(0), dataindex(0),
  numMaxHits(0), rawData(0), data(0), fDebugFile(0), didini(false),
  maxc(0), maxd(0), allocd(0), alloci(0) {}

THaSlotData::THaSlotData(int cra, int slo) :
  crate(cra), slot(slo), fModule(0), numhitperchan(0), numraw(0), numchanhit(0), firstfreedataidx(0),
  numholesdataidx(0), numHits(0), xnumHits(0), chanlist(0), idxlist (0), chanindex(0), dataindex(0),
  numMaxHits(0), rawData(0), data(0), fDebugFile(0), didini(false),
  maxc(0), maxd(0), allocd(0), alloci(0) {}


THaSlotData::~THaSlotData() {
  if( !didini ) return;
  if(fModule) delete fModule;
  delete [] numHits;
  delete [] xnumHits;
  delete [] chanlist;
  delete [] idxlist;
  delete [] chanindex;
  delete [] dataindex;
  delete [] numMaxHits;
  delete [] rawData;
  delete [] data;
}

void THaSlotData::define(int cra, int slo, UShort_t nchan, UShort_t ndata, UShort_t nhitperchan ) {
  // Must call define once if you are really going to use this slot.
  // Otherwise its an empty slot which does not use much memory.
  crate = cra;
  slot = slo;
  didini = true;
  maxc = nchan;
  maxd = ndata;
  numhitperchan=nhitperchan;
  // Initial allocation of data arrays
  allocd = nchan;
  alloci = nchan;
  // Delete arrays if defined so we can call define() more than once!
  delete [] numHits;
  delete [] xnumHits;
  delete [] chanlist;
  delete [] idxlist;
  delete [] chanindex;
  delete [] dataindex;
  delete [] numMaxHits;
  delete [] rawData;
  delete [] data;
  numHits   = new UShort_t[maxc];
  xnumHits   = new Int_t[maxc];
  chanlist  = new UShort_t[maxc];
  idxlist  = new UShort_t[maxc];
  chanindex = new UShort_t[maxc];
  rawData   = new int[allocd];
  data      = new int[allocd];
  dataindex = new UShort_t[alloci];
  numMaxHits = new UShort_t[maxc];
  numchanhit = numraw = firstfreedataidx = numholesdataidx= 0;
  memset(numHits,0,maxc*sizeof(UShort_t));
  memset(xnumHits,0,maxc*sizeof(Int_t));
}

int THaSlotData::loadModule(const THaCrateMap *map) {

  int modelnum = map->getModel(crate, slot);

   Int_t err=0;

   for( Module::TypeIter_t it = Module::fgModuleTypes().begin();
       !err && it != Module::fgModuleTypes().end(); ++it ) {
    const Module::ModuleType& loctype = *it;

    // Get the ROOT class for this type

    if (fDebugFile) {
      *fDebugFile << "THaSlotData:: loctype.fClassName  "<< loctype.fClassName<<endl;
      *fDebugFile << "THaSlotData:: loctype.fMapNum  "<< loctype.fMapNum<<endl;
      *fDebugFile << "THaSlotData:: fTClass ptr =  "<<loctype.fTClass<<endl;
    }

    if( !loctype.fTClass ) {

      loctype.fTClass = TClass::GetClass( loctype.fClassName );
      if (fDebugFile) {
	 *fDebugFile << "defining fTClass ptr =  "<<loctype.fTClass<<endl;
      }

    }

    if (modelnum == loctype.fMapNum) {

      if (fDebugFile) *fDebugFile << "THaSlotData::  Found Module !!!! "<<dec<<modelnum<<endl;

      if (loctype.fTClass) {
	if (fDebugFile) *fDebugFile << "THaSlotData:: Creating fModule"<<endl;
	fModule= static_cast<Module*>( loctype.fTClass->New() );
	if (fDebugFile) *fDebugFile << "fModule return "<<fModule<<endl;

	if (!fModule) {
	  if (fDebugFile) *fDebugFile << "failure to make module on crate "<<dec<<crate<<"  slot "<<slot<<endl;
	  return -1;
	}
	// Init first, then SetSlot
	fModule->Init();
	fModule->SetSlot( crate, slot, map->getHeader(crate, slot), map->getMask(crate, slot), map->getModel(crate,slot));
	if (fDebugFile) *fDebugFile << "THaSlotData:: about to init  module   "<<crate<<"  "<<slot<<" mod ptr "<<fModule<<"  header "<<hex<<map->getHeader(crate,slot)<<"  model num "<<dec<<map->getModel(crate,slot)<<endl;
	if (fDebugFile) {
	  fModule->SetDebugFile(fDebugFile);
	  fModule->DoPrint();
	}
      } else {
	if (fDebugFile) *fDebugFile << "THaSlotData:: SERIOUS problem :  fTClass still zero "<<endl;
      }

    }

   }

   return 1;

}

Int_t THaSlotData::LoadIfSlot(const UInt_t* p, const UInt_t *pstop) {
  // returns how many words seen.
  Int_t wordseen = 0;
  if ( !fModule ) {
// This is bad and should not happen; it means you didn't define a module
// for this slot.  Check db_cratemap.dat, e.g. erase things that dont exist.
    cerr << "THaSlotData::ERROR:   No module defined for slot. "<<crate<<"  "<<slot<<endl;
    return 0;
  }
  if (fDebugFile) *fDebugFile << "THaSlotData::LoadIfSlot:  " << dec<<crate<<"  "<<slot<<"   p "<<hex<<p<<"  "<<*p<<"  "<<dec<<((UInt_t(*p))>>27)<<hex<<"  "<<pstop<<"  "<<fModule<<dec<<endl;
  fModule->DoPrint();
  if ( !fModule->IsSlot( *p ) ) {
    if(fDebugFile) *fDebugFile << "THaSlotData:: Not slot ... return ... "<<endl;
    return 0;
  }
  if (fDebugFile) fModule->DoPrint();
  fModule->Clear("");
  wordseen = fModule->LoadSlot(this, p, pstop);  // increments p
  if (fDebugFile) *fDebugFile << "THaSlotData:: after LoadIfSlot:  wordseen =  "<<dec<<"  "<<wordseen<<endl;
  return wordseen;
}


int THaSlotData::loadData(const char* type, int chan, int dat, int raw) {

  const int very_verb=1;

  if( !didini ) {
    if (very_verb) {  // this might be your problem.
      cout << "THaSlotData: ERROR: Did not init slot."<<endl;
      cout << "  Fix your cratemap."<<endl;
    }
    return SD_ERR;
  }
  if (chan < 0 || chan >= maxc) {
    if (VERBOSE) {
      cout << "THaSlotData: Warning in loadData: channel ";
      cout <<chan<<" out of bounds, ignored,"
	   << " on crate " << crate << " slot "<< slot << endl;
    }
    return SD_WARN;
  }
  if( numraw >= maxd || numchanhit > maxc) {
    if (VERBOSE) {
      cout << "(1) THaSlotData: Warning in loadData: too many "
	   << ((numraw >= maxd ) ? "data words" : "(x) channels")
	   << " for crate/slot = "
	   << crate << " " << slot;
	   cout << ": " << (numraw>=maxd ? numraw : numchanhit) << " seen."
	   << endl;
    }
    return SD_WARN;
  }
  if( device.IsNull() ) device = type;

  if (( numchanhit == 0 )||(numHits[chan]==0)) {
    compressdataindex(numhitperchan);
    dataindex[firstfreedataidx]=numraw;
    idxlist[chan]=firstfreedataidx;
    numMaxHits[chan]=numhitperchan;
    firstfreedataidx=firstfreedataidx+numhitperchan;
    chanindex[chan]=numchanhit;
    chanlist[numchanhit++]=chan;
  } else {
    if (numHits[chan]<numMaxHits[chan]) {
      dataindex[idxlist[chan]+numHits[chan]]=numraw;
    } else {
      if (idxlist[chan]+numMaxHits[chan]==firstfreedataidx) {
	compressdataindex(numhitperchan);
	dataindex[idxlist[chan]+numHits[chan]]=numraw;
	numMaxHits[chan]=numMaxHits[chan]+numhitperchan;
	firstfreedataidx=firstfreedataidx+numhitperchan;
      } else {
	compressdataindex(numMaxHits[chan]+numhitperchan);
	numholesdataidx=numholesdataidx+numMaxHits[chan];
	for (Int_t i=0; i<numHits[chan]; i++  ) {
	  dataindex[firstfreedataidx+i]=dataindex[idxlist[chan]+i];
	}
	dataindex[firstfreedataidx+numHits[chan]]=numraw;
	idxlist[chan]=firstfreedataidx;
	numMaxHits[chan]=numMaxHits[chan]+numhitperchan;
	firstfreedataidx=firstfreedataidx+numMaxHits[chan];
      }
    }
  }

  // Grow data arrays if really necessary (rare)
  if( numraw >= allocd ) {
    UShort_t old_allocd = allocd;
    allocd *= 2; if( allocd > maxd ) allocd = maxd;
    int* tmp = new int[allocd];
    memcpy(tmp,data,old_allocd*sizeof(int));
    delete [] data; data = tmp;
    tmp = new int[allocd];
    memcpy(tmp,rawData,old_allocd*sizeof(int));
    delete [] rawData; rawData = tmp;
  }
  rawData[numraw] = raw;
  data[numraw++]  = dat;
  if( numHits[chan] == kMaxUShort ) {
    cout << "(2)  maxd, etc "<<maxd<< "  "<<numchanhit<<"  "<<numraw<<endl;
    if( VERBOSE )
      cout << "(2) THaSlotData: Warning in loadData: too many hits "
	   << "for module " << device << " in crate/slot = "
	   << dec << crate << " " << slot
	   << " chan = " << chan << endl;
    return SD_WARN;
  }
  numHits[chan]++;
  xnumHits[chan]++;
  return SD_OK;
}

int THaSlotData::loadData(int chan, int dat, int raw) {
  // NEW (6/2014).
  return loadData(NULL, chan, dat, raw);
}


void THaSlotData::print() const {
  if (fDebugFile) {
    print_to_file();
    return;
  }
  cout << "\n THaSlotData contents : " << endl;
  cout << "This is crate "<<dec<<crate<<" and slot "<<slot<<endl;
  cout << "Total Amount of Data : " << dec << getNumRaw() << endl;
  if (getNumRaw() > 0) {
    cout << "Raw Data Dump: " << hex << endl;
  } else {
    if(getNumRaw() == SD_ERR) cout << "getNumRaw ERROR"<<endl;
    return;
  }
  int i,j,k,chan,hit;
  bool first;
  k = 0;
  for (i=0; i<getNumRaw()/5; i++) {
    for(j=0; j<5; j++) cout << getRawData(k++) << "  ";
    cout << endl;
  }
  for (i=k; i<getNumRaw(); i++) cout << getRawData(i) << "  ";
  first = true;
  for (chan=0; chan<maxc; chan++) {
    if (getNumHits(chan) > 0) {
      if (first) {
	cout << "\nThis is "<<devType()<<" Data : "<<endl;
	first = false;
      }
      cout << dec << "Channel " << chan << "  ";
      cout << "numHits : " << getNumHits(chan) << endl;
      for (hit=0; hit<getNumHits(chan); hit++) {
	cout << "Hit # "<<dec<<hit;
	if(getData(chan,hit) == SD_ERR) cout<<"ERROR: getData"<<endl;
	cout << "  Data  = (hex) "<<hex<<getData(chan,hit);
	cout << "   or (decimal) = "<<dec<<getData(chan,hit)<<endl;
      }
    }
  }
  return;
}

void THaSlotData::print_to_file() const {
  if (!fDebugFile) return;
  *fDebugFile << "\n THaSlotData contents : " << endl;
  *fDebugFile << "This is crate "<<dec<<crate<<" and slot "<<slot<<endl;
  *fDebugFile << "Total Amount of Data : " << dec << getNumRaw() << endl;
  if (getNumRaw() > 0) {
    *fDebugFile << "Raw Data Dump: " << hex << endl;
  } else {
    if(getNumRaw() == SD_ERR) *fDebugFile << "getNumRaw ERROR"<<endl;
    return;
  }
  int i,j,k,chan,hit;

  bool first;
  k = 0;
  for (i=0; i<getNumRaw()/5; i++) {
    for(j=0; j<5; j++) *fDebugFile << getRawData(k++) << "  ";
    *fDebugFile << endl;
  }
  for (i=k; i<getNumRaw(); i++) *fDebugFile << getRawData(i) << "  ";
  first = true;
  for (chan=0; chan<maxc; chan++) {
    if (getNumHits(chan) > 0) {
      if (first) {
	*fDebugFile << "\nThis is "<<devType()<<" Data : "<<endl;
	first = false;
      }
      *fDebugFile << dec << "Channel " << chan << "  ";
      *fDebugFile << "numHits : " << getNumHits(chan) << endl;
      for (hit=0; hit<getNumHits(chan); hit++) {
	*fDebugFile << "Hit # "<<dec<<hit;
	if(getData(chan,hit) == SD_ERR) *fDebugFile<<"ERROR: getData"<<endl;
	*fDebugFile << "  Data  = (hex) "<<hex<<getData(chan,hit);
	*fDebugFile << "   or (decimal) = "<<dec<<getData(chan,hit)<<endl;
      }
    }
  }
  return;
}

}

ClassImp(Decoder::THaSlotData)
