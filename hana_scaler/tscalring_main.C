//--------------------------------------------------------
//  tscalring_main.C
//
//  Analysis of data from ROC10 and ROC11.
//  The ROC to find scalers are : 10/11 = R/L.
//  R. Michaels, Jan 2003
//  See also  http://www.jlab.org/~rom/scaler_roc10.html
//--------------------------------------------------------

// To printout (1) or not (0).  This is for debug.
#define PRINTOUT 0

// To get 6 data from Ring or 5.  If defined, we read 6, otherwise
// comment this out and we'll read 5 (for data prior to Jan 9, '03,
// see also halog 91208).  BTW, if this is set wrong you'll see messages
// about "DISASTER:  The helicity is wrong !!" but they are fake.
#define READ6 

#define NBCM 6
#define MAXRING 20
#define BCM_CUT1  1000   // cut on BCM (x1 gain) to require beam on.
#define BCM_CUT3  3000   // cut on BCM (x3 gain) to require beam on.
#define BCM_CUT10 7000   // cut on BCM (x10 gain) to require beam on.

#include <iostream>
#include <string>
#include "THaCodaFile.h"
#include "THaEvData.h"
#include "THaCodaDecoder.h"
#include "THaScaler.h"
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
void resetSums();
int incrementSums(int helicity_now, double prev_clock, 
       double prev_trig, double prev_bcm, double prev_l1a);

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

// Ring buffer sums, avg over ~2 sec, sorted by 2 helicities (index)

double rsum_clk[2],rsum_bcm[2],rsum_trig[2],rsum_l1a[2];
double rsum_num[2];
double corr_bcm0, corr_bcm1, ringped[2];

int main(int argc, char* argv[]) {
   int myroc, iev, nevents;
   int trig, clkp, clkm, lastclkp, lastclkm;
   Float_t time, sum, asy; 
   int helicity, qrt, gate, timestamp;
   int len, data, status, nscaler, header;
   int numread, badread, i, jstat;
   int ring_clock, ring_qrt, ring_helicity;
   int ring_trig, ring_bcm, ring_l1a, ring_hel2; 
   int prev_clock, prev_trig, prev_bcm, prev_l1a, prev_hel2; 
   int sum_clock, sum_trig, sum_bcm, sum_l1a;
   int latest_clock, latest_trig, latest_bcm, latest_l1a;
   int inquad, nrread, q1_helicity, helicity_now;
   int ring_data[MAXRING], rloc;
   string filename, bank, mybank;

   if (argc < 3) {
     cout << "Usage:  " << argv[0]<<" file  bank  [nevents]" << endl;
     cout << "where file = CODA file to analyze " << endl;
     cout << "and bank = 'Right', 'Left'  (spectrometer)" << endl;
     cout << "and nevents is the number of events you want (optional)"<<endl;
     cout << "(default nevents = all of them)"<<endl;
     return 1;
   }
   filename = argv[1];
   bank = argv[2];   
   if (bank != "Left" && bank != "Right") {
       cout << "2nd argument should be Left or Right but was "<<bank<<endl;
       cout << "I will assume Left"<<endl;
       bank = "Left";
   }
   mybank = "EvLeft";
   myroc = 11;
   if (bank == "Right") {
      mybank = "EvRight";
      myroc = 10;
   }
   nevents = 0;
   if (argc >= 4) nevents = atoi(argv[3]);
   cout << "bank "<<bank<<"  num events "<<nevents<<endl;

// Pedestals.  Left, Right Arms.  u1,u3,u10,d1,d3,d10
   Float_t bcmpedL[NBCM] = { 188.2, 146.2, 271.6, 37.8, 94.2, 260.2 };
   Float_t bcmpedR[NBCM] = { 53.0, 41.8, 104.1, 0., 101.6, 254.6 };
   Float_t bcmped[NBCM];

// Pedestals for Ring buffer analysis
// L-arm tuned from run e01012_20288, R-arm from e01012_1288
   Float_t ringpedL[2] = { 112.2, 112.2 };
   Float_t ringpedR[2] = { 112.2, 112.2 };
   Float_t ringped[2];

   if (myroc == 11) {
     cout << "Using Left Arm BCM pedestals"<<endl;
     for (i = 0; i < NBCM; i++) {
       bcmped[i] = bcmpedL[i];
     }
     for (i = 0; i < 2; i++) ringped[i] = ringpedL[i];
   } else {
     cout << "Using Right Arm BCM pedestals"<<endl;
     for (i = 0; i < NBCM; i++) {
       bcmped[i] = bcmpedR[i];
     }
     for (i = 0; i < 2; i++) ringped[i] = ringpedR[i];
   }
// Scaler object to extract data using scaler.map.
// WARNING:   bank = "Left" goes with event type 140 data,
// while "EvLeft" goes with ROC11 data.  Similarly "Right", "EvRight".
   THaScaler *scaler = new THaScaler(mybank.c_str());

   if (scaler->Init("1-1-2003") == -1) {  
      cout << "Error initializing scaler object. "<<endl;
      return 1;
   }
// Initialize root and output.  
   TROOT scalana("scalroot","Hall A scaler analysis");
   TFile hfile("scaler.root","RECREATE","Scaler data in Hall A");
// Define the ntuple here.  Part of the ntuple is filled from scaler
// object (scal_obj) and part is from event data (evdata).
// If you add a variable, I suggest you keep track of the indices the same way.
//                            0   1   2  3   4  5   6  7  8  9 10 11  12  13   14
   char scal_obj_rawrates[]="time:u1:u3:u10:d1:d3:d10:t1:t2:t3:t4:t5:clkp:clkm:tacc:";
//                              15  16   17  18  19   20  21   22  23  24  25  26
   char scal_obj_asymmetries[]="au1:au3:au10:ad1:ad3:ad10:at1:at2:at3:at4:at5:aclk:";
//                          27  28    29     30     31    32    33
   char evdata_rawrates[]="evt:evclk:evtrig:evbcm:evl1a:evmhel:evphel:";
// Asymmetries delayed various amounts, using RING BUFFER.  The correct
// one is presumably the one delayed by NDELAY quads, but we need to check.
//                   34   35   36   37   38   39   40   41   42   43
   char asydelay[]="a3d1:a3d2:a3d3:a3d4:a3d5:a3d6:a3d7:a3d8:a3d9:a3d10";
   int nlen = strlen(scal_obj_rawrates) + strlen(scal_obj_asymmetries) + strlen(evdata_rawrates) + strlen(asydelay);
   char *string_ntup = new char[nlen+1];
   strcpy(string_ntup, scal_obj_rawrates);
   strcat(string_ntup, scal_obj_asymmetries);
   strcat(string_ntup, evdata_rawrates);
   strcat(string_ntup,asydelay);
   TNtuple *ntup = new TNtuple("ascal","Scaler Rates",string_ntup);
   Float_t* farray_ntup = new Float_t[nlen+1];  
// 2nd ntuple for 1-sec averaged ring buffer
//                     0    1    2   3    4    5   6    7   8    9   10   11
   char ring_rates[]="evt:clkp:clkm:clk:bcmp:bcmm:bcm:trig:l1a:nump:numm:num:aclk:abcm:atrig:al1a";
//12  13   14  15
   TNtuple *rnt = new TNtuple("aring","Ring values", ring_rates);
   Float_t *farray2 = new Float_t[16];
   THaCodaFile *coda = new THaCodaFile(TString(filename.c_str()));
   THaEvData *evdata = new THaCodaDecoder();
   inquad = 0;
   q1_helicity = 0;
   rloc = 0;
   status = 0;
   sum_clock = 0;  sum_trig = 0;  sum_bcm = 0;  sum_l1a = 0;
   prev_clock = 0; prev_trig = 0; prev_bcm = 0; prev_l1a = 0; prev_hel2 = 0;
   lastclkp = 0;  lastclkm = 0;
   nrread = 0;
   iev = 0;
   resetSums();

   while (status == 0) {
     status = coda->codaRead();
     if (status != 0) break;
     evdata->LoadEvent(coda->getEvBuffer());
     len = evdata->GetRocLength(myroc);
     if (nevents > 0 && iev++ > nevents) goto finish;
     if (len <= 4) continue;
     if (evdata->GetEvType() == 140) continue;
     scaler->LoadData(*evdata);  
     if ( !scaler->IsRenewed() ) continue;
     memset(farray_ntup, 0, (nlen+1)*sizeof(Float_t));
// Having loaded the scaler object, we pull out what we can from it.
// Note, we must average the two helicities to get non-helicity rates because ROC10/11
// data only have helicity scalers.  (In contrast, event type 140 has all scalers.)
     time = (scaler->GetPulser(1,"clock") + scaler->GetPulser(-1,"clock"))/1024;
     latest_clock = (scaler->GetPulser(1,"clock") + scaler->GetPulser(-1,"clock"));
     if (myroc == 11) {
       latest_trig = (scaler->GetTrig(1,3) + scaler->GetTrig(-1,3));
     } else {   
       latest_trig = (scaler->GetTrig(1,1) + scaler->GetTrig(-1,1));
     }
     latest_bcm = (scaler->GetBcm(1,"bcm_u3") + scaler->GetBcm(-1,"bcm_u3"));
     latest_l1a = (scaler->GetNormData(1,"TS-accept") + scaler->GetNormData(-1,"TS-accept"));
     if (PRINTOUT) cout << dec << "latest data "<<latest_clock<<"  "<<latest_trig<<"  "<<latest_bcm<<"  "<<latest_l1a<<endl;
     farray_ntup[0] = time;
     farray_ntup[1] = 0.5*(scaler->GetBcmRate(1,"bcm_u1") + scaler->GetBcmRate(-1,"bcm_u1"));
     farray_ntup[2] = 0.5*(scaler->GetBcmRate(1,"bcm_u3") + scaler->GetBcmRate(-1,"bcm_u3"));
     farray_ntup[3] = 0.5*(scaler->GetBcmRate(1,"bcm_u10") + scaler->GetBcmRate(-1,"bcm_u10"));
     farray_ntup[4] = 0.5*(scaler->GetBcmRate(1,"bcm_d1") + scaler->GetBcmRate(-1,"bcm_d1"));
     farray_ntup[5] = 0.5*(scaler->GetBcmRate(1,"bcm_d3") + scaler->GetBcmRate(-1,"bcm_d3"));
     farray_ntup[6] = 0.5*(scaler->GetBcmRate(1,"bcm_d10") + scaler->GetBcmRate(-1,"bcm_d10"));
     for (trig = 1; trig <= 5; trig++) {
       farray_ntup[6+trig] = 0.5*(scaler->GetTrigRate(1,trig) + scaler->GetTrigRate(-1,trig));
     }
     if (PRINTOUT == 1 && len >= 16) {
       scaler->Print();
       for (i = 0; i < 6; i++) cout << "  bcm -> "<<farray_ntup[i];
       cout << endl << endl;
       for (i = 0; i < 5; i++) cout << "  trig -> "<<farray_ntup[i+7];
       cout << endl << endl;
     }
     clkp = scaler->GetNormData(1,"clock");
     clkm = scaler->GetNormData(-1,"clock");
     farray_ntup[12] = clkp - lastclkp;
     farray_ntup[13] = clkm - lastclkm;
     lastclkp = clkp;
     lastclkm = clkm;
     farray_ntup[14] = 0.5*(scaler->GetNormRate(1,"TS-accept") + 
                            scaler->GetNormRate(-1,"TS-accept"));
     string bcms[] = {"bcm_u1", "bcm_u3", "bcm_u10", "bcm_d1", "bcm_d3", "bcm_d10"};
//
// Next we construct the helicity correlated asymmetries.
//
     for (int ibcm = 0; ibcm < 6; ibcm++ ) {
       sum = scaler->GetBcmRate(1,bcms[ibcm].c_str()) - bcmped[ibcm]
                  + scaler->GetBcmRate(-1,bcms[ibcm].c_str()) - bcmped[ibcm];
       asy = -999;
       if (sum != 0) {
	  asy = (scaler->GetBcmRate(1,bcms[ibcm].c_str()) - 
                 scaler->GetBcmRate(-1,bcms[ibcm].c_str())) / sum;
       } 
       farray_ntup[15+ibcm] = asy;
     }
     for (trig = 1; trig <= 5; trig++) {
       asy = -999;
       if (scaler->GetBcmRate(1,"bcm_u3") > BCM_CUT3 && 
           scaler->GetBcmRate(-1,"bcm_u3") > BCM_CUT3) {    
           sum = scaler->GetTrigRate(1,trig)/scaler->GetBcmRate(1,"bcm_u3") 
              +  scaler->GetTrigRate(-1,trig)/scaler->GetBcmRate(-1,"bcm_u3");
           if (sum != 0) {
             asy = (scaler->GetTrigRate(1,trig)/scaler->GetBcmRate(1,"bcm_u3")
                 -  scaler->GetTrigRate(-1,trig)/scaler->GetBcmRate(-1,"bcm_u3")) / sum;
	   }
       }
       farray_ntup[20+trig] = asy;
     }
     sum = scaler->GetPulser(1,"clock") + scaler->GetPulser(-1,"clock");
     asy = -999;
     if (sum != 0) {
	asy = (scaler->GetPulser(1,"clock") - 
               scaler->GetPulser(-1,"clock")) / sum;
     }
     farray_ntup[26] = asy;
//  
//  Next we ignore the scaler object and obtain data directly from the event.
//  
     data = evdata->GetRawData(myroc,3);   
     helicity = (data & 0x10) >> 4;
     qrt = (data & 0x20) >> 5;
     gate = (data & 0x40) >> 6;
     timestamp = evdata->GetRawData(myroc,4);
     data = evdata->GetRawData(myroc,5);
     nscaler = data & 0x7;
//     if (evdata->GetEvType() == 9) cout << "Ev  9   qrt "<<qrt<<"  helicity "<<helicity<<"  gate "<<gate<<"  timestamp "<<timestamp<<endl;
     if (PRINTOUT) {
       cout << hex << "helicity " << helicity << "  qrt " << qrt;
       cout << " gate " << gate << "   time stamp " << timestamp << endl;
       cout << "nscaler in this event  " << nscaler << endl;
     }
     if (nscaler <= 0) continue;
     int index = 6;
     if (nscaler > 2) nscaler = 2;  // shouldn't be necessary
// 32 channels of scaler data for two helicities.
     if (PRINTOUT) cout << "Synch event ----> " << endl;
     for (int ihel = 0; ihel < nscaler; ihel++) { 
       header = evdata->GetRawData(myroc,index++);
       if (PRINTOUT) {
         cout << "Scaler for helicity = " << dec << ihel;
         cout << "  unique header = " << hex << header << endl;
       }
       for (int ichan = 0; ichan < 32; ichan++) {
	   data = evdata->GetRawData(myroc,index++);
           if (PRINTOUT) {       
              cout << "channel # " << dec << ichan+1;
              cout << "  (hex) data = " << hex << data << endl;
	   }
       }
     }         
     numread = evdata->GetRawData(myroc,index++);
     badread = evdata->GetRawData(myroc,index++);
     if (PRINTOUT) cout << "FIFO num of last good read " << dec << numread << endl;
     if (badread != 0) {
       cout << "DISASTER: There are bad readings " << endl;
       cout << "FIFO num of last bad read " << endl;
     }
// Ring buffer analysis: analyze subset of scaler channels in 30 Hz ring buffer.
     int nring = 0;
     while (index < len && nring == 0) {
       header = evdata->GetRawData(myroc,index++);
       if ((header & 0xffff0000) == 0xfb1b0000) {
           nring = header & 0x3ff;
       }
     }
     if (PRINTOUT) cout << "Num in ring buffer = " << dec << nring << endl;
// The following assumes three are (now 6) pieces of data per 'iring'
     for (int iring = 0; iring < nring; iring++) {
       ring_clock = evdata->GetRawData(myroc,index++);
       data = evdata->GetRawData(myroc,index++);
       ring_qrt = (data & 0x10) >> 4;
       ring_helicity = (data & 0x1);
       present_reading = ring_helicity;
       if (ring_qrt) {
  	  inquad = 1;
          if (loadHelicity()) {
	    //	     cout << "CHECK HEL "<<present_reading<<"  "
	    //         << predicted_reading << endl;
             if (present_reading != predicted_reading) {
        	cout << "DISASTER:  The helicity is wrong !!"<<endl;
                recovery_flag = 1;  // ask for recovery
             }
             q1_helicity = present_helicity;
	  }
       } else {
         inquad++;
       }
       if (inquad == 1 || inquad == 4) {
         helicity_now = q1_helicity;
       } else {
         helicity_now = 1 - q1_helicity;
       }
       ring_trig = evdata->GetRawData(myroc,index++);
       ring_bcm  = evdata->GetRawData(myroc,index++);
       ring_l1a  = evdata->GetRawData(myroc,index++);
#ifdef READ6
       ring_hel2  = evdata->GetRawData(myroc,index++);
#else
       ring_hel2  = -99;
#endif
       sum_clock = sum_clock + ring_clock;
       sum_trig  = sum_trig + ring_trig; 
       sum_bcm   = sum_bcm + ring_bcm; 
       sum_l1a   = sum_l1a + ring_l1a;
// We know the helicity bits come one window later, so we adjust
// for that here.
       jstat = incrementSums(helicity_now, prev_clock, prev_trig, 
                             prev_bcm, prev_l1a);
       if (jstat == 1) {
         farray2[0] = (Float_t) evdata->GetEvNum();
         farray2[1] = rsum_clk[0];
         farray2[2] = rsum_clk[1];
         farray2[3] = rsum_clk[0] + rsum_clk[1];
         corr_bcm0  = rsum_bcm[0] - ringped[0];  // the 2 peds had better
         corr_bcm1  = rsum_bcm[1] - ringped[1];  // be the same (but we check)
         farray2[4] = rsum_bcm[0];
         farray2[5] = rsum_bcm[1];
         farray2[6] = corr_bcm0 + corr_bcm1;
         farray2[7] = rsum_trig[0] + rsum_trig[1];
         farray2[8] = rsum_l1a[0] + rsum_l1a[1];
         farray2[9] = rsum_num[0];
         farray2[10] = rsum_num[1];
         farray2[11] = rsum_num[0] + rsum_num[1];
         sum = rsum_clk[0] + rsum_clk[1];
         asy = -999;
         if (sum != 0) asy = (rsum_clk[0] - rsum_clk[1])/sum;
         farray2[12] = asy;
// Charge asymmetry.  Don't forget to normalize to clock.
         asy = -999;
         sum = corr_bcm0 + corr_bcm1;
         if (sum != 0) asy = (corr_bcm0 - corr_bcm1)/sum;
         farray2[13] = asy;
         asy = -999;
         if (corr_bcm0 != 0 && corr_bcm1 != 0) {
	   Float_t trig0 = rsum_trig[0] / corr_bcm0;
	   Float_t trig1 = rsum_trig[1] / corr_bcm1;
           sum = trig0 + trig1;
           if (sum != 0) {
	     asy = (trig0 - trig1) / sum;
           }
         }
         farray2[14] = asy;         
         asy = -999;
         if (corr_bcm0 != 0 && corr_bcm1 != 0) {
	   Float_t l1a0 = rsum_l1a[0] / corr_bcm0;
	   Float_t l1a1 = rsum_l1a[1] / corr_bcm1;
           sum = l1a0 + l1a1;
           if (sum != 0) {
	     asy = (l1a0 - l1a1) / sum;
           }
         }
         farray2[15] = asy;
         rnt->Fill(farray2);
         resetSums();
       }
       prev_clock = ring_clock;
       prev_trig = ring_trig;
       prev_bcm = ring_bcm;
       prev_l1a = ring_l1a;
// Empirical check of delayed helicity scheme.
       ring_data[rloc%MAXRING] = ring_bcm;
       if (inquad == 1 && nrread++ > 12) {
  	  farray_ntup[27] = (float)nrread; // need to cut that this is >0 in analysis
          farray_ntup[28] = ring_clock;
          farray_ntup[29] = ring_trig;
          farray_ntup[30] = ring_bcm;
	  farray_ntup[31] = ring_l1a;
	  farray_ntup[32] = (float)present_reading;
	  farray_ntup[33] = (float)present_helicity;
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
// ishift = 9 is the correct one.  (should be 8, but the helicity
// bits are one cycle out of phase w.r.t. data)
            farray_ntup[31+ishift] = asy;
	    if (PRINTOUT) cout << "shift "<<ishift<<"  "<<asy<<endl;
	  }
       }
       ntup->Fill(farray_ntup);
       rloc++;
       if (PRINTOUT) {
          cout << "buff [" << dec << iring << "] ";
          cout << "  clock " << ring_clock << "  qrt " << ring_qrt;
          cout << "  helicity " << ring_helicity<<"  2nd hel "<<ring_hel2<<endl;
          cout << "  trigger " << ring_trig << "  bcm " << ring_bcm;
          cout << "  L1a "<<ring_l1a<<endl;
          cout << "  inquad "<<inquad<<endl;
          cout << "  sums:  "<<endl;
          cout << "  clock "<<sum_clock<<"   trig "<<sum_trig<<endl;
          cout << "  bcm  "<<sum_bcm<<"   l1a "<<sum_l1a<<endl;
       }
     }
   }
finish:
   cout << "All done"<<endl<<flush;
   cout << "Sums from ring buffer: "<<dec<<endl;
   cout << "  Clock "<<sum_clock<<"   trigger "<<sum_trig;
   cout << "  BCM  "<<sum_bcm<<"   L1a "<<sum_l1a<<endl;
   cout << "Latest data from online sums:  "<<endl;
   cout << "  Clock "<<latest_clock<<"   trigger "<<latest_trig;
   cout << "  BCM  "<<latest_bcm<<"   L1a "<<latest_l1a<<endl;

   hfile.Write();
   hfile.Close();

   exit(0);
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


// *************************************************************
//
//  Increment the sums from the ring buffer, sorted by helicity
//  returns:
//         0 = continue
//         1 = time to fill ntuple and zero the counters.
//
// *************************************************************

int incrementSums(int helicity_now, double clock, 
       double trig, double bcm, double l1a) {

  if (helicity_now < 0 || helicity_now > 1) {
     cout << "ERROR: illegal helicity value in incrementSums()"<<endl;
     return 0;
  }
  rsum_clk[helicity_now]  += clock;
  rsum_trig[helicity_now] += trig;
  rsum_bcm[helicity_now]  += bcm;
  rsum_l1a[helicity_now]  += l1a;
  rsum_num[helicity_now]  += 1;

  if (rsum_num[0] >= 30 && rsum_num[1] >= 30) return 1;
  return 0;

}

void resetSums() {
   memset(rsum_clk, 0, 2*sizeof(double));
   memset(rsum_bcm, 0, 2*sizeof(double));
   memset(rsum_trig, 0, 2*sizeof(double));
   memset(rsum_l1a, 0, 2*sizeof(double));
   memset(rsum_num, 0, 2*sizeof(double));
   return;
}



