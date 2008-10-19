//--------------------------------------------------------
//  tscalasy_main.C
//
//  Example to compute helicity correlated charge asymmetry
//  R. Michaels, April 2001
//--------------------------------------------------------

#include <iostream>
#include <string>
#include "THaScaler.h"
#include "TMath.h"

using namespace std;

int main(int argc, char* argv[]) {

   cout << "Enter name of CODA file ->" << endl;
   string filename;  cin >> filename;  
   cout << "Enter bank 'Right' or 'Left' (spectrometer) ->" << endl;
   string bank;  cin >> bank;

   THaScaler scaler(bank.c_str());
   if (scaler.Init("10-12-2004") == -1) {  // "day-month-year", or "now"
      cout << "Error initializing scalers"<<endl;  return 1;
   }

   Double_t beamcut = 50;
   Double_t au3,ad10;  // this example treats Upstream-x3 and Downstream-x10.
   Double_t au3sum,au3sq,ad10sum,ad10sq,sum,xcnt;
   au3sum=0; au3sq = 0; ad10sum = 0; ad10sq = 0; xcnt = 0;

   while ( scaler.LoadDataCodaFile(filename.c_str()) ) {  // each call gets 1 event

     Double_t iu3  = scaler.GetBcmRate("bcm_u3");  
     Double_t id10 = scaler.GetBcmRate("bcm_d10");

     if (iu3 > beamcut && id10 > beamcut) {   // probably want better cuts
       sum = scaler.GetBcmRate(1,"bcm_u3") + 
                 scaler.GetBcmRate(-1,"bcm_u3");
       if (sum > 0) {
	 au3 = (scaler.GetBcmRate(1,"bcm_u3") - 
                 scaler.GetBcmRate(-1,"bcm_u3"))/ sum;
       } // else  {  // error, do something  }

       sum = scaler.GetBcmRate(1,"bcm_d10") + 
                 scaler.GetBcmRate(-1,"bcm_d10");
       if (sum > 0) {
	 ad10 = (scaler.GetBcmRate(1,"bcm_d10") - 
                 scaler.GetBcmRate(-1,"bcm_d10"))/ sum;
       }  // else {  // error, do something }

// fill ntuple or whatever
       au3sum = au3sum + au3;  au3sq = au3sq + au3*au3;
       ad10sum = ad10sum + ad10;  ad10sq = ad10sq + ad10*ad10;
       xcnt = xcnt + 1.;
     }
   }

   if (xcnt > 0) {
     au3 = au3sum / xcnt;
     Double_t xx = au3sq/xcnt - au3*au3;
     Double_t au3sig = 0;
     if (xx > 0) au3sig = TMath::Sqrt(xx);
     ad10 = ad10sum / xcnt;
     xx = ad10sq/xcnt - ad10*ad10;
     Double_t ad10sig = 0;
     if (xx > 0) ad10sig = TMath::Sqrt(xx);
     cout << "Upstream x3  Asymmetry    = "<<au3<<" +/- "<<au3sig<<endl;
     cout << "Downstream x10  Asymmetry = "<<ad10<<" +/- "<<ad10sig<<endl;
   } else {
     cout << "No statistics !  Maybe no events ?\n";
   }
   return 0;
}


















