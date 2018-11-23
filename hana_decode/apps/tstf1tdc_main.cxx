// Test of F1 TDC decoding
//
// R. Michaels, Jan 2015


#define MYTYPE 0

#include <iostream>
#include <fstream>
#include <cstdlib>
#include "THaCodaFile.h"
#include "CodaDecoder.h"
#include "F1TDCModule.h"
#include "TString.h"
#include "TROOT.h"
#include "TFile.h"
#include "TH1.h"
#include "TH2.h"

using namespace std;
using namespace Decoder;

TH1F *h1,*h2,*h3,*h4,*h5;
TH1F *hinteg;

void dump(UInt_t *buffer, ofstream *file);
void process(Int_t i, THaEvData *evdata, ofstream *file);

int main(int /* argc */, char** /* argv */)
{

   TString filename("snippet.dat");

   ofstream *debugfile = new ofstream;;
   debugfile->open ("oodecoder2.txt");
   *debugfile << "Debug of OO decoder\n\n";

   THaCodaFile datafile;
   if (datafile.codaOpen(filename) != CODA_OK) {
     cerr << "ERROR:  Cannot open CODA data" << endl;
     cerr << "Perhaps you mistyped it" << endl;
     cerr << "... exiting." << endl;
     exit(2);
   }
   CodaDecoder *evdata = new CodaDecoder();
   evdata->SetCodaVersion(datafile.getCodaVersion());

   evdata->SetDebug(1);
   evdata->SetDebugFile(debugfile);

// Initialize root and output
  TROOT fadcana("fadcroot","Hall A FADC analysis, 1st version");
  TFile hfile("fadc.root","RECREATE","FADC data");

  h1 = new TH1F("h1","snapshot 1",1020,-5,505);
  h2 = new TH1F("h2","snapshot 2",1020,-5,505);
  h3 = new TH1F("h3","snapshot 3",1020,-5,505);
  h4 = new TH1F("h4","snapshot 4",1020,-5,505);
  h5 = new TH1F("h5","snapshot 5",1020,-5,505);
  hinteg = new TH1F("hinteg","Integral of ADC",1000,50000,120000);

  // Loop over events
  int NUMEVT=22;
  Int_t jnum=1;
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

      UInt_t* data = datafile.getEvBuffer();
      dump(data, debugfile);

      *debugfile << "\nAbout to Load Event "<<endl;
      evdata->LoadEvent( data );
      *debugfile << "\nFinished with Load Event "<<endl;

      if (evdata->GetEvType() == MYTYPE) {
	process (jnum, evdata, debugfile);
	jnum++;
      }

    }
  }

  hfile.Write();
  hfile.Close();
  datafile.codaClose();
}


void dump( UInt_t* data, ofstream *debugfile) {
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

void process (Int_t iev, THaEvData *evdata, ofstream *debugfile) {

  *debugfile << "\n\nHello.  Now we process evdata : "<<endl;

  *debugfile << "\nEvent type   " << dec << evdata->GetEvType() << endl;
  *debugfile << "Event number " << evdata->GetEvNum()  << endl;
  *debugfile << "Event length " << evdata->GetEvLength() << endl;
  if (evdata->GetEvType() != MYTYPE) return;
  if (evdata->IsPhysicsTrigger() ) { // triggers 1-14
    *debugfile << "Physics trigger " << endl;
  }

  Module *fadc;

  fadc = evdata->GetModule(9,5);
  *debugfile << "main:  fadc ptr = "<<fadc<<endl;

  if (fadc) {
    *debugfile << "main: num events "<<fadc->GetNumEvents()<<endl;
    *debugfile << "main: fadc mode "<<fadc->GetMode()<<endl;
    for (Int_t i=0; i < 500; i++) {
      *debugfile << "main:  fadc data on ch. 11   "<<dec<<i<<"  "<<fadc->GetData(1, 11,i)<<endl;
      if (fadc->GetMode()==1) {
        if (iev==5) h1->Fill(i,fadc->GetData(1,11,i));
        if (iev==6) h2->Fill(i,fadc->GetData(1,11,i));
        if (iev==7) h3->Fill(i,fadc->GetData(1,11,i));
        if (iev==8) h4->Fill(i,fadc->GetData(1,11,i));
        if (iev==9) h5->Fill(i,fadc->GetData(1,11,i));
      }
      if (fadc->GetMode()==2) {
        hinteg->Fill(fadc->GetData(1,11,i),1.);
      }
    }
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
