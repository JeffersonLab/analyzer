// Test of FADC decoding in multiblock mode
//
// R. Michaels, October 2016
//

// MYTYPE = 0 for TEDF or Bryan's,  1 for HCAL or SBS
#define MYTYPE 0

// (9,10) for TEDF,  (5,5) for Bryan's fcat files,  (10,3) for HCAL , (12,17) for SBS
#define MYCRATE 5
#define MYSLOT 5

// MYCHAN = 11 for TEDF, 13 for Bryan's fcat files,  0 for HCAL
#define MYCHAN 13
#define DEBUG 1

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstdio>
#include "THaCodaFile.h"
#include "CodaDecoder.h"
#include "Fadc250Module.h"
#include "TString.h"
#include "TROOT.h"
#include "TFile.h"
#include "TH1.h"
#include "TH2.h"

using namespace std;
using namespace Decoder;

vector <TH1F * > hsnaps;
TH1F *hinteg, *hinteg2;

Int_t use_module = 0;
Int_t nsnaps = 5;

void dump(UInt_t *buffer, ofstream *file);
void process(Int_t i, THaEvData *evdata, ofstream *file);

int main(int /* argc */, char** /* argv */)
{
  TString filename("snippet.dat");  // data file, can be a link

  ofstream *debugfile = 0;
  UInt_t *data;

#ifdef DEBUG
  debugfile = new ofstream;
  debugfile->open ("oodecoder1.txt");
  *debugfile << "Debug of OO decoder"<<endl<<endl;
#endif

  THaCodaFile datafile;
  if (datafile.codaOpen(filename) != CODA_OK) {
    cerr << "ERROR:  Cannot open CODA data" << endl;
    cerr << "Perhaps you mistyped it" << endl;
    cerr << "... exiting." << endl;
    exit(2);
  }
  CodaDecoder *evdata = new CodaDecoder();

  evdata->SetDebug(1);
  evdata->SetDebugFile(debugfile);

  // Initialize root and output
  TROOT fadcana("fadcroot","Hall A FADC analysis, 1st version");
  TFile hfile("fadc.root","RECREATE","FADC data");

  char cnum[50],ctitle[80];
  for (Int_t i = 0; i<nsnaps; i++) {
    sprintf(cnum,"h%d",i+1);
    sprintf(ctitle,"snapshot %d",i+1);
    hsnaps.push_back(new TH1F(cnum,ctitle,1020,-5,505));
  }
  hinteg = new TH1F("hinteg","Integral of ADC",1000,50000,120000);
  hinteg2 = new TH1F("hinteg2","Integral of ADC",1000,0,10000);

  // Loop over events
  int NUMEVT=50;
  int trigcnt=0;

  for (int iev=0; iev<NUMEVT; iev++) {

    if (debugfile) {
      if (evdata->IsMultiBlockMode()) {
        *debugfile << "Are in Multiblock mode "<<endl;
        if (evdata->BlockIsDone()) {
          *debugfile << "Block is done "<<endl;
        } else {
          *debugfile << "Block is NOT done "<<endl;
        }
      } else {
        *debugfile << "Not in Multiblock mode "<<endl;
      }
    }


    Int_t to_read_file = 0;
    if ( !evdata->IsMultiBlockMode() ||
        (evdata->IsMultiBlockMode() && evdata->BlockIsDone()) ) to_read_file=1;

    if (to_read_file) {

      cout << "CODA read --- "<<endl;

      if (debugfile) *debugfile << "Read from file ?  Yes "<<endl;

      int status = datafile.codaRead();

      if (status != CODA_OK) {
        if ( status == EOF) {
          cout << "Normal end of file.  Bye bye." << endl;
          break;
        } else {
          cout << "ERROR: codaRread status = " << status << endl;
          exit(status);
        }

      } else {

        data = datafile.getEvBuffer();
        dump(data, debugfile);

        cout << "LoadEvent --- "<<endl;

        status = evdata->LoadEvent( data );

        if( status != CodaDecoder::HED_OK && status != CodaDecoder::HED_WARN ) {
          cerr << "ERROR: LoadEvent status = " << status
              << " while decoding event " << iev << ". Exiting." << endl;
          exit(status);
        }
      }

    } else {

      if (debugfile) *debugfile << "Read from file ?  No "<<endl;

      int status = evdata->LoadFromMultiBlock();

      if( status != CodaDecoder::HED_OK && status != CodaDecoder::HED_WARN ) {
        cerr << "ERROR: LoadFromMultiBlock status = " << status
            << " while decoding event " << iev << ". Exiting." << endl;
        exit(status);
      }

    }

    if (evdata->GetEvType() == MYTYPE) process (trigcnt++, evdata, debugfile);

  }

  hfile.Write();
  hfile.Close();
  datafile.codaClose();
}


void dump( UInt_t* data, ofstream *debugfile) {
  // Crude event dump
  if (!debugfile) return;
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

void process (Int_t trignum, THaEvData *evdata, ofstream *debugfile) {

  Int_t rdata;

  if (debugfile) {
    *debugfile << "\n\nHello.  Now we process evdata : "<<endl;

    *debugfile << "\nEvent type   " << dec << evdata->GetEvType() << endl;
    *debugfile << "Event number " << evdata->GetEvNum()  << endl;
    *debugfile << "Event length " << evdata->GetEvLength() << endl;
  }
  if (evdata->GetEvType() != MYTYPE) return;
  if (evdata->IsPhysicsTrigger() ) { // triggers 1-14
    if (debugfile) *debugfile << "Physics trigger " << endl;
  }

  if (use_module) {  // Using data directly from the module from THaEvData

    Module *fadc;

    fadc = evdata->GetModule(MYCRATE,MYSLOT);
    if (debugfile) *debugfile << "main:  using module, fadc ptr = "<<fadc<<endl;

    if (fadc) {
      if (debugfile) *debugfile << "main: num events "<<fadc->GetNumEvents(MYCHAN)<<endl;
      if (debugfile) *debugfile << "main: fadc mode "<<fadc->GetMode()<<endl;
      if (fadc->GetMode()==1 || fadc->GetMode()==8) {
        for (Int_t i = 0; i < fadc->GetNumEvents(kSampleADC,MYCHAN); i++) {
          rdata = fadc->GetData(kSampleADC,MYCHAN, i);
          if (debugfile) *debugfile << "main:  SAMPLE fadc data on ch.   "<<dec<<MYCHAN<<"  "<<i<<"  "<<rdata<<endl;
          if (trignum < nsnaps) hsnaps[trignum]->Fill(i,rdata);
        }
      }
      if (fadc->GetMode()==7) {
        for (Int_t i = 0; i < fadc->GetNumEvents(kPulseIntegral,MYCHAN); i++) {
          rdata = fadc->GetData(kPulseIntegral,MYCHAN,i);
          if (debugfile) *debugfile << "main:  INTEG fadc data on ch.   "<<dec<<MYCHAN<<"  "<<i<<"  "<<rdata<<endl;
          hinteg->Fill(rdata);
          hinteg2->Fill(rdata);
        }
      }
    }


  } else {  // alternative is to use the THaEvData::GetData interface

    if (debugfile) *debugfile << "main:  using THaEvData  "<<endl;
    if (debugfile) {
      *debugfile << "main:  num hits "<< evdata->GetNumEvents(kSampleADC,MYCRATE,MYSLOT,MYCHAN)<<"   "<<evdata->GetNumEvents(kPulseIntegral,MYCRATE,MYSLOT,MYCHAN)<<endl;
      for (Int_t jj=0; jj<10; jj++) {
        for (Int_t kk=0; kk<15; kk++) {
          *debugfile << "burger "<<jj<<"  "<<kk<<"  "<< evdata->GetNumEvents(kSampleADC,MYCRATE,jj,kk)<<"   "<<evdata->GetNumEvents(kPulseIntegral,MYCRATE,jj,kk)<<endl;
        }
      }
    }


    for (Int_t i=0; i < evdata->GetNumEvents(kSampleADC,MYCRATE,MYSLOT,MYCHAN); i++) {
      rdata = evdata->GetData(kSampleADC,MYCRATE,MYSLOT,MYCHAN,i);
      if (debugfile) *debugfile << "main:  SAMPLE fadc data on ch.   "<<dec<<MYCHAN<<"  "<<i<<"  "<<rdata<<endl;
      if (trignum < nsnaps) hsnaps[trignum]->Fill(i,rdata);
    }
    for (Int_t i=0; i < evdata->GetNumEvents(kPulseIntegral,MYCRATE,MYSLOT,MYCHAN); i++) {
      rdata = evdata->GetData(kPulseIntegral,MYCRATE,MYSLOT,MYCHAN,i);
      if (debugfile) *debugfile << "main:  INTEG fadc data on ch.   "<<dec<<MYCHAN<<"  "<<i<<"  "<<rdata<<endl;
      hinteg->Fill(rdata);
      hinteg2->Fill(rdata);
    }
  }

}
