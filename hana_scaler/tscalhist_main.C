//--------------------------------------------------------
//  tscalhist_main.C
//
//  Test of scaler class, for reading scaler history (end-of-run)
//  files and obtaining data.
//  Hint: if you have two scaler history files (for L and R arm),
//  try to concatenate them 'cat file1 file2 > file3' and analyze
//  the resulting concatenated file3.
// 
//  R. Michaels, April 2001
//--------------------------------------------------------

#include <iostream>
#include <string>
#include "THaScaler.h"

using namespace std;

int main(int argc, char* argv[]) {

   const Double_t calib_u3  = 4114;  // calibrations (an example)
   const Double_t calib_d10 = 12728;
   const Double_t off_u3    = 167.1;
   const Double_t off_d10   = 199.5;

   cout << "Enter bank 'Right' or 'Left' (spectrometer) ->" << endl;
   string bank;  cin >> bank;
   cout << "enter [runlo, runhi] = interval of runs to summarize ->" << endl;
   int runlo; cout << "runlo: " << endl; cin >> runlo; 
   int runhi; cout << "runhi: " << endl; cin >> runhi;

   THaScaler scaler(bank.c_str());
   if (scaler.Init() == -1) {    // Init for default time ("now").
      cout << "Error initializing scaler "<<endl;  return 1;
   }

   int status;

   for (int run = runlo; run < runhi; run++) {

     status = scaler.LoadDataHistoryFile(run);  // load data from default history file
     if ( status != 0 ) continue;

     cout << "\nScalers for run = "<<run<<endl<<flush;
     cout << "Time    ---  Beam Current (uA)  ----    |   --- Triggers ---  "<<endl;
     cout << "(min)         u3           d10";
     cout << "        T1(Right)      T3(Left)       Tot accepted"<<endl;
     double time_sec = scaler.GetPulser("clock")/1024;
     double chg_u3  = (scaler.GetBcm("bcm_u3")/time_sec - off_u3)/calib_u3;
     double chg_d10 = (scaler.GetBcm("bcm_d10")/time_sec - off_d10)/calib_d10;
     printf("%7.2f     %7.2f     %7.2f       %d       %d         %d\n",
     time_sec/60, chg_u3, chg_d10, scaler.GetTrig(1), scaler.GetTrig(3),
	    scaler.GetNormData(0,12));

   }
   return 0;
}











