#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstdio>
#include <numeric>
#include <vector>
#include <time.h>
#include "Decoder.h"
#include "THaCodaFile.h"
#include "CodaDecoder.h"
#include "THaEvData.h"
#include "Module.h"
#include "Fadc250Module.h"
#include "evio.h"
#include "THaSlotData.h"
#include "TString.h"
#include "TROOT.h"
#include "TFile.h"
#include "TH1.h"
#include "TH2.h"
#include "TProfile.h"
#include "TNtuple.h"
#include "TDirectory.h"
#include "TGraph.h"
#include "TVector.h"
#include "TMultiGraph.h"
#include "TRandom3.h"
#include "TCanvas.h"

#define DEBUG
#define WITH_DEBUG
#define CRATE5 5         // Brian's setup
#define CRATE10 10       // Alex's setup
#define NUMSLOTS 21
#define NADCCHAN 16
#define NUMEVENTS 18000
#define NUMRAWEVENTS 25

using namespace std;
using namespace Decoder;

TFile *hfile;

TDirectory *mode_1_dir, *mode_7_dir, *mode_8_dir;
TDirectory *slot_dir[NUMSLOTS], *chan_dir[NADCCHAN];
TDirectory *raw_sample_dir_m1[NADCCHAN], *raw_sample_dir_m8[NADCCHAN];

TH1I *hm1_pinteg[NUMSLOTS][NADCCHAN];
TH1I *hm7_pinteg[NUMSLOTS][NADCCHAN], *hm7_ptime[NUMSLOTS][NADCCHAN], *hm7_pped[NUMSLOTS][NADCCHAN], *hm7_ppeak[NUMSLOTS][NADCCHAN];
TH1I *hm8_pinteg[NUMSLOTS][NADCCHAN], *hm8_ptime[NUMSLOTS][NADCCHAN], *hm8_pped[NUMSLOTS][NADCCHAN], *hm8_ppeak[NUMSLOTS][NADCCHAN];

TGraph *gm1_psamp_event[NUMSLOTS][NADCCHAN][NUMRAWEVENTS], *gm8_psamp_event[NUMSLOTS][NADCCHAN][NUMRAWEVENTS];
TCanvas *cm1_psamp[NUMSLOTS][NADCCHAN], *cm8_psamp[NUMSLOTS][NADCCHAN];

TMultiGraph *mgm1_psamp[NUMSLOTS][NADCCHAN], *mgm8_psamp[NUMSLOTS][NADCCHAN];

vector<uint32_t> m1_raw_samples_vector[NUMSLOTS][NADCCHAN], m8_raw_samples_vector[NUMSLOTS][NADCCHAN];

void CreateMode1Plots() {
  mode_1_dir = hfile->mkdir(Form("mode_1_data"));
  // Declare histos while looping over slots and channels
  for (uint32_t i = 3; i < NUMSLOTS; i++) {
      slot_dir[i] = mode_1_dir->mkdir(Form("slot_%d", i));
      slot_dir[i]->cd();
    for(uint32_t j = 0; j < NADCCHAN; j++) {
      chan_dir[j] = slot_dir[i]->mkdir(Form("chan_%d", j));
      chan_dir[j]->cd();
      // Mode 1 histos
      hm1_pinteg[i][j] = new TH1I("hm1_pinteg", Form("FADC Mode 1 Pulse Integral Data Slot %d Channel %d", i, j), 4096, 0, 40095);
      // Mode 1 graphs
      mgm1_psamp[i][j] = new TMultiGraph();
      mgm1_psamp[i][j]->SetName("mgm1_psamp");
      raw_sample_dir_m1[j] = chan_dir[j]->mkdir("raw_samples_m1");
      raw_sample_dir_m1[j]->cd();
      for (uint32_t k = 0; k < NUMRAWEVENTS; k++) {
	gm1_psamp_event[i][j][k] = new TGraph();
	gm1_psamp_event[i][j][k]->SetName(Form("gm1_psamp_event_%d", k));
      }
    }
  }
}

void CreateMode7Plots() {
  mode_7_dir = hfile->mkdir(Form("mode_7_data"));
  // Declare histos while looping over slots and channels
  for (uint32_t i = 3; i < NUMSLOTS; i++) {
      slot_dir[i] = mode_7_dir->mkdir(Form("slot_%d", i));
      slot_dir[i]->cd();
    for(uint32_t j = 0; j < NADCCHAN; j++) {
      chan_dir[j] = slot_dir[i]->mkdir(Form("chan_%d", j));
      chan_dir[j]->cd();
      // Mode 7 histos
      hm7_pinteg[i][j] = new TH1I("hm7_pinteg", Form("FADC Mode 7 Pulse Integral Data Slot %d Channel %d", i, j), 4096, 0, 40095);
      hm7_ptime[i][j]  = new TH1I("hm7_ptime",  Form("FADC Mode 7 Pulse Time Data Slot %d Channel %d", i, j), 4096, 0, 4095);
      hm7_pped[i][j]   = new TH1I("hm7_pped",   Form("FADC Mode 7 Pulse Pedestal Data Slot %d Channel %d", i, j), 4096, 0, 4095);
      hm7_ppeak[i][j]  = new TH1I("hm7_ppeak",  Form("FADC Mode 7 Pulse Peak Data Slot %d Channel %d", i, j), 4096, 0, 4095);
    }
  }
}

void CreateMode8Plots() {
  mode_8_dir = hfile->mkdir(Form("mode_8_data"));
  // Declare histos while looping over slots and channels
  for (uint32_t i = 3; i < NUMSLOTS; i++) {
      slot_dir[i] = mode_8_dir->mkdir(Form("slot_%d", i));
      slot_dir[i]->cd();
    for(uint32_t j = 0; j < NADCCHAN; j++) {
      chan_dir[j] = slot_dir[i]->mkdir(Form("chan_%d", j));
      chan_dir[j]->cd();
      // Mode 8 histos
      hm8_pinteg[i][j] = new TH1I("hm8_pinteg", Form("FADC Mode 8 Pulse Integral Data Slot %d Channel %d", i, j), 4096, 0, 40095);
      hm8_ptime[i][j]  = new TH1I("hm8_ptime",  Form("FADC Mode 8 Pulse Time Data Slot %d Channel %d", i, j), 4096, 0, 4095);
      hm8_pped[i][j]   = new TH1I("hm8_pped",   Form("FADC Mode 8 Pulse Pedestal Data Slot %d Channel %d", i, j), 4096, 0, 4095);
      hm8_ppeak[i][j]  = new TH1I("hm8_ppeak",  Form("FADC Mode 8 Pulse Peak Data Slot %d Channel %d", i, j), 4096, 0, 4095);
      // Mode 8 graphs
      mgm8_psamp[i][j] = new TMultiGraph();
      mgm8_psamp[i][j]->SetName("mgm8_psamp");
      raw_sample_dir_m8[j] = chan_dir[j]->mkdir("raw_samples_m8");
      raw_sample_dir_m8[j]->cd();
      for (uint32_t k = 0; k < NUMRAWEVENTS; k++) {
	gm8_psamp_event[i][j][k] = new TGraph();
	gm8_psamp_event[i][j][k]->SetName(Form("gm8_psamp_event_%d", k));
      }
    }
  }
}

void test(uint32_t islot, uint32_t chan) {
  if (mode_8_dir == NULL) {mode_8_dir = hfile->mkdir("mode_8_data"); mode_8_dir->cd(); cout << " made m8 dir" << endl;}
  else hfile->cd("/mode_8_data");
  
  if (slot_dir[islot] == NULL) {mode_8_dir->mkdir(Form("slot_%d", islot)); slot_dir[islot]->cd(); cout << " made slot dir" << endl;}
  else hfile->cd(Form("/mode_8_data/slot_%d", islot)); 

  if (chan_dir[chan] == NULL) {slot_dir[islot]->mkdir(Form("chan_%d", chan)); chan_dir[chan]->cd(); cout << " made chan dir" << endl;}
  else hfile->cd(Form("/mode_8_data/slot_%d/chan_%d", islot, chan)); 

  if (chan_dir[chan] == NULL) {
  // Mode 8 histos
  hm8_pinteg[islot][chan] = new TH1I("hm8_pinteg", Form("FADC Mode 8 Pulse Integral Data Slot %d Channel %d", islot, chan), 4096, 0, 40095);
  hm8_ptime[islot][chan]  = new TH1I("hm8_ptime",  Form("FADC Mode 8 Pulse Time Data Slot %d Channel %d", islot, chan), 4096, 0, 4095);
  hm8_pped[islot][chan]   = new TH1I("hm8_pped",   Form("FADC Mode 8 Pulse Pedestal Data Slot %d Channel %d", islot, chan), 4096, 0, 4095);
  hm8_ppeak[islot][chan]  = new TH1I("hm8_ppeak",  Form("FADC Mode 8 Pulse Peak Data Slot %d Channel %d", islot, chan), 4096, 0, 4095);
  // Mode 8 graphs
  mgm8_psamp[islot][chan] = new TMultiGraph();
  mgm8_psamp[islot][chan]->SetName("mgm8_psamp");
  }
}

void test_raw(uint32_t islot, uint32_t chan, uint32_t ievent) {
  if (raw_sample_dir_m8[chan] == NULL) {chan_dir[chan]->mkdir("raw_samples_m8"); raw_sample_dir_m8[chan]->cd();}
  else hfile->cd(Form("/mode_8_data/slot_%d/chan_%d/raw_samples_m8", islot, chan)); 
  gm8_psamp_event[islot][chan][ievent] = new TGraph();
  gm8_psamp_event[islot][chan][ievent]->SetName(Form("gm8_psamp_event_%d", ievent));
}

Int_t SumVectorElements(vector<uint32_t> data_vector) {
  Int_t sum_of_elements = 0;
  sum_of_elements = accumulate(data_vector.begin(), data_vector.end(), 0);
  return sum_of_elements;
}

int main(int argc, char* argv[])
{
  // Initialize the analysis clock
  clock_t t;
  t = clock();

  // Define the data file to be analyzed
  TString filename("snippet.dat");

  // Define the analysis debug output
  ofstream *debugfile = new ofstream;;
  //debugfile->open ("tstfadc_main_debug.txt");
  
  // Initialize the CODA decoder
  THaCodaFile datafile(filename);
  THaEvData *evdata = new CodaDecoder();

  // Initialize the evdata debug output
  evdata->SetDebug(1);
  evdata->SetDebugFile(debugfile);

  // Initialize root and output
  TROOT fadcana("tstfadcroot", "Hall C analysis");
  hfile = new TFile("tstfadc.root", "RECREATE", "fadc module data");

  // Create Plots
  CreateMode1Plots(); CreateMode7Plots(); CreateMode8Plots();

  // Loop over events
  cout << "***************************************" << endl;
  cout << NUMEVENTS << " events will be processed" << endl;
  cout << "***************************************" << endl;
  uint32_t iievent = 1;
  //for(uint32_t ievent = 0; ievent < NUMEVENTS; ievent++) {
  for(uint32_t ievent = 0; ievent < iievent; ievent++) {
    // Read in data file
    int status = datafile.codaRead();
    if (status == S_SUCCESS) {
      UInt_t *data = datafile.getEvBuffer();
      evdata->LoadEvent(data);

      if (debugfile) *debugfile << "****************" << endl;
      if (debugfile) *debugfile << "Event Number = " << evdata->GetEvNum() << endl;
      if (debugfile) *debugfile << "****************\n" << endl;

      // Loop over slots
      for(uint32_t islot = 3; islot < NUMSLOTS; islot++) {
      	//if (evdata->GetNumRaw(CRATE5, islot) != 0) {
	if (evdata->GetNumRaw(CRATE10, islot) != 0) {
	  Module *fadc = NULL;
	  //fadc = evdata->GetModule(CRATE5, islot);  // Bryan's setup
	  fadc = evdata->GetModule(CRATE10, islot);  // Alex's setup
	  if (fadc != NULL) {
	    //fadc->CheckDecoderStatus(fadc->GetCrate(), fadc->GetSlot());
	    if (debugfile) *debugfile << "\n///////////////////////////////\n"
				      << "Results for crate " 
				      << fadc->GetCrate() << ", slot " 
				      << fadc->GetSlot() << endl;
				      
	    if (debugfile) *debugfile << hex << "fadc pointer = " << fadc << "\n" << dec 
				      << "///////////////////////////////\n" << endl;
	    
	    // Loop over channels
	    for (uint32_t chan = 0; chan < NADCCHAN; chan++) {

	      //if (fadc->GetFadcMode(chan) == 8) test(islot, chan);

	      if (debugfile) *debugfile << "Channel " << chan << " is in FADC Mode " << fadc->GetFadcMode(chan) << endl;			
	      // Generate plots
	      if (fadc->GetFadcMode(chan) == 1) {
		Int_t numadcevents = 0;
		numadcevents = fadc->GetNumFadcEvents(chan);
		if (numadcevents > 0) {
		  uint32_t numfadcsamples = 0;
		  numfadcsamples = fadc->GetNumFadcSamples(chan, ievent);
		  hm1_pinteg[islot][chan]->Fill(fadc->GetEmulatedPulseIntegralData(chan));

		  if (debugfile) *debugfile << "NUM ADC EVENTS = " << numadcevents << endl;
		  if (debugfile) *debugfile << "FADC EMULATED PI DATA = " << fadc->GetEmulatedPulseIntegralData(chan) << endl;
		  if (debugfile) *debugfile << "NUM FADC SAMPLES = " << numfadcsamples << endl;
		  if (debugfile) *debugfile << "=============================" << endl;

		  if (ievent < NUMRAWEVENTS) {
		    // Acquire the raw samples vector and populate graphs
		    m1_raw_samples_vector[islot][chan] = fadc->GetPulseSamplesVector(chan);
		    for (Int_t sample_num = 0; sample_num < Int_t (m1_raw_samples_vector[islot][chan].size()); sample_num++) 
		      gm1_psamp_event[islot][chan][ievent]->SetPoint(sample_num, sample_num + 1, Int_t (m1_raw_samples_vector[islot][chan][sample_num]));
		    mgm1_psamp[islot][chan]->Add(gm1_psamp_event[islot][chan][ievent]);
		  }  // Number of raw events condition
		}  // Valid number of fadc events condition
	      }  // Mode 1 condition

	      if (fadc->GetFadcMode(chan) == 7 || fadc->GetFadcMode(chan) == 8) {
		Int_t numadcevents = 0;
		numadcevents = Int_t (fadc->GetNumFadcEvents(chan));
		if (debugfile) *debugfile << "numadcevents = " << numadcevents << endl;
		// Loop over events in fadc 
		if ((fadc->GetFadcMode(chan) == 7) && (numadcevents > 0)) {
		  for (Int_t jevent = 0; jevent < numadcevents; jevent++) {
		    if (debugfile) *debugfile << "FADC PI DATA = " << fadc->GetPulseIntegralData(chan, jevent) << endl;
		    if (debugfile) *debugfile << "FADC PT DATA = " << fadc->GetPulseTimeData(chan, jevent) << endl;
		    if (debugfile) *debugfile << "FADC PPED DATA = " << fadc->GetPulsePedestalData(chan, jevent) << endl;
		    if (debugfile) *debugfile << "FADC PPEAK DATA = " << fadc->GetPulsePeakData(chan, jevent) << endl;
		    if (debugfile) *debugfile << "=============================" << endl;
		    
		    hm7_pinteg[islot][chan]->Fill(fadc->GetPulseIntegralData(chan, jevent));
		    hm7_ptime[islot][chan]->Fill(fadc->GetPulseTimeData(chan, jevent));
		    hm7_pped[islot][chan]->Fill(fadc->GetPulsePedestalData(chan, jevent));
		    hm7_ppeak[islot][chan]->Fill(fadc->GetPulsePeakData(chan, jevent));
		  }  // FADC number of events loop
		}  // Mode 7 and valid numfadcevents condition

		if (fadc->GetFadcMode(chan) == 8 && (numadcevents > 0)) {
		  for (Int_t jevent = 0; jevent < numadcevents; jevent++) {
		    if (debugfile) *debugfile << "FADC EMULATED PI DATA = " << fadc->GetEmulatedPulseIntegralData(chan) << endl;
		    if (debugfile) *debugfile << "FADC PT DATA = " << fadc->GetPulseTimeData(chan, jevent) << endl;
		    if (debugfile) *debugfile << "FADC PPED DATA = " << fadc->GetPulsePedestalData(chan, jevent) << endl;
		    if (debugfile) *debugfile << "FADC PPEAK DATA = " << fadc->GetPulsePeakData(chan, jevent) << endl;

		    hm8_pinteg[islot][chan]->Fill(fadc->GetEmulatedPulseIntegralData(chan));
		    hm8_ptime[islot][chan]->Fill(fadc->GetPulseTimeData(chan, jevent));
		    hm8_pped[islot][chan]->Fill(fadc->GetPulsePedestalData(chan, jevent));
		    hm8_ppeak[islot][chan]->Fill(fadc->GetPulsePeakData(chan, jevent));
    		    					
		    uint32_t numfadcsamples = 0;
		    numfadcsamples = fadc->GetNumFadcSamples(chan, jevent);
		    if (debugfile) *debugfile << "NUM FADC SAMPLES = " << numfadcsamples << endl;
		    if (debugfile) *debugfile << "=============================" << endl;

		    if (ievent < NUMRAWEVENTS) {
		      // Acquire the raw samples vector and populate graphs
		      m8_raw_samples_vector[islot][chan] = fadc->GetPulseSamplesVector(chan);
		      for (Int_t sample_num = 0; sample_num < Int_t (m8_raw_samples_vector[islot][chan].size()); sample_num++) 
		    	gm8_psamp_event[islot][chan][ievent]->SetPoint(sample_num, sample_num + 1, Int_t (m8_raw_samples_vector[islot][chan][sample_num]));
		      mgm8_psamp[islot][chan]->Add(gm8_psamp_event[islot][chan][ievent], "ACP");
		     }  // NUMRAWEVENTS condition
		  }  // FADC number of events loop
		}  // Mode 8 and valid num fadc events condition
	      }  // Mode 7 OR 8 condition
	    }  //FADC channel loop
	  }  // FADC module found condition
	  else 
	    if (debugfile) *debugfile << "FADC MODULE NOT FOUND!!!" << endl;
	}  // Number raw words condition
      }  // Slot loop
    }  // CODA file read status condition
    if (iievent % 1000 == 0)
    //if (iievent % 1 == 0)
      cout << iievent << " events have been processed" << endl;
    iievent++;
    if (status == EOF) break;
  }  // Event loop 

  TRandom3 *rand = new TRandom3();
  // Populate waveform graphs and multigraphs and write to root file
  for(uint32_t islot = 3; islot < NUMSLOTS; islot++) {
    for (uint32_t chan = 0; chan < NADCCHAN; chan++) {

      cm8_psamp[islot][chan] = new TCanvas(Form("cm8_psamp_slot_%d_chan_%d", islot, chan), Form("cm8_psamp_slot_%d_chan_%d", islot, chan), 800, 800);
      cm8_psamp[islot][chan]->Divide(5, 5);

      for( uint32_t ievent = 0; ievent < NUMRAWEVENTS; ievent++) {
	// Mode 1 graphs
	// if (gm1_psamp_event[islot][chan][ievent] != NULL) {
	//   hfile->cd(Form("/mode_1_data/slot_%d/chan_%d/raw_samples_m1", islot, chan));
	//   gm1_psamp_event[islot][chan][ievent]->Draw("ACP");
	//   gm1_psamp_event[islot][chan][ievent]->Write();
	// }

	// Mode 8 graphs
	if (gm8_psamp_event[islot][chan][ievent] != NULL) {
	  UInt_t rand_int = 1 + rand->Integer(9);
	  hfile->cd(Form("/mode_8_data/slot_%d/chan_%d/raw_samples_m8", islot, chan));
	  cm8_psamp[islot][chan]->cd(ievent + 1);
	  gm8_psamp_event[islot][chan][ievent]->Draw("ACP");
	  gm8_psamp_event[islot][chan][ievent]->SetTitle(Form("FADC Mode 8 Pulse Peak Data Slot %d Channel %d Event %d", islot, chan, ievent));
	  gm8_psamp_event[islot][chan][ievent]->GetXaxis()->SetTitle("Sample Number");
	  gm8_psamp_event[islot][chan][ievent]->GetYaxis()->SetTitle("Sample Value");
	  gm8_psamp_event[islot][chan][ievent]->SetLineColor(rand_int);
	  gm8_psamp_event[islot][chan][ievent]->SetMarkerColor(rand_int);
	  gm8_psamp_event[islot][chan][ievent]->SetMarkerStyle(20);
	  gm8_psamp_event[islot][chan][ievent]->SetDrawOption("ACP");
	  //gm8_psamp_event[islot][chan][ievent]->Write();
	}
      }
      cm8_psamp[islot][chan]->Write();

      // Mode 1 multigraphs
      if (mgm1_psamp[islot][chan] != NULL) {
	hfile->cd(Form("/mode_1_data/slot_%d/chan_%d", islot, chan));
	mgm1_psamp[islot][chan]->Draw("ACP");
	// mgm1_psamp[islot][chan]->SetTitle(Form("%d Raw Events of FADC Mode 1 Pulse Peak Data Slot %d Channel %d", NUMRAWEVENTS, islot, chan));
	// mgm1_psamp[islot][chan]->GetXaxis()->SetTitle("Sample Number");
	// mgm1_psamp[islot][chan]->GetYaxis()->SetTitle("Sample Value");
	mgm1_psamp[islot][chan]->Write(); 
      }
      // Mode 8 multigraphs
      if (mgm8_psamp[islot][chan] != NULL) {
	hfile->cd(Form("/mode_8_data/slot_%d/chan_%d", islot, chan));
	mgm8_psamp[islot][chan]->Draw("ACP");
	// mgm8_psamp[islot][chan]->SetTitle(Form("%d Raw Events of FADC Mode 8 Pulse Peak Data Slot %d Channel %d", NUMRAWEVENTS, islot, chan));
	// mgm8_psamp[islot][chan]->GetXaxis()->SetTitle("Sample Number");
	// mgm8_psamp[islot][chan]->GetYaxis()->SetTitle("Sample Value");
	mgm8_psamp[islot][chan]->Write(); 
      }
    }  // Channel loop
  }  // Slot loop

  cout << "***************************************" << endl;
  cout << iievent - 1 << " events were processed" << endl;
  cout << "***************************************" << endl;
  
  // Write and clode the data file
  hfile->Write();
  hfile->Close();

  // Calculate the analysis rate
  t = clock() - t;
  printf ("The analysis took %.1f seconds \n", ((float) t) / CLOCKS_PER_SEC);
  printf ("The analysis event rate is %.1f Hz \n", (iievent - 1) / (((float) t) / CLOCKS_PER_SEC));

}  // main()
