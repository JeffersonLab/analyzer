//--------------------------------------------------------
//  tscalroc11_main.C
//
//  Test of the ROC10/11 scalers which are read in the datastream
//  typically every 100 events.  As of Aug 2002 this is a new 
//  readout.  (Actually there was also a similar ROC11 since 
//  Aug 2001, but the data format has changed due to G0 mode.)
// 
//  R. Michaels, Aug 2002
//--------------------------------------------------------

#define MYROC 11
#include <iostream>
#include "THaCodaFile.h"
#include "THaEvData.h"

int main(int argc, char* argv[]) {
   int helicity, qrt, gate, timestamp;
   int len, data, status, nscaler, header;
   int numread, badread;
   int ring_clock, ring_qrt, ring_helicity;
   int ring_trig, ring_bcm, ring_l1a; 
   if (argc < 2) {
      cout << "You made a mistake... bye bye !\n" << endl;
      cout << "Usage:   'tscalroc" << MYROC << " filename'" << endl;
      cout << "  where 'filename' is the CODA file."<<endl;
      exit(0);
   }
   THaCodaFile *coda = new THaCodaFile(TString(argv[1]));
   THaEvData evdata;
   status = 0;
   while (status == 0) {
     status = coda->codaRead();
     if (status != 0) break;
     evdata.LoadEvent(coda->getEvBuffer());
     len = evdata.GetRocLength(MYROC);
     if (len <= 4) continue;
     data = evdata.GetRawData(MYROC,3);   
     helicity = (data & 0x10) >> 4;
     qrt = (data & 0x20) >> 5;
     gate = (data & 0x40) >> 6;
     timestamp = evdata.GetRawData(MYROC,4);
     data = evdata.GetRawData(MYROC,5);
     nscaler = data & 0x7;
     cout << hex << "helicity " << helicity << "  qrt " << qrt;
     cout << " gate " << gate << "   time stamp " << timestamp << endl;
     cout << "nscaler in this event  " << nscaler << endl;
     if (nscaler <= 0) continue;
     int index = 6;
     if (nscaler > 2) nscaler = 2;  // shouldn't be necessary
// 32 channels of scaler data for two helicities.
     cout << "Synch event ----> " << endl;
     for (int ihel = 0; ihel < nscaler; ihel++) { 
       header = evdata.GetRawData(MYROC,index++);
       cout << "Scaler for helicity = " << dec << ihel;
       cout << "  unique header = " << hex << header << endl;
       for (int ichan = 0; ichan < 32; ichan++) {
	 data = evdata.GetRawData(MYROC,index++);
         cout << "channel # " << dec << ichan+1;
         cout << "  (hex) data = " << hex << data << endl;
       }
     }
     numread = evdata.GetRawData(MYROC,index++);
     badread = evdata.GetRawData(MYROC,index++);
     cout << "FIFO num of last good read " << dec << numread << endl;
     if (badread != 0) {
       cout << "DISASTER: There are bad readings " << endl;
       cout << "FIFO num of last bad read " << endl;
     }
// Subset of scaler channels stored in a 30 Hz ring buffer.
     int nring = 0;
     while (index < len && nring == 0) {
       header = evdata.GetRawData(MYROC,index++);
       if ((header & 0xffff0000) == 0xfb1b0000) {
           nring = header & 0x3ff;
       }
     }
     cout << "Num in ring buffer = " << dec << nring << endl;
     for (int iring = 0; iring < nring; iring++) {
       ring_clock = evdata.GetRawData(MYROC,index++);
       data = evdata.GetRawData(MYROC,index++);
       ring_qrt = (data & 0x10) >> 4;
       ring_helicity = (data & 0x1);
       ring_trig = evdata.GetRawData(MYROC,index++);
       ring_bcm = evdata.GetRawData(MYROC,index++);
       ring_l1a = evdata.GetRawData(MYROC,index++);
       cout << "buff [" << dec << iring << "] ";
       cout << "  clock " << ring_clock << "  qrt " << ring_qrt;
       cout << "  helicity " << ring_helicity;
       cout << "  trigger " << ring_trig << "  bcm " << ring_bcm;
       cout << "  L1a "<<ring_l1a<<endl;
     }
   }
   return 0;
}







