#include <string>
#include <cstring>
#include <iostream>
#include <cstdio>
#include "TObject.h"
#include "TH1.h"
#include "TF1.h"
#include "TH2.h"
#include "TTree.h"
#include "TList.h"
#include "TDatime.h"
#include "TDirectory.h"
#include "TVirtualPad.h"
#include "TCanvas.h"
#include "TLine.h"
#include "TFile.h"

#include "THaRun.h"
#include "THaApparatus.h"
#include "THaDetectorBase.h"

using namespace std;

// R.J. Feuerbach
// Jan 2003.

// operate on a file created for this purpose
// must contain the {R,L}.vdc.{u1,u2,v1,v2}.rawtime and .wire variables
// in the tree. 100k white-spectrum events should be enough
//
//
// Creates new db files in the current directory.
//
const Int_t NWire = 368;
const Int_t kNumWires = 368;
Int_t NENTRIES=(Int_t)1.e9;
const Int_t NWireGrp = 16;

const Int_t kBUFLEN = 150;


Bool_t QUIT = kFALSE;

Double_t *table_R=0;
Double_t *table_L=0;

Double_t *Build_T0(const char *HRS, TTree *tree);
Int_t Initialize_detectors(TDatime &run_time);
Int_t Find_TDC0(TH1 *hist, Double_t &t0);
Int_t Find_TDC0_2(TH1 *hist, Double_t &t0);
int SaveNewT0Data(const TDatime& run_date, Double_t* new_t0, const char* planename);

const char *vdc_planes[4] = {"vdc.u1","vdc.v1","vdc.u2","vdc.v2"};

TCanvas *c1;

Int_t Determine_VDC_T0(Int_t TDC_Low=-500, Int_t TDC_High=500) {

  const char *vdc_planes[4] = {"vdc.u1","vdc.v1","vdc.u2","vdc.v2"};

  // for each set of wires in the VDC's, determine their T0 offset
  Int_t ret;

  c1 = new TCanvas("c1");
  c1->cd();
  
  // First, make sure we have the required detectors initialized
  THaRun *run = (THaRun*)gDirectory->Get("Run_Data");
  if (!run) {
    cerr << "FAILED: No Run information to initialize Detectors!" << endl;
    ret = -1;
    return ret;
  }
  //  TDatime run_time = run->GetDate();

  TTree *T = (TTree*)gDirectory->Get("T");
  if (!T) {
    cerr << "FAILED: No tree to use!" << endl;
    ret = -2;
    return ret;
  }

  TFile *vdc_out = new TFile("VDC_out.root","RECREATE");
  
  table_R = Build_T0("R",T);
  table_L = Build_T0("L",T);
  
  TH1D *R_t0 = new TH1D("t0_R","T0 for Right arm (u1, v1, u2, v2)",4*NWire,0.,4*NWire);
  TH1D *L_t0 = new TH1D("t0_L","T0 for Left arm (u1, v1, u2, v2)",4*NWire,0.,4*NWire);

  for (Int_t i=0; i<4*NWire; i++) {
    R_t0->Fill(i,table_R[i]);
    L_t0->Fill(i,table_L[i]);
  }

  c1->cd();
  R_t0->Draw();
  TCanvas *c2 = new TCanvas("c2");
  c2->Draw();
  c2->cd();
  L_t0->Draw();
  c1->Update();
  c2->Update();

  char tmpchar;
  cerr << "Save new offsets? " << endl;
  cin >> tmpchar;
  
  if (tmpchar == 'y' || tmpchar == 'Y') {
    for (Int_t i=0; i<4; i++) {
      SaveNewT0Data(run->GetDate(),&(table_R[i*NWire]),Form("R.%s",vdc_planes[i]));
      SaveNewT0Data(run->GetDate(),&(table_L[i*NWire]),Form("L.%s",vdc_planes[i]));
    }
    cerr << " Offsets saved... to *.dat.NEW files in current directory" << endl;
  }

  vdc_out->Write();
  
  return 0;
}

Double_t *Build_T0(const char *HRS, TTree *T) {
  
  TH2F *tdcvw = new TH2F(Form("tdcvw_%s",HRS),"",NWire,0.,NWire,1000,1000,2000);
  tdcvw->GetXaxis()->SetTitle("Wire number");
  tdcvw->GetYaxis()->SetTitle("Raw TDC");

  int event_type=0;
  
  if (strncmp(HRS,"R",1)==0) event_type=1;
  if (strncmp(HRS,"L",1)==0) event_type=3;
  
  Double_t *t0_set = new Double_t[4*NWire];
  
  bool fail=kFALSE;
  char cut_string[80];
  char draw_string[200];

  Int_t sl, nbinsx;
  for (Int_t i=0; i<4 && !QUIT; i++) {
    TH2F *h;

    h = (TH2F*)tdcvw->Clone(Form("%s_%s",HRS,vdc_planes[i]));

    strncpy(cut_string,Form("D.evtype==%d||D.evtype>=5",event_type),80);
    strncpy(draw_string,Form("%s.%s.rawtime:%s.%s.wire>>%s",
			     HRS,vdc_planes[i],HRS,vdc_planes[i],h->GetName()),200);
    cerr << "Going to do :" << draw_string << endl;
    T->Draw(draw_string, cut_string, "", NENTRIES);
    nbinsx = h->GetNbinsX();
    
    Double_t t0;
    for (sl=0; sl<nbinsx && !QUIT; sl+=NWireGrp) {
      Int_t fret;
      TH1 *h_sl;
      
      cerr << "looking at channels " << sl << " to " << sl+NWireGrp-1 << endl;
      h_sl = h->ProjectionY(Form("%s_%d",h->GetName(),sl),sl+1,sl+NWireGrp,"err");
      if ( (fret=Find_TDC0_2(h_sl,t0)) == 0 ) {
	for (Int_t j=0; j<NWireGrp; j++) {
	  t0_set[i*NWire+sl+j] = t0;
	}
      } else {
	for (Int_t j=0; j<NWireGrp; j++) {
	  t0_set[i*NWire+sl+j] = fret;
	}
      }
      //      delete h_sl;
    }
    // look through this plane, and identify those wires for which we couldn't
    // get an offset. Look for nearest group that does have a reasonable value
    // and copy it over
    for (sl=0; sl<nbinsx; sl+=NWireGrp) {
      if (t0_set[i*NWire+sl] <= 0) {
	// look for a good match
	Int_t sltry=sl;
	Double_t t0Lo=0, t0Hi=0;
	Int_t slgood;

	while ( sltry>=0    && (t0Lo = t0_set[i*NWire+sltry])<=0 ) sltry-=NWireGrp;
	if (t0Lo>0) slgood = sltry;
	while ( sltry<NWire && (t0Hi = t0_set[i*NWire+sltry])<=0 ) sltry+=NWireGrp;

	if (t0Hi<=0 && t0Lo<=0) {       // could not find match
	  slgood = sl;
	} else if (t0Hi>0 &&            // the higher sets are useful
		   ( t0Lo<=0 || (sl-slgood > sltry-sl) ) ) {
	  slgood = sltry;
	}

	t0 = t0_set[i*NWire+slgood];
	for (Int_t j=0; j<NWireGrp; j++) {
	  t0_set[i*NWire+sl+j] = t0;
	}
      }
    }
  }
  return t0_set;
}

Int_t Initialize_detectors(TDatime &run_time) {
  // taken to be like TAnalyzer's initialization
  extern TList *gHaApps;
  TIter next(gHaApps);
  TObject *obj;
  Int_t ret=0;
  bool fail=kFALSE;
  while ( !fail && (obj = next() ) ) {
    if ( !obj->IsA()->InheritsFrom("THaApparatus")) {
      cerr << "FAILED: Apparatus " << obj->GetName() << " is not a THaApparatus." << endl;
      ret = -20;
      fail = kTRUE;
    } else {
      THaApparatus *theApp = static_cast<THaApparatus*>(obj);
      theApp->Init(run_time);
      if ( !theApp->IsOK() ) {
	cerr << "FAILED: Apparatus " << obj->GetName() << " is not OKAY." << endl;
	ret = -21;
	fail = kTRUE;
      }
    }
  }
  return ret;
}

Int_t Find_TDC0(TH1 *hist, Double_t &t0) {
  // look through histogram, find highest peak, and follow down to
  // noise "grass"
  Int_t ret=0;
  Int_t nbins = hist->GetNbinsX();
  Int_t maxb = 0;
  Double_t max_cont = -1.;
  while ( max_cont < 50. && (hist->GetBinWidth(2)+.5) <4) {
    hist->Rebin(2);
    max_cont=-1.;
    for (Int_t i=1; i<=nbins; i++) {
      if ( hist->GetBinContent(i) > max_cont ) {
	maxb = i;
	max_cont = hist->GetBinContent(maxb);
      }
    }
  }
  
  //  cerr << "Have " << max_cont << " at maximum in the bin." << endl;
  if (max_cont<20) {
    cerr << "FAILED: Need more statistics (only " << max_cont << " at max) to reliably find T0" << endl;
    ret = -12;
    return ret;
  }

  // find half-way point down the slope
  Int_t downhalf=0;
  for (Int_t i=maxb; i<=nbins; i++) {
    if (hist->GetBinContent(i)<.5*max_cont) {
      downhalf = i;
      break;
    }
  }

  Int_t dbin = downhalf-maxb;
  if (dbin<0 || (maxb+4*dbin)>nbins) {
    cerr << "FAILED: Upper range of histogram is too close to peak" << endl;
    ret = -13;
    return ret;
  }
  
  // estimate level of the "grass", so we aren't fooled by it
  Double_t grass = hist->Integral(maxb+2*dbin,maxb+4*dbin)/(2*dbin);
  
  Int_t grassb=0;
  for (Int_t i=downhalf; i<nbins; i++) {
    if (hist->GetBinContent(i)<=grass) {
      grassb=i;
      break;
    }
  }
  dbin = TMath::Max(grassb-downhalf,4);
  
  hist->GetXaxis()->SetRange((Int_t)(downhalf-.5*dbin),(Int_t)(downhalf+.5*dbin));
  hist->Fit("pol1");
  
  hist->SetAxisRange(1700.,2000.);
  hist->Draw();
  c1->Update();
  cerr << '\t' << "binsize = " << hist->GetBinWidth(2) << endl;
  cerr << '\t' << "maxb = " << maxb << "    max_cont = " << max_cont << endl;
  cerr << '\t' << "downhalf = " << downhalf << endl;
  cerr << '\t' << "grassb = " << grassb << "    grass level = " << grass << endl;
  cerr << '\t' << "dbin = " << dbin << endl;
  TF1 *ffunc = hist->GetFunction("pol1");
  if (!ffunc) {
    cerr << "FAILED: Fit failed" << endl;
    ret = -14;
  }
  if (ffunc->GetParameter(1)>=0.) {
    cerr << "FAILED: Zero or Positive slope???" << endl;
    ret = -15;
  }

  cerr << " **** NEXT? " << endl;
  char tmpc;
  cin >> tmpc;
  if (tmpc=='Q' || tmpc=='q') QUIT=kTRUE;
  
  if (ret) return ret;
  
  t0 = -ffunc->GetParameter(0)/ffunc->GetParameter(1);
  return 0;
}


Int_t Find_TDC0_2(TH1 *hist, Double_t &t0) {
  static Bool_t stop_and_wait = kTRUE;
  // look through histogram, smooth it, and then
  // look for the point of steepest descent

  Int_t ret=0;

  while ( (hist->GetBinWidth(2)+.5) <4) {
    hist->Rebin(2);
  }

  hist->Smooth();
  
  Int_t nbins = hist->GetNbinsX();

  // create utility histos, showing the collection of slopes
  TH1 *hist_sl = (TH1*)hist->Clone(Form("%s_sl",hist->GetName()));
  hist_sl->Reset();
  
  Double_t dx = hist->GetBinWidth(2);
  Double_t x;
  Double_t slope=0;
  Double_t slope_min=0.;
  Int_t ch_save=0;

  // start at the bin with the maximum content
  Int_t maxbin = hist->GetMaximumBin();
  for (Int_t i=maxbin+1; i<=nbins; i++) {
    if (i>1 && i<nbins) {
      slope = (hist->GetBinContent(i+1)-hist->GetBinContent(i-1))/(2.*dx);
    } else if (i==1) {
      slope = (hist->GetBinContent(i+1)-hist->GetBinContent(i))/(dx);
    } else if (i==nbins) {
      slope = (hist->GetBinContent(i)-hist->GetBinContent(i-1))/(dx);
    }

    if (TMath::Abs(slope)<1.e-4) {
      slope = 0.;
    }
    x = hist->GetBinCenter(i);
    hist_sl->Fill(x,slope);
    if (slope < slope_min) {
      slope_min = slope;
      ch_save = i;
    }
  }
  
  if (slope_min < -1. && hist->GetEntries()>1000. ) {
    t0 = hist->GetBinCenter(ch_save) - hist->GetBinContent(ch_save)/slope_min;
    cerr << "  * Slope found to be " << slope_min;
    cerr << " at " << hist->GetBinCenter(ch_save);
    cerr << ", pop. = " << hist->GetBinContent(ch_save) << endl;
    cerr << "  * t0 is then " << t0 << endl;
  } else {
    t0 = 0.;
    cerr << "FAILED: Zero of Positive slope???" << endl;
    
    ret = -16;
  }

  hist->SetAxisRange(1700.,2000.);
  hist->Draw();
  TLine line;
  line.SetLineColor(2);
  line.DrawLine(t0,0.,t0,hist->GetMaximum());
  c1->Update();
  
  if (stop_and_wait) {
    cerr << " **** NEXT? (q to quit, c for continuous) " << endl;
    char tmpc;
    
    cin >> tmpc;
    if (tmpc=='Q' || tmpc=='q') QUIT=kTRUE;
    if (tmpc=='C' || tmpc=='c') stop_and_wait=kFALSE;
  }
  
  if (ret) return ret;
  
  return 0;
}

void show_hists() {
  TIter nexth(gDirectory->GetList());
  TObject *obj;
  char tmpch;
  while ( obj = nexth() ) {
    if (obj->InheritsFrom("TH1D")) {
      ((TH1D*)(obj))->Draw();
      gPad->Update();
      cout << "Next: " << flush;
      cin >> tmpch;
      if (tmpch == 'q' || tmpch == 'Q') break;
    }
  }
}

// following routine modified from dobbs's scripts/calct0table.C
int SaveNewT0Data(const TDatime &run_date, Double_t *new_t0, const char *planename)
{
  char buff[kBUFLEN], db_filename[kBUFLEN], tag[kBUFLEN];
  
  sprintf(db_filename, "%c.vdc.", planename[0]);
  sprintf(buff,"db_%sdat.NEW",db_filename);
  FILE *db_file = 0;
  FILE *db_out = fopen(buff,"r+"); // a new DB file to copy to the final home
  if (db_out) db_file = db_out;
  else {
    db_file = THaDetectorBase::OpenFile(db_filename, run_date, 
					"OpenFile()", "r+");
    db_out = fopen(buff,"w+");
  }
    
  if (!db_out) {
    fprintf(stderr,"Cannot open %s for output\n",buff);
    return kFALSE;
  }
  
  // Build the search tag and find it in the file. Search tags
  // are of form [ <prefix> ], e.g. [ R.vdc.u1 ].
  sprintf(tag, "[ %s ]", planename);
  bool found = false;
  while (!found && fgets (buff, kBUFLEN, db_file) != NULL) {
    if (db_out != db_file) fputs(buff, db_out);
    if(strlen(buff) > 0 && buff[strlen(buff)-1] == '\n')
      buff[strlen(buff)-1] = '\0';

    if(strcmp(buff, tag) == 0)
      found = true;
  }
  if( !found ) {
    cerr<<"Database entry "<<tag<<" not found!"<<endl;;
    return kFALSE;
  }
  
  // read in other info -- skip 7 lines
  for(int i=0; i<7; ++i) {
    fgets(buff, kBUFLEN, db_file);
    if (db_out != db_file) fputs(buff, db_out);
  }
  
  // save the new t0 values
  for(int i=1; i<=kNumWires; ++i) {
    // if we have an offset <= 0, then something went wrong in
    // our calculations, so we don't want to overwrite anything
    if(new_t0[i-1] <= 0.0) {
      int oldi;
      float oldf;
      char endc;

      fseek(db_file, 0L, SEEK_CUR); // synchronized
      fscanf(db_file, "%d %f%c", &oldi, &oldf, &endc);
      if (db_out != db_file) fprintf(db_out,"%4d %5.1f%c", oldi, oldf, endc);
      continue;
    }
    if (db_out != db_file) fscanf(db_file,"%*d %*f%*c");
    fseek(db_out, 0L, SEEK_CUR); // be certain we are synchronized
    fprintf(db_out, "%4d %5.1f", i, new_t0[i-1]);

    if(i%8 == 0)
      fprintf(db_out, "\n");
    else
      fprintf(db_out, " ");
  }

  fseek(db_file, 0L, SEEK_CUR); // synchronized
  // run through the rest of the file
  while ( fgets (buff, kBUFLEN, db_file) != NULL) {
    if (db_out != db_file) fputs(buff, db_out);
  }

  fclose(db_file);
  if (db_file != db_out) fclose(db_out);
  
  return kTRUE;
}

