// Test of a OO decoder
//
// R. Michaels, Sept 2014

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstdio>
#include "Decoder.h"
#include "THaCodaFile.h"
#include "CodaDecoder.h"
#include "THaEvData.h"
#include "evio.h"
#include "THaSlotData.h"
#include "TString.h"

using namespace std;
using namespace Decoder;

void dump(UInt_t *buffer, ofstream *file);
void process(THaEvData *evdata, ofstream *file);

int main(int argc, char* argv[])
{

   TString filename("snippet.dat");

   ofstream *debugfile = new ofstream;;
   debugfile->open ("oodecoder1.txt");
   *debugfile << "Debug of OO decoder\n\n";

   THaCodaFile datafile(filename);
   THaEvData *evdata = new CodaDecoder();
   evdata->SetDebug(1);
   evdata->SetDebugFile(debugfile);

   // Loop over events
   int NUMEVT=10;
   for (int iev=0; iev<NUMEVT; iev++) {
     int status = datafile.codaRead();
     if (status != S_SUCCESS) {
       if ( status == EOF) {
	 *debugfile << "Normal end of file.  Bye bye." << endl;
       } else {
	 *debugfile << hex << "ERROR: codaRread status = " << status << endl;
       }
       exit(1);
     } else {

       UInt_t *data = datafile.getEvBuffer();
       dump(data, debugfile);

       *debugfile << "\nAbout to Load Event "<<endl;
       evdata->LoadEvent( data );
       *debugfile << "\nFinished with Load Event "<<endl;

       process (evdata, debugfile);

     }
   }


}


void dump( UInt_t* data, ofstream *debugfile)
{
  // Crude event dump
  int evnum = data[4];
  int len = data[0] + 1;
  int evtype = data[1]>>16;
  *debugfile << "\n\n Event number " << dec << evnum << endl;
  *debugfile << " length " << len << " type " << evtype << endl;
  int ipt = 0;
  for (int j=0; j<(len/5); j++) {
    *debugfile << dec << "\n evbuffer[" << ipt << "] = ";
    for (int k=j; k<j+5; k++) {
      *debugfile << hex << data[ipt++] << " ";
    }
    *debugfile << endl;
  }
  if (ipt < len) {
    *debugfile << dec << "\n evbuffer[" << ipt << "] = ";
    for (int k=ipt; k<len; k++) {
      *debugfile << hex << data[ipt++] << " ";
    }
    *debugfile << endl;
  }
}

void process (THaEvData *evdata, ofstream *debugfile) {

     *debugfile << "\n\nHello.  Now we process evdata : "<<endl;

     *debugfile << "\nEvent type   " << dec << evdata->GetEvType() << endl;
     *debugfile << "Event number " << evdata->GetEvNum()  << endl;
     *debugfile << "Event length " << evdata->GetEvLength() << endl;
     if (evdata->IsPhysicsTrigger() ) { // triggers 1-14
	*debugfile << "Physics trigger " << endl;
     }

// Now we want data from a particular crate and slot.
// E.g. crates are 1,2,3,13,14,15 (roc numbers), Slots are 1,2,3...
// This is like what one might do in a detector decode() routine.

      int crate = 1;    // for example
      int slot = 25;

//  Here are raw 32-bit CODA words for this crate and slot
      *debugfile << "Raw Data Dump for crate "<<dec<<crate<<" slot "<<slot<<endl;
      int hit;
      *debugfile << "Num raw "<<evdata->GetNumRaw(crate,slot)<<endl;
      for(hit=0; hit<evdata->GetNumRaw(crate,slot); hit++) {
	*debugfile<<dec<<"raw["<<hit<<"] =   ";
	*debugfile<<hex<<evdata->GetRawData(crate,slot,hit)<<endl;
      }
// You can alternatively let evdata print out the contents of a crate and slot:
      *debugfile << "To print slotdata "<<crate<<"  "<<slot<<endl;
      //      evdata->PrintSlotData(crate,slot);
      *debugfile << "finished with print slot data"<<endl;
}
