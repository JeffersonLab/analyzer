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

extern void InitGui();
VoidFuncPtr_t initfuncs[] = { InitGui, 0 };
TROOT root("Scalers","Hall A Scaler GUI", initfuncs);

int main(int argc, char **argv) {

   string bank;  

   if (argc > 1) {
     bank = argv[1];
     if (bank != "Left" && bank != "LEFT" && bank != "Right" && bank != "RIGHT") {
         cout << "Error, undefined bank.  Must be `Left' or `Right'"<<endl;
         cout << "exiting... "<<endl;
         return 1;
     }
   } else {
     bank = "RCS";       // RCS is default
   }

   TApplication theApp("App", &argc, argv);
   THaScalerGui scalergui(gClient->GetRoot(), 500, 400, bank);
   theApp.Run();

   return 0;
}








