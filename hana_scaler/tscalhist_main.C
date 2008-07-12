//--------------------------------------------------------
//  tscalhist_main.C
//
//  Test of scaler class, for reading scaler history (end-of-run)
//  files and obtaining data.
//  This version also fills an ntuple
//  Hint: if you have two scaler history files (for L and R arm),
//  try to concatenate them 'cat file1 file2 > file3' and analyze
//  the resulting concatenated file3.
// 
//  R. Michaels, updated Mar 2005
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

   Int_t lprint = 3;  // choice of printout

   const Double_t calib_u3  = 7180;  // calibrations (an example)
   const Double_t calib_d3  = 7508; 
   const Double_t calib_u10 = 21897;
   const Double_t calib_d10 = 23633;
   const Double_t off_u3    = 369;
   const Double_t off_d3    = 110.0;
   const Double_t off_u10   = 477;
   const Double_t off_d10   = 253;
   Float_t clockrate;
   Float_t totchg = 0;

// Initialize root and output
   TROOT scalana("scalroot","Hall A scaler analysis");
   TFile hfile("scalerhist.root","RECREATE","Scaler history in Hall A");

// Define the ntuple here
//                0   1    2    3   4   5   6   7   8   9  10    11   12   13   14   15
   char cdata[]="run:time:u3:d10:ct1:ct2:ct3:ct4:ct5:ct6:ct7:ctacc:charge:clkp:clkm:clk:d3:u10";
   int nlen = strlen(cdata);
   char *string_ntup = new char[nlen+1];
   strcpy(string_ntup,cdata);
   TNtuple *ntup = new TNtuple("ascal","Scaler History Data",string_ntup);

   Float_t farray_ntup[18];       // Note, dimension is same as size of string_ntup 

   cout << "Enter bank 'Right','Left' (spectr) ->" << endl;
   string bank;  cin >> bank;
   if (bank == "gen" || bank == "GEN" ) {
     clockrate = 105000;
   } else {
     clockrate = 1024;
   }
   cout << "enter [runlo, runhi] = interval of runs to summarize ->" << endl;
   int runlo; cout << "runlo: " << endl; cin >> runlo; 
   int runhi; cout << "runhi: " << endl; cin >> runhi;
   cout << "is data in hex(1) or decimal(0) format ? "<<endl;
   int hdeci;
   cin >> hdeci;

   THaScaler scaler(bank.c_str());
   if (scaler.Init() == -1) {    // Init for default time ("now").
      cout << "Error initializing scaler "<<endl;  return 1;
   }
   //   printf("\n Using clock rate %f Hz\n",clockrate);

   int status;

   for (int run = runlo; run <= runhi; run++) {

     status = scaler.LoadDataHistoryFile(run, hdeci);  // load data from default history file
     if ( status != 0 ) continue;

     for (int i = 0; i < 18; i++) farray_ntup[i] = 0;

     double time_sec = scaler.GetPulser("clock")/clockrate;
     double curr_u3  = (scaler.GetBcm("bcm_u3")/time_sec - off_u3)/calib_u3;
     double curr_d3  = (scaler.GetBcm("bcm_d3")/time_sec - off_d3)/calib_d3;
     double curr_u10 = (scaler.GetBcm("bcm_u10")/time_sec - off_u10)/calib_u10;
     double curr_d10 = (scaler.GetBcm("bcm_d10")/time_sec - off_d10)/calib_d10;
     farray_ntup[0] = run;
     farray_ntup[1] = time_sec;
     farray_ntup[2] = curr_u3;
     farray_ntup[3] = curr_d10;
     for (int itrig = 1; itrig <= 7; itrig++) farray_ntup[3+itrig]=scaler.GetTrig(itrig);
     farray_ntup[11] = scaler.GetNormData(0,12);
     Float_t charge = curr_u3*time_sec;
     Float_t chgu3 = curr_u3*time_sec;
     Float_t chgd3 = curr_d3*time_sec;
     Float_t chgu10 = curr_u10*time_sec;
     Float_t chgd10 = curr_d10*time_sec;
     Float_t clkplus = scaler.GetPulser(1,"clock");
     Float_t clkminus = scaler.GetPulser(-1,"clock");
     Float_t clock = scaler.GetPulser("clock");
     farray_ntup[12] = charge; 
     totchg += charge;
     farray_ntup[13] = clkplus;
     farray_ntup[14] = clkminus;
     farray_ntup[15] = clock;
     farray_ntup[16] = curr_d3;
     farray_ntup[17] = curr_u10;
     ntup->Fill(farray_ntup);
     Float_t clkdiff = (clock - 1.016*(clkplus+clkminus))/clockrate; // units: seconds

     if (lprint == 1) {
        cout << "\nScalers for run = "<<run<<endl<<flush;
        cout << "Time    ---  Beam Current (uA)  ----    |   --- Triggers ---  "<<endl;
        cout << "(min)         u3           d10";
        cout << "        T1       T2          T3    Accepted "<<endl;
        printf("%7.2f     %7.2f     %7.2f       %d       %d         %d     %d\n",
	    time_sec/60, curr_u3, curr_d10, scaler.GetTrig(1), scaler.GetTrig(2),
            scaler.GetTrig(3),scaler.GetNormData(0,12));
     }
     if (lprint == 2) {
       if (run == runlo) printf("Run    Events    Time    Current   Charge(uC)      T1      T3      T5    T6\n");
       printf("%d    %d   %6.1f   %7.2f   %7.2f    %d   %d   %d   %d\n",
              run,scaler.GetNormData(0,12),time_sec/60,curr_u3,charge,
              scaler.GetTrig(1), scaler.GetTrig(3),scaler.GetTrig(5),scaler.GetTrig(6));
     }
     if (lprint == 3) {
       if (run == runlo) printf("Run    Events    Time    Current (u3, d3, u10, d10)  Charge (u3, d3, u10, d10)\n");
       printf("%d    %d   %6.1f   %7.2f  %7.2f  %7.2f  %7.2f    %7.2f  %7.2f  %7.2f  %7.2f \n",      
              run,scaler.GetNormData(0,12),time_sec/60,curr_u3,curr_d3,curr_u10,curr_d10,
              chgu3,chgd3,chgu10,chgd10);
     }

     //  printf("==== %d  %d \n",run,scaler.GetNormData(1,11));


   }

   hfile.Write();
   hfile.Close();

   cout << "\n\nTotal charge from run "<<runlo<<"  to run "<<runhi<<"  = "<<1e-6*totchg<<"  Coul "<<endl;


   return 0;
}











