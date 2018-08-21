//------------------------------------------------
// prfact  -- To print out the prescale factors found
//            in CODA file, then exit.
// 
#include <iostream>
#include <cstdlib>
#include "THaCodaFile.h"
#include "CodaDecoder.h"
#include "TString.h"
#include "evio.h"

using namespace std;
using namespace Decoder;

int main(int argc, char* argv[])
{

   if (argc < 2) {
      cout << "You made a mistake... \n" << endl;
      cout << "Usage:   prfact filename" << endl;
      cout << "  where 'filename' is the CODA file"<<endl;
      cout << "\n... exiting." << endl;
      exit(0);
   }
   TString filename = argv[1];
   THaCodaFile datafile;
   if (datafile.codaOpen(filename) != S_SUCCESS) {
        cout << "ERROR:  Cannot open CODA data" << endl;
        cout << "Perhaps you mistyped it" << endl;
        cout << "... exiting." << endl;
        exit(0);
   }
      
   THaEvData *evdata = new CodaDecoder();

   // Can tell evdata whether to use evtype 
   // 133 or 120 for prescale data.  Default is 120.
   evdata->SetOrigPS(133);   // args are 120 or 133
   cout << "Origin of PS data "<<evdata->GetOrigPS()<<endl;

// Loop over a finite number of events

   int NUMEVT=100000;
   for (int i=0; i<NUMEVT; i++) {

     int status = datafile.codaRead();  
	                                            
     if ( status != S_SUCCESS ) {
        if ( status == EOF) {
           cout << "This is end of file !" << endl;
           cout << "... exiting " << endl;
           exit(1);
        } else {
  	   cout << hex << "ERROR: codaRread status = " << status << endl;
           exit(0);
        }
     }

     evdata->LoadEvent( datafile.getEvBuffer() );   

     if(evdata->IsPrescaleEvent()) {
       cout <<"\n Prescale factors from CODA file = " << filename << endl;
       cout <<"\n Trigger      Prescale Factor"<< endl;
       for (int trig=1; trig<=8; trig++) {
	 cout <<"   "<<dec<<trig<<"             ";
         int ps = evdata->GetPrescaleFactor(trig);
         int psmax;
	 if (trig <= 4) psmax = 16777216;  // 2^24
         if (trig >= 5) psmax = 65536;     // 2^16
         ps = ps % psmax;
         if (ps == 0) ps = psmax;
         cout << ps << endl;
       }
       cout << "\nReminder: A 'zero' was interpreted as maximum."<<endl;
       cout << "Max for trig 1-4 = 2^24, for trig 5-8 = 2^16 \n"<<endl;
       exit(0);
     }

   }  //  end of event loop

   cout << "ERROR: prescale factors not found in the first "; 
   cout << dec << NUMEVT << " events " << endl;

}







