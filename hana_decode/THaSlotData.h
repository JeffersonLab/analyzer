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


#include "Rtypes.h"
#include <iostream>
#include "TString.h"

const int SD_ERR = -1; 
const int SD_OK = 1;

class THaSlotData {

public:

       THaSlotData();
       THaSlotData(int crate, int slot);
       virtual ~THaSlotData();
       TString devType() const;             // "adc", "tdc", "scaler"
       int getNumRaw() const { return numraw; };  // Amount of raw CODA data
       int getRawData(int ihit) const;            // Returns raw data words
       int getRawData(int chan, int hit) const;
       int getNumHits(int chan) const;      // Num hits on a channel
       int getNumChan() const;              // Num unique channels hit
       int getNextChan(int index) const;    // List of unique channels hit
       int getData(int chan, int hit) const;  // Data (adc,tdc,scaler) on 1 chan
       void clearEvent();                   // clear event counters
       int loadData(const char* type, int chan, int dat, int raw);
       void define(int crate, int slot);    // Define crate, slot
       void print();

private:

       static const int VERBOSE = 1;  // 1= verbose warnings (recommended)
       static const int MAXCHAN = 500;  // No device has more channels.
       static const int MAXDATA = 1000; // No device contains more data.
       int crate;
       int slot;
       TString device;
       int numraw;           // Hit counters (numraw, numHits, numchanhit)
       int numchanhit;       // can be zero'd by clearEvent each event.
       int* numHits;         // numHits[channel]
       int* chanlist;        // chanlist[hit]
       int* chanindex;       // chanindex[channel] - pointer to 1st data
       int* rawData;         // rawData[hit] (all bits)
       int* data;            // data[hit] (only data bits)
       int didini,candel;
       int maxc,maxd,ncharc;

       ClassDef(THaSlotData,0)   //  Data in one slot of fastbus, vme, camac
};

//______________ inline functions _____________________________________________
inline int THaSlotData::getRawData(int hit) const {  
  // Returns the raw data (all bits)
  if (hit >= 0 && hit < maxd) return rawData[hit];
  return 0;
};

//_____________________________________________________________________________
// Data (words on 1 chan)
inline 
int THaSlotData::getRawData(int chan, int hit) const {  
  static int index;
  if (chan >= 0 && chan < maxc) index = chanindex[chan] + hit;
  if (index >= 0 && index < maxd) return rawData[index];
  return 0; 
};

//_____________________________________________________________________________
inline 
int THaSlotData::getNumHits(int chan) const {    
  // Num hits on a channel
  if (chan >= 0 && chan < maxc) return numHits[chan];
  return 0;
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
  if (index >= 0 && index < maxd) return chanlist[index];
  return 0;
};

//_____________________________________________________________________________
// Data (words on 1 chan)
inline 
int THaSlotData::getData(int chan, int hit) const {  
  static int index;
  if (chan >= 0 && chan < maxc) index = chanindex[chan] + hit;
  if (index >= 0 && index < maxd) return data[index];
  return 0; 
};

//_____________________________________________________________________________
// Device type (adc, tdc, scaler)
inline
TString THaSlotData::devType() const {  
  return device;
};

//_____________________________________________________________________________
inline
void THaSlotData::clearEvent() {
  // Only the minimum is cleared; e.g. data array is not cleared.
  numraw = 0;
  numchanhit = 0;
  memset(numHits,0,ncharc);
};
  
#endif
