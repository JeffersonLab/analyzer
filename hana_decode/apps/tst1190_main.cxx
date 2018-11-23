#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstdio>
#include <numeric>
#include <vector>
#include <time.h>
#include "THaCodaFile.h"
#include "CodaDecoder.h"
#include "Caen1190Module.h"
#include "TString.h"
#include "TROOT.h"
#include "TFile.h"
#include "TH1.h"
#include "TH2.h"
#include "TDirectory.h"
#include "TGraph.h"
#include "TVector.h"
#include "TMultiGraph.h"
#include "TRandom3.h"
#include "TCanvas.h"

//#define DEBUG
//#define WITH_DEBUG

#define CRATE1 1         // HMS single arm setup
#define CRATE3 3         // HMS DC setup
#define SLOTMIN 3
#define NUMSLOTS 21
#define NTDCCHAN 128
#define NUMEVENTS 10
#define NUMRAWEVENTS 10

using namespace std;
using namespace Decoder;

TFile *hfile;
TDirectory *slot_dir[NUMSLOTS], *chan_dir[NTDCCHAN];
TH1I *h_rawtdc[NUMSLOTS][NTDCCHAN], *h_occupancy[NUMSLOTS];

void GeneratePlots(uint32_t islot, uint32_t chan) {
  // Slot directory
  slot_dir[islot] = dynamic_cast <TDirectory*> (hfile->Get(Form("slot_%d", islot)));
  if (!slot_dir[islot]) {slot_dir[islot] = hfile->mkdir(Form("slot_%d", islot)); slot_dir[islot]->cd();}
  else hfile->cd(Form("/slot_%d", islot)); 
  // Histos
  if (!h_occupancy[islot]) h_occupancy[islot] = new TH1I("h_occupancy", Form("CAEN1190 TDC Occupancy Slot %d", islot), 128, 0, 127);
  // Channel directory
  chan_dir[chan] = dynamic_cast <TDirectory*> (slot_dir[islot]->Get(Form("chan_%d", chan)));
  if (!chan_dir[chan]) {chan_dir[chan] = slot_dir[islot]->mkdir(Form("chan_%d", chan)); chan_dir[chan]->cd();}
  else hfile->cd(Form("/slot_%d/chan_%d", islot, chan));
  // Histos
  if (!h_rawtdc[islot][chan]) h_rawtdc[islot][chan] = new TH1I("h_rawtdc", Form("CAEN1190 RAW TDC Slot %d Channel %d", islot, chan), 50001, 0, 50000);
}

int main(int /* argc */, char** /* argv */)
{
  // Initialize the analysis clock
  clock_t t;
  t = clock();

  // Define the data file to be analyzed
  TString filename("snippet.dat");

  // Define the analysis debug output
  ofstream *debugfile = new ofstream;;
  // debugfile->open ("tst1190_main_debug.txt");
  
  // Initialize the CODA decoder
  THaCodaFile datafile;
  if (datafile.codaOpen(filename) != CODA_OK) {
    cerr << "ERROR:  Cannot open CODA data" << endl;
    cerr << "Perhaps you mistyped it" << endl;
    cerr << "... exiting." << endl;
    exit(2);
  }
  CodaDecoder *evdata = new CodaDecoder();
  evdata->SetCodaVersion(datafile.getCodaVersion());

  // Initialize the evdata debug output
  evdata->SetDebug(1);
  evdata->SetDebugFile(debugfile);

  // Initialize root and output
  TROOT fadcana("tst1190root", "Hall C analysis");
  hfile = new TFile("tst1190.root", "RECREATE", "1190 module data");

  // Loop over events
  cout << "***************************************" << endl;
  cout << NUMEVENTS << " events will be processed" << endl;
  cout << "***************************************" << endl;
  int status = 0;
  uint32_t iievent = 1;
  //for(uint32_t ievent = 0; ievent < NUMEVENTS + 1; ievent++) {
  for(uint32_t ievent = 0; ievent < iievent; ievent++) {
    // Read in data file
    status = datafile.codaRead();
    if (status == CODA_OK) {
      status = evdata->LoadEvent(datafile.getEvBuffer());
      if( status != CodaDecoder::HED_OK && status != CodaDecoder::HED_WARN ) {
        cerr << "ERROR " << status << " while decoding event " << ievent
             << ". Exiting." << endl;
        break;
      }

      if (debugfile) *debugfile << "****************" << endl;
      if (debugfile) *debugfile << "Event Number = " << evdata->GetEvNum() << endl;
      if (debugfile) *debugfile << "****************\n" << endl;

      // Loop over slots
      for(uint32_t islot = SLOTMIN; islot < NUMSLOTS; islot++) {
	if (evdata->GetNumRaw(CRATE3, islot) != 0) {  // HMS Single arm setup
	  Caen1190Module *tdc =
	      dynamic_cast <Caen1190Module*> (evdata->GetModule(CRATE3, islot));   // HMS single arm setup
	  if (tdc != NULL) {
	    if (debugfile) *debugfile << "\n///////////////////////////////\n"
				      << "Results for crate " 
				      << tdc->GetCrate() << ", slot " 
				      << tdc->GetSlot() << endl;
				      
	    if (debugfile) *debugfile << hex << "tdc pointer = " << tdc << "\n" << dec 
				      << "///////////////////////////////\n" << endl;
	    
	    // Loop over channels
	    for (uint32_t chan = 0; chan < NTDCCHAN; chan++) {
	      // Generate FADC plots
	      GeneratePlots(islot, chan);
	      for (Int_t hitno = 0; hitno < 16; hitno++) {  // Assume for now up-to 16 hits???
		if (tdc->GetData(chan, hitno) != 0)
		  h_rawtdc[islot][chan]->Fill(tdc->GetData(chan, hitno));
	      } // TDC hit loop
	    }  // TDC channel loop
	  }  // TDC module found condition
	  else 
	    if (debugfile) *debugfile << "TDC MODULE NOT FOUND!!!" << endl;
	}  // Number raw words condition
      }  // Slot loop
    } else if( status != EOF ) {
      cerr << "ERROR " << status << " while reading CODA event " << ievent
           << ". Twonk." << endl;
      break;
    } // CODA file read status condition
    if (iievent % 1000 == 0)
      //if (iievent % 1 == 0)
      cout << iievent << " events have been processed" << endl;
    iievent++;
    if (status == EOF) break;
  }  // Event loop 

  cout << "***************************************" << endl;
  cout << iievent - 1 << " events were processed" << endl;
  cout << "***************************************" << endl;
  
  // Write and clode the data file
  hfile->Write();
  hfile->Close();
  datafile.codaClose();

  // Calculate the analysis rate
  t = clock() - t;
  printf ("The analysis took %.1f seconds \n", ((float) t) / CLOCKS_PER_SEC);
  printf ("The analysis event rate is %.1f Hz \n", (iievent - 1) / (((float) t) / CLOCKS_PER_SEC));

  return status;
}  // main()
