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
#include "THaEvData.h"

#ifndef __CINT__
#include "TROOT.h"
#include "TFile.h"
#include "TH1.h"
#include "TH2.h"
#include "TProfile.h"
#include "TNtuple.h"
#include "TRandom.h"
#endif

#define BCM_CUT1  3000   // cut on BCM (x1 gain) to require beam on.
#define BCM_CUT3  10000  // cut on BCM (x3 gain) to require beam on.
#define BCM_CUT10 30000  // cut on BCM (x10 gain) to require beam on.
#define NBCM 6         // number of BCM signals 

int main(int argc, char* argv[]) {

   int trig;
   Float_t sum,asy;
   TString filename = "run.dat";
   
   char bank[100] = "EvLeft";  // Event stream, Left HRS
   cout << "Analyzing CODA file  "<<filename<<endl;   
   cout << "Bank = "<<bank<<endl<<flush;

   THaScaler *scaler = new THaScaler(bank);

   if (scaler->Init("1-9-2002") == -1) {  
      cout << "Error initializing scaler "<<endl;
      return 1;
   }

   THaCodaFile *coda = new THaCodaFile(filename);
   THaEvData evdata;

// Pedestals.  Left, Right Arms.  u1,u3,u10,d1,d3,d10
   Float_t bcmpedL[NBCM] = { 52.5, 44.1, 110.7, 0., 94.2, 227. };
   Float_t bcmpedR[NBCM] = { 53.0, 41.8, 104.1, 0., 101.6, 254.6 };
   Float_t bcmped[NBCM];

   int i;
   if (bank == "EvLeft") {
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
//                   0   1  2   3  4   5  6  7   8  9 10 11 12  13   
   char rawrates[]="time:u1:u3:u10:d1:d3:d10:t1:t2:t3:t4:t5:clk:tacc:";
//                      14 15   16  17  18   19  20   21  22  23  24  25
   char asymmetries[]="au1:au3:au10:ad1:ad3:ad10:at1:at2:at3:at4:at5:aclk";
   int nlen = strlen(rawrates) + strlen(asymmetries);
   char *string_ntup = new char[nlen+1];
   strcpy(string_ntup,rawrates);
   strcat(string_ntup,asymmetries);
   TNtuple *ntup = new TNtuple("ascal","Scaler Rates",string_ntup);

   Float_t farray_ntup[26];     // Note, dimension is same as size of string_ntup (i.e. nlen+1)


   int status = 0;
   
   while (status == 0) {

     status = coda->codaRead();
     if (status != 0) {
       cout << "coda status nonzero.  assume EOF"<<endl;
       goto quit;
     }
     evdata.LoadEvent(coda->getEvBuffer());
     scaler->LoadData(evdata);

// Not every trigger has new scaler data, so skip if not new.
     if ( !scaler->IsRenewed() ) continue;

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
     farray_ntup[12] = 0.5*(scaler->GetNormRate(1,"clock") + scaler->GetNormRate(-1,"clock"));
     farray_ntup[13] = 0.5*(scaler->GetNormRate(1,"TS-accept") + 
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
       farray_ntup[14+ibcm] = asy;
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
       farray_ntup[19+trig] = asy;
     }
     sum = scaler->GetPulser(1,"clock") + scaler->GetPulser(-1,"clock");
     asy = -999;
     if (sum != 0) {
	asy = (scaler->GetPulser(1,"clock") - 
               scaler->GetPulser(-1,"clock")) / sum;
     }
     farray_ntup[25] = asy;

     ntup->Fill(farray_ntup);
   }

quit:
   cout << "writing file "<<endl<<flush;
   hfile.Write();
   cout << "closing file "<<endl<<flush;
   hfile.Close();
   cout << "all done "<<endl<<flush;
   
   exit(0);

}







