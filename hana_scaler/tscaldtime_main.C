//--------------------------------------------------------
//  tscaldtime_main.C
//
//  Read a CODA file and check the deadtime from scalers.
// 
//  R. Michaels, May 2001 
//--------------------------------------------------------

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

   int detailed_printout = 1; // set =1 (0) for printout(or not)
   int trig;

   if (argc < 3) {
     cout << "Usage:  " << argv[0] << " file  bank" << endl;
     cout << "where file = CODA file to analyze " << endl;
     cout << "and bank = 'Right', 'Left'  (spectrometer)" << endl;
     return 1;
   }
   TString filename = argv[1];
   string bank = argv[2];   

   THaScaler *scaler;
   scaler = new THaScaler(bank.c_str());
   if (scaler->Init() == -1) {  
      cout << "Error initializing scaler " << endl;
      return 1;
   }

   cout << "Enter ps factors" << endl;
   cout << "(A value 'zero' means essentially infinite)" << endl;
   Int_t psf[20];  
   for (trig=1; trig<=12; trig++) psf[trig]=0;
   if (bank == "Right") {
      cout << "ps1: "; cin >> psf[1];
      cout << "ps2: "; cin >> psf[2];
   } else {
      cout << "ps3: "; cin >> psf[3];
      cout << "ps4: "; cin >> psf[4];
   }
   cout << "ps8: "; cin >> psf[8];
   for (trig=1; trig<=4; trig++) {
       if (psf[trig] == 0) psf[trig]=16777216;   //  2^24 is max
       psf[trig] = psf[trig]%16777216;
   }
   for (trig=5; trig<=8; trig++) {
       if (psf[trig] == 0) psf[trig]=65536;  //  2^16 is max
       psf[trig] = psf[trig]%65536;
   }
   for (trig=9; trig<=12; trig++) psf[trig]=1;  // cannot prescale

   Double_t tsum,ltime;
   int status = 1;
   
   while (status) {
     status = scaler->LoadDataCodaFile(filename);   // load data for 'filename'
     if (!status) goto quit;
     Double_t time = scaler->GetPulser("clock")/1024;

     if (detailed_printout) {
       cout << "\n\n------------  SCALER EVENT DATA ---------------\n" << endl;
       scaler->Print();     // raw diagnostic printout
       cout << "Time    " << time << "\n Counts :   " << endl;
       for (trig = 1; trig <= 5; trig++ ) {
         cout << "Trigger " << trig << "   hel+  0  hel- " << scaler->GetTrig(1,trig) << "   " << scaler->GetTrig(trig) << "   " << scaler->GetTrig(-1,trig) << endl;
       }
       cout << "Rates (Hz) = " << endl;
       cout << "Bcm u3   hel+  0  hel- " << scaler->GetBcmRate(1,"bcm_u3") << "   " << scaler->GetBcmRate("bcm_u3") << "   " << scaler->GetBcmRate(-1,"bcm_u3") << endl;
       for (trig = 1; trig <= 5; trig++ ) {
         cout << "Trigger " << trig << "   hel+  0  hel- " << scaler->GetTrigRate(1,trig) << "   " << scaler->GetTrigRate(trig) << "   " << scaler->GetTrigRate(-1,trig) << endl;
       }
     }

     tsum = 0;
     for (int i = 1; i <= 3; i++) {
       if (bank == "Right") {
          trig=i;
       } else {
          trig=i+2;
       }
       if (i==3) trig=8;
       tsum = tsum + scaler->GetTrigRate(0,trig)/psf[trig];
     }
     Double_t tsacc;
// Since the Norm scaler for helicity=0 is in slot 8 and 'ts-accept' in chan 12...
     tsacc = scaler->GetScalerRate(8,12);  // equivalent to next line (slot 8, chan 12)
     tsacc = scaler->GetNormRate(0,12);  // equivalent to next line (helicity 0, chan 12)
     tsacc = scaler->GetNormRate(0,"TS-accept");  // for helicity 0
     ltime = -1;
     if(tsum != 0) ltime = tsacc / tsum;
     cout << "Sum trig " << tsum << "  TS acc " 
          << tsacc << "   live time = " << ltime << endl;

              
   }  // <--- loop over scaler events in CODA file

quit:

   return 0;
}







