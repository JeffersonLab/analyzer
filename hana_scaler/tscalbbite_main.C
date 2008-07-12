//--------------------------------------------------------
//  tscalbbite_main.C
//
//  Read a CODA file and fill an ntuple with scaler data.
//  This version uses the "old" event type 140 data
//  and looks at bigbite.
// 
//  R. Michaels, Mar 2008
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

   int printout = 1;   // To printout lots of stuff (1) or not (0)
   Float_t clockrate = 1024;

   if (argc < 2) {
     cout << "Usage:  " << argv[0]<<" file" << endl;
     return 1;
   }
   TString filename = argv[1];
   string bank = "bbite";   

   cout << "Analyzing the scaler bank = "<<bank<<endl;

   THaScaler *scaler;
   scaler = new THaScaler(bank.c_str());

// Init must be done once.  If you leave out the date, it assumes "now".
   if (scaler->Init("1-3-2008") == -1) {  // March 1, 2007 for e04007
      cout << "Error initializing scaler " << endl;
      return 1;
   }

// Initialize root and output
   TROOT scalana("scalroot","Hall A scaler analysis");
   TFile hfile("scaler.root","RECREATE","Scaler data in Hall A");

// Define the ntuple here
//                   0   1   2  3    4   5  
   char rawrates[]="e1l:e2l:e3l:e4l:e5l:e6l:e7l:e8l:e9l:e10l:e11l:e12l:e13l:e14l:e15l:e16l:e17l:e18l:e19l:e20l:e21l:e22l:e23l:e24l:e1r:e2r:e3r:e4r:e5r:e6r:e7r:e8r:e9r:e10r:e11r:e12r:e13r:e14r:e15r:e16r:e17r:e18r:e19r:e20r:e21r:e22r:e23r:e24r:de1l:de2l:de3l:de4l:de5l:de6l:de7l:de8l:de9l:de10l:de11l:de12l:de13l:de14l:de15l:de16l:de17l:de18l:de19l:de20l:de21l:de22l:de23l:de24l:de1r:de2r:de3r:de4r:de5r:de6r:de7r:de8r:de9r:de10r:de11r:de12r:de13r:de14r:de15r:de16r:de17r:de18r:de19r:de20r:de21r:de22r:de23r:de24r:t1:t2:t3:t4:t5:t6:t7:t8:l1a:bcm:edtm";
   char counts[]=  "ce1l:ce2l:ce3l:ce4l:ce5l:ce6l:ce7l:ce8l:ce9l:ce10l:ce11l:ce12l:ce13l:ce14l:ce15l:ce16l:ce17l:ce18l:ce19l:ce20l:ce21l:ce22l:ce23l:ce24l:ce1r:ce2r:ce3r:ce4r:ce5r:ce6r:ce7r:ce8r:ce9r:ce10r:ce11r:ce12r:ce13r:ce14r:ce15r:ce16r:ce17r:ce18r:ce19r:ce20r:ce21r:ce22r:ce23r:ce24r:cde1l:cde2l:cde3l:cde4l:cde5l:cde6l:cde7l:cde8l:cde9l:cde10l:cde11l:cde12l:cde13l:cde14l:cde15l:cde16l:cde17l:cde18l:cde19l:cde20l:cde21l:cde22l:cde23l:cde24l:cde1r:cde2r:cde3r:cde4r:cde5r:cde6r:cde7r:cde8r:cde9r:cde10r:cde11r:cde12r:cde13r:cde14r:cde15r:cde16r:cde17r:cde18r:cde19r:cde20r:cde21r:cde22r:cde23r:cde24r:ct1:ct2:ct3:ct4:ct5:ct6:ct7:ct8:cl1a:cbcm:cedtm";
   int nlen = strlen(rawrates) + strlen(counts);
   char *string_ntup = new char[nlen+1];
   strcpy(string_ntup,rawrates);
   strcat(string_ntup,counts);
   TNtuple *ntup = new TNtuple("ascal","Bigbite Scalers",string_ntup);

   Float_t farray_ntup[214];       // Note, dimension must be = number of variables 

   for (int i=0; i<214; i++) farray_ntup[i] = 0;

   int status = 1;
   int iev = 0;
   
   while (status) {
     status = scaler->LoadDataCodaFile(filename);   // load data for 'filename'
     if (!status) goto quit;
     Double_t time = scaler->GetPulser("clock")/clockrate;

     if ( iev > 0 && (iev < 10 || ((iev % 10) == 0)) ) cout << "iev = " << iev << endl;

// Optional printout
     if (printout == 1) {
       cout << "\n\n--------------  SCALER EVENT DATA -----------------------\n"<<endl;
       scaler->Print();     // raw diagnostic printout
       cout << "Time    " << time << "\n Counts :   " << endl;
       for (int islot=0; islot<4; islot++) {
         cout << dec << "\n----------  SLOT "<<dec<<islot+1<<"  -------------------------- "<<endl;
	 for (int ichan=0; ichan<32; ichan++) {
           cout << " 0x"<<hex<<scaler->GetScaler(islot,ichan);
	 }
       }
       cout << endl;
     }  // <--  if(printout)


      char device[25];
      Float_t rate,count;

      for (int i=0; i<24; i++) {
	sprintf(device,"E%dL",i+1);
        rate = scaler->GetScalerRate(device,0);
        count = scaler->GetScaler(device,0);
	if (printout) cout << "time "<<time<<"  device "<<device<<"   rate "<<rate<<"   count "<<count<<endl;
        farray_ntup[i] = rate;
        farray_ntup[i+107] = count;
      }     
   

      for (int i=0; i<24; i++) {
	sprintf(device,"E%dR",i+1);
        rate = scaler->GetScalerRate(device,0);
        count = scaler->GetScaler(device,0);
	if (printout) cout << "time "<<time<<"  device "<<device<<"   rate "<<rate<<"   count "<<count<<endl;
        farray_ntup[i+24] = rate;
        farray_ntup[i+107+24] = count;
      }     

      for (int i=0; i<24; i++) {
	sprintf(device,"dE%dL",i+1);
        rate = scaler->GetScalerRate(device,0);
        count = scaler->GetScaler(device,0);
	if (printout) cout << "time "<<time<<"  device "<<device<<"   rate "<<rate<<"   count "<<count<<endl;
        farray_ntup[i+48] = rate;
        farray_ntup[i+107+48] = count;
      }     

      for (int i=0; i<24; i++) {
	sprintf(device,"dE%dR",i+1);
        rate = scaler->GetScalerRate(device,0);
        count = scaler->GetScaler(device,0);

        farray_ntup[i+72] = rate;
        farray_ntup[i+107+72] = count;
      }     
   
      farray_ntup[48] = scaler->GetScalerRate("T1",0);
      farray_ntup[49] = scaler->GetScalerRate("T2",0);
      farray_ntup[50] = scaler->GetScalerRate("T3",0);
      farray_ntup[51] = scaler->GetScalerRate("T4",0);
      farray_ntup[52] = scaler->GetScalerRate("T5",0);
      farray_ntup[53] = scaler->GetScalerRate("T6",0);
      farray_ntup[54] = scaler->GetScalerRate("T7",0);
      farray_ntup[55] = scaler->GetScalerRate("T8",0);
      farray_ntup[56] = scaler->GetScalerRate("L1A",0);
      farray_ntup[57] = scaler->GetScalerRate("current",0);
      farray_ntup[58] = scaler->GetScalerRate("EDTM",0);

      farray_ntup[48+107] = scaler->GetScaler("T1",0);
      farray_ntup[49+107] = scaler->GetScaler("T2",0);
      farray_ntup[50+107] = scaler->GetScaler("T3",0);
      farray_ntup[51+107] = scaler->GetScaler("T4",0);
      farray_ntup[52+107] = scaler->GetScaler("T5",0);
      farray_ntup[53+107] = scaler->GetScaler("T6",0);
      farray_ntup[54+107] = scaler->GetScaler("T7",0);
      farray_ntup[55+107] = scaler->GetScaler("T8",0);
      farray_ntup[56+107] = scaler->GetScaler("L1A",0);
      farray_ntup[57+107] = scaler->GetScaler("current",0);
      farray_ntup[58+107] = scaler->GetScaler("EDTM",0);

      ntup->Fill(farray_ntup);
      

   }  // <--- loop over scaler events in CODA file


quit:
   hfile.Write();
   hfile.Close();

   // Here is a a summary
   cout << endl << endl;
   cout << " --------------  Summary of Bigbite Scalers -------------- "<<endl<<endl;
   cout << "Time of run (sec)  "<<dec<<scaler->GetPulser("clock")/clockrate<<endl;
   cout << "Triggers : " << endl;
   for (int i=0; i<8; i++) {
      char ctrig[10];
      sprintf(ctrig,"T%d",i+1);
      cout << "  "<<ctrig<<"  = "<<scaler->GetScaler(ctrig,0);
      if (i==3) cout << endl;
   }
   cout << "\nCharge (arb units): "<<scaler->GetScaler("current",0)<<endl;
   cout << "Events (L1A) : "<<scaler->GetScaler("L1A",0) <<endl;


   return 0;
}







