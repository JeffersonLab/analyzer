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

ClassImp(THaSlotData)

  THaSlotData::THaSlotData() {  
      didini = 0;
      candel = 1;
      maxc = 0;
      maxd = 0;
      ncharc = 0;
      crate = -1;
      slot = -1; 
  };
  THaSlotData::THaSlotData(int cra, int slo) {
      didini = 0;
      candel = 1;
      maxc = 0;
      maxd = 0;
      ncharc = 0;
      crate = cra;
      slot = slo;
  };
  THaSlotData::~THaSlotData() {
    if( didini && candel ) {
       delete [] numHits;
       delete [] chanlist;
       delete [] chanindex;
       delete [] rawData;
       delete [] data;
       candel = 0;
    }
  };

 void THaSlotData::define(int cra, int slo) {  
// Must call define once if you are really going to use this slot.
// Otherwise its an empty slot which does not use much memory.
      crate = cra;
      slot = slo;
      didini = 1;
      maxc = MAXCHAN;
      maxd = MAXDATA;
      ncharc = maxc*sizeof(int)/sizeof(char);
      numHits = new int[MAXCHAN];
      chanlist = new int[MAXDATA];
      chanindex = new int[MAXCHAN];
      rawData = new int[MAXDATA];
      data = new int[MAXDATA];
  };

  int THaSlotData::loadData(const char* type, int chan, int dat, int raw) {
// loadData loads the data into storage arrays.
// Note, this algorithm relies on channels being read out
// in a sequential, not random, order.
      static int index;
      if ((chan < 0) || (chan > MAXCHAN)) {
	if (VERBOSE) {
           cout << "THaSlotData: Warning in loadData: channel ";
           cout <<dec<<chan<<" out of bounds, ignored."<<endl;
	}
        return SD_ERR;
      }
      device = type;
      int unique = 1;
      if (numchanhit > 0) {
         index = numchanhit-1;
         if (index < maxd) {
           if (chan == chanlist[index]) unique = 0;
         }
      }
      if(unique) {
         if (chan >= 0 && chan < maxc) chanindex[chan] = numraw;
         if (numchanhit >= 0 && numchanhit < maxd) chanlist[numchanhit++]=chan;
      }
      if (numraw >= 0 && numraw < maxd) {
         rawData[numraw] = raw;
         data[numraw] = dat;
         numraw++;
      }
      if (chan >= 0 && chan < maxc) {
         numHits[chan] = numHits[chan] + 1;
      }         
      return SD_OK;
  };

  void THaSlotData::print() {
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
     for (chan=0; chan<MAXCHAN; chan++) {
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































