//--------------------------------------------------------
//  tscalroc11_main.C
//
//  Test of the ROC10/11 scalers which are read in the datastream
//  typically every 100 events.  As of Aug 2002 this is a new 
//  readout.  (Actually there was also a similar ROC11 since 
//  Aug 2001, but the data format has changed due to G0 mode.)
//  The focus is on the ring buffer data. 
//  R. Michaels, Oct 2002
//--------------------------------------------------------

// The ROC to find scalers (10/11 are R/L).
#define MYROC 11

// To printout (1) or not (0)
#define PRINTOUT 0

#define NBCM 6
#define MAXRING 20

#include <iostream>
#include "THaCodaFile.h"
#include "THaEvData.h"
#include "THaCodaDecoder.h"
#ifndef __CINT__
#include "TROOT.h"
#include "TFile.h"
#include "TH1.h"
#include "TH2.h"
#include "TProfile.h"
#include "TNtuple.h"
#include "TRandom.h"
#endif

using namespace std;

int loadHelicity();
int ranBit(unsigned int& seed);   
unsigned int getSeed();

// Global variables  --------------------------------------

#define NBIT 24
int hbits[NBIT];         // The NBIT shift register

int present_reading;     // present reading of helicity
int predicted_reading;   // prediction of present reading (should = present_reading)
int present_helicity;    // present helicity (using prediction)

#define NDELAY 2         // number of quartets between
                         // present reading and present helicity 
                         // (i.e. the delay in reporting the helicity)

int recovery_flag = 0;   // flag to determine if we need to recover
                         // from an error (1) or not (0)
unsigned int iseed;            // value of iseed for present_helicity
unsigned int iseed_earlier;    // iseed for predicted_reading


int main(int argc, char* argv[]) {
   int helicity, qrt, gate, timestamp;
   int len, data, status, nscaler, header;
   int numread, badread, i, found, index;
   int ring_clock, ring_qrt, ring_helicity;
   int ring_trig, ring_bcm, ring_l1a, ring_v2fh; 
   int sum_clock, sum_trig, sum_bcm, sum_l1a;
   int inquad, nrread, q1_helicity;
   int ring_data[MAXRING], rloc;
   if (argc < 2) {
      cout << "You made a mistake... bye bye !\n" << endl;
      cout << "Usage:   'tscalroc" << MYROC << " filename'" << endl;
      cout << "  where 'filename' is the CODA file."<<endl;
      exit(0);
   }
// Setup 
// Pedestals.  Left, Right Arms.  u1,u3,u10,d1,d3,d10
   Float_t bcmpedL[NBCM] = { 188.2, 146.2, 271.6, 37.8, 94.2, 260.2 };
   Float_t bcmpedR[NBCM] = { 53.0, 41.8, 104.1, 0., 101.6, 254.6 };
   Float_t bcmped[NBCM];
   if (MYROC == 11) {
     cout << "Using Left Arm BCM pedestals"<<endl;
     for (i = 0; i < NBCM; i++) {
       bcmped[i] = bcmpedL[i];
     }
   } else {
     cout << "Using Right Arm BCM pedestals"<<endl;
     for (i = 0; i < NBCM; i++) {
       bcmped[i] = bcmpedR[i];
     }
   }
// Initialize root and output.  
   TROOT scalana("scalroot","Hall A scaler analysis");
   TFile hfile("scaler.root","RECREATE","Scaler data in Hall A");
// Define the ntuple here
//                   0    1   2   3   4    5   6
   char rawrates[]="evt:clk:trig:bcm:l1a:mhel:phel:";
// Asymmetries delayed various amounts, using RING BUFFER
//                    7    8    9    10   11  12   13   14   15   16
   char asydelay[]="a3d1:a3d2:a3d3:a3d4:a3d5:a3d6:a3d7:a3d8:a3d9:a3d10";
   int nlen = strlen(rawrates) + strlen(asydelay);
   char *string_ntup = new char[nlen+1];
   strcpy(string_ntup,rawrates);
   strcat(string_ntup,asydelay);
   TNtuple *ntup = new TNtuple("ascal","Scaler Rates",string_ntup);
   Float_t farray_ntup[17];  // Dimension = size of string_ntup (i.e. nlen+1)
   THaCodaFile *coda = new THaCodaFile(TString(argv[1]));
   THaEvData *evdata = new THaCodaDecoder();
   inquad = 0;
   q1_helicity = 0;
   rloc = 0;
   status = 0;
   sum_clock = 0;  sum_trig = 0;  sum_bcm = 0;  sum_l1a = 0;
   nrread = 0;
   memset(ring_data, 0, MAXRING*sizeof(int));
   while (status == 0) {
     status = coda->codaRead();
     if (status != 0) break;
     evdata->LoadEvent(coda->getEvBuffer());
     len = evdata->GetRocLength(MYROC);
     if (len <= 4) continue;
     data = evdata->GetRawData(MYROC,3);   
     helicity = (data & 0x10) >> 4;
     qrt = (data & 0x20) >> 5;
     gate = (data & 0x40) >> 6;
     timestamp = evdata->GetRawData(MYROC,4);
     found = 0;
     index = 5;
     while ( !found ) {
       data = evdata->GetRawData(MYROC,index++);
       if ( (data & 0xffff0000) == 0xfb0b0000) found = 1;
       if (index >= len) break;
     }
     if (!found) break;
     nscaler = data & 0x7;
     if (PRINTOUT) {
       cout << hex << "helicity " << helicity << "  qrt " << qrt;
       cout << " gate " << gate << "   time stamp " << timestamp << endl;
       cout << "nscaler in this event  " << nscaler << endl;
     }
     if (nscaler <= 0) continue;

     if (nscaler > 2) nscaler = 2;  // shouldn't be necessary
// 32 channels of scaler data for two helicities.
     if (PRINTOUT) cout << "Synch event ----> " << endl;
     for (int ihel = 0; ihel < nscaler; ihel++) { 
       header = evdata->GetRawData(MYROC,index++);
       if (PRINTOUT) {
         cout << "Scaler for helicity = " << dec << ihel;
         cout << "  unique header = " << hex << header << endl;
       }
       for (int ichan = 0; ichan < 32; ichan++) {
	   data = evdata->GetRawData(MYROC,index++);
           if (PRINTOUT) {       
              cout << "channel # " << dec << ichan+1;
              cout << "  (hex) data = " << hex << data << endl;
	   }
       }
     }         
     numread = evdata->GetRawData(MYROC,index++);
     badread = evdata->GetRawData(MYROC,index++);
     if (PRINTOUT) cout << "FIFO num of last good read " << dec << numread << endl;
     if (badread != 0) {
       cout << "DISASTER: There are bad readings " << endl;
       cout << "FIFO num of last bad read " << endl;
     }
// Subset of scaler channels stored in a 30 Hz ring buffer.
     int nring = 0;
     while (index < len && nring == 0) {
       header = evdata->GetRawData(MYROC,index++);
       if ((header & 0xffff0000) == 0xfb1b0000) {
           nring = header & 0x3ff;
       }
     }
     if (PRINTOUT) cout << "Num in ring buffer = " << dec << nring << endl;
// The following assumes three are 6 pieces of data per 'iring'
// This was true after Jan 10, 2003.
     for (int iring = 0; iring < nring; iring++) {
       ring_clock = evdata->GetRawData(MYROC,index++);
       data = evdata->GetRawData(MYROC,index++);
       ring_qrt = (data & 0x10) >> 4;
       ring_helicity = (data & 0x1);
       present_reading = ring_helicity;
       if (ring_qrt) {
  	  inquad = 1;
          if (loadHelicity()) {
             if (present_reading != predicted_reading) {
        	cout << "DISASTER:  The helicity is wrong !!"<<endl;
                recovery_flag = 1;  // ask for recovery
             }
             q1_helicity = present_helicity;
	  }
       } else {
         inquad++;
       }
       ring_trig = evdata->GetRawData(MYROC,index++);
       ring_bcm  = evdata->GetRawData(MYROC,index++);
       ring_l1a  = evdata->GetRawData(MYROC,index++);
       ring_v2fh = evdata->GetRawData(MYROC,index++);
       sum_clock = sum_clock + ring_clock;
       sum_trig  = sum_trig + ring_trig; 
       sum_bcm   = sum_bcm + ring_bcm; 
       sum_l1a   = sum_l1a + ring_l1a;
       ring_data[rloc%MAXRING] = ring_bcm;
       if (inquad == 1 && nrread++ > 12) {
  	  farray_ntup[0] = (float)nrread;
          farray_ntup[1] = ring_clock;
          farray_ntup[2] = ring_trig;
          farray_ntup[3] = ring_bcm;
	  farray_ntup[4] = ring_l1a;
	  farray_ntup[5] = (float)present_reading;
	  farray_ntup[6] = (float)present_helicity;
          for (int ishift = 3; ishift <= 12; ishift++) {
	    Float_t pdat = ring_data[(rloc-ishift)%MAXRING];
            Float_t mdat = ring_data[(rloc-ishift+1)%MAXRING];
            Float_t sum  = pdat + mdat;
            Float_t asy  = 99999;
            if (sum != 0) {
	      if (present_reading == 1) {
   	        asy = (pdat - mdat)/sum;
	      } else {
	        asy = (mdat - pdat)/sum;
	      }
	    }
            farray_ntup[4+ishift] = asy;
	  }
          ntup->Fill(farray_ntup);
       }
       rloc++;
       if (PRINTOUT) {
          cout << "buff [" << dec << iring << "] ";
          cout << "  clock " << ring_clock << "  qrt " << ring_qrt;
          cout << "  helicity " << ring_helicity;
          cout << "  trigger " << ring_trig << "  bcm " << ring_bcm;
          cout << "  L1a "<<ring_l1a<<endl;
          cout << "  ring_v2fh (helicity in v2f) "<<ring_v2fh<<endl;
          cout << "  inquad "<<inquad<<endl;
          cout << "  sums:  "<<endl;
          cout << "  clock "<<sum_clock<<"   trig "<<sum_trig<<endl;
          cout << "  bcm  "<<sum_bcm<<"   l1a "<<sum_l1a<<endl;
       }
     }
   }
   cout << "All done"<<endl;
   hfile.Write();
   hfile.Close();
   return 0;
}

// *************************************************************
// loadHelicity()
// Loads the helicity to determine the seed.
// After loading (nb==NBIT), predicted helicities are available.
// *************************************************************

int loadHelicity() {
  int i;
  static int nb;
  if (recovery_flag) nb = 0;
  recovery_flag = 0;
  if (nb < NBIT) {
      hbits[nb] = present_reading;
      nb++;
      return 0;
  } else if (nb == NBIT) {   // Have finished loading
      iseed_earlier = getSeed();
      for (i = 0; i < NBIT+1; i++) 
          predicted_reading = ranBit(iseed_earlier);
      iseed = iseed_earlier;
      for (i = 0; i < NDELAY; i++)
          present_helicity = ranBit(iseed);
      nb++;
      return 1;
  } else {      
      predicted_reading = ranBit(iseed_earlier);
      present_helicity = ranBit(iseed);
      if (PRINTOUT) {
        cout << "helicities  "<<predicted_reading<<"  "<<
              present_reading<<"  "<<present_helicity<<endl;
      }
      return 1;
  }

}


// *************************************************************
// This is the random bit generator according to the G0
// algorithm described in "G0 Helicity Digital Controls" by 
// E. Stangland, R. Flood, H. Dong, July 2002.
// Argument:
//        ranseed = seed value for random number. 
//                  This value gets modified.
// Return value:
//        helicity (0 or 1)
// *************************************************************

int ranBit(unsigned int& ranseed) {

  static int IB1 = 1;           // Bit 1
  static int IB3 = 4;           // Bit 3
  static int IB4 = 8;           // Bit 4
  static int IB24 = 8388608;    // Bit 24 
  static int MASK = IB1+IB3+IB4+IB24;

  if(ranseed & IB24) {    
      ranseed = ((ranseed^MASK)<<1) | IB1;
      return 1;
  } else  { 
      ranseed <<= 1;
      return 0;
  }

};



// *************************************************************
// getSeed
// Obtain the seed value from a collection of NBIT bits.
// This code is the inverse of ranBit.
// Input:
//       int hbits[NBIT]  -- global array of bits of shift register
// Return:
//       seed value
// *************************************************************


unsigned int getSeed() {
  int seedbits[NBIT];
  unsigned int ranseed = 0;
  if (NBIT != 24) {
    cout << "ERROR: NBIT is not 24.  This is unexpected."<<endl;
    cout << "Code failure..."<<endl;  // admittedly awkward, but
    return 0;                         // you probably won't care.
  }
  for (int i = 0; i < 20; i++) seedbits[23-i] = hbits[i];
  seedbits[3] = hbits[20]^seedbits[23];
  seedbits[2] = hbits[21]^seedbits[22]^seedbits[23];
  seedbits[1] = hbits[22]^seedbits[21]^seedbits[22];
  seedbits[0] = hbits[23]^seedbits[20]^seedbits[21]^seedbits[23];
  for (int i=NBIT-1; i >= 0; i--) ranseed = ranseed<<1|(seedbits[i]&1);
  ranseed = ranseed&0xFFFFFF;
  return ranseed;
}









