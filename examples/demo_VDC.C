#define demo_VDC_cxx
#include "demo_VDC.h"
#include "TH2.h"
#include "TStyle.h"
#include "TCanvas.h"

void demo_VDC::Loop()
{
//   In a Root session, you can do:
//      Root > .L demo_VDC.C
//      Root > demo_VDC t
//      Root > t.GetEntry(12); // Fill t data members with entry number 12
//      Root > t.Show();       // Show values of entry 12
//      Root > t.Show(16);     // Read and show values of entry 16
//      Root > t.Loop();       // Loop on all entries
//

//     This is the loop skeleton
//       To read only selected branches, Insert statements like:
// METHOD1:
//    fChain->SetBranchStatus("*",0);  // disable all branches
//    fChain->SetBranchStatus("branchname",1);  // activate branchname
// METHOD2: replace line
//    fChain->GetEntry(i);  // read all branches
//by  b_branchname->GetEntry(i); //read only this branch

  if (fChain == 0) return;

  // Number of hits on a plane, including all hits in case of multihit:
  TH1F *nhitu1 = new TH1F("nhitu1","U1: Number of hits (including multihits)",
			  50,0,50);
  TH1F *nhitv1 = new TH1F("nhitv1","V1: Number of hits (including multihits)",
			  50,0,50);
  TH1F *nhitu2 = new TH1F("nhitu2","U2: Number of hits (including multihits)",
			  50,0,50);
  TH1F *nhitv2 = new TH1F("nhitv2","V2: Number of hits (including multihits)",
			  50,0,50);

  // Number of fired wires on a plane:
  TH1F *nwiru1 = new TH1F("nwiru1","U1: Number of fired wires", 50,0,50);
  TH1F *nwirv1 = new TH1F("nwirv1","V1: Number of fired wires", 50,0,50);
  TH1F *nwiru2 = new TH1F("nwiru2","U2: Number of fired wires", 50,0,50);
  TH1F *nwirv2 = new TH1F("nwirv2","V2: Number of fired wires", 50,0,50);

// Fired wires numbers:
  TH1F *wirsu1 = new TH1F("wirsu1","U1: Fired wires numbers", 369,0,369);
  TH1F *wirsv1 = new TH1F("wirsv1","V1: Fired wires numbers", 369,0,369);
  TH1F *wirsu2 = new TH1F("wirsu2","U2: Fired wires numbers", 369,0,369);
  TH1F *wirsv2 = new TH1F("wirsv2","V2: Fired wires numbers", 369,0,369);

// TDC times - in case of multihit only last hit is considered:
  TH1F *timsu1 = new TH1F("timsu1","U1: Last hit TDCs", 250,0,2500);
  TH1F *timsv1 = new TH1F("timsv1","V1: Last hit TDCs", 250,0,2500);
  TH1F *timsu2 = new TH1F("timsu2","U2: Last hit TDCs", 250,0,2500);
  TH1F *timsv2 = new TH1F("timsv2","V2: Last hit TDCs", 250,0,2500);

// Number of reconstructed clusters:
  TH1F *ncluu1 = new TH1F("ncluu1","U1: Number of clusters", 10,0,10);
  TH1F *ncluv1 = new TH1F("ncluv1","V1: Number of clusters", 10,0,10);
  TH1F *ncluu2 = new TH1F("ncluu2","U2: Number of clusters", 10,0,10);
  TH1F *ncluv2 = new TH1F("ncluv2","V2: Number of clusters", 10,0,10);

// Center of cluster (in wires), including all clusters in case of multiclust.:
  TH1F *clpsu1 = new TH1F("clpsu1","U1: Centers of all clusters", 369,0,369);
  TH1F *clpsv1 = new TH1F("clpsv1","V1: Centers of all clusters", 369,0,369);
  TH1F *clpsu2 = new TH1F("clpsu2","U2: Centers of all clusters", 369,0,369);
  TH1F *clpsv2 = new TH1F("clpsv2","V2: Centers of all clusters", 369,0,369);

// Size of cluster (in wires), including all clusters in case of multiclusters:
  TH1F *clszu1 = new TH1F("clszu1","U1: Size of all clusters", 15,0,15);
  TH1F *clszv1 = new TH1F("clszv1","V1: Size of all clusters", 15,0,15);
  TH1F *clszu2 = new TH1F("clszu2","U2: Size of all clusters", 15,0,15);
  TH1F *clszv2 = new TH1F("clszv2","V2: Size of all clusters", 15,0,15);

// Start main loop over all events: ==========================================

  Int_t nentries = Int_t(fChain->GetEntries());
  printf(" nevent = %d\n", nentries);
     
  int nwire, wires[51], times[51];
  Axis_t x;

  Int_t nbytes = 0, nb = 0;
  for (Int_t nev=0; nev<nentries; nev++) {
    Int_t ientry = LoadTree(nev);
    nb = fChain->GetEntry(nev);   
    nbytes += nb;
    if (Cut(ientry) < 0) continue;

    if( (nev+1)%1000 == 0 )
      printf("==== Event: %d ======\n", nev+1 ); 

    int i, nw;

    nwire = 0;    nw = 0;              // U1: ---------------------------------
    for ( i = 0; i < fR_U1_nhit; i++ ) {
      wires[nwire] = fR_U1_wire[i];
      times[nwire] = fR_U1_time[i];
      if ( nw != fR_U1_wire[i]) nwire++;
      nw = fR_U1_wire[i];
    }
    x = fR_U1_nhit;      nhitu1->Fill(x); // Number of hits, including multi
    x = nwire;           nwiru1->Fill(x); // Number of fired wires
    for ( i = 0; i < nwire; i++ ) {
      x = wires[i];        wirsu1->Fill(x); // Fired wires numbers
      x = times[i];        timsu1->Fill(x); // TDCs: only last hits if multihit
    }
    x = fR_U1_nclust;    ncluu1->Fill(x); // Number of clusters:
    for ( i = 0; i < fR_U1_nclust; i++ ) {
      x = fR_U1_clpos[i];  clpsu1->Fill(x); // Center of all clusters(in wires)
      x = fR_U1_clsiz[i];  clszu1->Fill(x); // Size of all clusters (in wires)
    }
    if ( nev<3 ) {
      printf("==== Event: %d ======\n", nev+1 ); 
      printf("fR_U1_nhit = %f\n", fR_U1_nhit);
      for ( i=0; i<fR_U1_nhit; i++) 
	printf(" i = %2d   fR_U1_wire[i] = %f   fR_U1_time[i] = %f\n",
	       i,        fR_U1_wire[i],       fR_U1_time[i] );
      printf("nwire = %d  (Only last hits selected)\n", nwire);
      for ( i=0; i<nwire; i++) 
	printf(" i = %2d   wires[i] = %d   times[i] = %d\n",
	       i,        wires[i],       times[i] );
      printf("fR_U1_nclust = %f\n", fR_U1_nclust);
      for ( i=0; i<fR_U1_nclust; i++) 
	printf(" i = %2d   fR_U1_clpos[i] = %f   fR_U1_clsiz[i] = %f\n",
	       i,        fR_U1_clpos[i],       fR_U1_clsiz[i] );
    }
    nwire = 0;    nw = 0;              // V1: ---------------------------------
    for ( i = 0; i < fR_V1_nhit; i++ ) {
      wires[nwire] = fR_V1_wire[i];
      times[nwire] = fR_V1_time[i];
      if ( nw != fR_V1_wire[i]) nwire++;
      nw = fR_V1_wire[i];
    }
    x = fR_V1_nhit;      nhitv1->Fill(x); // Number of hits, including multi
    x = nwire;           nwirv1->Fill(x); // Number of fired wires
    for ( i = 0; i < nwire; i++ ) {
      x = wires[i];        wirsv1->Fill(x); // Fired wires numbers
      x = times[i];        timsv1->Fill(x); // TDCs: only last hits if multihit
    }
    x = fR_V1_nclust;    ncluv1->Fill(x); // Number of clusters:
    for ( i = 0; i < fR_V1_nclust; i++ ) {
      x = fR_V1_clpos[i];  clpsv1->Fill(x); // Center of all clusters(in wires)
      x = fR_V1_clsiz[i];  clszv1->Fill(x); // Size of all clusters (in wires)
    }
    if ( nev<3 ) {
      printf("fR_V1_nhit = %f\n", fR_V1_nhit);
      for ( i=0; i<fR_V1_nhit; i++) 
	printf(" i = %2d   fR_V1_wire[i] = %f   fR_V1_time[i] = %f\n",
	       i,        fR_V1_wire[i],       fR_V1_time[i] );
      printf("nwire = %d  (Only last hits selected)\n", nwire);
      for ( i=0; i<nwire; i++) 
	printf(" i = %2d   wires[i] = %d   times[i] = %d\n",
	       i,        wires[i],       times[i] );
      printf("fR_V1_nclust = %f\n", fR_V1_nclust);
      for ( i=0; i<fR_V1_nclust; i++) 
	printf(" i = %2d   fR_V1_clpos[i] = %f   fR_V1_clsiz[i] = %f\n",
	       i,        fR_V1_clpos[i],       fR_V1_clsiz[i] );
    }
    nwire = 0;    nw = 0;              // U2: ---------------------------------
    for ( i = 0; i < fR_U2_nhit; i++ ) {
      wires[nwire] = fR_U2_wire[i];
      times[nwire] = fR_U2_time[i];
      if ( nw != fR_U2_wire[i]) nwire++;
      nw = fR_U2_wire[i];
    }
    x = fR_U2_nhit;      nhitu2->Fill(x); // Number of hits, including multi
    x = nwire;           nwiru2->Fill(x); // Number of fired wires
    for ( i = 0; i < nwire; i++ ) {
      x = wires[i];        wirsu2->Fill(x); // Fired wires numbers
      x = times[i];        timsu2->Fill(x); // TDCs: only last hits if multihit
    }
    x = fR_U2_nclust;    ncluu2->Fill(x); // Number of clusters:
    for ( i = 0; i < fR_U2_nclust; i++ ) {
      x = fR_U2_clpos[i];  clpsu2->Fill(x); // Center of all clusters(in wires)
      x = fR_U2_clsiz[i];  clszu2->Fill(x); // Size of all clusters (in wires)
    }
    if ( nev<3 ) {
      printf("fR_U2_nhit = %f\n", fR_U2_nhit);
      for ( i=0; i<fR_U2_nhit; i++) 
	printf(" i = %2d   fR_U2_wire[i] = %f   fR_U2_time[i] = %f\n",
	       i,        fR_U2_wire[i],       fR_U2_time[i] );
      printf("nwire = %d  (Only last hits selected)\n", nwire);
      for ( i=0; i<nwire; i++) 
	printf(" i = %2d   wires[i] = %d   times[i] = %d\n",
	       i,        wires[i],       times[i] );
      printf("fR_U2_nclust = %f\n", fR_U2_nclust);
      for ( i=0; i<fR_U2_nclust; i++) 
	printf(" i = %2d   fR_U2_clpos[i] = %f   fR_U2_clsiz[i] = %f\n",
	       i,        fR_U2_clpos[i],       fR_U2_clsiz[i] );
    }
    nwire = 0;    nw = 0;              // V2: ---------------------------------
    for ( i = 0; i < fR_V2_nhit; i++ ) {
      wires[nwire] = fR_V2_wire[i];
      times[nwire] = fR_V2_time[i];
      if ( nw != fR_V2_wire[i]) nwire++;
      nw = fR_V2_wire[i];
    }
    x = fR_V2_nhit;      nhitv2->Fill(x); // Number of hits, including multi
    x = nwire;           nwirv2->Fill(x); // Number of fired wires
    for ( i = 0; i < nwire; i++ ) {
      x = wires[i];        wirsv2->Fill(x); // Fired wires numbers
      x = times[i];        timsv2->Fill(x); // TDCs: only last hits if multihit
    }
    x = fR_V2_nclust;    ncluv2->Fill(x); // Number of clusters:
    for ( i = 0; i < fR_V2_nclust; i++ ) {
      x = fR_V2_clpos[i];  clpsv2->Fill(x); // Center of all clusters(in wires)
      x = fR_V2_clsiz[i];  clszv2->Fill(x); // Size of all clusters (in wires)
    }
    if ( nev<3 ) {
      printf("fR_V2_nhit = %f\n", fR_V2_nhit);
      for ( i=0; i<fR_V2_nhit; i++) 
	printf(" i = %2d   fR_V2_wire[i] = %f   fR_V2_time[i] = %f\n",
	       i,        fR_V2_wire[i],       fR_V2_time[i] );
      printf("nwire = %d  (Only last hits selected)\n", nwire);
      for ( i=0; i<nwire; i++) 
	printf(" i = %2d   wires[i] = %d   times[i] = %d\n",
	       i,        wires[i],       times[i] );
      printf("fR_V2_nclust = %f\n", fR_V2_nclust);
      for ( i=0; i<fR_V2_nclust; i++) 
	printf(" i = %2d   fR_V2_clpos[i] = %f   fR_V2_clsiz[i] = %f\n",
	       i,        fR_V2_clpos[i],       fR_V2_clsiz[i] );
    }

  }
  printf("entries = T->GetEntries() = %d,  nbytes = %d\n", 
	 T->GetEntries(),                 nbytes);

  // Draw the histograms: =======================================================
  // Number of hits on a plane, including all hits in case of multihit: ---------
  TCanvas *cnhit = new TCanvas("cnhit", "Number of hits (including multihits)");
  TPad *pad1 = new TPad("pad1","This is pad1",0.03,0.52,0.48,0.97);
  TPad *pad2 = new TPad("pad2","This is pad2",0.52,0.52,0.97,0.97);
  TPad *pad3 = new TPad("pad3","This is pad3",0.03,0.03,0.48,0.48);
  TPad *pad4 = new TPad("pad4","This is pad4",0.52,0.03,0.97,0.48);
  pad1->Draw();
  pad2->Draw();
  pad3->Draw();
  pad4->Draw();
  pad1->cd();                          // Go to Pad1.
  nhitu1->Draw();                      // Number of hits on U1.
  pad2->cd();                          // Go to Pad2.
  nhitv1->Draw();                      // Number of hits on V1.
  pad3->cd();                          // Go to Pad3.
  nhitu2->Draw();                      // Number of hits on U2.
  pad4->cd();                          // Go to Pad4.
  nhitv2->Draw();                      // Number of hits on V2.
  cnhit->Update();
  // Number of fired wires on a plane: ------------------------------------------
  TCanvas *cnwir = new TCanvas("cnwir", "Number of fired wires");
  TPad *pad1 = new TPad("pad1","This is pad1",0.03,0.52,0.48,0.97);
  TPad *pad2 = new TPad("pad2","This is pad2",0.52,0.52,0.97,0.97);
  TPad *pad3 = new TPad("pad3","This is pad3",0.03,0.03,0.48,0.48);
  TPad *pad4 = new TPad("pad4","This is pad4",0.52,0.03,0.97,0.48);
  pad1->Draw();
  pad2->Draw();
  pad3->Draw();
  pad4->Draw();
  pad1->cd();
  nwiru1->Draw();
  pad2->cd();
  nwirv1->Draw();
  pad3->cd();
  nwiru2->Draw();
  pad4->cd();
  nwirv2->Draw();
  cnwir->Update();
  // Fired wires numbers: -------------------------------------------------------
  TCanvas *cwirs = new TCanvas("cwirs", "Fired wires numbers");
  TPad *pad1 = new TPad("pad1","This is pad1",0.03,0.52,0.48,0.97);
  TPad *pad2 = new TPad("pad2","This is pad2",0.52,0.52,0.97,0.97);
  TPad *pad3 = new TPad("pad3","This is pad3",0.03,0.03,0.48,0.48);
  TPad *pad4 = new TPad("pad4","This is pad4",0.52,0.03,0.97,0.48);
  pad1->Draw();
  pad2->Draw();
  pad3->Draw();
  pad4->Draw();
  pad1->cd();
  wirsu1->Draw();
  pad2->cd();
  wirsv1->Draw();
  pad3->cd();
  wirsu2->Draw();
  pad4->cd();
  wirsv2->Draw();
  cwirs->Update();
  // TDC times - in case of multihit only last hit is considered: ---------------
  TCanvas *ctims = new TCanvas("ctims", "Last hit TDCs");
  TPad *pad1 = new TPad("pad1","This is pad1",0.03,0.52,0.48,0.97);
  TPad *pad2 = new TPad("pad2","This is pad2",0.52,0.52,0.97,0.97);
  TPad *pad3 = new TPad("pad3","This is pad3",0.03,0.03,0.48,0.48);
  TPad *pad4 = new TPad("pad4","This is pad4",0.52,0.03,0.97,0.48);
  pad1->Draw();
  pad2->Draw();
  pad3->Draw();
  pad4->Draw();
  pad1->cd();
  timsu1->Draw();
  pad2->cd();
  timsv1->Draw();
  pad3->cd();
  timsu2->Draw();
  pad4->cd();
  timsv2->Draw();
  ctims->Update();
  // Number of reconstructed clusters: ------------------------------------------
  TCanvas *cnclu = new TCanvas("cnclu", "Number of clusters");
  TPad *pad1 = new TPad("pad1","This is pad1",0.03,0.52,0.48,0.97);
  TPad *pad2 = new TPad("pad2","This is pad2",0.52,0.52,0.97,0.97);
  TPad *pad3 = new TPad("pad3","This is pad3",0.03,0.03,0.48,0.48);
  TPad *pad4 = new TPad("pad4","This is pad4",0.52,0.03,0.97,0.48);
  pad1->Draw();
  pad2->Draw();
  pad3->Draw();
  pad4->Draw();
  pad1->cd();
  ncluu1->Draw();
  pad2->cd();
  ncluv1->Draw();
  pad3->cd();
  ncluu2->Draw();
  pad4->cd();
  ncluv2->Draw();
  cnclu->Update();
  // Center of cluster (in wires), including all clusters if multiclust: --------
  TCanvas *cclps = new TCanvas("cclps", "Centers of all clusters");
  TPad *pad1 = new TPad("pad1","This is pad1",0.03,0.52,0.48,0.97);
  TPad *pad2 = new TPad("pad2","This is pad2",0.52,0.52,0.97,0.97);
  TPad *pad3 = new TPad("pad3","This is pad3",0.03,0.03,0.48,0.48);
  TPad *pad4 = new TPad("pad4","This is pad4",0.52,0.03,0.97,0.48);
  pad1->Draw();
  pad2->Draw();
  pad3->Draw();
  pad4->Draw();
  pad1->cd();
  clpsu1->Draw();
  pad2->cd();
  clpsv1->Draw();
  pad3->cd();
  clpsu2->Draw();
  pad4->cd();
  clpsv2->Draw();
  cclps->Update();
  // Size of cluster (in wires), including all clusters if multiclusters: -------
  TCanvas *cclsz = new TCanvas("cclsz", "Size of all clusters");
  TPad *pad1 = new TPad("pad1","This is pad1",0.03,0.52,0.48,0.97);
  TPad *pad2 = new TPad("pad2","This is pad2",0.52,0.52,0.97,0.97);
  TPad *pad3 = new TPad("pad3","This is pad3",0.03,0.03,0.48,0.48);
  TPad *pad4 = new TPad("pad4","This is pad4",0.52,0.03,0.97,0.48);
  pad1->Draw();
  pad2->Draw();
  pad3->Draw();
  pad4->Draw();
  pad1->cd();
  clszu1->Draw();
  pad2->cd();
  clszv1->Draw();
  pad3->cd();
  clszu2->Draw();
  pad4->cd();
  clszv2->Draw();
  // ----------------------------------------------------------------------------
  cclsz->Update();
}


