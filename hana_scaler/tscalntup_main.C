//--------------------------------------------------------
//  tscalntup_main.C
//
//  Read a CODA file and fill an ntuple with scaler data.
//  This version uses the "old" event type 140 data.
// 
//  R. Michaels, May 2001 
//--------------------------------------------------------

#define BCM_CUT1  1000   // cut on BCM (x1 gain) to require beam on.
#define BCM_CUT3  3000   // cut on BCM (x3 gain) to require beam on.
#define BCM_CUT10 7000   // cut on BCM (x10 gain) to require beam on.
#define NBCM 6         // number of BCM signals 
#define PRINTSUM 1
#define OUTFILENAME "asytst.dat"

#include <iostream>
#include <string>
#include "THaScaler.h"
#include "THaCodaFile.h"

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

int main(int argc, char* argv[]) {

   int printout = 0;   // To printout lots of stuff (1) or not (0)
   int trig;
   Float_t sum,asy;

   if (argc < 3) {
     cout << "Usage:  " << argv[0]<<" file  bank" << endl;
     cout << "where file = CODA file to analyze " << endl;
     cout << "and bank = 'Right', 'Left'  (spectrometer)" << endl;
     return 1;
   }
   TString filename = argv[1];
   string bank = argv[2];   
   if (bank != "Left" && bank != "Right") {
       cout << "2nd argument should be Left or Right but was "<<argv[2]<<endl;
       cout << "I will assume Left"<<endl;
       bank = "Left";
    }
    cout << "Bank = "<<bank<<endl;

   THaScaler *scaler;
   scaler = new THaScaler(bank.c_str());

// Init must be done once.  If you leave out the date, it assumes "now".
   if (scaler->Init("1-10-2004") == -1) {  
      cout << "Error initializing scaler " << endl;
      return 1;
   }

// Initialize root and output
   TROOT scalana("scalroot","Hall A scaler analysis");
   TFile hfile("scaler.root","RECREATE","Scaler data in Hall A");

// Define the ntuple here
//                   0   1  2  3   4   5  6  7   8  9 10 11 12  13   14   15  16  17  18  19  20  21  22  23  24  25 
   char rawrates[]="time:u1:u3:u10:d1:d3:d10:t1:t2:t3:t4:t5:clk:tacc:s11:s12:s13:s14:s15:s16:s21:s22:s23:s24:s25:s26:";
//                      26  27  28  29  30  31   32  33  34  35  36  37 
   char asymmetries[]="au1:au3:au10:ad1:ad3:ad10:at1:at2:at3:at4:at5:aclk";
   int nlen = strlen(rawrates) + strlen(asymmetries);
   char *string_ntup = new char[nlen+1];
   strcpy(string_ntup,rawrates);
   strcat(string_ntup,asymmetries);
   TNtuple *ntup = new TNtuple("ascal","Scaler Rates",string_ntup);

   Float_t farray_ntup[39];       // Note, dimension is same as size of string_ntup (i.e. nlen+1)

// Statistics kept on x1,3,10 upstream and downstream BCMs
   Double_t bcmsum[NBCM],bcmsq[NBCM],xcnt[NBCM];
   for (int ibcm = 0; ibcm < NBCM; ibcm++) {
       bcmsum[ibcm] = 0;
       bcmsq[ibcm] = 0;
       xcnt[ibcm] = 0;
   } 

// Pedestals.  Left, Right Arms.  u1,u3,u10,d1,d3,d10
   Float_t bcmpedL[NBCM] = { 52.5, 44.1, 110.7, 0., 94.2, 227. };
   Float_t bcmpedR[NBCM] = { 53.0, 41.8, 104.1, 0., 101.6, 254.6 };
   Float_t bcmped[NBCM];

   int i;
   if (bank == "Left") {
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

   int status = 1;
   int iev = 0;
   
   while (status) {
     status = scaler->LoadDataCodaFile(filename);   // load data for 'filename'
     if (!status) goto quit;
     Double_t time = scaler->GetPulser("clock")/1024;
     if ( iev > 0 && (iev < 10 || ((iev % 10) == 0)) ) cout << "iev = " << iev << endl;

// Optional printout
     if (printout == 1) {
       cout << "\n\n--------------  SCALER EVENT DATA -----------------------\n"<<endl;
       scaler->Print();     // raw diagnostic printout
       cout << "Time    " << time << "\n Counts :   " << endl;
       cout << "Bcm u1   hel+  0  hel- " << scaler->GetBcm(1,"bcm_u1") << "   " << scaler->GetBcm("bcm_u1") << "   " << scaler->GetBcm(-1,"bcm_u1") << endl;
       cout << "Bcm u3   hel+  0  hel- " << scaler->GetBcm(1,"bcm_u3") << "   " << scaler->GetBcm("bcm_u3") << "   " << scaler->GetBcm(-1,"bcm_u3") << endl;
       cout << "Bcm u10   hel+  0  hel- " << scaler->GetBcm(1,"bcm_u10") << "   " << scaler->GetBcm("bcm_u10") << "   " << scaler->GetBcm(-1,"bcm_u10") << endl;
       cout << "Bcm d1   hel+  0  hel- " << scaler->GetBcm(1,"bcm_d1") << "   " << scaler->GetBcm("bcm_d1") << "   " << scaler->GetBcm(-1,"bcm_d1") << endl;
       cout << "Bcm d3   hel+  0  hel- " << scaler->GetBcm(1,"bcm_d3") << "   " << scaler->GetBcm("bcm_d3") << "   " << scaler->GetBcm(-1,"bcm_d3") << endl;
       cout << "Bcm d10   hel+  0  hel- " << scaler->GetBcm(1,"bcm_d10") << "   " << scaler->GetBcm("bcm_d10") << "   " << scaler->GetBcm(-1,"bcm_d10") << endl;
       cout << "Clock   hel+  0  hel- " << scaler->GetBcm(1,"clock") << "   " << scaler->GetBcm("clock") << "   " << scaler->GetBcm(-1,"clock") << endl;
       for (trig = 1; trig <= 5; trig++ ) {
          cout << "Trigger " << trig << "   hel+  0  hel- " << scaler->GetTrig(1,trig) << "   " << scaler->GetTrig(trig) << "   " << scaler->GetTrig(-1,trig) << endl;
       }
       cout << "Rates (Hz) = " << endl;
       cout << "Bcm u1   hel+  0  hel- " << scaler->GetBcmRate(1,"bcm_u1") << "   " << scaler->GetBcmRate("bcm_u1") << "   " << scaler->GetBcmRate(-1,"bcm_u1") << endl;
       cout << "Bcm u3   hel+  0  hel- " << scaler->GetBcmRate(1,"bcm_u3") << "   " << scaler->GetBcmRate("bcm_u3") << "   " << scaler->GetBcmRate(-1,"bcm_u3") << endl;
       cout << "Bcm u10   hel+  0  hel- " << scaler->GetBcmRate(1,"bcm_u10") << "   " << scaler->GetBcmRate("bcm_u10") << "   " << scaler->GetBcmRate(-1,"bcm_u10") << endl;
       cout << "Bcm d1   hel+  0  hel- " << scaler->GetBcmRate(1,"bcm_d1") << "   " << scaler->GetBcmRate("bcm_d1") << "   " << scaler->GetBcmRate(-1,"bcm_d1") << endl;
       cout << "Bcm d3   hel+  0  hel- " << scaler->GetBcmRate(1,"bcm_d3") << "   " << scaler->GetBcmRate("bcm_d3") << "   " << scaler->GetBcmRate(-1,"bcm_d3") << endl;
       cout << "Bcm d10   hel+  0  hel- " << scaler->GetBcmRate(1,"bcm_d10") << "   " << scaler->GetBcmRate("bcm_d10") << "   " << scaler->GetBcmRate(-1,"bcm_d10") << endl;
       cout << "Clock   hel+  0  hel- " << scaler->GetBcmRate(1,"clock") << "   " << scaler->GetBcmRate("clock") << "   " << scaler->GetBcmRate(-1,"clock") << endl;
       for (trig = 1; trig <= 5; trig++ ) {
          cout << "Trigger " << trig << "   hel+  0  hel- " << scaler->GetTrigRate(1,trig) << "   " << scaler->GetTrigRate(trig) << "   " << scaler->GetTrigRate(-1,trig) << endl;
       }
     }  // <--  if(printout)

// Fill the ntuple here

       farray_ntup[0] = time;
       farray_ntup[1] = scaler->GetBcmRate("bcm_u1");
       farray_ntup[2] = scaler->GetBcmRate("bcm_u3");
       farray_ntup[3] = scaler->GetBcmRate("bcm_u10");
       farray_ntup[4] = scaler->GetBcmRate("bcm_d1");
       farray_ntup[5] = scaler->GetBcmRate("bcm_d3");
       farray_ntup[6] = scaler->GetBcmRate("bcm_d10");
       for (trig = 1; trig <= 5; trig++) farray_ntup[6+trig] = scaler->GetTrigRate(trig);
       farray_ntup[12] = scaler->GetPulserRate("clock");
       farray_ntup[13] = scaler->GetNormRate(0,"TS-accept");
       for (int pad = 0; pad < 6; pad++) {
	 farray_ntup[14+pad] = scaler->GetScalerRate("s1",pad);
       }
       for (int pad = 0; pad < 6; pad++) {
	 farray_ntup[20+pad] = scaler->GetScalerRate("s2",pad);
       }
       string bcms[] = {"bcm_u1", "bcm_u3", "bcm_u10", "bcm_d1", "bcm_d3", "bcm_d10"};
       for (int ibcm = 0; ibcm < 6; ibcm++ ) {
         sum = scaler->GetBcmRate(1,bcms[ibcm].c_str()) - bcmped[ibcm]
                 + scaler->GetBcmRate(-1,bcms[ibcm].c_str()) - bcmped[ibcm];
         asy = -999;
         if (sum != 0) {
	   asy = (scaler->GetBcmRate(1,bcms[ibcm].c_str()) - scaler->GetBcmRate(-1,bcms[ibcm].c_str())) / sum;
         } 
         farray_ntup[26+ibcm] = asy;
         Float_t bcm_cut = 0;
         if (ibcm == 0 || ibcm == 3) bcm_cut = BCM_CUT1;
         if (ibcm == 1 || ibcm == 4) bcm_cut = BCM_CUT3;
         if (ibcm == 2 || ibcm == 5) bcm_cut = BCM_CUT10;
         if (scaler->GetBcmRate(bcms[ibcm].c_str()) > bcm_cut && 
             asy > -0.9 && asy < 0.9) {
                 bcmsum[ibcm] = bcmsum[ibcm] + asy;
                 bcmsq[ibcm] = bcmsq[ibcm] + asy*asy;
                 xcnt[ibcm] = xcnt[ibcm] + 1.;
	 }
       }
       for (trig = 1; trig <= 5; trig++) {
         asy = -999;
         if (scaler->GetBcmRate(1,"bcm_u3") > BCM_CUT3 && scaler->GetBcmRate(-1,"bcm_u3") > BCM_CUT3) {    
           sum = scaler->GetTrigRate(1,trig)/scaler->GetBcmRate(1,"bcm_u3") 
              +  scaler->GetTrigRate(-1,trig)/scaler->GetBcmRate(-1,"bcm_u3");
           if (sum != 0) {
             asy = (scaler->GetTrigRate(1,trig)/scaler->GetBcmRate(1,"bcm_u3")
                 -  scaler->GetTrigRate(-1,trig)/scaler->GetBcmRate(-1,"bcm_u3")) / sum;
	   }
	 }
         farray_ntup[31+trig] = asy;
       }
       sum = scaler->GetPulser(1,"clock") + scaler->GetPulser(-1,"clock");
       asy = -999;
       if (sum != 0) {
	 asy = (scaler->GetPulser(1,"clock") - scaler->GetPulser(-1,"clock")) / sum;
       }
       farray_ntup[37] = asy;

       ntup->Fill(farray_ntup);


   }  // <--- loop over scaler events in CODA file


quit:
   hfile.Write();
   hfile.Close();

   scaler->PrintSummary();

#ifdef PRINTSUM
// Use all the BCM numbers to average.  They are not independent.
// Take error from bcm_u3.
   Double_t asyavg,asyerr,avg,diff,xsum,xtot;
   asyavg = 0;  xsum = 0; xtot = 0;   
   for (int ibcm = 0; ibcm < NBCM; ibcm++) {
     if (xcnt[ibcm] > 0 && ibcm != 0 && ibcm != 3) {  // only x3 and x10
       asyavg = asyavg + bcmsum[ibcm]/xcnt[ibcm];
       xsum = xsum + xcnt[ibcm];
       xtot = xtot + 1;
       if (printout == 2) cout << "bcm " << ibcm << "  cnt = " 
          << xcnt[ibcm] << "  " << xsum << "   A = "
          << 1000000*bcmsum[ibcm]/xcnt[ibcm] << endl; 
     }
   }
   if (xtot > 0) asyavg = asyavg / xtot;
   asyerr = 1000000;
   if (xcnt[1] > 0) {  // index 1 --> bcm_u3
     avg = bcmsum[1]/xcnt[1];
     diff = bcmsq[1]/xcnt[1] - avg*avg;
     if (diff < 0) diff = -1*diff;
     asyerr = sqrt(diff)/sqrt(xcnt[1]);
   }
   if (xtot > 0 && asyerr < 1000 && xsum > 200) {
     ofstream outfile(OUTFILENAME);
     int asyi = (int) (1000000*asyavg);  
     int erri = (int) (1000000*asyerr); 
     if (printout == 2) cout << "(exclu x1)  Asy " << asyi << " +/- " 
          << erri << "  N bcms " << xtot << endl;
     outfile << asyi << "   " << erri << endl;  
   }
#endif

   return 0;
}







