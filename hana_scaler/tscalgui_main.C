//--------------------------------------------------------
//  tscalgui_main.C
//
//  Example to use the THaScalerGui class to display scaler 
//  data in a GUI ("xscaler" style) from VME.
//  Under development -- this is a test code and may be buggy.
//  R. Michaels, May 2001
//--------------------------------------------------------

#include <iostream>
#include <string>
#include "TROOT.h"
#include "TApplication.h"
#include "TCanvas.h"
#include "TPad.h"
#include "TH1.h"
#include "TPaveLabel.h"
#include "THaScalerGui.h"

using namespace std;

extern void InitGui();
VoidFuncPtr_t initfuncs[] = { InitGui, 0 };
TROOT root("Scalers","Hall A Scaler GUI", initfuncs);
void usage();


int main(int argc, char **argv) {

   string bank;  

   if (argc > 1) {
     bank = argv[1];
     if (bank != "Left" && bank != "LEFT" && bank != "Right" && bank != "RIGHT" && bank != "DVCS_CALO" ) {
         usage();
         return 1;
     }
   } else {
     bank = "Left";       // Left is default
     cout << "Since you provided no argument, we assume you"<<endl;
     cout << "want to use the "<<bank<<" spectrometer."<<endl;
     cout << "And for your reference, here are usage instructions: "<<endl;
     usage();
   }

   TApplication theApp("App", &argc, argv);
   THaScalerGui scalergui(gClient->GetRoot(), 500, 400, bank);
   theApp.Run();

   return 0;
}

void usage() {
  cout << endl << "Usage:  ./xscaler [bank]"<<endl;
  cout << "where bank = `Left' or `Right' or 'DVCS_CALO'"<<endl;
  cout << "(without the quotes)"<<endl;
  cout << "and default bank is `Left' (i.e. if no arg)"<<endl;
}






