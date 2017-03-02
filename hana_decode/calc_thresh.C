// Author Eric Pooser, pooser@jlab.org
// Script to calculate pedestal values on crate, slot, channel basis

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstdio>
#include <numeric>
#include <vector>
#include <time.h>
#include "TH1.h"
#include "TH2.h"
#include "TFile.h"
#include "TF1.h"

#define SLOTMIN  1
#define NUMSLOTS 22
#define NADCCHAN 16
#define NPOINTS 100
#define NSIGMAFIT 5
#define NSIGMATHRESH 5

using namespace std;

TFile *rif, *rof;
TH1I *h_pped[NUMSLOTS][NADCCHAN];
TF1 *init_gfit[NUMSLOTS][NADCCHAN], *gfit[NUMSLOTS][NADCCHAN];
UInt_t nentries[NUMSLOTS][NADCCHAN], threshhold[NUMSLOTS][NADCCHAN];
Double_t init_max[NUMSLOTS][NADCCHAN], init_mean[NUMSLOTS][NADCCHAN], init_stddev[NUMSLOTS][NADCCHAN];
Double_t iter_max[NUMSLOTS][NADCCHAN], iter_mean[NUMSLOTS][NADCCHAN], iter_stddev[NUMSLOTS][NADCCHAN];
Double_t finl_max[NUMSLOTS][NADCCHAN], finl_mean[NUMSLOTS][NADCCHAN], finl_stddev[NUMSLOTS][NADCCHAN];
Double_t finl_max_err[NUMSLOTS][NADCCHAN], finl_mean_err[NUMSLOTS][NADCCHAN], finl_stddev_err[NUMSLOTS][NADCCHAN];
Double_t init_fr_low[NUMSLOTS][NADCCHAN], init_fr_high[NUMSLOTS][NADCCHAN];
Double_t fr_low[NUMSLOTS][NADCCHAN], fr_high[NUMSLOTS][NADCCHAN];
TDirectory *mode_dir, *slot_dir[NUMSLOTS], *chan_dir[NADCCHAN];

void calc_thresh() {

  Int_t fadc_mode  = 0;
  // Acquire number of channels above baseline the thresholds should be
  if (fadc_mode == 0) {
    cout << "\nEnter The Mode of The F250's: ";
    cin >> fadc_mode;
    if (fadc_mode <= 0) {
      cerr << "...Invalid entry\n" << endl;
      exit(EXIT_FAILURE);
    }
  }

  Int_t nchan  = 0;
  // Acquire number of channels above baseline the thresholds should be
  if (nchan == 0) {
    cout << "\nEnter The Number of Channels Above Baseline the Threshold Should Be (40 recommended): ";
    cin >> nchan;
    if (nchan <= 0) {
      cerr << "...Invalid entry\n" << endl;
      exit(EXIT_FAILURE);
    }
  }

  // Get the input file and declare the output file
  rif = new TFile("fadc_data.root");
  rof = new TFile("fadc_ped_data.root", "RECREATE", "FADC Pedestal Data");

  // Declare the output text file
  ofstream outputfile;
  
  mode_dir = dynamic_cast <TDirectory*> (rof->Get(Form("mode_%d_data", fadc_mode)));
  if(!mode_dir) {
    mode_dir = rof->mkdir(Form("mode_%d_data", fadc_mode)); 
    rof->cd(Form("/mode_%d_data", fadc_mode));
  }
  else rof->cd(Form("/mode_%d_data", fadc_mode));

  for (UInt_t islot = SLOTMIN; islot < NUMSLOTS; islot++) {
    if (islot == 11 || islot == 12) continue;

    slot_dir[islot] = dynamic_cast <TDirectory*> (mode_dir->Get(Form("slot_%d", islot)));
    if(!slot_dir[islot]) {
      slot_dir[islot] = rof->mkdir(Form("mode_%d_data/slot_%d", fadc_mode, islot)); 
      rof->cd(Form("/mode_%d_data/slot_%d", fadc_mode, islot));
    }
    else rof->cd(Form("/mode_%d_data/slot_%d", fadc_mode, islot));

    for (UInt_t ichan = 0; ichan < NADCCHAN; ichan++) {

      // Write pedestal histos and fits to rof
      chan_dir[ichan] = dynamic_cast <TDirectory*> (slot_dir[islot]->Get(Form("chan_%d", ichan)));
      if(!chan_dir[ichan]) {
	chan_dir[ichan] = rof->mkdir(Form("mode_%d_data/slot_%d/chan_%d", fadc_mode, islot, ichan)); 
	rof->cd(Form("/mode_%d_data/slot_%d/chan_%d", fadc_mode, islot, ichan));
      }
      else rof->cd(Form("/mode_%d_data/slot_%d/chan_%d", fadc_mode, islot, ichan));

      if (!h_pped[islot][ichan]) h_pped[islot][ichan] = (TH1I*) (rif->Get(Form("mode_%d_data/slot_%d/chan_%d/h_pped", fadc_mode, islot, ichan)));
      // Acquire the number of entries as a restriction for a good fit
      if (h_pped[islot][ichan]) nentries[islot][ichan] = h_pped[islot][ichan]->GetEntries();
      // cout << "CHANNEL NO PED = " << ichan << ", HISTO PTR = "
      //      << h_pped[islot][ichan] << ", NENTRIES = " << nentries[islot][ichan] << endl;
      if (h_pped[islot][ichan] && (nentries[islot][ichan] >= NPOINTS)) {

	//cout << "Found Pedestal Histo for Slot " << islot << ", Channel " << ichan << endl;
	// Define the fit
	init_gfit[islot][ichan] = new TF1(Form("init_fit_slot_%d_chan_%d", islot, ichan), "gaus(0)");
	init_gfit[islot][ichan]->SetLineColor(2);

	// Obtain intial fit parameters
	Int_t    init_max_bin = h_pped[islot][ichan]->GetMaximumBin();
	Double_t init_max_val = h_pped[islot][ichan]->GetMaximum();
	Double_t init_max_bin_center = h_pped[islot][ichan]->GetBinCenter(init_max_bin);

	Int_t curr_bin = init_max_bin+1;
	while (true) {
	  Double_t curr_val = h_pped[islot][ichan]->GetBinContent(curr_bin);
	  if (curr_val < init_max_val/2.) break;
	  ++curr_bin;
	}
	Double_t init_sigma = h_pped[islot][ichan]->GetBinCenter(curr_bin) - init_max_bin_center;

	// Define the initial fit parameters and initialize the fit
	init_max[islot][ichan]    = init_max_val;
	init_mean[islot][ichan]   = init_max_bin_center;
	init_stddev[islot][ichan] = init_sigma;
	init_gfit[islot][ichan]->SetParameters(init_max[islot][ichan],
					       init_mean[islot][ichan],
					       init_stddev[islot][ichan]);

	// Perform the initial fit over the full range of the histo
	init_fr_low[islot][ichan]  = init_mean[islot][ichan] - NSIGMAFIT*init_stddev[islot][ichan];
	init_fr_high[islot][ichan] = init_mean[islot][ichan] + NSIGMAFIT*init_stddev[islot][ichan];
	init_gfit[islot][ichan]->SetRange(init_fr_low[islot][ichan], init_fr_high[islot][ichan]); 
	h_pped[islot][ichan]->Fit(Form("init_fit_slot_%d_chan_%d", islot, ichan), "QR");
	//h_pped[islot][ichan]->Fit(Form("init_fit_slot_%d_chan_%d", islot, ichan));
	init_gfit[islot][ichan]->Draw();
	// Declare and initialize the second and final fit
	gfit[islot][ichan] = new TF1(Form("iter_fit_slot_%d_chan_%d", islot, ichan), "gaus(0)");
	gfit[islot][ichan]->SetLineColor(4);
	// Acquire the fit parameters from the first fit and initialize the second fit
	iter_max[islot][ichan]    = init_gfit[islot][ichan]->GetParameter(0);
	iter_mean[islot][ichan]   = init_gfit[islot][ichan]->GetParameter(1);
	iter_stddev[islot][ichan] = init_gfit[islot][ichan]->GetParameter(2);
	gfit[islot][ichan]->SetParameters(iter_max[islot][ichan],
					  iter_mean[islot][ichan],
					  iter_stddev[islot][ichan]);
	gfit[islot][ichan]->SetParNames("Amp.", "#mu (ADC)", "#sigma (ADC)");
	// Fit over a mean +/- NSIGMAFIT*mean range
	fr_low[islot][ichan]  = iter_mean[islot][ichan] - NSIGMAFIT*iter_stddev[islot][ichan];
	fr_high[islot][ichan] = iter_mean[islot][ichan] + NSIGMAFIT*iter_stddev[islot][ichan];
	// cout << "MEAN = " << iter_mean[islot][ichan]
	//      << ", FR LOW = " << fr_low[islot][ichan]
	//      << ", FR HIGH = " << fr_high[islot][ichan] << endl;
	gfit[islot][ichan]->SetRange(fr_low[islot][ichan], fr_high[islot][ichan]); 
	// Perform the fit and store the parameters
	h_pped[islot][ichan]->Fit(Form("iter_fit_slot_%d_chan_%d", islot, ichan), "QR");
	//h_pped[islot][ichan]->Fit(Form("iter_fit_slot_%d_chan_%d", islot, ichan), "R");
	gfit[islot][ichan]->Draw("same");
	// Acquire the final fit paramters
	finl_max[islot][ichan]    = gfit[islot][ichan]->GetParameter(0);
	finl_mean[islot][ichan]   = gfit[islot][ichan]->GetParameter(1);
	finl_stddev[islot][ichan] = gfit[islot][ichan]->GetParameter(2);

	if ((NSIGMATHRESH * (UInt_t (finl_stddev[islot][ichan] + 0.5))) < nchan)
	  threshhold[islot][ichan] = UInt_t (finl_mean[islot][ichan] + 0.5) + nchan;
	else
	  threshhold[islot][ichan] = UInt_t (finl_mean[islot][ichan] + 0.5) + NSIGMATHRESH * (UInt_t (finl_stddev[islot][ichan] + 0.5));
	//threshhold[islot][ichan]  = UInt_t (finl_mean[islot][ichan] + 0.5) + nchan; // Handle rounding correctly
	//threshhold[islot][ichan]  = UInt_t (finl_mean[islot][ichan] + 0.5) + NSIGMATHRESH * (UInt_t (finl_stddev[islot][ichan] + 0.5)); // Handle rounding correctly
	// cout << "Slot = " << islot << ", Channel = " << ichan
	//      << ", Mean = " << finl_mean[islot][ichan]
	//      << ", Threshold = " << threshhold[islot][ichan] << endl;
	// Acquire the final fit parameter errors
	finl_max_err[islot][ichan]    = gfit[islot][ichan]->GetParError(0);
	finl_mean_err[islot][ichan]   = gfit[islot][ichan]->GetParError(1);
	finl_stddev_err[islot][ichan] = gfit[islot][ichan]->GetParError(2);

	// Write histos and fits to root output file
	h_pped[islot][ichan]->Write();
      }  // Histo object and nentries requirement
    }  // Channel loop
  }  // Slot loop

  //rof->Write();

  outputfile.open("thresholds.dat");
  for (UInt_t islot = SLOTMIN; islot < NUMSLOTS; islot++) {
    if (islot == 11 || islot == 12) continue;
    outputfile << "slot=" << islot << endl;
    for (UInt_t ichan = 0; ichan < NADCCHAN; ichan++)
      if (threshhold[islot][ichan] == 0)
	outputfile << 4095 << endl;
      else 
	outputfile << threshhold[islot][ichan] << endl;
  }  // Slot loop
  
}

