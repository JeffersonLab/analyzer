// Test of Skeleton Module class
// which is a test class for modules.
//
// R. Michaels, Feb, 2015


#define MYTYPE 5

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstdio>
#include "Decoder.h"
#include "THaCodaFile.h"
#include "CodaDecoder.h"
#include "THaEvData.h"
#include "Module.h"
#include "SkeletonModule.h"
#include "evio.h"
#include "THaSlotData.h"
#include "TString.h"
#include "TROOT.h"
#include "TFile.h"
#include "TH1.h"
#include "TH2.h"
#include "TProfile.h"
#include "TNtuple.h"

using namespace std;
using namespace Decoder;

TH1F *h1, *h2;

void dump(UInt_t *buffer, ofstream *file);
void process(Int_t i, THaEvData *evdata, ofstream *file);

int main(int argc, char* argv[])
{

  TString filename("snippet.dat");

  ofstream *debugfile = new ofstream;;
  debugfile->open ("oodecoder2.txt");
  *debugfile << "Debug of OO decoder\n\n";

  THaCodaFile datafile(filename);
  THaEvData *evdata = new CodaDecoder();

  //   evdata->SetDebug(1);
  //   evdata->SetDebugFile(debugfile);

  // Initialize root and output
  TROOT fadcana("tstskelroot","Hall A test analysis");
  TFile hfile("tstskel.root","RECREATE","skeleton module data");

  h1 = new TH1F("h1","num raw data",100,-1,100.);
  h2 = new TH1F("h2","raw data",201,-1,200.);

  // Loop over events
  int NUMEVT=5000;
  Int_t jnum=1;
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

      if (evdata->GetEvType() == MYTYPE) {
	process (jnum, evdata, debugfile);
	jnum++;
      }

    }
  }

  hfile.Write();
  hfile.Close();


}


void dump( UInt_t* data, ofstream *debugfile)
{
  // Crude event dump
  UInt_t evnum = data[4];
  UInt_t len = data[0] + 1;
  UInt_t evtype = data[1]>>16;
  *debugfile << "\n\n Event number " << dec << evnum << endl;
  *debugfile << " length " << len << " type " << evtype << endl;
  UInt_t ipt = 0;
  for (UInt_t j=0; j<(len/5); j++) {
    *debugfile << dec << "\n evbuffer[" << ipt << "] = ";
    for (UInt_t k=j; k<j+5; k++) {
      *debugfile << hex << data[ipt++] << " ";
    }
    *debugfile << endl;
  }
  if (ipt < len) {
    *debugfile << dec << "\n evbuffer[" << ipt << "] = ";
    for (UInt_t k=ipt; k<len; k++) {
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

  Module *fskel;

  fskel = evdata->GetModule(5,16);
  *debugfile << "main:  fskel ptr = "<<fskel<<endl;

  if (fskel) {
    *debugfile << "fskel: num events "<<fskel->GetNumChan()<<endl;
    h1->Fill(fskel->GetNumChan());
    for (Int_t i=0; i < fskel->GetNumChan(); i++) {
      *debugfile << "main:  fskel data on ch.   "<<dec<<i<<"   "<<fskel->GetData(i)<<"   what "<< -1.0e-6*fskel->GetData(i) << endl;
      //             h2->Fill(-1.0e-6*fskel->GetData(i));
      h2->Fill(fskel->GetData(i));
    }
  }


}
