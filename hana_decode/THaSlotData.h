#ifndef Podd_THaSlotData_h_
#define Podd_THaSlotData_h_

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
#include "Module.h"
#include <cassert>
#include <iostream>
#include <fstream>

const int SD_WARN = -2;
const int SD_ERR = -1;
const int SD_OK = 1;

namespace Decoder {

class THaSlotData {

public:

       static const UInt_t DEFNCHAN;  // Default number of channels
       static const UInt_t DEFNDATA; // Default number of data words
       static const UInt_t DEFNHITCHAN; // Default number of hits per channel

       THaSlotData();
       THaSlotData(UInt_t crate, UInt_t slot);
       virtual ~THaSlotData();
       const char* devType() const;             // "adc", "tdc", "scaler"
       int loadModule(const THaCrateMap *map);
       UInt_t getNumRaw() const { return numraw; };  // Amount of raw CODA data
       UInt_t getRawData(UInt_t ihit) const;         // Returns raw data words
       UInt_t getRawData(UInt_t chan, UInt_t hit) const;
       UInt_t getNumHits(UInt_t chan) const;      // Num hits on a channel
       UInt_t getNumChan() const;              // Num unique channels hit
       UInt_t getNextChan(UInt_t index) const;    // List of unique channels hit
       UInt_t getData(UInt_t chan, UInt_t hit) const;  // Data (adc,tdc,scaler) on 1 chan
       UInt_t getCrate() const { return crate; }
       UInt_t getSlot()  const { return slot; }
       void clearEvent();                   // clear event counters
       int loadData(const char* type, UInt_t chan, UInt_t dat, UInt_t raw);
       int loadData(UInt_t chan, UInt_t dat, UInt_t raw);

       // new
       Int_t LoadIfSlot(const UInt_t* evbuffer, const UInt_t *pstop);
       Int_t LoadBank(const UInt_t* p, Int_t pos, Int_t len); 
       Int_t LoadNextEvBuffer();
       Bool_t IsMultiBlockMode() { if (fModule) return fModule->IsMultiBlockMode(); return kFALSE; };
       Bool_t BlockIsDone() { if (fModule) return fModule->BlockIsDone(); return kFALSE; };

       void SetDebugFile(std::ofstream *file) { fDebugFile = file; };
       Module* GetModule() { return fModule; };

       void define(UInt_t crate, UInt_t slot, UInt_t nchan=DEFNCHAN,
		   UInt_t ndata=DEFNDATA, UInt_t nhitperchan=DEFNHITCHAN );// Define crate, slot
       void print() const;
       void print_to_file() const;
       int compressdataindex(UInt_t numidx);

private:


       UInt_t crate;
       UInt_t slot;
       TString device;
       Module *fModule;
       UInt_t numhitperchan; // expected number of hits per channel
       UInt_t numraw;        // Hit counters (numraw, numHits, numchanhit)
       UInt_t numchanhit;    // can be zero'd by clearEvent each event.
       UInt_t firstfreedataidx;  // pointer to first free space in dataindex array
       UInt_t numholesdataidx;
       UInt_t* numHits;     // numHits[channel]
       UInt_t *xnumHits;    // same as numHits
       UInt_t* chanlist;    // chanlist[hitindex]
       UInt_t* idxlist;     // [channel] pointer to 1st entry in dataindex
       UInt_t* chanindex;   // [channel] gives hitindex
       UInt_t* dataindex;   // [idxlist] pointer to rawdata and  data
       UInt_t* numMaxHits;  // [channel] current maximum number of hits
       UInt_t* rawData;     // rawData[hit] (all bits)
       UInt_t* data;        // data[hit] (only data bits)
       std::ofstream *fDebugFile; // debug output to this file, if nonzero
       bool didini;         // true if object initialized via define()
       UInt_t maxc;         // Number of channels for this device
       UInt_t maxd;         // Max number of data words per event
       UInt_t allocd;       // Allocated size of data arrays
       UInt_t alloci;       // Allocated size of dataindex array

       int compressdataindexImpl(UInt_t numidx);

       ClassDef(THaSlotData,0)   //  Data in one slot of fastbus, vme, camac
};

//______________ inline functions _____________________________________________
inline UInt_t THaSlotData::getRawData(UInt_t hit) const {
  // Returns the raw data (all bits)
  assert( hit < numraw );
  if (hit < numraw) return rawData[hit];
  return 0;
};

//_____________________________________________________________________________
// Data (words on 1 chan)
inline
UInt_t THaSlotData::getRawData(UInt_t chan, UInt_t hit) const {
  assert( chan < maxc && hit < numHits[chan] );
  if (chan >= maxc || numHits[chan]<=hit)
    return 0;
  UInt_t index = dataindex[idxlist[chan]+hit];
  assert(index < numraw);
  if (index < numraw) return rawData[index];
  return 0;
};

//_____________________________________________________________________________
inline
UInt_t THaSlotData::getNumHits(UInt_t chan) const {
  // Num hits on a channel
  assert( chan < maxc );
  return (chan < maxc) ? numHits[chan] : 0;
};

//_____________________________________________________________________________
inline
UInt_t THaSlotData::getNumChan() const {
  // Num unique channels # hit
  return numchanhit;
};

//_____________________________________________________________________________
inline
UInt_t THaSlotData::getNextChan(UInt_t index) const {
  // List of unique channels hit
  assert(index < numchanhit);
  if (index < numchanhit)
    return chanlist[index];
  return 0;
};

//_____________________________________________________________________________
// Data (words on 1 chan)
inline
UInt_t THaSlotData::getData(UInt_t chan, UInt_t hit) const {
  assert( chan < maxc && hit < numHits[chan] );
  if (chan >= maxc || numHits[chan]<=hit)
    return 0;
  UInt_t index = dataindex[idxlist[chan]+hit];
  assert(index < numraw);
  if (index < numraw) return data[index];
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
int THaSlotData::compressdataindex(UInt_t numidx) {

  if( firstfreedataidx+numidx >= alloci )
    return compressdataindexImpl(numidx);

  return 0;
}

}

#endif
