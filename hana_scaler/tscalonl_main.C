//--------------------------------------------------------
//  tscalonl_main.C
//
//  Example to use the online scaler code that reads 
//  data from VME.
//  R. Michaels, April 2001
//--------------------------------------------------------

#include <iostream>
#include <string>
#include "THaScaler.h"

using namespace std;

int main(int argc, char* argv[]) {

  //   cout << "Enter bank 'Right' or 'Left' (spectrometer) ->" << endl;
   string bank = "Left";  
   // cin >> bank;

   THaScaler scaler(bank.c_str());
   if (scaler.Init() == -1) {  
      cout << "Error initializing scalers"<<endl;  return 1;
   }

   int event = 0;
   string detector[] = {"s1", "s2", "gasC"};

   while ( scaler.LoadDataOnline() != SCAL_ERROR ) {  // each call gets 1 event

//   scaler.Print();

     int i,chan, nchan;
     cout << "\nPMT Counts --------"<<endl;
     for (i = 0; i < 3; i++) {
       cout << "Detector "<<detector[i]<<endl;
       nchan = 6;
       if (i == 3) nchan = 10;
       for (chan = 0; chan < nchan; chan++) {
         if (i < 2) {
           cout << "Left PMT  "<<chan<<" = "
		<<scaler.GetScaler(detector[i].c_str(),"left",chan)<<endl;
           cout << "Right PMT "<<chan<<" = "
		<<scaler.GetScaler(detector[i].c_str(),"right",chan)<<endl;
	 }
         cout << "PMT "<<chan<<" = "
	   <<scaler.GetScaler(detector[i].c_str(),chan)<<endl; // L+R for scint
       }                                    // or just a PMT for gasC
     }
     cout << "\nPMT Rates --------"<<endl;
     for (i = 0; i < 3; i++) {
       cout << "Detector "<<detector[i].c_str()<<endl;
       for (chan = 0; chan < nchan; chan++) {  // C indices start at 0
         if (i < 2) {
           cout << "Left PMT["<<chan<<"] = "
		<<scaler.GetScalerRate(detector[i].c_str(),"left",chan)<<endl;
           cout << "Right PMT["<<chan<<"] = "
		<<scaler.GetScalerRate(detector[i].c_str(),"right",chan)<<endl;
	 }
         cout << "PMT["<<chan<<"] = "
	 <<scaler.GetScalerRate(detector[i].c_str(),chan)<<endl; // L+R for scint
       }                                      // or just a PMT for gasC
     }
     cout << "/nTriggers ---------"<<endl;
     for (i = 1; i <= 8; i++) {
       cout << "Trigger "<<i<<
 	   "    Counts = "<<scaler.GetTrig(i)<<
           "    Rate = "<<scaler.GetTrigRate(i)<<" Hz"<<endl;
     }

     system("sleep 2");
     if (event++ > 4) break;

   }
   return 0;
}








