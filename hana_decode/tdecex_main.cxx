//-----------------------------------------------------------
// Tests of Hall A Event Decoder and an example of usage
// with a "detector class" THaGenDetTest.
// To test for speed, compile THaGenDetTest.h with PRINTOUT=0.
// To get lots of print output, put PRINTOUT=1. (say, 50 events)
// 
// R. Michaels,  rom@jlab.org,  Mar, 2000

#include <iostream>
#include <cstdlib>
#include "THaCodaFile.h"
#include "CodaDecoder.h"
#include "THaSlotData.h"
#include "THaGenDetTest.h"
#include "TString.h"
#include "evio.h"

using namespace std;
using namespace Decoder;

int main(int argc, char* argv[])
{

   if (argc < 2) {
      cout << "You made a mistake... \n" << endl;
      cout << "Usage:   tdecex filename" << endl;
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
   THaGenDetTest mydetector;
   mydetector.init();

// Loop over a finite number of events

   int NUMEVT=50000;   
   int ievent;
   for (ievent=0; ievent<NUMEVT; ievent++) {
     if ( ievent > 0 && (
        ( (ievent <= 1000) && ((ievent%100) == 0) ) ||
        ( (ievent > 1000) && ((ievent%1000) == 0) ) ) )
              cout << "\n ---- Event " << ievent <<endl;
     int status = datafile.codaRead();  
     if ( status != S_SUCCESS ) {
        if ( status == EOF) {
           cout << "This is end of file !" << endl;
           cout << "... exiting " << endl;
           goto Finish;
        } else {
  	   cout << hex << "ERROR: codaRread status = " << status << endl;
           exit(0);
        }
     }
     evdata->LoadEvent( datafile.getEvBuffer() );   
     mydetector.process_event(evdata);

   }  //  end of event loop

Finish:
   cout << "\n Finished processing " << ievent << " events " << endl;
}








