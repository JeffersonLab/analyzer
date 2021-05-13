// Test of a OO decoder
//
// R. Michaels, Sept 2014

#include <iostream>
#include <fstream>
#include <cstdlib>
#include "THaCodaFile.h"
#include "CodaDecoder.h"
#include "TString.h"

using namespace std;
using namespace Decoder;

void dump(UInt_t *data, ofstream *file);
void process(THaEvData *evdata, ofstream *file);

int main(int /* argc */, char** /* argv */)
{
  TString filename("snippet.dat");

  auto* debugfile = new ofstream;
  debugfile->open ("oodecoder1.txt");
  *debugfile << "Debug of OO decoder"<<endl<<endl;

  THaCodaFile datafile;
  if (datafile.codaOpen(filename) != CODA_OK) {
    cerr << "ERROR:  Cannot open CODA data" << endl;
    cerr << "Perhaps you mistyped it" << endl;
    cerr << "... exiting." << endl;
    exit(2);
  }
  auto* evdata = new CodaDecoder;
  evdata->SetCodaVersion(datafile.getCodaVersion());
  evdata->SetDebug(1);
  evdata->SetDebugFile(debugfile);

  // Loop over events
  int NUMEVT=10;
  for (int iev=0; iev<NUMEVT; iev++) {
    int status = datafile.codaRead();
    if (status != CODA_OK) {
      if ( status == EOF) {
        *debugfile << "Normal end of file.  Bye bye." << endl;
        break;
      } else {
        *debugfile << hex << "ERROR: codaRread status = " << status << endl;
        exit(status);
      }
    } else {

      UInt_t *data = datafile.getEvBuffer();
      dump(data, debugfile);

      *debugfile << "\nAbout to Load Event "<<endl;
      status = evdata->LoadEvent( data );
      *debugfile << "\nFinished with Load Event, status = "<<status<<endl;
      if( status != CodaDecoder::HED_OK && status != CodaDecoder::HED_WARN ) {
        cerr << "ERROR " << status << " while decoding event " << iev
            << ". Exiting." << endl;
        exit(status);
      }
      process (evdata, debugfile);

    }
  }
  datafile.codaClose();
  return 0;
}


void dump( UInt_t* data, ofstream *debugfile)
{
  // Crude event dump
  unsigned evnum = data[4];
  unsigned len = data[0] + 1;
  unsigned evtype = data[1]>>16;
  *debugfile << "\n\n Event number " << dec << evnum << endl;
  *debugfile << " length " << len << " type " << evtype << endl;
  unsigned ipt = 0;
  for (unsigned j=0; j<(len/5); j++) {
    *debugfile << dec << "\n evbuffer[" << ipt << "] = ";
    for (unsigned k=j; k<j+5; k++) {
      *debugfile << hex << data[ipt++] << " ";
    }
    *debugfile << endl;
  }
  if (ipt < len) {
    *debugfile << dec << "\n evbuffer[" << ipt << "] = ";
    for (unsigned k=ipt; k<len; k++) {
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

      unsigned crate = 1;    // for example
      unsigned slot = 25;

//  Here are raw 32-bit CODA words for this crate and slot
      *debugfile << "Raw Data Dump for crate "<<dec<<crate<<" slot "<<slot<<endl;
      *debugfile << "Num raw "<<evdata->GetNumRaw(crate,slot)<<endl;
      for(unsigned hit=0; hit<evdata->GetNumRaw(crate,slot); hit++) {
	*debugfile<<dec<<"raw["<<hit<<"] =   ";
	*debugfile<<hex<<evdata->GetRawData(crate,slot,hit)<<endl;
      }
// You can alternatively let evdata print out the contents of a crate and slot:
      *debugfile << "To print slotdata "<<crate<<"  "<<slot<<endl;
      //      evdata->PrintSlotData(crate,slot);
      *debugfile << "finished with print slot data"<<endl;
}
