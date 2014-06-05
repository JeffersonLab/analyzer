// Test of a TOY decoder model for the OO decoder upgrade
// Purpose: express the design changes with a simple code
//
// Main changes :
//    1. Event handlers corresponding to each event type.
//    2. Modules take care of decoding.
//
// R. Michaels, Dec 5, 2013

#include <iostream>
#include <cstdlib>
#include <cstdio>
#include "THaCodaFile.h"
#include "ToyCodaDecoder.h"
#include "THaEvData.h"
#include "evio.h"
#include "THaSlotData.h"
#include "TString.h"

using namespace std;

void dump(int *buffer);
void process(THaEvData *evdata);

int main(int argc, char* argv[])
{

   TString filename("snippet.dat");

   THaCodaFile datafile(filename);
   THaEvData *evdata = new ToyCodaDecoder();
   evdata->SetDebug(1);

    // Loop over events
      int NUMEVT=100;
      for (int iev=0; iev<NUMEVT; iev++) {
   	 int status = datafile.codaRead();  
         if (status != S_SUCCESS) {
	   if ( status == EOF) {
             cout << "Normal end of file.  Bye bye." << endl;
	   } else {
  	     cout << hex << "ERROR: codaRread status = " << status << endl;
 	   }
           exit(1);
	 } else {

            int *data = datafile.getEvBuffer();
	    //	    dump(data);

            cout << "Hi, about to Load Event "<<endl;
            evdata->LoadEvent( data );   
            cout << "Hi, Finished with Load Event "<<endl;

            process (evdata);


	 }
      }

 
}
     

void dump( int* data) {
    // Crude event dump
            int evnum = data[4];
            int len = data[0] + 1;
            int evtype = data[1]>>16;
            cout << "\n\n Event number " << dec << evnum << endl;
            cout << " length " << len << " type " << evtype << endl;
            int ipt = 0;
	    for (int j=0; j<(len/5); j++) {
	      cout << dec << "\n evbuffer[" << ipt << "] = ";
              for (int k=j; k<j+5; k++) {
		cout << hex << data[ipt++] << " ";
	      }
              cout << endl;
	    }
            if (ipt < len) {
	      cout << dec << "\n evbuffer[" << ipt << "] = ";
              for (int k=ipt; k<len; k++) {
		cout << hex << data[ipt++] << " ";
              }
              cout << endl;
            }
}

void process (THaEvData *evdata) {

     cout << "\n\nHello.  Now we process evdata : "<<endl;

     cout << "\nEvent type   " << dec << evdata->GetEvType() << endl;
     cout << "Event number " << evdata->GetEvNum()  << endl;
     cout << "Event length " << evdata->GetEvLength() << endl;
     if (evdata->IsPhysicsTrigger() ) { // triggers 1-14
        cout << "Physics trigger " << endl;
     }

// Now we want data from a particular crate and slot.
// E.g. crates are 1,2,3,13,14,15 (roc numbers), Slots are 1,2,3... 
// This is like what one might do in a detector decode() routine.

      int crate = 1;    // for example
      int slot = 24;

//  Here are raw 32-bit CODA words for this crate and slot
      cout << "Raw Data Dump for crate "<<dec<<crate<<" slot "<<slot<<endl; 
      int hit;
      cout << "Num raw "<<evdata->GetNumRaw(crate,slot)<<endl;
      for(hit=0; hit<evdata->GetNumRaw(crate,slot); hit++) {
        cout<<dec<<"raw["<<hit<<"] =   ";
        cout<<hex<<evdata->GetRawData(crate,slot,hit)<<endl;  
      }
// You can alternatively let evdata print out the contents of a crate and slot:
      cout << "To print slotdata "<<crate<<"  "<<slot<<endl;
      evdata->PrintSlotData(crate,slot);
      cout << "finished with print slot data"<<endl;
}




