//--------------------------------------------------------
//  tscalfile_main.C
//
//  Test of scaler class, for reading a CODA file
// 
//  R. Michaels, April 2001
//--------------------------------------------------------

#include <iostream>
#include <string>
#include "THaScaler.h"
#include "THaCodaFile.h"

using namespace std;

int main(int argc, char* argv[]) {

   int printout = 1;   // To printout lots of stuff (1) or not (0)
   int trig;
   TString filename = "run.dat";

   if (argc < 2) {
     cout << "Usage:  "<<argv[0]<<" bank"<<endl;
     cout << "where bank = 'Right', 'Left', or 'RCS'"<<endl;
     return 1;
   }
   char bank[100];
   strcpy(bank,argv[1]);   
   cout << "Bank = "<<bank<<endl<<flush;

   THaScaler *scaler;
   scaler = new THaScaler(bank);

   if (scaler->Init("15-10-2004") == -1) {  // Init MUST be done once
      cout << "Error initializing scaler "<<endl;
      return 1;
   }

   int status = 1;
   
   while (status) {
     status = scaler->LoadDataCodaFile(filename);   // load data for 'filename'
     if (!status) break;
     if (printout) {
       cout << "\n\n--------------  SCALER EVENT DATA -----------------------\n"<<endl;
       scaler->Print();     // raw diagnostic printout
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
       cout << "S1 counts (hex) "<<hex<<scaler->GetScaler("s1",0)<<"  "<<scaler->GetScaler("s1",1)<<"  "<<scaler->GetScaler("s1",2)<<"  "<<scaler->GetScaler("s1",3)<<"  "<<scaler->GetScaler("s1",4)<<"  "<<scaler->GetScaler("s1",5)<<endl;
       cout << "S2 counts (hex) "<<scaler->GetScaler("s2",0)<<"  "<<scaler->GetScaler("s2",1)<<"  "<<scaler->GetScaler("s2",2)<<"  "<<scaler->GetScaler("s2",3)<<"  "<<scaler->GetScaler("s2",4)<<"  "<<scaler->GetScaler("s2",5)<<endl;
       cout << "gasC. counts (hex) "<<scaler->GetScaler("gasC",0)<<"  "<<scaler->GetScaler("gasC",1)<<"  "<<scaler->GetScaler("gasC",2)<<"  "<<scaler->GetScaler("gasC",3)<<"  "<<scaler->GetScaler("gasC",4)<<"  "<<scaler->GetScaler("gasC",5)<<"  "<<scaler->GetScaler("gasC",6)<<"  "<<scaler->GetScaler("gasC",7)<<"  "<<scaler->GetScaler("gasC",8)<<"  "<<scaler->GetScaler("gasC",9)<<dec<<endl;
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

   }

   return 0;
}







