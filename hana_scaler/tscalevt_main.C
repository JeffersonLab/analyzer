//--------------------------------------------------------
//  tscalevt_main.C
//
//  Test of the ROC10/11 scalers which are read in the datastream
//  every synch event (typ. every 100 events).  
// 
//  R. Michaels, Jan 2002
//--------------------------------------------------------

#include <iostream>
#include <string>

#include "THaScaler.h"
#include "THaCodaFile.h"
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

#define DEBUG 1

#define SKIPEVENT 0     // How many scaler events to skip
                         // (This reduces effect of clock granularity)
#define MYROC     11     // Needed for the SKIPEVENT trick
#define BCM_CUT1  3000   // cut on BCM (x1 gain) to require beam on.
#define BCM_CUT3  10000  // cut on BCM (x3 gain) to require beam on.
#define BCM_CUT10 30000  // cut on BCM (x10 gain) to require beam on.
#define NBCM 6           // number of BCM signals 

using namespace std;

int main(int argc, char* argv[]) {

   int i,iev,evnum,trig,iskip,status;
   Int_t clkp, clkm, lastclkp, lastclkm;
   Float_t sum,asy;
   TString filename = "run.dat";
   
   char bank[100] = "EvLeft";  // Event stream, Left HRS
   cout << "Analyzing CODA file  "<<filename<<endl;   
   cout << "Bank = "<<bank<<endl<<flush;

   evnum = 10000000;
   if (argc > 1) evnum = atoi(argv[1]);

   THaScaler *scaler = new THaScaler(bank);

   if (scaler->Init("1-10-2004") == -1) {  
      cout << "Error initializing scaler "<<endl;
      return 1;
   }

   THaCodaData *coda = new THaCodaFile(filename);
   THaEvData *evdata = new THaCodaDecoder();

// Pedestals.  Left, Right Arms.  u1,u3,u10,d1,d3,d10
   Float_t bcmpedL[NBCM] = { 188.2, 146.2, 271.6, 37.8, 94.2, 260.2 };
   Float_t bcmpedR[NBCM] = { 53.0, 41.8, 104.1, 0., 101.6, 254.6 };
   Float_t bcmped[NBCM];

   if (strcmp(bank,"EvLeft") == 0) {
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

// Initialize root and output
   TROOT scalana("scalroot","Hall A scaler analysis");
   TFile hfile("scaler.root","RECREATE","Scaler data in Hall A");

// Define the ntuple here
//                   0   1  2   3  4   5  6  7   8  9 10 11  12   13   14
   char rawrates[]="time:u1:u3:u10:d1:d3:d10:t1:t2:t3:t4:t5:clkp:clkm:tacc:";
//                      15 16   17  18  19   20  21   22  23  24  25  26
   char asymmetries[]="au1:au3:au10:ad1:ad3:ad10:at1:at2:at3:at4:at5:aclk";
   int nlen = strlen(rawrates) + strlen(asymmetries);
   char *string_ntup = new char[nlen+1];
   strcpy(string_ntup,rawrates);
   strcat(string_ntup,asymmetries);
   TNtuple *ntup = new TNtuple("ascal","Scaler Rates",string_ntup);

   Float_t farray_ntup[27];     // Note, dimension is same as size of string_ntup (i.e. nlen+1)

   status = 0;
   iev = 0;
   iskip = 0;
   lastclkp = 0;
   lastclkm = 0;

   while (status == 0) {

     if(coda) status = coda->codaRead();
     if (status != 0) {
       cout << "coda status nonzero.  assume EOF"<<endl;
       goto quit;
     }
      evdata->LoadEvent(coda->getEvBuffer());

// Dirty trick to average over larger time intervals (depending on 
// SKIPEVENT) to reduce the fluctuations due to clock.  
     if (evdata->GetRocLength(MYROC) > 16) iskip++;
     if (SKIPEVENT != 0 && iskip < SKIPEVENT) continue;
     iskip = 0;

     scaler->LoadData(*evdata);

// Not every trigger has new scaler data, so skip if not new.
     if ( !scaler->IsRenewed() ) {
       cout << "not renewed "<<endl<<flush;
       continue;
     }

     if (iev++ > evnum) goto quit;

// Fill the ntuple here

// Note, we must average the two helicities here to get non-helicity rates
     Double_t time = (scaler->GetPulser(1,"clock") + scaler->GetPulser(-1,"clock"))/1024;
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
     clkp = scaler->GetNormData(1,"clock");
     clkm = scaler->GetNormData(-1,"clock");
     farray_ntup[12] = clkp - lastclkp;
     farray_ntup[13] = clkm - lastclkm;
     lastclkp = clkp;
     lastclkm = clkm;
     farray_ntup[14] = 0.5*(scaler->GetNormRate(1,"TS-accept") + 
                            scaler->GetNormRate(-1,"TS-accept"));
     string bcms[] = {"bcm_u1", "bcm_u3", "bcm_u10", "bcm_d1", "bcm_d3", "bcm_d10"};
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

     if (DEBUG) {
        cout << "event type "<<evdata->GetEvType()<< "   event number "<<evdata->GetEvNum()<<endl;
        scaler->Print();
        if (farray_ntup[12] < 1000) cout << "Low clock"<<endl;
        cout << " clock(+)  "<<scaler->GetNormRate(1,"clock");
        cout << "   clock(-)  "<<scaler->GetNormRate(-1,"clock")<<endl;
        if (farray_ntup[16]>0.005||farray_ntup[16]<-0.005) cout << "Big asy"<<endl;
     }

     ntup->Fill(farray_ntup);
   }

quit:
   hfile.Write();
   hfile.Close();
   
   exit(0);

}







