{
TFile* f = new TFile("/data/proc_data.root","READ");
TTree* t = (TTree *)f->Get("TR");

Int_t nev = (Int_t)t->GetEntries();
Float_t t0 = 16.0091*TMath::Pi()/180.0;
Float_t cost0 = cos(t0);
Float_t p0 = 838.;
Float_t E  = 826.;
Float_t M  = 12.*931.;

Float_t dp, th, ph, y;
Float_t f_dp, f_th, f_ph, f_y;
t->SetBranchAddress("CR_TG_dp", &dp);
t->SetBranchAddress("CR_TG_th", &th);
t->SetBranchAddress("CR_TG_ph", &ph);
t->SetBranchAddress("CR_TG_y", &y);

t->SetBranchAddress("FR_TG_dp", &f_dp);
t->SetBranchAddress("FR_TG_th", &f_th);
t->SetBranchAddress("FR_TG_ph", &f_ph);
t->SetBranchAddress("FR_TG_y", &f_y);

gROOT->cd();
TH1* h1 = new TH1F("dp","dp raw",500,-.05,0.0);
TH1* h2 = new TH1F("dp_kin","dp kinematically corrected",250,-.04,-0.0125);
TH1* h3 = new TH1F("dp_peak", "dp peak", 100, -0.019, -0.015);

TH1* h12 = new TH1F("f_dp_kin","dp kinematically corrected",250,-.04,-0.0125);
TH1* h13 = new TH1F("f_dp_peak", "dp peak", 100, -0.019, -0.015);

Float_t cost, cor, n = 0;
for(Int_t i=0; i<nev; i++) {
  t->GetEntry(i);
  if(i%5000==0)  cout << i << endl << flush;
  if( fabs(dp+0.025) > 0.015
      || fabs(y+0.006) > 0.003
      || fabs(th) > 0.1
      || fabs(ph) > 0.1 )
    continue;
  n++;
  cost = cos(t0+ph)*cos(th);
  cor  = (cost - cost0) / 
    ( M/E + 2. - cost - cost0 + E/M*( 1. - cost - cost0 + cost*cost0 ));
  cor *= E/p0;
  //  cout << i << "\t" << dp << "\t" << cor << endl << flush;;
  h1->Fill(dp);
  h2->Fill(dp-cor);

  if(fabs(dp-cor+0.0170) < 0.0014)
    h3->Fill(dp-cor);
}
//cout << n << endl;

for(Int_t i=0; i<nev; i++) {
  t->GetEntry(i);
  if(i%5000==0)  cout << i << endl << flush;
  if( fabs(f_dp+0.025) > 0.015
      || fabs(f_y+0.006) > 0.003
      || fabs(f_th) > 0.1
      || fabs(f_ph) > 0.1 )
    continue;
  n++;
  cost = cos(t0+f_ph)*cos(f_th);
  cor  = (cost - cost0) / 
    ( M/E + 2. - cost - cost0 + E/M*( 1. - cost - cost0 + cost*cost0 ));
  cor *= E/p0;
  //  cout << i << "\t" << dp << "\t" << cor << endl << flush;;
  h12->Fill(f_dp-cor);

  if(fabs((f_dp-cor)+0.0170) < 0.0014)
    h13->Fill(f_dp-cor);
}
//cout << n << endl;

//h1->Draw();
c1 = new TCanvas("c1", "Corrected dp");
h2->Draw();
c1->Update();
h12->SetLineColor(kRed);
h12->Draw("same");
c1->Update();

c2 = new TCanvas("c2", "Corrected dp - first peak");
h3->Fit("gaus");
h3->GetXaxis()->SetTitle("Delta");
h3->Draw();

 TF1 *xf = h3->GetFunction("gaus");
 cout<<"chi-squared         = "<<xf->GetChisquare()<<endl;
 cout<<"reduced chi-squared = "
     <<xf->GetChisquare()/(float)(xf->GetNDF())<<endl; 
 cout<<"mean value          = "<<h3->GetMean()<<endl;
 cout<<"RMS value           = "<<h3->GetRMS()<<endl;
c2->Update();

c12 = new TCanvas("c12", "Fortran Corrected dp - first peak");
h13->Fit("gaus");
h13->GetXaxis()->SetTitle("Delta");
h13->Draw();

 TF1 *xf = h13->GetFunction("gaus");
 cout<<"chi-squared         = "<<xf->GetChisquare()<<endl;
 cout<<"reduced chi-squared = "
     <<xf->GetChisquare()/(float)(xf->GetNDF())<<endl; 
 cout<<"mean value          = "<<h13->GetMean()<<endl;
 cout<<"RMS value           = "<<h13->GetRMS()<<endl;
c12->Update();

delete f;
}
