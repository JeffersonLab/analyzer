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

#include "THaSlotData.h"
#include "TMath.h"
#include <iostream>
#include <cstring>

using namespace std;

static const bool VERBOSE = true;

THaSlotData::THaSlotData() : 
  crate(-1), slot(-1), numraw(0), numchanhit(0), numHits(0),
  chanlist(0), chanindex(0), rawData(0), data(0), didini(false),
  maxc(0), maxd(0), allocd(0) {}

THaSlotData::THaSlotData(int cra, int slo) :
  crate(cra), slot(slo), numraw(0), numchanhit(0), numHits(0),
  chanlist(0), chanindex(0), rawData(0), data(0), didini(false),
  maxc(0), maxd(0), allocd(0) {}

THaSlotData::~THaSlotData() {
  if( !didini ) return;
  delete [] numHits;
  delete [] chanlist;
  delete [] chanindex;
  delete [] rawData;
  delete [] data;
}

void THaSlotData::define(int cra, int slo, UShort_t nchan, UShort_t ndata ) {
  // Must call define once if you are really going to use this slot.
  // Otherwise its an empty slot which does not use much memory.
  crate = cra;
  slot = slo;
  didini = true;
  maxc = nchan;
  maxd = ndata;
  // Initial allocation of data arrays
  allocd = nchan;
  // Delete arrays if defined so we can call define() more than once!
  delete [] numHits; delete [] chanlist; delete [] chanindex;
  delete [] rawData; delete [] data;
  numHits   = new UChar_t[maxc];
  chanlist  = new UShort_t[maxc];
  chanindex = new UShort_t[maxc];
  rawData   = new int[allocd];
  data      = new int[allocd];
  numchanhit = numraw = 0;
  memset(numHits,0,maxc*sizeof(UChar_t));
}

int THaSlotData::loadData(const char* type, int chan, int dat, int raw) {
// loadData loads the data into storage arrays.
// Note, this algorithm relies on channels being read out
// in a sequential, not random, order.
  if( !didini ) return SD_ERR;
  if (chan < 0 || chan >= maxc) {
    if (VERBOSE) {
      cout << "THaSlotData: Warning in loadData: channel ";
      cout <<chan<<" out of bounds, ignored."<<endl;
    }
    return SD_ERR;
  }
  if( numraw >= maxd || numchanhit >= maxc) {
    if (VERBOSE) {
      cout << "THaSlotData: Warning in loadData: too many "
	   << ((numraw >= maxd ) ? "data words" : "channels")
	   << " for crate/slot = " 
	   << crate << " " << slot << endl;
    }
    return SD_ERR;
  }
  if( device.IsNull() ) device = type;
  if( numchanhit == 0 || chan != chanlist[numchanhit-1] ) {
    //New channel
    chanindex[chan] = numraw;
    chanlist[numchanhit++]=chan;
  }
  // Grow data arrays if really necessary (rare)
  if( numraw >= allocd ) {
    UShort_t old_allocd = allocd;
    allocd = TMath::Min(2*allocd,maxd);
    int* tmp = new int[allocd];
    memcpy(tmp,data,old_allocd*sizeof(int));
    delete [] data; data = tmp;
    tmp = new int[allocd];
    memcpy(tmp,rawData,old_allocd*sizeof(int));
    delete [] rawData; rawData = tmp;
  }
  rawData[numraw] = raw;
  data[numraw++]  = dat;
  if( numHits[chan] == (UChar_t)~0 ) {
    if( VERBOSE ) 
      cout << "THaSlotData: Warning in loadData: too many hits " 
	   << "for module " << device << " in crate/slot = " 
	   << dec << crate << " " << slot 
	   << " chan = " << chan << endl;
    return SD_ERR;
  }
  numHits[chan]++;
  return SD_OK;
}

void THaSlotData::print() const {
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
};

ClassImp(THaSlotData)
