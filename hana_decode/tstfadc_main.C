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

//#define DEBUG
//#define WITH_DEBUG
#define CRATE1 1         // HMS single arm setup
#define CRATE5 5         // Brian's setup
#define CRATE10 10       // Alex's setup
#define CRATE11 11       // Temp HMS multicrate
#define CRATE12 12       // Mark's setup
#define SLOTMIN 3
#define NUMSLOTS 21
#define NADCCHAN 16
#define NUMEVENTS 500
#define NUMRAWEVENTS 25
#define NPED 4

using namespace std;
using namespace Decoder;

TFile *hfile;
TDirectory *mode_dir, *slot_dir[NUMSLOTS], *chan_dir[NADCCHAN], *raw_samples_dir[NADCCHAN];
TH1I *h_pinteg[NUMSLOTS][NADCCHAN], *h_ptime[NUMSLOTS][NADCCHAN], *h_pped[NUMSLOTS][NADCCHAN], *h_ppeak[NUMSLOTS][NADCCHAN];
TH2I *h2_pinteg[NUMSLOTS], *h2_ptime[NUMSLOTS], *h2_pped[NUMSLOTS], *h2_ppeak[NUMSLOTS];
TGraph *g_psamp_event[NUMSLOTS][NADCCHAN][NUMRAWEVENTS];
TCanvas *c_psamp[NUMSLOTS][NADCCHAN];
TMultiGraph *mg_psamp[NUMSLOTS][NADCCHAN];
vector<uint32_t> raw_samples_vector[NUMSLOTS][NADCCHAN];
uint32_t raw_samp_index[NUMSLOTS][NADCCHAN];

static int fadc_mode_const;

void GeneratePlots(Int_t mode, uint32_t islot, uint32_t chan) {
  // Mode Directory
  mode_dir = dynamic_cast <TDirectory*> (hfile->Get(Form("mode_%d_data", mode)));
  if(!mode_dir) {mode_dir = hfile->mkdir(Form("mode_%d_data", mode)); mode_dir->cd();}
  else hfile->cd(Form("/mode_%d_data", mode));
  // Slot directory
  slot_dir[islot] = dynamic_cast <TDirectory*> (mode_dir->Get(Form("slot_%d", islot)));
  if (!slot_dir[islot]) {slot_dir[islot] = mode_dir->mkdir(Form("slot_%d", islot)); slot_dir[islot]->cd();}
  else hfile->cd(Form("/mode_%d_data/slot_%d", mode, islot));
  if (mode !=1) {
    if (!h2_pinteg[islot]) h2_pinteg[islot] = new TH2I("h2_pinteg", Form("FADC Mode %d Pulse Integral Data Slot %d; Channel Number; FADC Units (Channels)", mode, islot), 16, 0.5, 16.5, 4096, 0, 40095);
    if (!h2_ptime[islot]) h2_ptime[islot]   = new TH2I("h2_ptime", Form("FADC Mode %d Pulse Time Data Slot %d; Channel Number; FADC Units (Channels)", mode, islot), 16, 0.5, 16.5, 40096, 0, 40095);
    if (!h2_pped[islot]) h2_pped[islot]     = new TH2I("h2_pped", Form("FADC Mode %d Pulse Pedestal Data Slot %d; Channel Number; FADC Units (Channels)", mode, islot), 16, 0.5, 16.5, 4096, 0, 4095);
    if (!h2_ppeak[islot]) h2_ppeak[islot]   = new TH2I("h2_ppeak", Form("FADC Mode %d Pulse Peak Data Slot %d; Channel Number; FADC Units (Channels)", mode, islot), 16, 0.5, 16.5, 4096, 0, 4095);
  }
  // Channel directory
  chan_dir[chan] = dynamic_cast <TDirectory*> (slot_dir[islot]->Get(Form("chan_%d", chan)));
  if (!chan_dir[chan]) {chan_dir[chan] = slot_dir[islot]->mkdir(Form("chan_%d", chan)); chan_dir[chan]->cd();}
  else hfile->cd(Form("/mode_%d_data/slot_%d/chan_%d", mode, islot, chan));
  // Histos
  if (!h_pinteg[islot][chan]) h_pinteg[islot][chan] = new TH1I("h_pinteg", Form("FADC Mode %d Pulse Integral Data Slot %d Channel %d; FADC Units (Channels); Counts / 10 Channels", mode, islot, chan), 4096, 0, 40095);
  if (mode != 1) {
    if (!h_ptime[islot][chan]) h_ptime[islot][chan]  = new TH1I("h_ptime",  Form("FADC Mode %d Pulse Time Data Slot %d Channel %d; FADC Units (Channels); Counts / Channel", mode, islot, chan), 40096, 0, 40095);
    if (!h_pped[islot][chan])  h_pped[islot][chan]   = new TH1I("h_pped",   Form("FADC Mode %d Pulse Pedestal Data Slot %d Channel %d; FADC Units (Channels); Counts / Channel", mode, islot, chan), 4096, 0, 4095);
    if (!h_ppeak[islot][chan]) h_ppeak[islot][chan]  = new TH1I("h_ppeak",  Form("FADC Mode %d Pulse Peak Data Slot %d Channel %d; FADC Units (Channels); Counts / Channel", mode, islot, chan), 4096, 0, 4095);
  }
  // Graphs
  if (!mg_psamp[islot][chan] && ((mode == 1) || (mode == 8) || (mode == 10))) {
    mg_psamp[islot][chan] = new TMultiGraph();
  }
  // Canvas'
  if (!c_psamp[islot][chan] && ((mode == 1) || (mode == 8) || (mode == 10))) {
    c_psamp[islot][chan] = new TCanvas(Form("c_psamp_slot_%d_chan_%d", islot, chan), Form("c_psamp_slot_%d_chan_%d", islot, chan), 800, 800);
    c_psamp[islot][chan]->Divide(5, 5);
  }
  // Raw samples directoy & graphs
  if ((mode == 1) || (mode == 8) || (mode == 10)) {   
    raw_samples_dir[chan] = dynamic_cast <TDirectory*> (chan_dir[chan]->Get("raw_samples"));
    if (!raw_samples_dir[chan]) {raw_samples_dir[chan] = chan_dir[chan]->mkdir("raw_samples"); raw_samples_dir[chan]->cd();}
    else hfile->cd(Form("/mode_%d_data/slot_%d/chan_%d/raw_samples", mode, islot, chan));
    for (uint32_t raw_index = 0; raw_index < NUMRAWEVENTS; raw_index++) {
      if (!g_psamp_event[islot][chan][raw_index]) {
	g_psamp_event[islot][chan][raw_index] = new TGraph();
	g_psamp_event[islot][chan][raw_index]->SetName(Form("g_psamp_event_%d", raw_index));
      }
    }
  }
}

Int_t SumVectorElements(vector<uint32_t> data_vector) {
  Int_t sum_of_elements = 0;
  sum_of_elements = accumulate(data_vector.begin(), data_vector.end(), 0);
  return sum_of_elements;
}

int main(int argc, char* argv[])
{
  // Initialize raw samples index array
  memset(raw_samp_index, 0, sizeof(raw_samp_index));
  
  // Initialize the analysis clock
  clock_t t;
  t = clock();

  // Define the data file to be analyzed
  TString filename("snippet.dat");

  // Define the analysis debug output
  ofstream *debugfile = new ofstream;;
  debugfile->open ("tstfadc_main_debug.txt");
  
  // Initialize the CODA decoder
  THaCodaFile datafile(filename);
  THaEvData *evdata = new CodaDecoder();

  // Initialize the evdata debug output
  evdata->SetDebug(1);
  evdata->SetDebugFile(debugfile);

  // Initialize root and output
  TROOT fadcana("tstfadcroot", "Hall C analysis");
  hfile = new TFile("tstfadc.root", "RECREATE", "fadc module data");

  // Loop over events
  uint32_t iievent = 1;
  //for(uint32_t ievent = 0; ievent < NUMEVENTS; ievent++) {
  for(uint32_t ievent = 0; ievent < iievent; ievent++) {
    // Read in data file
    int status = datafile.codaRead();
    if (status == S_SUCCESS) {
      UInt_t *data = datafile.getEvBuffer();
      evdata->LoadEvent(data);
      // Loop over slots
      for(uint32_t islot = SLOTMIN; islot < NUMSLOTS; islot++) {
	if (evdata->GetNumRaw(CRATE11, islot) != 0) {  // HMS Single arm setup
	  Fadc250Module *fadc = NULL;
	  fadc = dynamic_cast <Fadc250Module*> (evdata->GetModule(CRATE11, islot));   // HMS single arm setup
	  if (fadc != NULL) {
	    //fadc->CheckDecoderStatus();
	    // Loop over channels
	    for (uint32_t chan = 0; chan < NADCCHAN; chan++) {
	      // Initilize variables
	      Int_t  fadc_mode, num_fadc_events, num_fadc_samples;
	      Bool_t raw_mode; 
	      fadc_mode = num_fadc_events = num_fadc_samples = raw_mode = 0;
	      // Acquire the FADC mode
	      fadc_mode = fadc->GetFadcMode(); fadc_mode_const = fadc_mode;
	      if (debugfile) *debugfile << "Channel " << chan << " is in FADC Mode " << fadc_mode << endl;
	      raw_mode  = ((fadc_mode == 1) || (fadc_mode == 8) || (fadc_mode == 10));
	      
	      // Acquire the number of FADC events
	      num_fadc_events = fadc->GetNumFadcEvents(chan);
	      // If in raw mode, acquire the number of FADC samples
	      if (raw_mode) {
		num_fadc_samples = 0;
		num_fadc_samples = fadc->GetNumFadcSamples(chan, ievent);
	      }
	      if (num_fadc_events > 0) {
		// Generate FADC plots
		GeneratePlots(fadc_mode, islot, chan);
	      	for (Int_t jevent = 0; jevent < num_fadc_events; jevent++) {
		  // Debug output
		  if ((fadc_mode == 1 || fadc_mode == 8) && num_fadc_samples > 0) 
		    if (debugfile) *debugfile << "FADC EMULATED PI DATA = " << fadc->GetEmulatedPulseIntegralData(chan) << endl;
		  if (fadc_mode == 7 || fadc_mode == 8 || fadc_mode == 9 || fadc_mode == 10) {
		    if (fadc_mode != 8) {if (debugfile) *debugfile << "FADC PI DATA = " << fadc->GetPulseIntegralData(chan, jevent) << endl;}
		    if (debugfile) *debugfile << "FADC PT DATA = " << fadc->GetPulseTimeData(chan, jevent) << endl;
		    if (debugfile) *debugfile << "FADC PPED DATA = " << fadc->GetPulsePedestalData(chan, jevent) / NPED << endl;
		    if (debugfile) *debugfile << "FADC PPEAK DATA = " << fadc->GetPulsePeakData(chan, jevent) << endl;
		  }
		  // Fill histos
	      	  if ((fadc_mode == 1 || fadc_mode == 8) && num_fadc_samples > 0) h_pinteg[islot][chan]->Fill(fadc->GetEmulatedPulseIntegralData(chan));
		  else if (fadc_mode == 7 || fadc_mode == 8 || fadc_mode == 9 || fadc_mode == 10) {
		    if (fadc_mode != 8) {h_pinteg[islot][chan]->Fill(fadc->GetPulseIntegralData(chan, jevent));}
		    if (fadc_mode == 8 || fadc_mode == 1) {h_pinteg[islot][chan]->Fill(fadc->GetEmulatedPulseIntegralData(chan));}
		    h_ptime[islot][chan]->Fill(fadc->GetPulseTimeData(chan, jevent));
		    h_pped[islot][chan]->Fill(fadc->GetPulsePedestalData(chan, jevent) / NPED);
		    h_ppeak[islot][chan]->Fill(fadc->GetPulsePeakData(chan, jevent));
		    // 2D Histos
		    if (fadc_mode != 8) {h2_pinteg[islot]->Fill(chan, fadc->GetPulseIntegralData(chan, jevent));}
		    if (fadc_mode == 8 || fadc_mode == 1) {h2_pinteg[islot]->Fill(chan, fadc->GetEmulatedPulseIntegralData(chan));}
		    h2_ptime[islot]->Fill(chan, fadc->GetPulseTimeData(chan, jevent));
		    h2_pped[islot]->Fill(chan, fadc->GetPulsePedestalData(chan, jevent) / NPED);
		    h2_ppeak[islot]->Fill(chan, fadc->GetPulsePeakData(chan, jevent));
		  }
		  // Raw sample events
	      	  if (raw_mode && num_fadc_samples > 0) {
		    // Debug output
	      	    if (debugfile) *debugfile << "NUM FADC SAMPLES = " << num_fadc_samples << endl;
		    // Acquire the raw samples vector and populate graphs
		    raw_samples_vector[islot][chan] = fadc->GetPulseSamplesVector(chan);
		    if (raw_samp_index[islot][chan] < NUMRAWEVENTS) {
		      for (Int_t sample_num = 0; sample_num < Int_t (raw_samples_vector[islot][chan].size()); sample_num++)
			g_psamp_event[islot][chan][raw_samp_index[islot][chan]]->SetPoint(sample_num, sample_num + 1, Int_t (raw_samples_vector[islot][chan][sample_num]));
		      mg_psamp[islot][chan]->Add(g_psamp_event[islot][chan][raw_samp_index[islot][chan]], "ACP");
		      raw_samp_index[islot][chan] += 1;
		    }  // NUMRAWEVENTS condition
		  }  // Raw mode condition
	      	}  //  FADC event loop
	      }  // Number of FADC events condition
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

  // Populate waveform graphs and multigraphs and write to root file
  TRandom3 *rand = new TRandom3();
  for(uint32_t islot = 3; islot < NUMSLOTS; islot++) {
    for (uint32_t chan = 0; chan < NADCCHAN; chan++) {
      for( uint32_t raw_index = 0; raw_index < NUMRAWEVENTS; raw_index++) {
      // Raw sample plots
  	if (g_psamp_event[islot][chan][raw_index] != NULL) {
  	  UInt_t rand_int = 1 + rand->Integer(9);
  	  hfile->cd(Form("/mode_%d_data/slot_%d/chan_%d/raw_samples", fadc_mode_const, islot, chan));
  	  c_psamp[islot][chan]->cd(raw_index + 1);
  	  g_psamp_event[islot][chan][raw_index]->Draw("ACP");
  	  g_psamp_event[islot][chan][raw_index]->SetTitle(Form("FADC Mode %d Pulse Peak Data Slot %d Channel %d Event %d", fadc_mode_const, islot, chan, raw_index));
  	  g_psamp_event[islot][chan][raw_index]->GetXaxis()->SetTitle("Sample Number");
  	  g_psamp_event[islot][chan][raw_index]->GetYaxis()->SetTitle("Sample Value");
  	  g_psamp_event[islot][chan][raw_index]->SetLineColor(rand_int);
  	  g_psamp_event[islot][chan][raw_index]->SetMarkerColor(rand_int);
  	  g_psamp_event[islot][chan][raw_index]->SetMarkerStyle(20);
  	  g_psamp_event[islot][chan][raw_index]->SetDrawOption("ACP");
  	}  // Graph condition
      }  // Raw event loop
      // Write the canvas to file
      if (c_psamp[islot][chan] != NULL) c_psamp[islot][chan]->Write();
      // Write the multigraphs to file
      if (mg_psamp[islot][chan] != NULL) {
        hfile->cd(Form("/mode_%d_data/slot_%d/chan_%d", fadc_mode_const, islot, chan));
        mg_psamp[islot][chan]->Draw("ACP");
        mg_psamp[islot][chan]->SetTitle(Form("%d Raw Events of FADC Mode %d Sample Data Slot %d Channel %d; Sample Number; Sample Value", raw_samp_index[islot][chan], fadc_mode_const, islot, chan));
        mg_psamp[islot][chan]->Write(); 
      }  // Mulitgraph condition
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
