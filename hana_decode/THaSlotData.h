#ifndef THaSlotData_
#define THaSlotData_

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

#include "TString.h"
#include "Decoder.h"
#include <cassert>
#include <fstream>

const int SD_WARN = -2;
const int SD_ERR = -1;
const int SD_OK = 1;

namespace Decoder {

class THaSlotData {

public:

       static const int DEFNCHAN;  // Default number of channels
       static const int DEFNDATA; // Default number of data words
       static const int DEFNHITCHAN; // Default number of hits per channel

       THaSlotData();
       THaSlotData(int crate, int slot);
       virtual ~THaSlotData();
       const char* devType() const;             // "adc", "tdc", "scaler"
       int loadModule(const THaCrateMap *map);
       int getNumRaw() const { return numraw; };  // Amount of raw CODA data
       int getRawData(int ihit) const;            // Returns raw data words
       int getRawData(int chan, int hit) const;
       int getNumHits(int chan) const;      // Num hits on a channel
       int getNumChan() const;              // Num unique channels hit
       int getNextChan(int index) const;    // List of unique channels hit
       int getData(int chan, int hit) const;  // Data (adc,tdc,scaler) on 1 chan
       int getCrate() const { return crate; }
       int getSlot()  const { return slot; }
       void clearEvent();                   // clear event counters
       int loadData(const char* type, int chan, int dat, int raw);
       int loadData(int chan, int dat, int raw);

       // new
       Int_t LoadIfSlot(const UInt_t* evbuffer, const UInt_t *pstop);
       void SetDebugFile(std::ofstream *file) { fDebugFile = file; };
       Module* GetModule() { return fModule; };

       void define(int crate, int slot, UShort_t nchan=DEFNCHAN,
		   UShort_t ndata=DEFNDATA, UShort_t nhitperchan=DEFNHITCHAN );// Define crate, slot
       void print() const;
       void print_to_file() const;
       int compressdataindex(int numidx);

private:


       int crate;
       int slot;
       TString device;
       Module *fModule;
       UShort_t numhitperchan; // expected number of hits per channel
       UShort_t numraw;      // Hit counters (numraw, numHits, numchanhit)
       UShort_t numchanhit;  // can be zero'd by clearEvent each event.
       UShort_t firstfreedataidx;     // pointer to first free space in dataindex array
       UShort_t numholesdataidx;
       UShort_t* numHits;     // numHits[channel]
       Int_t *xnumHits;       // same as numHits
       UShort_t* chanlist;   // chanlist[hitindex]
       UShort_t* idxlist;    // [channel] pointer to 1st entry in dataindex
       UShort_t* chanindex;  // [channel] gives hitindex
       UShort_t* dataindex;  // [idxlist] pointer to rawdata and  data
       UShort_t* numMaxHits;  // [channel] current maximum number of hits
       int* rawData;         // rawData[hit] (all bits)
       int* data;            // data[hit] (only data bits)
       std::ofstream *fDebugFile; // debug output to this file, if nonzero
       bool didini;          // true if object initialized via define()
       UShort_t maxc;        // Number of channels for this device
       UShort_t maxd;        // Max number of data words per event
       UShort_t allocd;      // Allocated size of data arrays
       UShort_t alloci;      // Allocated size of dataindex array

       ClassDef(THaSlotData,0)   //  Data in one slot of fastbus, vme, camac
};

//______________ inline functions _____________________________________________
inline int THaSlotData::getRawData(int hit) const {
  // Returns the raw data (all bits)
  assert( hit >= 0 && hit < numraw );
  if (hit >= 0 && hit < numraw) return rawData[hit];
  return 0;
};

//_____________________________________________________________________________
// Data (words on 1 chan)
inline
int THaSlotData::getRawData(int chan, int hit) const {
  assert( chan >= 0 && chan < maxc && hit >= 0 &&  hit < numHits[chan] );
  if (chan < 0 || chan >= maxc || numHits[chan]<=hit || hit<0 )
    return 0;
  int index = dataindex[idxlist[chan]+hit];
  assert(index >= 0 && index < numraw);
  if (index >= 0 && index < numraw) return rawData[index];
  return 0;
};

//_____________________________________________________________________________
inline
int THaSlotData::getNumHits(int chan) const {
  // Num hits on a channel
  assert( chan >= 0 && chan < maxc );
  return (chan >= 0 && chan < maxc) ? numHits[chan] : 0;
};

//_____________________________________________________________________________
inline
int THaSlotData::getNumChan() const {
  // Num unique channels # hit
  return numchanhit;
};

//_____________________________________________________________________________
inline
int THaSlotData::getNextChan(int index) const {
  // List of unique channels hit
  assert(index >= 0 && index < numchanhit);
  if (index >= 0 && index < numchanhit)
    return chanlist[index];
  return 0;
};

//_____________________________________________________________________________
// Data (words on 1 chan)
inline
int THaSlotData::getData(int chan, int hit) const {
  assert( chan >= 0 && chan < maxc && hit >= 0 &&  hit < numHits[chan] );
  if (chan < 0 || chan >= maxc || numHits[chan]<=hit || hit<0 )
    return 0;
  int index = dataindex[idxlist[chan]+hit];
  assert(index >= 0 && index < numraw);
  if (index >= 0 && index < numraw) return data[index];
  return 0;
};

//_____________________________________________________________________________
// Device type (adc, tdc, scaler)
inline
const char* THaSlotData::devType() const {
  return device.Data();
};

//_____________________________________________________________________________
inline
void THaSlotData::clearEvent() {
  // Only the minimum is cleared; e.g. data array is not cleared.
  // CAUTION: this code is critical for performance
  numraw = 0;
  firstfreedataidx=0;
  numholesdataidx=0;
  while( numchanhit>0 ) numHits[chanlist[--numchanhit]] = 0;
};

//_____________________________________________________________________________
inline
int THaSlotData::compressdataindex(int numidx) {

  // first check if it is more favourable to expand it, or to reshuffle
  if (firstfreedataidx+numidx>=alloci){
    if (((numholesdataidx/alloci)>0.5)&&(numholesdataidx>numidx)) {
      // reshuffle, lots of holes
      UShort_t* tmp = new UShort_t[alloci];
      firstfreedataidx=0;
      for (UShort_t i=0; i<numchanhit; i++) {
	UShort_t chan=chanlist[i];
	for (UShort_t j=0; j<numHits[chan]; j++) {
	  tmp[firstfreedataidx+j]=dataindex[idxlist[chan]+j];
	}
	firstfreedataidx=firstfreedataidx+numMaxHits[chan];
      }
      delete [] dataindex; dataindex=tmp;
    } else {
      UShort_t old_alloci = alloci;
      alloci *= 2;
      // FIX ME one should check that it doesnt grow too much
      UShort_t* tmp = new UShort_t[alloci];
      memcpy(tmp,dataindex,old_alloci*sizeof(UShort_t));
      delete [] dataindex; dataindex = tmp;
    }
  }
  return 0;
};

}

#endif
