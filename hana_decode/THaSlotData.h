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

const int SD_ERR = -1; 
const int SD_OK = 1;

class THaSlotData {

public:

       static const int DEFNCHAN;  // Default number of channels
       static const int DEFNDATA; // Default number of data words

       THaSlotData();
       THaSlotData(int crate, int slot);
       virtual ~THaSlotData();
       const char* devType() const;             // "adc", "tdc", "scaler"
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
       void define(int crate, int slot, UShort_t nchan=DEFNCHAN, 
		   UShort_t ndata=DEFNDATA );// Define crate, slot
       void print() const;

private:

       int crate;
       int slot;
       TString device;
       UShort_t numraw;      // Hit counters (numraw, numHits, numchanhit)
       UShort_t numchanhit;  // can be zero'd by clearEvent each event.
       UChar_t* numHits;     // numHits[channel]
       UShort_t* chanlist;   // chanlist[hit]
       UShort_t* chanindex;  // chanindex[channel] - pointer to 1st data
       int* rawData;         // rawData[hit] (all bits)
       int* data;            // data[hit] (only data bits)
       bool didini;          // true if object initialized via define()
       UShort_t maxc;        // Number of channels for this device
       UShort_t maxd;        // Max number of data words per event
       UShort_t allocd;      // Allocated size of data arrays

       ClassDef(THaSlotData,0)   //  Data in one slot of fastbus, vme, camac
};

//______________ inline functions _____________________________________________
inline int THaSlotData::getRawData(int hit) const {  
  // Returns the raw data (all bits)
  if (hit >= 0 && hit < numraw) return rawData[hit];
  return 0;
};

//_____________________________________________________________________________
// Data (words on 1 chan)
inline 
int THaSlotData::getRawData(int chan, int hit) const {  
  if (chan < 0 || chan >= maxc || numHits[chan] == 0)
    return 0;
  int index = chanindex[chan] + hit;
  if (index >= 0 && index < numraw) return rawData[index];
  return 0; 
};

//_____________________________________________________________________________
inline 
int THaSlotData::getNumHits(int chan) const {    
  // Num hits on a channel
  return (chan >= 0 && chan < maxc ) ?
    numHits[chan] : 0;
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
  if (index >= 0 && index < numchanhit) return chanlist[index];
  return 0;
};

//_____________________________________________________________________________
// Data (words on 1 chan)
inline 
int THaSlotData::getData(int chan, int hit) const {  
  if (chan < 0 || chan >= maxc || numHits[chan] == 0) 
    return 0;
  int index = chanindex[chan] + hit;
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
  while( numchanhit>0 ) numHits[chanlist[--numchanhit]] = 0;
};
  
#endif
