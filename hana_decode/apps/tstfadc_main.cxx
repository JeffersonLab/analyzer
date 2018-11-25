#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstdio>
#include <numeric>
#include <vector>
#include <time.h>
#include "THaCodaFile.h"
#include "CodaDecoder.h"
#include "Fadc250Module.h"
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

static const uint32_t SLOTMIN      = 1;
static const uint32_t NUMSLOTS     = 22;
static const uint32_t NADCCHAN     = 16;
static const uint32_t NUMRAWEVENTS = 1000;
static const uint32_t NPED         = 4;
static const uint32_t NPEAK        = 4;

using namespace std;
using namespace Decoder;

TFile *hfile;
TDirectory *mode_dir, *slot_dir[NUMSLOTS], *chan_dir[NADCCHAN], *raw_samples_dir[NADCCHAN];
TDirectory *raw_samples_npeak_dir[NADCCHAN][NPEAK];
TH1I *h_pinteg[NUMSLOTS][NADCCHAN], *h_ptime[NUMSLOTS][NADCCHAN], *h_pped[NUMSLOTS][NADCCHAN], *h_ppeak[NUMSLOTS][NADCCHAN];
TH2I *h2_pinteg[NUMSLOTS], *h2_ptime[NUMSLOTS], *h2_pped[NUMSLOTS], *h2_ppeak[NUMSLOTS];
TGraph *g_psamp_event[NUMSLOTS][NADCCHAN][NUMRAWEVENTS];
TGraph *g_psamp_npeak_event[NUMSLOTS][NADCCHAN][NPEAK][NUMRAWEVENTS];
TCanvas *c_psamp[NUMSLOTS][NADCCHAN], *c_psamp_npeak[NUMSLOTS][NADCCHAN][NPEAK];
TMultiGraph *mg_psamp[NUMSLOTS][NADCCHAN], *mg_psamp_npeak[NUMSLOTS][NADCCHAN][NPEAK];
vector<uint32_t> raw_samples_vector[NUMSLOTS][NADCCHAN], raw_samples_npeak_vector[NUMSLOTS][NADCCHAN][NPEAK];
uint32_t raw_samp_index[NUMSLOTS][NADCCHAN], raw_samp_npeak_index[NUMSLOTS][NADCCHAN][NPEAK];

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
  if (mode != 1) {
    if (!h2_pinteg[islot]) h2_pinteg[islot] = new TH2I("h2_pinteg", Form("FADC Mode %d Pulse Integral Data Slot %d; Channel Number; FADC Units (Channels)", mode, islot), 16, -0.5, 15.5, 4096, 0, 40095);
    if (!h2_ptime[islot]) h2_ptime[islot]   = new TH2I("h2_ptime", Form("FADC Mode %d Pulse Time Data Slot %d; Channel Number; FADC Units (Channels)", mode, islot), 16, -0.5, 15.5, 40096, 0, 40095);
    if (!h2_pped[islot]) h2_pped[islot]     = new TH2I("h2_pped", Form("FADC Mode %d Pulse Pedestal Data Slot %d; Channel Number; FADC Units (Channels)", mode, islot), 16, -0.5, 15.5, 4096, 0, 4095);
    if (!h2_ppeak[islot]) h2_ppeak[islot]   = new TH2I("h2_ppeak", Form("FADC Mode %d Pulse Peak Data Slot %d; Channel Number; FADC Units (Channels)", mode, islot), 16, -0.5, 15.5, 4096, 0, 4095);
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
    mg_psamp[islot][chan] = new TMultiGraph("samples", "samples");
    for (uint32_t ipeak = 0; ipeak < NPEAK; ipeak++) 
      mg_psamp_npeak[islot][chan][ipeak] = new TMultiGraph(Form("samples_npeak_%d", ipeak+1), Form("samples_npeak_%d", ipeak+1));
  }
  // Canvas'
  if (!c_psamp[islot][chan] && ((mode == 1) || (mode == 8) || (mode == 10))) {
    c_psamp[islot][chan] = new TCanvas(Form("c_psamp_slot_%d_chan_%d", islot, chan), Form("c_psamp_slot_%d_chan_%d", islot, chan), 800, 800);
    c_psamp[islot][chan]->Divide(5, 5);
    for (uint32_t ipeak = 0; ipeak < NPEAK; ipeak++) {
      c_psamp_npeak[islot][chan][ipeak] = new TCanvas(Form("c_psamp_npeak_%d_slot_%d_chan_%d", ipeak+1, islot, chan), Form("c_psamp_npeak_%d_slot_%d_chan_%d", ipeak+1, islot, chan), 800, 800);
      c_psamp_npeak[islot][chan][ipeak]->Divide(5, 5);
    }
  }
  // Raw samples directories & graphs
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
    for (uint32_t ipeak = 0; ipeak < NPEAK; ipeak++) {
      raw_samples_npeak_dir[chan][ipeak] = dynamic_cast <TDirectory*> (chan_dir[chan]->Get(Form("raw_samples_npeak_%d", ipeak+1)));
      if (!raw_samples_npeak_dir[chan][ipeak]) {raw_samples_npeak_dir[chan][ipeak] = chan_dir[chan]->mkdir(Form("raw_samples_npeak_%d", ipeak+1)); raw_samples_npeak_dir[chan][ipeak]->cd();}
      else hfile->cd(Form("/mode_%d_data/slot_%d/chan_%d/raw_samples_npeak_%d", mode, islot, chan, ipeak+1));
      for (uint32_t raw_index = 0; raw_index < NUMRAWEVENTS; raw_index++) {
	if (!g_psamp_npeak_event[islot][chan][ipeak][raw_index]) {
	  g_psamp_npeak_event[islot][chan][ipeak][raw_index] = new TGraph();
	  g_psamp_npeak_event[islot][chan][ipeak][raw_index]->SetName(Form("g_psamp_npeak_%d_event_%d", ipeak+1, raw_index));
	}
      }
    }
  }
}

Int_t SumVectorElements(const vector<uint32_t>& data_vector) {
  Int_t sum_of_elements = 0;
  sum_of_elements = accumulate(data_vector.begin(), data_vector.end(), 0);
  return sum_of_elements;
}

int main(int /* argc */, char** /* argv */)
{

  Int_t runNumber = 0;
  Int_t maxEvent = 0;
  Int_t crateNumber = 0;
  TString spectrometer = "";
  TString fileLocation = "";
  TString runFile, rootFile;

  // Acquire run number, max number of events, spectrometer name, and crate number
  if (fileLocation == "") {
    cout << "\nEnter the location of the raw CODA file (raw or cache): ";
    cin >> fileLocation;
    if (fileLocation != "raw" && fileLocation != "cache") {
      cerr << "...Invalid entry\n"; 
      return 1;
    }
  }
  if (runNumber == 0) {
    cout << "\nEnter a Run Number (-1 to exit): ";
    cin >> runNumber;
    if (runNumber <= 0) {
      cerr << "...Invalid entry\n" << endl;
      return 1;
    }
  }
  if (maxEvent == 0) {
    cout << "\nEnter Number of Events to Analyze (-1 = All): ";
    cin >> maxEvent;
    if (maxEvent <= 0 && maxEvent != -1) {
      cerr << "...Invalid entry\n" << endl;
      return 1;
    }
    if (maxEvent == -1)
      cout << "\nAll Events in Run " << runNumber << " Will be Analayzed" << endl;
  }
  if (spectrometer == "") {
    cout << "\nEnter the Spectrometer Name (hms or shms): ";
    cin >> spectrometer;
    if (spectrometer != "hms" && spectrometer != "shms") {
      cerr << "...Invalid entry\n"; 
      return 1;
    }
  }
  if (crateNumber == 0) {
    cout << "\nEnter the Crate Number to be Analyzed: ";
    cin >> crateNumber;
    if (crateNumber <= 0) {
      cerr << "...Invalid entry\n"; 
      return 1;
    }
  }

  // Create file name patterns
  if (fileLocation == "raw")
    runFile = fileLocation+"/";
  if (fileLocation == "cache")
    runFile = fileLocation+"/";
  if (spectrometer == "hms")
    runFile += Form("hms_all_%05d.dat", runNumber);
  if (spectrometer == "shms")
    runFile += Form("shms_all_%05d.dat", runNumber);
  rootFile = Form("ROOTfiles/raw_fadc_%d.root", runNumber);

  // Initialize raw samples index array
  memset(raw_samp_index, 0, sizeof(raw_samp_index));
  memset(raw_samp_npeak_index, 0, sizeof(raw_samp_npeak_index));
  
  // Initialize the analysis clock
  clock_t t;
  t = clock();

  // Define the data file to be analyzed
  TString filename(runFile);

  // Define the analysis debug output
  ofstream *debugfile = new ofstream;;
  // debugfile->open ("tstfadc_main_debug.txt");
  
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
  TROOT fadcana("tstfadcroot", "Hall C analysis");
  hfile = new TFile("fadc_data.root", "RECREATE", "fadc module data");

  // Set the number of event to be analyzed
  uint32_t iievent;
  if (maxEvent == -1) iievent = 1;
  else iievent = maxEvent;   
  // Loop over events
  int status = 0;
  for(uint32_t ievent = 0; ievent < iievent; ievent++) {
    // Read in data file
    status = datafile.codaRead();
    if (status == CODA_OK) {
      status = evdata->LoadEvent(datafile.getEvBuffer());
      if( status != CodaDecoder::HED_OK && status != CodaDecoder::HED_WARN ) {
        cerr << "ERROR " << status << " while decoding event " << ievent
             << ". Exiting." << endl;
        break;;
      }
      // Loop over slots
      for(uint32_t islot = SLOTMIN; islot < NUMSLOTS; islot++) {
        if (evdata->GetNumRaw(crateNumber, islot) != 0) {
          Fadc250Module *fadc =
              dynamic_cast <Fadc250Module*> (evdata->GetModule(crateNumber, islot));
          if (fadc != NULL) {
            //fadc->CheckDecoderStatus();
            // Loop over channels
            for (uint32_t chan = 0; chan < NADCCHAN; chan++) {
              // Initilize variables
              Int_t num_fadc_events = 0, num_fadc_samples = 0;
              // Acquire the FADC mode
              Int_t fadc_mode = fadc->GetFadcMode(); fadc_mode_const = fadc_mode;
              if (debugfile) *debugfile << "Channel " << chan << " is in FADC Mode " << fadc_mode << endl;
              Bool_t raw_mode  = ((fadc_mode == 1) || (fadc_mode == 8) || (fadc_mode == 10));

              // Acquire the number of FADC events
              num_fadc_events = fadc->GetNumFadcEvents(chan);
              // If in raw mode, acquire the number of FADC samples
              if (raw_mode) {
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
                    for (uint32_t ipeak = 0; ipeak < NPEAK; ipeak++) {
                      if (uint32_t (num_fadc_events) == ipeak+1)
                        raw_samples_npeak_vector[islot][chan][ipeak] = fadc->GetPulseSamplesVector(chan);
                    }
                    if (raw_samp_index[islot][chan] < NUMRAWEVENTS) {
                      for (Int_t sample_num = 0; sample_num < Int_t (raw_samples_vector[islot][chan].size()); sample_num++)
                        g_psamp_event[islot][chan][raw_samp_index[islot][chan]]->SetPoint(sample_num, sample_num + 1, Int_t (raw_samples_vector[islot][chan][sample_num]));
                      mg_psamp[islot][chan]->Add(g_psamp_event[islot][chan][raw_samp_index[islot][chan]], "ACP");
                      raw_samp_index[islot][chan] += 1;
                    }  // NUMRAWEVENTS condition
                    for (uint32_t ipeak = 0; ipeak < NPEAK; ipeak++) {
                      if (uint32_t (num_fadc_events) == ipeak+1) {
                        if (raw_samp_npeak_index[islot][chan][ipeak] < NUMRAWEVENTS) {
                          for (Int_t sample_num = 0; sample_num < Int_t (raw_samples_npeak_vector[islot][chan][ipeak].size()); sample_num++)
                            g_psamp_npeak_event[islot][chan][ipeak][raw_samp_npeak_index[islot][chan][ipeak]]->SetPoint(sample_num, sample_num + 1, Int_t (raw_samples_npeak_vector[islot][chan][ipeak][sample_num]));
                          mg_psamp_npeak[islot][chan][ipeak]->Add(g_psamp_npeak_event[islot][chan][ipeak][raw_samp_npeak_index[islot][chan][ipeak]], "ACP");
                          raw_samp_npeak_index[islot][chan][ipeak] += 1;
                        } // NUMRAWEVENTS condition
                      } // npeak condition
                    } // npeak loop
                  } // Raw mode condition
                } // FADC event loop
              } // Number of FADC events condition
            } // FADC channel loop
          } // FADC module found condition
          else
            if (debugfile) *debugfile << "FADC MODULE NOT FOUND!!!" << endl;
        }  // Number raw words condition
      }  // Slot loop
    } else if( status != EOF ) {
      cerr << "ERROR " << status << " while reading CODA event " << ievent
          << ". Twonk." << endl;
      break;
    }  // CODA file read status condition
    if (ievent % 1000 == 0 && ievent != 0)
      cout << ievent << " events have been processed" << endl;
    if (maxEvent == -1) iievent++;
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
          g_psamp_event[islot][chan][raw_index]->SetTitle(Form("FADC Mode %d Pulse Peak Data Slot %d Channel %d Event %d", fadc_mode_const, islot, chan, raw_index+1));
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
        hfile->cd(Form("/mode_%d_data/slot_%d/chan_%d/raw_samples", fadc_mode_const, islot, chan));
        mg_psamp[islot][chan]->Draw("ACP");
        mg_psamp[islot][chan]->SetTitle(Form("%d Raw Events of FADC Mode %d Sample Data Slot %d Channel %d; Sample Number; Sample Value", raw_samp_index[islot][chan], fadc_mode_const, islot, chan));
        mg_psamp[islot][chan]->Write(); 
      }  // Mulitgraph condition
      for (uint32_t ipeak = 0; ipeak < NPEAK; ipeak++) {
	for( uint32_t raw_index = 0; raw_index < NUMRAWEVENTS; raw_index++) {
	  // Raw sample plots
	  if (g_psamp_npeak_event[islot][chan][ipeak][raw_index] != NULL) {
	    UInt_t rand_int = 1 + rand->Integer(9);
	    hfile->cd(Form("/mode_%d_data/slot_%d/chan_%d/raw_samples_npeak_%d", fadc_mode_const, islot, chan, ipeak+1));
	    c_psamp_npeak[islot][chan][ipeak]->cd(raw_index + 1);
	    g_psamp_npeak_event[islot][chan][ipeak][raw_index]->Draw("ACP");
	    g_psamp_npeak_event[islot][chan][ipeak][raw_index]->SetTitle(Form("FADC Mode %d Pulse Peak Data Slot %d Channel %d nPeak = %d Event %d", fadc_mode_const, islot, chan, ipeak+1, raw_index+1));
	    g_psamp_npeak_event[islot][chan][ipeak][raw_index]->GetXaxis()->SetTitle("Sample Number");
	    g_psamp_npeak_event[islot][chan][ipeak][raw_index]->GetYaxis()->SetTitle("Sample Value");
	    g_psamp_npeak_event[islot][chan][ipeak][raw_index]->SetLineColor(rand_int);
	    g_psamp_npeak_event[islot][chan][ipeak][raw_index]->SetMarkerColor(rand_int);
	    g_psamp_npeak_event[islot][chan][ipeak][raw_index]->SetMarkerStyle(20);
	    g_psamp_npeak_event[islot][chan][ipeak][raw_index]->SetDrawOption("ACP");
	  }  // Graph condition
	}  // Raw event loop
	// Write the canvas to file
	if (c_psamp_npeak[islot][chan][ipeak] != NULL) c_psamp_npeak[islot][chan][ipeak]->Write();
	// Write the multigraphs to file
	if (mg_psamp_npeak[islot][chan][ipeak] != NULL) {
	  hfile->cd(Form("/mode_%d_data/slot_%d/chan_%d/raw_samples_npeak_%d", fadc_mode_const, islot, chan, ipeak+1));
	  mg_psamp_npeak[islot][chan][ipeak]->Draw("ACP");
	  mg_psamp_npeak[islot][chan][ipeak]->SetTitle(Form("%d Raw Events of FADC Mode %d Sample Data Slot %d Channel %d nPeak = %d; Sample Number; Sample Value", raw_samp_npeak_index[islot][chan][ipeak], fadc_mode_const, islot, chan, ipeak+1));
	  mg_psamp_npeak[islot][chan][ipeak]->Write(); 
	}  // Mulitgraph condition
      }  // npeak loop
    }  // Channel loop
  }  // Slot loop

  // Write and clode the data file
  hfile->Write();
  hfile->Close();
  datafile.codaClose();

  // Calculate the analysis rate
  t = clock() - t;
  printf ("The analysis took %.1f seconds \n", ((float) t) / CLOCKS_PER_SEC);
  printf ("The analysis event rate is %.1f Hz \n", (iievent - 1) / (((float) t) / CLOCKS_PER_SEC));

}  // main()
