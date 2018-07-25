{
#include <TStyle.h>
const int kBUFLEN = 150;

// open trees
TFile *f = new TFile("/data/proc_data.root");
TTree *t = (TTree *)f->Get("TL");

Int_t new_nentries, old_nentries, nentries;

nentries = (Int_t)t->GetEntries();


char input[kBUFLEN];
cout<<"Which graphs do you want to see?"<<endl;
cout<<"  (s)imple comparisons"<<endl;
cout<<"  (t)wo-d comparisons"<<endl;
cout<<"  (c)ross correlations"<<endl;
cout<<"  (d)ifferences"<<endl;

cout<<"  rotated sim(p)le comparisons"<<endl;
cout<<"  rotated t(w)o-d comparisons"<<endl;
cout<<"  rotated cr(o)ss correlations"<<endl;
cout<<"  rotated diff(e)rences"<<endl;

cout<<"  ta(r)get values"<<endl;
cout<<"  t(a)rget two-d"<<endl;
cout<<"  tar(g)et cross-correlations"<<endl;
cout<<"  target di(f)erences"<<endl;
input[0] = '\0';
fgets(input, kBUFLEN, stdin);

switch(input[0])
{
// simple comparisons
 case 's':
 case 'S':
   
c1 = new TCanvas("c1", "X");
t->Draw("CL_TR_x", "abs(CL_TR_x+0.25)<0.5");
c1->Update();
t->SetLineColor(kRed);
t->Draw("FL_TR_x", "abs(FL_TR_x+0.25)<0.5", "same");
t->SetLineColor(kBlack);
c1->Update();

c2 = new TCanvas("c2", "Y");
t->Draw("CL_TR_y", "abs(CL_TR_y)<0.15"); 
c2->Update();
t->SetLineColor(kRed);
t->Draw("FL_TR_y", "abs(FL_TR_y)<0.15", "same");

t->SetLineColor(kBlack);
c2->Update();

c3 = new TCanvas("c3", "Theta");
t->Draw("CL_TR_th", "abs(CL_TR_th+0.05)<0.1");
c3->Update();
t->SetLineColor(kRed);
t->Draw("FL_TR_th", "abs(FL_TR_th+0.05)<0.1", "same");
t->SetLineColor(kBlack);
c3->Update();

c4 = new TCanvas("c4", "Phi");
t->Draw("CL_TR_ph", "abs(CL_TR_ph)<0.15");
c4->Update();
t->SetLineColor(kRed);
t->Draw("FL_TR_ph", "abs(FL_TR_ph)<0.15", "same");
t->SetLineColor(kBlack);
c4->Update();

 break;



// 2d comparisons
 case 't':
 case 'T':
c9 = new TCanvas("c9", "New X v Y");
t->Draw("CL_TR_y:CL_TR_x",
	"abs(CL_TR_x+0.4)<0.3&&abs(CL_TR_y)<0.05");
c10 = new TCanvas("c10", "Old X v Y");
t->Draw("FL_TR_y:FL_TR_x",
	"abs(FL_TR_x+0.4)<0.3&&abs(FL_TR_y)<0.05");

c11 = new TCanvas("c11", "New Theta v Phi");
t->Draw("CL_TR_ph:CL_TR_th", 
	"abs(CL_TR_th+0.05)<0.06&&abs(CL_TR_ph)<.05");
c12 = new TCanvas("c12", "Old Theta v Phi");
t->Draw("FL_TR_ph:FL_TR_th", 
	"abs(FL_TR_th+0.05)<0.06&&abs(FL_TR_ph)<.05");

c13 = new TCanvas("c13", "New X v Theta");
t->Draw("CL_TR_th:CL_TR_x", 
	"abs(CL_TR_x+0.4)<0.3&&abs(CL_TR_th+0.05)<0.1");
c14 = new TCanvas("c14", "Old X v Theta");
t->Draw("FL_TR_th:FL_TR_x", 
	"abs(FL_TR_x+0.4)<0.3&&abs(FL_TR_th+0.05)<0.1");
  
c15 = new TCanvas("c15", "New Y v Phi");
t->Draw("CL_TR_ph:CL_TR_y",
	"abs(CL_TR_ph)<0.05&&abs(CL_TR_y)<0.05");
c16 = new TCanvas("c16", "Old Y v Phi");
t->Draw("FL_TR_ph:FL_TR_y",
	"abs(FL_TR_ph)<0.05&&abs(FL_TR_y)<0.05");
 break;

// cross correlations
 case 'c':
 case 'C':

c20 = new TCanvas("c20", "New X v Old X");
t->Draw("FL_TR_x:CL_TR_x", "abs(FL_TR_x+0.25)<0.5&&abs(CL_TR_x+0.25)<0.5");
c20->Update();

c21 = new TCanvas("c21", "New Y v Old Y");
t->Draw("FL_TR_y:CL_TR_y", "abs(FL_TR_y)<0.15&&abs(CL_TR_y)<0.15");
c21->Update();

c22 = new TCanvas("c22", "New Theta v Old Theta");
t->Draw("FL_TR_th:CL_TR_th", "abs(CL_TR_th+0.05)<0.1&&abs(FL_TR_th+0.05)<0.1");
c22->Update();

c23 = new TCanvas("c23", "New Phi v Old Phi");
t->Draw("FL_TR_ph:CL_TR_ph", "abs(FL_TR_ph)<0.15&&abs(CL_TR_ph)<0.15");
c23->Update();
 break;

// differences
 case 'd':
 case 'D':
c30 = new TCanvas("c30", "New X - Old X");
t->Draw("CL_TR_x-FL_TR_x", "(CL_TR_x-FL_TR_x)>-0.005&&(CL_TR_x-FL_TR_x)<0.005");
TH1F *xdhist = (TH1F*)gPad->GetPrimitive("htemp");
xdhist->Fit("gaus");
 TF1 *xf = xdhist->GetFunction("gaus");
 cout<<"chi-squared         = "<<xf->GetChisquare()<<endl;
 cout<<"reduced chi-squared = "
     <<xf->GetChisquare()/(float)(xf->GetNDF())<<endl;
 cout<<"mean value          = "<<xdhist->GetMean()<<endl;
 cout<<"RMS value           = "<<xdhist->GetRMS()<<endl;
c30->Update();

c31 = new TCanvas("c31", "New Y - Old Y");
t->Draw("CL_TR_y-FL_TR_y", "(CL_TR_y-FL_TR_y)>-0.005&&(CL_TR_y-FL_TR_y)<0.005");
TH1F *ydhist = (TH1F*)gPad->GetPrimitive("htemp");
ydhist->Fit("gaus");
 TF1 *yf = ydhist->GetFunction("gaus");
 cout<<"chi-squared         = "<<yf->GetChisquare()<<endl;
 cout<<"reduced chi-squared = "
     <<yf->GetChisquare()/(float)(yf->GetNDF())<<endl;
 cout<<"mean value          = "<<ydhist->GetMean()<<endl;
 cout<<"RMS value           = "<<ydhist->GetRMS()<<endl;
c31->Update();

c32 = new TCanvas("c32", "New Theta - Old Theta");
t->Draw("CL_TR_th-FL_TR_th", "(CL_TR_th-FL_TR_th)>-0.01&&(CL_TR_th-FL_TR_th)<0.01");
TH1F *tdhist = (TH1F*)gPad->GetPrimitive("htemp");
tdhist->Fit("gaus");
 TF1 *tf = tdhist->GetFunction("gaus");
 cout<<"chi-squared         = "<<tf->GetChisquare()<<endl;
 cout<<"reduced chi-squared = "
     <<tf->GetChisquare()/(float)(tf->GetNDF())<<endl;
 cout<<"mean value          = "<<tdhist->GetMean()<<endl;
 cout<<"RMS value           = "<<tdhist->GetRMS()<<endl;
c32->Update();

c33 = new TCanvas("c33", "New Phi - Old Phi");
t->Draw("CL_TR_ph-FL_TR_ph", "(CL_TR_ph-FL_TR_ph)>-0.01&&(CL_TR_ph-FL_TR_ph)<0.01");
TH1F *pdhist = (TH1F*)gPad->GetPrimitive("htemp");
pdhist->Fit("gaus");
 TF1 *pf = pdhist->GetFunction("gaus");
 cout<<"chi-squared         = "<<pf->GetChisquare()<<endl;
 cout<<"reduced chi-squared = "
     <<pf->GetChisquare()/(float)(pf->GetNDF())<<endl;
 cout<<"mean value          = "<<pdhist->GetMean()<<endl;
 cout<<"RMS value           = "<<pdhist->GetRMS()<<endl;
c33->Update();
 break;

// target histograms
 case 'r':
 case 'R':

c51 = new TCanvas("c51", "delta");
t->Draw("CL_TG_dp", "abs(CL_TG_dp+0.025)<0.025");
c51->Update(); 
t->SetLineColor(kRed); 
t->Draw("FL_TG_dp", "abs(FL_TG_dp+0.025)<0.025", "same"); 
t->SetLineColor(kBlack);
c51->Update();

c52 = new TCanvas("c52", "Y");
t->Draw("CL_TG_y", "abs(CL_TG_y)<0.06"); 
c52->Update();
t->SetLineColor(kRed);
t->Draw("FL_TG_y", "abs(FL_TG_y)<0.06", "same");
t->SetLineColor(kBlack);
c52->Update();

c53 = new TCanvas("c53", "Theta");
t->Draw("CL_TG_th", "abs(CL_TG_th+0.05)<0.2");
c53->Update();
t->SetLineColor(kRed);
t->Draw("FL_TG_th", "abs(FL_TG_th+0.05)<0.2", "same");
t->SetLineColor(kBlack);
c53->Update();

c54 = new TCanvas("c4", "Phi");
t->Draw("CL_TG_ph", "abs(CL_TG_ph)<0.1");
c54->Update();
t->SetLineColor(kRed);
t->Draw("FL_TG_ph", "abs(FL_TG_ph)<0.1", "same");
t->SetLineColor(kBlack);
c54->Update();

 break;

 case 'g':
 case 'G':
   c250 = new TCanvas("c250", "New Delta v. Old Delta");
   t->Draw("FL_TG_dp:CL_TG_dp", "abs(CL_TG_dp+0.025)<0.025&&abs(FL_TG_dp+0.025)<0.025");
   c250->Update();

   c251 = new TCanvas("c251", "New Y v. Old Y");
   t->Draw("FL_TG_y:CL_TG_y", "abs(CL_TG_y)<0.06&&abs(FL_TG_y)<0.06");
   c251->Update();

   c252 = new TCanvas("c252", "New Theta v. Old Theta");
   t->Draw("FL_TG_th:CL_TG_th", "abs(CL_TG_th+0.05)<0.2&&abs(FL_TG_th+0.05)<0.2");
   c252->Update();

   c253 = new TCanvas("c253", "New Phi v. Old Phi");
   t->Draw("FL_TG_ph:CL_TG_ph", "abs(CL_TG_ph)<0.1&&abs(FL_TG_ph)<0.1");
   c253->Update();
   break;

 case 'a':
 case 'A':

c55 = new TCanvas("c55", "Target Theta v. Phi");
 c55->Divide(1,2);
 c55->cd(1);
 t->Draw("CL_TG_th:CL_TG_ph", "abs(CL_TG_th)<0.1&&abs(CL_TG_ph)<0.1");
 c55->cd(2);
 t->Draw("FL_TG_th:FL_TG_ph", "abs(FL_TG_th)<0.1&&abs(FL_TG_ph)<0.1");
c55->Update();
   
   
c56 = new TCanvas("c56", "Target Theta v. Phi - Peak 0");
 c56->Divide(1,2);
 c56->cd(1);
 t->Draw("CL_TG_th:CL_TG_ph", 
	 "abs(CL_TG_th)<0.1&&abs(CL_TG_ph)<0.1&&abs(CL_TG_y)<0.003");
 c56->cd(2);
 t->Draw("FL_TG_th:FL_TG_ph", 
	 "abs(FL_TG_th)<0.1&&abs(FL_TG_ph)<0.1&&abs(FL_TG_y)<0.003");
c56->Update();

c57 = new TCanvas("c57", "Target Theta v. Phi - Peak 1");
 c57->Divide(1,2);
 c57->cd(1);
 t->Draw("CL_TG_th:CL_TG_ph", 
	 "abs(CL_TG_th)<0.1&&abs(CL_TG_ph)<0.1&&abs(CL_TG_y+1*0.0125)<0.003");
 c57->cd(2);
 t->Draw("FL_TG_th:FL_TG_ph", 
	 "abs(FL_TG_th)<0.1&&abs(FL_TG_ph)<0.1&&abs(FL_TG_y+1*0.0125)<0.003");
c57->Update();

c58 = new TCanvas("c58", "Target Theta v. Phi - Peak 2");
 c58->Divide(1,2);
 c58->cd(1);
 t->Draw("CL_TG_th:CL_TG_ph", 
	 "abs(CL_TG_th)<0.1&&abs(CL_TG_ph)<0.1&&abs(CL_TG_y+2*0.0125)<0.003");
 c58->cd(2);
 t->Draw("FL_TG_th:FL_TG_ph", 
	 "abs(FL_TG_th)<0.1&&abs(FL_TG_ph)<0.1&&abs(FL_TG_y+2*0.0125)<0.003");
c58->Update();

c59 = new TCanvas("c59", "Target Theta v. Phi - Peak 3");
 c59->Divide(1,2);
 c59->cd(1);
 t->Draw("CL_TG_th:CL_TG_ph", 
	 "abs(CL_TG_th)<0.1&&abs(CL_TG_ph)<0.1&&abs(CL_TG_y+3*0.0125)<0.003");
 c59->cd(2);
 t->Draw("FL_TG_th:FL_TG_ph", 
	 "abs(FL_TG_th)<0.1&&abs(FL_TG_ph)<0.1&&abs(FL_TG_y+3*0.0125)<0.003");
c59->Update();

c100 = new TCanvas("c100", "Target Theta v. Phi - Peak 4");
 c100->Divide(1,2);
 c100->cd(1);
 t->Draw("CL_TG_th:CL_TG_ph", 
	 "abs(CL_TG_th)<0.1&&abs(CL_TG_ph)<0.1&&abs(CL_TG_y+4*0.0125)<0.003");
 c100->cd(2);
 t->Draw("FL_TG_th:FL_TG_ph", 
	 "abs(FL_TG_th)<0.1&&abs(FL_TG_ph)<0.1&&abs(FL_TG_y+4*0.0125)<0.003");
c100->Update();

c101 = new TCanvas("c100", "Target Theta v. Phi - Peak -1");
 c101->Divide(1,2);
 c101->cd(1);
 t->Draw("CL_TG_th:CL_TG_ph", 
	 "abs(CL_TG_th)<0.1&&abs(CL_TG_ph)<0.1&&abs(CL_TG_y-1*0.0125)<0.003");
 c101->cd(2);
 t->Draw("FL_TG_th:FL_TG_ph", 
	 "abs(FL_TG_th)<0.1&&abs(FL_TG_ph)<0.1&&abs(FL_TG_y-1*0.0125)<0.003");
c101->Update();

c102 = new TCanvas("c102", "Target Theta v. Phi - Peak -2");
 c102->Divide(1,2);
 c102->cd(1);
 t->Draw("CL_TG_th:CL_TG_ph", 
	 "abs(CL_TG_th)<0.1&&abs(CL_TG_ph)<0.1&&abs(CL_TG_y-2*0.0125)<0.003");
 c102->cd(2);
 t->Draw("FL_TG_th:FL_TG_ph", 
	 "abs(FL_TG_th)<0.1&&abs(FL_TG_ph)<0.1&&abs(FL_TG_y-2*0.0125)<0.003");
c102->Update();

c103 = new TCanvas("c103", "Target Theta v. Phi - Peak -3");
 c103->Divide(1,2);
 c103->cd(1);
 t->Draw("CL_TG_th:CL_TG_ph", 
	 "abs(CL_TG_th)<0.1&&abs(CL_TG_ph)<0.1&&abs(CL_TG_y-3*0.0125)<0.003");
 c103->cd(2);
 t->Draw("FL_TG_th:FL_TG_ph", 
	 "abs(FL_TG_th)<0.1&&abs(FL_TG_ph)<0.1&&abs(FL_TG_y-3*0.0125)<0.003");
c103->Update();

c104 = new TCanvas("c104", "Target Theta v. Phi - Peak -4");
 c104->Divide(1,2);
 c104->cd(1);
 t->Draw("CL_TG_th:CL_TG_ph", 
	 "abs(CL_TG_th)<0.1&&abs(CL_TG_ph)<0.1&&abs(CL_TG_y-4*0.0125)<0.003");
 c104->cd(2);
 t->Draw("FL_TG_th:FL_TG_ph", 
	 "abs(FL_TG_th)<0.1&&abs(FL_TG_ph)<0.1&&abs(FL_TG_y-4*0.0125)<0.003");
c104->Update();
   
c105 = new TCanvas("c105", "Target Theta v. Delta");
 c105->Divide(1,2);
 c105->cd(1);
 t->Draw("CL_TG_dp:CL_TG_ph", "abs(CL_TG_ph)<0.05&&abs(CL_TG_dp+0.025)<0.025");
 c105->cd(2);
 t->Draw("FL_TG_dp:FL_TG_ph", "abs(FL_TG_ph)<0.05&&abs(FL_TG_dp+0.025)<0.025");
c105->Update();

c106 = new TCanvas("c106", "Target Theta v. Delta - Peak 0");
 c106->Divide(1,2);
 c106->cd(1);
 t->Draw("CL_TG_dp:CL_TG_ph", 
	 "abs(CL_TG_ph)<0.05&&abs(CL_TG_dp+0.025)<0.025&&abs(CL_TG_y)<0.003");
 c106->cd(2);
 t->Draw("FL_TG_dp:FL_TG_ph", 
	 "abs(FL_TG_ph)<0.05&&abs(FL_TG_dp+0.025)<0.025&&abs(CL_TG_y)<0.003");
c106->Update();

 break;

 case 'f':
 case 'F':
   /* delta diff for each peak */
c60 = new TCanvas("c60", "New delta - Old delta");
t->Draw("CL_TG_dp-FL_TG_dp", 
	"(CL_TG_dp-FL_TG_dp)>-0.01&&(CL_TG_dp-FL_TG_dp)<0.01");
TH1F *xdhist = (TH1F*)gPad->GetPrimitive("htemp");
xdhist->Fit("gaus");
 TF1 *xf = xdhist->GetFunction("gaus");
 cout<<"chi-squared         = "<<xf->GetChisquare()<<endl;
 cout<<"reduced chi-squared = "
     <<xf->GetChisquare()/(float)(xf->GetNDF())<<endl; 
 cout<<"mean value          = "<<xdhist->GetMean()<<endl;
 cout<<"RMS value           = "<<xdhist->GetRMS()<<endl;
c60->Update();

c61 = new TCanvas("c61", "New Target Y - Old Target Y");
t->Draw("CL_TG_y-FL_TG_y", "(CL_TG_y-FL_TG_y)>-0.02&&(CL_TG_y-FL_TG_y)<0.02");
TH1F *ydhist = (TH1F*)gPad->GetPrimitive("htemp");
ydhist->Fit("gaus");
 TF1 *yf = ydhist->GetFunction("gaus");
 cout<<"chi-squared         = "<<yf->GetChisquare()<<endl;
 cout<<"reduced chi-squared = "
     <<yf->GetChisquare()/(float)(yf->GetNDF())<<endl;
 cout<<"mean value          = "<<ydhist->GetMean()<<endl;
 cout<<"RMS value           = "<<ydhist->GetRMS()<<endl;
c61->Update();

c62 = new TCanvas("c62", "New Target Theta - Old Target Theta");
t->Draw("CL_TG_th-FL_TG_th", 
	"(CL_TG_th-FL_TG_th)>-0.05&&(CL_TG_th-FL_TG_th)<0.05");
TH1F *tdhist = (TH1F*)gPad->GetPrimitive("htemp");
tdhist->Fit("gaus");
 TF1 *tf = tdhist->GetFunction("gaus");
 cout<<"chi-squared         = "<<tf->GetChisquare()<<endl;
 cout<<"reduced chi-squared = "
     <<tf->GetChisquare()/(float)(tf->GetNDF())<<endl;
 cout<<"mean value          = "<<tdhist->GetMean()<<endl;
 cout<<"RMS value           = "<<tdhist->GetRMS()<<endl;
c62->Update();

c63 = new TCanvas("c63", "New Target Phi - Old Target Phi");
t->Draw("CL_TG_ph-FL_TG_ph", 
	"(CL_TG_ph-FL_TG_ph)>-0.01&&(CL_TG_ph-FL_TG_ph)<0.01");
TH1F *pdhist = (TH1F*)gPad->GetPrimitive("htemp");
pdhist->Fit("gaus");
 TF1 *pf = pdhist->GetFunction("gaus");
 cout<<"chi-squared         = "<<pf->GetChisquare()<<endl;
 cout<<"reduced chi-squared = "
     <<pf->GetChisquare()/(float)(pf->GetNDF())<<endl;
 cout<<"mean value          = "<<pdhist->GetMean()<<endl;
 cout<<"RMS value           = "<<pdhist->GetRMS()<<endl;
c63->Update();

 break;

// rotated simple comparisons
 case 'p':
 case 'P':
   
c71 = new TCanvas("c71", "X");
t->Draw("CL_TR_rx", "abs(CL_TR_rx+0.25)<0.5");
c71->Update();
t->SetLineColor(kRed);
t->Draw("FL_TR_rx", "abs(FL_TR_rx+0.25)<0.5", "same");
t->SetLineColor(kBlack);
c71->Update();

c72 = new TCanvas("c72", "Y");
t->Draw("CL_TR_ry", "abs(CL_TR_ry)<0.15"); 
c72->Update();
t->SetLineColor(kRed);
t->Draw("FL_TR_ry", "abs(FL_TR_ry)<0.15", "same");
t->SetLineColor(kBlack);
c72->Update();

c73 = new TCanvas("c73", "Theta");
t->Draw("CL_TR_rth", "abs(CL_TR_rth+0.05)<0.1");
c73->Update();
t->SetLineColor(kRed);
t->Draw("FL_TR_rth", "abs(FL_TR_rth+0.05)<0.1", "same");
t->SetLineColor(kBlack);
c73->Update();

c74 = new TCanvas("c74", "Phi");
t->Draw("CL_TR_rph", "abs(CL_TR_rph)<0.15");
c74->Update();
t->SetLineColor(kRed);
t->Draw("FL_TR_rph", "abs(FL_TR_rph)<0.15", "same");
t->SetLineColor(kBlack);
c74->Update();

 break;

// rotated 2d comparisons
 case 'w':
 case 'W':
c89 = new TCanvas("c89", "New X v Y");
t->Draw("CL_TR_ry:CL_TR_rx",
	"abs(CL_TR_rx+0.4)<0.3&&abs(CL_TR_ry)<0.05");
c80 = new TCanvas("c80", "Old X v Y");
t->Draw("FL_TR_ry:FL_TR_rx",
	"abs(FL_TR_rx+0.4)<0.3&&abs(FL_TR_ry)<0.05");

c81 = new TCanvas("c81", "New Theta v Phi");
t->Draw("CL_TR_rph:CL_TR_rth", 
	"abs(CL_TR_rth+0.05)<0.06&&abs(CL_TR_rph)<.05");
c82 = new TCanvas("c82", "Old Theta v Phi");
t->Draw("FL_TR_rph:FL_TR_rth", 
	"abs(FL_TR_rth+0.05)<0.06&&abs(FL_TR_rph)<.05");

c83 = new TCanvas("c83", "New X v Theta");
t->Draw("CL_TR_rth:CL_TR_rx", 
	"abs(CL_TR_rx+0.4)<0.3&&abs(CL_TR_rth+0.05)<0.1");
c84 = new TCanvas("c84", "Old X v Theta");
t->Draw("FL_TR_rth:FL_TR_rx", 
	"abs(FL_TR_rx+0.4)<0.3&&abs(FL_TR_rth+0.05)<0.1");
  
c85 = new TCanvas("c85", "New Y v Phi");
t->Draw("CL_TR_rph:CL_TR_ry",
	"abs(CL_TR_rph)<0.05&&abs(CL_TR_ry)<0.05");
c86 = new TCanvas("c86", "Old Y v Phi");
t->Draw("FL_TR_rph:FL_TR_ry",
	"abs(FL_TR_rph)<0.05&&abs(FL_TR_ry)<0.05");
 break;


// rotated cross correlations
 case 'o':
 case 'O':

c90 = new TCanvas("c20", "New X v Old X");
t->Draw("FL_TR_rx:CL_TR_rx", "abs(FL_TR_rx+0.25)<0.5&&abs(CL_TR_rx+0.25)<0.5");
c20->Update();

c91 = new TCanvas("c21", "New Y v Old Y");
t->Draw("FL_TR_ry:CL_TR_ry", "abs(FL_TR_ry)<0.15&&abs(CL_TR_ry)<0.15");
c21->Update();

c92 = new TCanvas("c22", "New Theta v Old Theta");
t->Draw("FL_TR_rth:CL_TR_rth", "abs(CL_TR_rth+0.05)<0.1&&abs(FL_TR_rth+0.05)<0.1");
c22->Update();

c93 = new TCanvas("c23", "New Phi v Old Phi");
t->Draw("FL_TR_rph:CL_TR_rph", "abs(FL_TR_rph)<0.15&&abs(CL_TR_rph)<0.15");
c93->Update();
 break;

// differences
 case 'e':
 case 'E':
c94 = new TCanvas("c94", "New X - Old X");
t->Draw("CL_TR_rx-FL_TR_rx", "(CL_TR_rx-FL_TR_rx)>-0.005&&(CL_TR_rx-FL_TR_rx)<0.005");
TH1F *xdhist = (TH1F*)gPad->GetPrimitive("htemp");
xdhist->Fit("gaus");
 TF1 *xf = xdhist->GetFunction("gaus");
 cout<<"chi-squared         = "<<xf->GetChisquare()<<endl;
 cout<<"reduced chi-squared = "
     <<xf->GetChisquare()/(float)(xf->GetNDF())<<endl;
 cout<<"mean value          = "<<xdhist->GetMean()<<endl;
 cout<<"RMS value           = "<<xdhist->GetRMS()<<endl;
c94->Update();

c95 = new TCanvas("c95", "New Y - Old Y");
t->Draw("CL_TR_ry-FL_TR_ry", "(CL_TR_ry-FL_TR_ry)>-0.005&&(CL_TR_ry-FL_TR_ry)<0.005");
TH1F *ydhist = (TH1F*)gPad->GetPrimitive("htemp");
ydhist->Fit("gaus");
 TF1 *yf = ydhist->GetFunction("gaus");
 cout<<"chi-squared         = "<<yf->GetChisquare()<<endl;
 cout<<"reduced chi-squared = "
     <<yf->GetChisquare()/(float)(yf->GetNDF())<<endl;
 cout<<"mean value          = "<<ydhist->GetMean()<<endl;
 cout<<"RMS value           = "<<ydhist->GetRMS()<<endl;
c95->Update();

c96 = new TCanvas("c96", "New Theta - Old Theta");
t->Draw("CL_TR_rth-FL_TR_rth", 
	"(CL_TR_rth-FL_TR_rth)>-0.02&&(CL_TR_rth-FL_TR_rth)<0.02");
TH1F *tdhist = (TH1F*)gPad->GetPrimitive("htemp");
tdhist->Fit("gaus");
 TF1 *tf = tdhist->GetFunction("gaus");
 cout<<"chi-squared         = "<<tf->GetChisquare()<<endl;
 cout<<"reduced chi-squared = "
     <<tf->GetChisquare()/(float)(tf->GetNDF())<<endl;
 cout<<"mean value          = "<<tdhist->GetMean()<<endl;
 cout<<"RMS value           = "<<tdhist->GetRMS()<<endl;
c96->Update();

c97 = new TCanvas("c97", "New Phi - Old Phi");
t->Draw("CL_TR_rph-FL_TR_rph", 
	"(CL_TR_rph-FL_TR_rph)>-0.01&&(CL_TR_rph-FL_TR_rph)<0.01");
TH1F *pdhist = (TH1F*)gPad->GetPrimitive("htemp");
pdhist->Fit("gaus");
 TF1 *pf = pdhist->GetFunction("gaus");
 cout<<"chi-squared         = "<<pf->GetChisquare()<<endl;
 cout<<"reduced chi-squared = "
     <<pf->GetChisquare()/(float)(pf->GetNDF())<<endl;
 cout<<"mean value          = "<<pdhist->GetMean()<<endl;
 cout<<"RMS value           = "<<pdhist->GetRMS()<<endl;
c97->Update();
 break;
}

// cleanup 
delete f;
}
