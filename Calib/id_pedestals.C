#include "TTree.h"
#include "TCanvas.h"
#include "TH2.h"
#include "TH1.h"
#include "TDirectory.h"
#include "TAxis.h"
#include "TF1.h"
#include "stdio.h"
#include "iostream.h"

using namespace std;

void id_pedestals(TTree *T, const char* expr, int minch, int maxch,
		  Double_t goal=500., Double_t MaxADC=1000., Double_t MinADC=10.) {
  Int_t nevents = 50000;

  TCanvas *c1 = new TCanvas("c1");
  if (T->GetEntries() < nevents) nevents = T->GetEntries();
  
  // identify location of pedestals for pedestal-suppressed data
  // going through it the long way (draw each item separately)
  char sele[100], cuts[100];
  int nchan = maxch-minch+1;
  Double_t *pedestals = new Double_t[nchan];
  Double_t *coeff = new Double_t[nchan];
  memset(pedestals,0,(nchan)*sizeof(pedestals[0]));
  TH1F ahist("ahist","ADC dist",1000,0.,1000.); // pedestal is less than 1000
  TH1F aphist("aphist","ADC dist",500,0.,MaxADC); // above pedestal

  // is pedestal suppression OFF?
  int ped_sup = 1;  // pedestal suppression assumed to be on
  sprintf(sele,Form("%s>>ahist",expr),minch);
  sprintf(cuts,Form("%s>0",expr),minch);
  T->Draw(sele,cuts,"",nevents);

  Int_t maxb, minb;
  Double_t maxx, mbinc;
  
  for (int i=0; i<nchan; i++) {
    cerr << "Looking at channel " << i;
    ahist.Reset();
    ahist.GetXaxis()->SetRange(0,0);
    sprintf(sele,expr,i+minch);
    sprintf(cuts,"%s>0",sele);
    T->Project("ahist",sele,cuts,"",nevents);
    if ( ahist.GetEntries() > .8*nevents )  // too many entries for honest pedestal sup
      ped_sup = 0;
    else
      ped_sup = 1;
    
    if ( ped_sup ) {
      for (int j=1; j<=1000; j++) {
	if ( ahist.GetBinContent(j)>0 && ahist.GetBinContent(j+1)>0 &&
	     ahist.GetBinContent(j+2)>0 ) {
	  pedestals[i] = ahist.GetBinLowEdge(j+1);
	  break;
	}
      }
    } else { // pedestal suppression off, get maximum bin
      pedestals[i] = ahist.GetBinLowEdge(ahist.GetMaximumBin()+1)+10;
    }
    //    cerr << " pedestal at " << pedestals[i];
    // get estimate of width of pedestal
    ahist.SetAxisRange(pedestals[i]-1.,pedestals[i]+20);
    Double_t ped_w = ahist.GetRMS();

    // find local minimum between pedestal and first peak, to make
    // certain we can find our way off the pedestal's tail
    minb=ahist.FindBin(pedestals[i])+1;
    Double_t minc = ahist.GetBinContent(minb);
    ped_w = 3*ped_w/ahist.GetBinWidth(1)+10; // to bin count, minimum of 4 bins
    //    cerr << " pedw " << ped_w;
    Int_t bin = minb+1;
    while ( bin<ahist.GetNbinsX() && bin-minb < ped_w ) {
      if (ahist.GetBinContent(bin) <= minc) {
	minb = bin;
	minc = ahist.GetBinContent(minb);
      }
      bin++;
    }
    //    if (bin >= ahist.GetNbinsX()) minb = ahist.FindBin(pedestals[i])+ped_w;
    Double_t minx = ahist.GetBinLowEdge(minb+1)-pedestals[i];

    // Enforce a minimum distance from the pedestal
    if (MinADC > minx) {
      minx = MinADC;
    }

    // now, a lower-resolution histogram to find the peak signal peak
    aphist.Reset();
    Int_t nbinsx = aphist.GetNbinsX();
    aphist.GetXaxis()->SetRange(0,0);
    
    sprintf(sele,Form("(%s-%f)",expr,pedestals[i]),i+minch);
    T->Project("aphist",sele,"","",nevents);
    aphist.Smooth();

    // set minb to be appropriate for aphist now
    minb = aphist.FindBin(minx);
    
    //    cerr << "  pedestal stops at val " << minx << ' ' << minb;

    aphist.SetAxisRange(minx,MaxADC);
    // now should (hopefully) be above the pedestal tail
    maxb = aphist.GetMaximumBin();
    if (maxb == minb) {
      cerr << "MINB == MAXB !?!?!?";
    }
    //    cerr << " maxb " << maxb;
    maxx = aphist.GetBinCenter(maxb);
    //    cerr << " maxx " << maxx;
    mbinc = aphist.GetBinContent(maxb);
    if (mbinc <=0) {
      coeff[i] = 1.;
      continue;
    } else {
      //      aphist.SetAxisRange(minx,maxx+(maxx-minx)); // symmetric about peak
      Double_t	rms = aphist.GetRMS();
      Double_t
	rng1 = maxx-rms,
	rng2 = maxx+rms;
      
      cerr << " max in ( " << rng1 << " , " <<  rng2 << " ) ";
      aphist.Fit("gaus","RQ","",rng1,rng2);
      c1->Update();
      Double_t mean = aphist.GetFunction("gaus")->GetParameter(1);
      cerr << " : " << mean;
      if ( mean > 0 )
	coeff[i] = goal/mean;
      else
	coeff[i] = 1.;
      char tmpch(' ');
//        cout << "next? ";
//        cin >> tmpch;
      if ( tmpch == 'q' ) {
	TCanvas *c2 = new TCanvas("c2");
	c2->cd();
	ahist.DrawCopy()->SetDirectory(gDirectory);
	c1->cd();
	aphist.DrawCopy()->SetDirectory(gDirectory);
	return;
      }
    }
    cerr << endl;
  }
  
  printf("Pedestals for channels %d through %d are:\n",minch,maxch);
  int i;
  for (i=0; i<nchan; i++) {
    printf("  %5.0f",pedestals[i]);
    if (i % 10 == 9) printf("\n");
  }
  if (i % 10 != 0) printf("\n");
  
  printf("Amplitude coeff average of %.0f are:\n",goal);
  for (i=0; i<nchan; i++) {
    printf(" %7.3f",coeff[i]);
    if (i % 10 == 9) printf("\n");
  }
  if ( i % 10 != 0) printf("\n");
  
  // make summary 'calibrated' histogram
  TH2F *calib_h = new TH2F("calib_h",Form("Calibration of %s",expr),100,0.,4*goal,
			   nchan,minch,maxch+1);
  for (int i=0; i<nchan; i++) {
    sprintf(sele,Form("%%d:(%s-%f)*%f>>+calib_h",expr,pedestals[i],coeff[i]),minch+i,minch+i);
    
    T->Draw(sele,"","",nevents);
  }
  Double_t maxz = 4*calib_h->Integral(10,50,1,nchan)/(40*nchan);
  calib_h->SetMaximum(maxz);
  calib_h->Draw("colz");
}
