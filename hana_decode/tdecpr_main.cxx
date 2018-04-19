//------------------------------------------------
// tdecpr  (Test of DECoding, PRintout)
// 
// Primitive example of decoding Hall A Data
// R. Michaels,  Feb, 2000

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

   int debug=0;
   if (argc > 1) debug=1;

// CODA file "snippet.dat" is a disk file of CODA data.  
   THaCodaFile datafile;   //     We could also open the data using a 
                        //     different constructor:                   
                        //     THaCodaFile datafile("snippet.dat");

   TString filename("snippet.dat");
   if (datafile.codaOpen(filename) != S_SUCCESS) {
        cout << "ERROR:  Cannot open CODA data" << endl;
        exit(0);
   }
      
   THaEvData *evdata = new THaCodaDecoder();

// Loop over events
 
   int NUMEVT=100;
   int ievent;
   for (ievent=0; ievent<NUMEVT; ievent++) {

     int status = datafile.codaRead();  
	                                            
     if ( status != S_SUCCESS ) {
        if ( status == EOF) {
           cout << "This is normal end of file.  Goodbye !" << endl;
        } else {
  	   cout << hex << "ERROR: codaRread status = " << status << endl;
        }
        goto Finish;
     }

// load_evbuffer() must be called each event before you access evdata contents.
// If you use the version of load_evbuffer() shown here, 
// evdata uses its private crate map (recommended).
// Alternatively you could use load_evbuffer(int* evbuffer, haCrateMap& map)
     
     evdata->LoadEvent( datafile.getEvBuffer() );   

     cout << "\nEvent type   " << dec << evdata->GetEvType() << endl;
     cout << "Event number " << evdata->GetEvNum()  << endl;
     cout << "Event length " << evdata->GetEvLength() << endl;
     if (evdata->IsPhysicsTrigger() ) { // triggers 1-14
        cout << "Physics trigger " << endl;
     }
     if(evdata->IsScalerEvent()) cout << "Scaler `event' " << endl;

// Now we want data from a particular crate and slot.
// E.g. crates are 1,2,3,13,14,15 (roc numbers), Slots are 1,2,3... 
// This is like what one might do in a detector decode() routine.

      int crate = 1;    // for example
      int slot = 24;

//  Here are raw 32-bit CODA words for this crate and slot
      cout << "Raw Data Dump for crate "<<dec<<crate<<" slot "<<slot<<endl; 
      int hit;
      for(hit=0; hit<evdata->GetNumRaw(crate,slot); hit++) {
        cout<<dec<<"raw["<<hit<<"] =   ";
        cout<<hex<<evdata->GetRawData(crate,slot,hit)<<endl;  
      }
// You can alternatively let evdata print out the contents of a crate and slot:
      evdata->PrintSlotData(crate,slot);

      if (evdata->IsPhysicsTrigger()) {
// Below are interpreted data, device types are ADC, TDC, or scaler.
// One needs to know the channel number within the device
        int channel = 7;    // for example
        cout << "Device type = ";
        cout << evdata->DevType(crate,slot) << endl;
        for (hit=0; hit<evdata->GetNumHits(crate,slot,channel); hit++) {
	   cout << "Channel " <<dec<<channel<<" hit # "<<hit<<"  ";
           cout << "data = " << evdata->GetData(crate,slot,channel,hit)<<endl;
        }
// Helicity data
        cout << "Helicity on left spectrometer "<<evdata->GetHelicity("left")<<endl;
        cout << "Helicity on right spectrometer "<<evdata->GetHelicity("right")<<endl;
        cout << "Helicity "<<evdata->GetHelicity()<<endl;
      }

// Scalers:  Although the getData methods works if you happen
// to know what crate & slot contain scaler data, here is
// another way to get scalers directly from evdata

      for (slot=0; slot<5; slot++) {
        cout << "\n scaler slot -> " << dec << slot << endl;; 
        for (int chan=0; chan<16; chan++) {
	  cout << "Scaler chan " <<  chan << "  ";
          cout << evdata->GetScaler("left",slot,chan);
          cout << "  "  << evdata->GetScaler(7,slot,chan) << endl;
	}
      }
 

   }  //  end of event loop

Finish:
   cout<<"\nAll done; processed "<<dec<<ievent<<" events"<<endl;
}







