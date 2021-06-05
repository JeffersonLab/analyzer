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

#include "Decoder.h"
#include "Module.h"
#include "CustomAlloc.h"
#include <cassert>
#include <fstream>
#include <string>
#include <vector>
#include <memory>

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
       const char* devType() const;                  // "adc", "tdc", "scaler"
       Int_t  loadModule( const THaCrateMap* map );
       UInt_t getNumRaw() const { return numraw; };  // Amount of raw CODA data
       UInt_t getRawData(UInt_t ihit) const;         // Returns raw data words
       UInt_t getRawData(UInt_t chan, UInt_t hit) const;
       UInt_t getNumHits(UInt_t chan) const;         // Num hits on a channel
       UInt_t getNumChan() const;                    // Num unique channels hit
       UInt_t getNextChan(UInt_t index) const;       // List of unique channels hit
       UInt_t getData(UInt_t chan, UInt_t hit) const;// Data (adc,tdc,scaler) on 1 chan
       UInt_t getCrate() const { return crate; }
       UInt_t getSlot()  const { return slot; }
       UInt_t getNchan() const { return fNchan; }
       void   clearEvent();                          // clear event counters
       Int_t  loadData( const char* type, UInt_t chan, UInt_t dat, UInt_t raw );
       Int_t  loadData( UInt_t chan, UInt_t dat, UInt_t raw );

       // new
       UInt_t LoadIfSlot( const UInt_t* evbuffer, const UInt_t* pstop );
       UInt_t LoadBank( const UInt_t* p, UInt_t pos, UInt_t len );
       UInt_t LoadNextEvBuffer();
       Bool_t IsMultiBlockMode() { if (fModule) return fModule->IsMultiBlockMode(); return false; };
       Bool_t BlockIsDone() { if (fModule) return fModule->BlockIsDone(); return false; };

       void SetDebugFile(std::ofstream *file) { fDebugFile = file; };
       Module* GetModule() { return fModule.get(); };

       // Define crate, slot
       void define( UInt_t crate, UInt_t slot, UInt_t nchan = DEFNCHAN,
                    UInt_t ndata = DEFNDATA, UInt_t nhitperchan = DEFNHITCHAN );
       void print() const;
       void print_to_file() const;
       void compressdataindex(UInt_t numidx);

private:

       UInt_t crate;
       UInt_t slot;
       std::string device;
       std::unique_ptr<Module> fModule;
       UInt_t numhitperchan; // expected number of hits per channel
       UInt_t numraw;        // Hit counters (numraw, numHits, numchanhit)
       UInt_t numchanhit;    // can be zero'd by clearEvent each event.
       UInt_t firstfreedataidx;  // pointer to first free space in dataindex array
       UInt_t numholesdataidx;
       VectorUInt   numHits;     // numHits[channel]
       VectorUIntNI chanlist;    // chanlist[hitindex]
       VectorUIntNI idxlist;     // [channel] pointer to 1st entry in dataindex
       VectorUIntNI chanindex;   // [channel] gives hitindex
       VectorUIntNI dataindex;   // [idxlist] pointer to rawdata and data
       VectorUIntNI numMaxHits;  // [channel] current maximum number of hits
       VectorUIntNI rawData;     // rawData[hit] (all bits)
       VectorUIntNI data;        // data[hit] (only data bits)
       std::ofstream *fDebugFile; // debug output to this file, if nonzero
       bool didini;         // true if object initialized via define()
       UInt_t fNchan;       // Number of channels for this device

       void compressdataindexImpl(UInt_t numidx);

       ClassDef(THaSlotData,0)   //  Data in one slot of fastbus, vme, camac
};

//______________ inline functions _____________________________________________
inline UInt_t THaSlotData::getRawData(UInt_t hit) const {
  // Returns the raw data (all bits)
  assert( hit < numraw );
  if (hit < numraw) return rawData[hit];
  return 0;
}

//_____________________________________________________________________________
// Data (words on 1 chan)
inline
UInt_t THaSlotData::getRawData(UInt_t chan, UInt_t hit) const {
  assert(chan < fNchan && hit < numHits[chan] );
  if ( chan >= fNchan || numHits[chan] <= hit)
    return 0;
  UInt_t index = dataindex[idxlist[chan]+hit];
  assert(index < numraw);
  if (index < numraw) return rawData[index];
  return 0;
}

//_____________________________________________________________________________
inline
UInt_t THaSlotData::getNumHits(UInt_t chan) const {
  // Num hits on a channel
  assert(chan < fNchan );
  return (chan < fNchan) ? numHits[chan] : 0;
}

//_____________________________________________________________________________
inline
UInt_t THaSlotData::getNumChan() const {
  // Num unique channels # hit
  return numchanhit;
}

//_____________________________________________________________________________
inline
UInt_t THaSlotData::getNextChan(UInt_t index) const {
  // List of unique channels hit
  assert(index < numchanhit);
  if (index < numchanhit)
    return chanlist[index];
  return 0;
}

//_____________________________________________________________________________
// Data (words on 1 chan)
inline
UInt_t THaSlotData::getData(UInt_t chan, UInt_t hit) const {
  assert(chan < fNchan && hit < numHits[chan] );
  if ( chan >= fNchan || numHits[chan] <= hit)
    return 0;
  UInt_t index = dataindex[idxlist[chan]+hit];
  assert(index < numraw);
  if (index < numraw) return data[index];
  return 0;
}

//_____________________________________________________________________________
// Device type (adc, tdc, scaler)
inline
const char* THaSlotData::devType() const {
  return device.c_str();
}

//_____________________________________________________________________________
inline
void THaSlotData::clearEvent() {
  // Only the minimum is cleared; e.g. data array is not cleared.
  // CAUTION: this code is critical for performance
  numraw = 0;
  firstfreedataidx=0;
  numholesdataidx=0;
  while( numchanhit>0 ) numHits[chanlist[--numchanhit]] = 0;
}

//_____________________________________________________________________________
inline
void THaSlotData::compressdataindex(UInt_t numidx) {

  if( firstfreedataidx+numidx >= dataindex.size() )
    compressdataindexImpl(numidx);
}

}

#endif
