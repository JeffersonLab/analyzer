//--------------------------------------------------------
//  tscalbad_main.C
//
//  Test of scaler class, for reading scaler "bad" data
//  from scaler_badread.dat end-of-run history.  These are
//  data where the QRT or other helicity info made no sense,
//  so online packaging didn't work.
//
//  Need to do this on private copies of the datafiles:
//     ln -s scaler_badread.dat scaler_history.dat
// 
//  R. Michaels, updated Feb 2006
//--------------------------------------------------------

#include <iostream>
#include <string>
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

int main(int argc, char* argv[]) {

   const Double_t calib_u3  = 4114;  // calibrations (an example)
   const Double_t calib_d10 = 12728;
   const Double_t off_u3    = 167.1;
   const Double_t off_d10   = 199.5;
   Float_t clockrate;
   Int_t printout = 0;
   Int_t hel;

// Initialize root and output
   TROOT scalana("scalroot","Hall A scaler analysis");
   TFile hfile("scalerhist.root","RECREATE","Scaler history in Hall A");

// Define the ntuple here
//                0   1    2    3   4   5   6   7   8   9  10    11
   char cdata[]="run:time:u3:d10:ct1:ct2:ct3:ct4:ct5:ct6:ct7:ctacc";
   int nlen = strlen(cdata);
   char *string_ntup = new char[nlen+1];
   strcpy(string_ntup,cdata);
   TNtuple *ntup = new TNtuple("ascal","Scaler History Data",string_ntup);

   Float_t farray_ntup[12];       // Note, dimension is same as size of string_ntup 

   cout << "Enter bank 'gen' or 'Right','Left' (spectr) ->" << endl;
   string bank;  cin >> bank;
   if (bank == "gen" || bank == "GEN" ) {
     clockrate = 105000;
   } else {
     clockrate = 1024;
   }
   cout << "enter [runlo, runhi] = interval of runs to summarize ->" << endl;
   int runlo; cout << "runlo: " << endl; cin >> runlo; 
   int runhi; cout << "runhi: " << endl; cin >> runhi;

   THaScaler scaler(bank.c_str());
   if (scaler.Init() == -1) {    // Init for default time ("now").
      cout << "Error initializing scaler "<<endl;  return 1;
   }
   if (printout) printf("\n Using clock rate %f Hz\n",clockrate);

   int status;

   for (int run = runlo; run < runhi; run++) {

     status = scaler.LoadDataHistoryFile(run);  // load data from default history file
     if ( status != 0 ) continue;

     //     for (int i = 0; i < 12; i++) farray_ntup[i] = 0;

     if (printout) {
       cout << "\nBad Scalers for run = "<<run<<endl<<flush;
       cout << "Time    ---  Beam Current (uA)  ----    |   --- Triggers ---  "<<endl;
       cout << "(min)         u3           d10";
       cout << "        T1       T2          T3    Accepted "<<endl;
       for (int ihel = 0; ihel < 2; ihel++) {
         hel = -1; 
         if (ihel == 1) hel = 1;
         cout << ">>>>>>>>   helicity "<<hel<<endl;
         double time_sec = scaler.GetPulser(hel,"clock")/clockrate;
         double curr_u3  = (scaler.GetBcm(hel,"bcm_u3")/time_sec - off_u3)/calib_u3;
         double curr_d10 = (scaler.GetBcm(hel,"bcm_d10")/time_sec - off_d10)/calib_d10;
         printf("%7.2f     %7.2f     %7.2f       %d       %d         %d     %d\n",
	    time_sec/60, curr_u3, curr_d10, scaler.GetTrig(hel,1), scaler.GetTrig(hel,2),
            scaler.GetTrig(hel,3),scaler.GetNormData(hel,11));
       }
     }
     printf("%d  %d \n",run,scaler.GetNormData(1,11));

   }


   return 0;
}











