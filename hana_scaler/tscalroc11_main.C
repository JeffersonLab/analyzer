//--------------------------------------------------------
//  tscalfile_main.C
//
//  Test of the ROC11 scalers which are read in the datastream
//  every synch event (typ. every 100 events).  As of Oct 2001 this
//  is a new readout.  The old readout was the "event type 140"
//  which was inserted asynchronously every few seconds using ET.
//  Those "event type 140" were called scaler events by THaEvData,
//  and will continue to be supported.  The main advantage of ROC11
//  (and ROC12 when it exists) is that it comes synchronously with
//  physics triggers, and so you know the time relationship from the
//  location in the sequence of events.
// 
//  R. Michaels, Jan 2002
//--------------------------------------------------------

#include <iostream>
#include <string>

#include "THaScaler.h"
#include "THaCodaFile.h"
#include "THaEvData.h"

int main(int argc, char* argv[]) {

   int printout1 = 1;   // To printout lots of stuff (1) or not (0)
   int printout2 = 1;   // Another printout option
   int trig;
   TString filename = "run.dat";
   
   char bank[100] = "EvLeft";  // Event stream, Left HRS
   cout << "Bank = "<<bank<<endl<<flush;

   THaScaler *scaler = new THaScaler(bank);

   if (scaler->Init("17-1-2002") == -1) {  // Init MUST be done once
      cout << "Error initializing scaler "<<endl;
      return 1;
   }

   THaCodaFile *coda = new THaCodaFile(filename);
   THaEvData evdata;

   int status = 0;
   
   while (status == 0) {

     status = coda->codaRead();
     if (status != 0) break;
     evdata.LoadEvent(coda->getEvBuffer());
     scaler->LoadData(evdata);

// Not every trigger has new scaler data, so skip if not new.
     if ( !scaler->IsRenewed() ) continue;

     if (printout1) {
       cout << "\n\n--------------  SCALER EVENT DATA -----------------------\n"<<endl;
       //       scaler->Print();     // raw diagnostic printout
       Double_t time = scaler->GetPulser("clock")/1024;
       cout << "Time    " << time << "\n Counts :   " << endl;
       cout << "Bcm u1   hel+  0  hel- " << scaler->GetBcm(1,"bcm_u1") << "   " << scaler->GetBcm("bcm_u1") << "   " << scaler->GetBcm(-1,"bcm_u1") << endl;
       cout << "Bcm u3   hel+  0  hel- " << scaler->GetBcm(1,"bcm_u3") << "   " << scaler->GetBcm("bcm_u3") << "   " << scaler->GetBcm(-1,"bcm_u3") << endl;
       cout << "Bcm u10   hel+  0  hel- " << scaler->GetBcm(1,"bcm_u10") << "   " << scaler->GetBcm("bcm_u10") << "   " << scaler->GetBcm(-1,"bcm_u10") << endl;
       cout << "Bcm d1   hel+  0  hel- " << scaler->GetBcm(1,"bcm_d1") << "   " << scaler->GetBcm("bcm_d1") << "   " << scaler->GetBcm(-1,"bcm_d1") << endl;
       cout << "Bcm d3   hel+  0  hel- " << scaler->GetBcm(1,"bcm_d3") << "   " << scaler->GetBcm("bcm_d3") << "   " << scaler->GetBcm(-1,"bcm_d3") << endl;
       cout << "Bcm d10   hel+  0  hel- " << scaler->GetBcm(1,"bcm_d10") << "   " << scaler->GetBcm("bcm_d10") << "   " << scaler->GetBcm(-1,"bcm_d10") << endl;
       cout << "Clock   hel+  0  hel- " << scaler->GetBcm(1,"clock") << "   " << scaler->GetBcm("clock") << "   " << scaler->GetBcm(-1,"clock") << endl;
       for (trig = 1; trig <= 5; trig++ ) {
          cout << "Trigger " << trig << "   hel+  0  hel- " << scaler->GetTrig(1,trig) << "   " << scaler->GetTrig(trig) << "   " << scaler->GetTrig(-1,trig) << endl;
       }
       cout << "Rates (Hz) = " << endl;
       cout << "Bcm u1   hel+  0  hel- " << scaler->GetBcmRate(1,"bcm_u1") << "   " << scaler->GetBcmRate("bcm_u1") << "   " << scaler->GetBcmRate(-1,"bcm_u1") << endl;
       cout << "Bcm u3   hel+  0  hel- " << scaler->GetBcmRate(1,"bcm_u3") << "   " << scaler->GetBcmRate("bcm_u3") << "   " << scaler->GetBcmRate(-1,"bcm_u3") << endl;
       cout << "Bcm u10   hel+  0  hel- " << scaler->GetBcmRate(1,"bcm_u10") << "   " << scaler->GetBcmRate("bcm_u10") << "   " << scaler->GetBcmRate(-1,"bcm_u10") << endl;
       cout << "Bcm d1   hel+  0  hel- " << scaler->GetBcmRate(1,"bcm_d1") << "   " << scaler->GetBcmRate("bcm_d1") << "   " << scaler->GetBcmRate(-1,"bcm_d1") << endl;
       cout << "Bcm d3   hel+  0  hel- " << scaler->GetBcmRate(1,"bcm_d3") << "   " << scaler->GetBcmRate("bcm_d3") << "   " << scaler->GetBcmRate(-1,"bcm_d3") << endl;
       cout << "Bcm d10   hel+  0  hel- " << scaler->GetBcmRate(1,"bcm_d10") << "   " << scaler->GetBcmRate("bcm_d10") << "   " << scaler->GetBcmRate(-1,"bcm_d10") << endl;
       cout << "Clock   hel+  0  hel- " << scaler->GetBcmRate(1,"clock") << "   " << scaler->GetBcmRate("clock") << "   " << scaler->GetBcmRate(-1,"clock") << endl;
       for (trig = 1; trig <= 5; trig++ ) {
          cout << "Trigger " << trig << "   hel+  0  hel- " << scaler->GetTrigRate(1,trig) << "   " << scaler->GetTrigRate(trig) << "   " << scaler->GetTrigRate(-1,trig) << endl;
       }
     }

     if (printout2) {
       cout << "\n\n--------------  SCALERS in EVDATA -----------------------\n"<<endl;
       for (int slot = 1; slot <= 3; slot++) {
         for (int chan = 0; chan < 32; chan++) {
	   cout << "slot "<<dec<<slot<<"   chan "<<chan<<"   data (dec) "<<evdata.GetScaler("evleft",slot,chan)<<"   hex "<<hex<<evdata.GetScaler("evleft",slot,chan)<<dec<<endl;
	 }
       }
     }
     

   }

   scaler->PrintSummary();

   return 0;
}







