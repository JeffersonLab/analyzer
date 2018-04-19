//------------------------------------------------
// epics_main.C   -- to find EPICS data and 
//                   do something with them.
// 
#include <iostream>
#include <cstdlib>
#include "THaCodaFile.h"
#include "THaCodaDecoder.h"
#include "TString.h"
#include "evio.h"

using namespace std;
using namespace Decoder;

int main(int argc, char* argv[])
{

   if (argc < 2) {
      cout << "You made a mistake... \n" << endl;
      cout << "Usage:   epicsd filename" << endl;
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
      
   THaEvData *evdata = new THaCodaDecoder();

// Loop over a finite number of events

   int NUMEVT=1000000;
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

     if(evdata->IsEpicsEvent()) {

       cout << "Some epics data --> "<<endl;
       cout << "hac_bcm_average "<<
	 evdata->GetEpicsData("hac_bcm_average")<<endl;
       cout << "IPM1H04A.XPOS  "<<
         evdata->GetEpicsData("IPM1H04A.XPOS")<<endl;
       cout << "IPM1H04A.YPOS  "<<
         evdata->GetEpicsData("IPM1H04A.YPOS")<<endl;

     }

   }  //  end of event loop


}







