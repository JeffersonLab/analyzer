{
#include <TStyle.h>
const int kBUFLEN = 150;

// open trees
TFile *f = new TFile("/data/proc_data.root");
TTree *t = (TTree *)f->Get("CTL");

Int_t new_nentries, old_nentries, nentries;

nentries = (Int_t)t->GetEntries();

char in_choose[kBUFLEN];
cout<<"Which plane do you want to see?"<<endl;
in_choose[0] = '\0';
fgets(in_choose, kBUFLEN, stdin);

if( (in_choose[0]=='\n') || (in_choose[0]=='\0') ) {
  in_choose[0] = 'u';
  in_choose[1] = '1';
}

switch(in_choose[0]) {

 case 'u':
 case 'U':

   switch(in_choose[1]) {

   case '1':
char input[kBUFLEN];
cout<<"Which graphs do you want to see?"<<endl;
cout<<"  (s)imple comparisons"<<endl;
cout<<"  (t)wo-d comparisons"<<endl;
cout<<"  (c)ross correlations"<<endl;
cout<<"  (d)ifferences"<<endl;

input[0] = '\0';
fgets(input, kBUFLEN, stdin);

switch(input[0])
{
// simple comparisons
 case 's':
 case 'S':
   
c1 = new TCanvas("c1", "U1 position");
t->Draw("CL_U1_pos", "abs(CL_U1_pos+0.3)<0.3");
c1->Update();
t->SetLineColor(kRed);
t->Draw("FL_U1_pos", "abs(FL_U1_pos+0.3)<0.3", "same");
t->SetLineColor(kBlack);
c1->Update();

c2 = new TCanvas("c2", "U1 slope");
t->Draw("CL_U1_slope", "abs(CL_U1_slope-0.8)<0.3"); 
c2->Update();
t->SetLineColor(kRed);
t->Draw("FL_U1_slope", "abs(FL_U1_slope-0.8)<0.3", "same");
t->SetLineColor(kBlack);
c2->Update();

c3 = new TCanvas("c3", "U1 pivot wire");
t->Draw("CL_U1_pivot", "abs(CL_U1_pivot-300)<100");
c3->Update();
t->SetLineColor(kRed);
t->Draw("FL_U1_pivot", "abs(FL_U1_pivot-300)<100", "same");
t->SetLineColor(kBlack);
c3->Update();

 break;



// 2d comparisons
 case 't':
 case 'T':

c9 = new TCanvas("c9", "New Pos v Slope");
t->Draw("CL_U1_slope:CL_U1_pos",
	"abs(CL_U1_pos+0.3)<0.3&&abs(CL_U1_slope-0.8)<0.3");
c10 = new TCanvas("c10", "Old Pos v Slope");
t->Draw("FL_U1_slope:FL_U1_pos",
	"abs(FL_U1_pos+0.3)<0.3&&abs(FL_U1_slope-0.8)<0.3");

c9 = new TCanvas("c9", "New Pos v Slope");
t->Draw("CL_U1_slope:CL_U1_pos",
	"abs(CL_U1_pos+0.3)<0.3&&abs(CL_U1_slope-0.8)<0.3&&abs(CL_U1_pos-FL_U1_pos)<0.0002");
c10 = new TCanvas("c10", "Old Pos v Slope");
t->Draw("FL_U1_slope:FL_U1_pos",
	"abs(FL_U1_pos+0.3)<0.3&&abs(FL_U1_slope-0.8)<0.3&&abs(CL_U1_pos-FL_U1_pos)<0.0002");

c109 = new TCanvas("c109", "New Pos v Slope");
t->Draw("CL_U1_slope:CL_U1_pos",
	"abs(CL_U1_pos+0.3)<0.3&&abs(CL_U1_slope-0.8)<0.3&&abs(CL_U1_pos-FL_U1_pos+0.0006)<0.0002");
c110 = new TCanvas("c110", "Old Pos v Slope");
t->Draw("FL_U1_slope:FL_U1_pos",
	"abs(FL_U1_pos+0.3)<0.3&&abs(FL_U1_slope-0.8)<0.3&&abs(CL_U1_pos-FL_U1_pos+0.0006)<0.0002");

c209 = new TCanvas("c209", "New Pos v Slope");
t->Draw("CL_U1_slope:CL_U1_pos",
	"abs(CL_U1_pos+0.3)<0.3&&abs(CL_U1_slope-0.8)<0.3&&abs(CL_U1_pos-FL_U1_pos-0.0006)<0.0002");
c210 = new TCanvas("c210", "Old Pos v Slope");
t->Draw("FL_U1_slope:FL_U1_pos",
	"abs(FL_U1_pos+0.3)<0.3&&abs(FL_U1_slope-0.8)<0.3&&abs(CL_U1_pos-FL_U1_pos-0.0006)<0.0002");

c9 = new TCanvas("c9", "New Pos v Slope");
t->Draw("CL_U1_slope:CL_U1_pos",
	"abs(CL_U1_pos+0.3)<0.3&&abs(CL_U1_slope-0.8)<0.3");
c10 = new TCanvas("c10", "Old Pos v Slope");
t->Draw("FL_U1_slope:FL_U1_pos",
	"abs(FL_U1_pos+0.3)<0.3&&abs(FL_U1_slope-0.8)<0.3");

c11 = new TCanvas("c11", "New Pivot v Slope");
t->Draw("CL_U1_slope:CL_U1_pivot",
	"abs(CL_U1_pivot-300)<100&&abs(CL_U1_slope-0.8)<0.3");
c12 = new TCanvas("c12", "Old Pivot v Slope");
t->Draw("FL_U1_slope:FL_U1_pivot",
	"abs(FL_U1_pivot-300)<100&&abs(FL_U1_slope-0.8)<0.3");

c13 = new TCanvas("c13", "New Pos v Pivot");
t->Draw("CL_U1_pivot:CL_U1_pos",
	"abs(CL_U1_pos+0.3)<0.3&&abs(CL_U1_pivot-300)<100");
c14 = new TCanvas("c14", "Old Pos v Pivot");
t->Draw("FL_U1_pivot:FL_U1_pos",
	"abs(FL_U1_pos+0.3)<0.3&&abs(FL_U1_pivot-300)<100");

 break;

// cross correlations
 case 'c':
 case 'C':

c20 = new TCanvas("c20", "New Pos v Old Pos");
t->Draw("FL_U1_pos:CL_U1_pos", "abs(FL_U1_pos+0.3)<0.3&&abs(CL_U1_pos+0.3)<0.3");
c20->Update();

c21 = new TCanvas("c21", "New Slope v Old Slope");
 t->Draw("FL_U1_slope:CL_U1_slope", 
	 "abs(FL_U1_slope-0.8)<0.3&&abs(CL_U1_slope-0.8)<0.3");
c21->Update();

c22 = new TCanvas("c22", "New Pivot v Old Pivot");
t->Draw("FL_U1_pivot:CL_U1_pivot", 
	"abs(FL_U1_pivot-300)<100&&abs(CL_U1_pivot-300)<100");
c22->Update();

 break;

// differences
 case 'd':
 case 'D':
c30 = new TCanvas("c30", "New Pos - Old Pos");
t->Draw("CL_U1_pos-FL_U1_pos", 
	"(CL_U1_pos-FL_U1_pos)>-0.005&&(CL_U1_pos-FL_U1_pos)<0.005");
TH1F *xdhist = (TH1F*)gPad->GetPrimitive("htemp");
xdhist->Fit("gaus");
 TF1 *xf = xdhist->GetFunction("gaus");
 cout<<"chi-squared         = "<<xf->GetChisquare()<<endl;
 cout<<"reduced chi-squared = "
     <<xf->GetChisquare()/(float)(xf->GetNDF())<<endl;
 cout<<"mean value          = "<<xdhist->GetMean()<<endl;
 cout<<"RMS value           = "<<xdhist->GetRMS()<<endl;
c30->Update();

c31 = new TCanvas("c31", "New Slope - Old Slope");
t->Draw("CL_U1_slope-FL_U1_slope", 
	"(CL_U1_slope-FL_U1_slope+0.2)>-0.2&&(CL_U1_slope-FL_U1_slope+0.2)<0.2");
TH1F *ydhist = (TH1F*)gPad->GetPrimitive("htemp");
ydhist->Fit("gaus");
 TF1 *yf = ydhist->GetFunction("gaus");
 cout<<"chi-squared         = "<<yf->GetChisquare()<<endl;
 cout<<"reduced chi-squared = "
     <<yf->GetChisquare()/(float)(yf->GetNDF())<<endl;
 cout<<"mean value          = "<<ydhist->GetMean()<<endl;
 cout<<"RMS value           = "<<ydhist->GetRMS()<<endl;
c31->Update();

c32 = new TCanvas("c32", "New Pivot - Old Pivot");
t->Draw("CL_U1_pivot-FL_U1_pivot", 
	"(CL_U1_pivot-FL_U1_pivot)>-3&&(CL_U1_pivot-FL_U1_pivot)<3");
c32->Update();

c33 = new TCanvas("c33", "New Pivot - Pos");
t->Draw("(CL_U1_pivot*-.0042426)-CL_U1_pos", 
	"(CL_U1_pivot*-.0042426)-CL_U1_pos+0.78>-0.01&&(CL_U1_pivot*-.0042426)-CL_U1_pos+0.78<0.01");
c33->Update();

c34 = new TCanvas("c34", "New Pivot - Pos");
t->Draw("(FL_U1_pivot*-.0042426)-FL_U1_pos", 
	"(FL_U1_pivot*-.0042426)-FL_U1_pos+0.78>-0.01&&(FL_U1_pivot*-.0042426)-FL_U1_pos+0.78<0.01");
 break;
 }

 break;

   case '2':
char input[kBUFLEN];
cout<<"Which graphs do you want to see?"<<endl;
cout<<"  (s)imple comparisons"<<endl;
cout<<"  (t)wo-d comparisons"<<endl;
cout<<"  (c)ross correlations"<<endl;
cout<<"  (d)ifferences"<<endl;

input[0] = '\0';
fgets(input, kBUFLEN, stdin);

switch(input[0])
{
// simple comparisons
 case 's':
 case 'S':
   
c1 = new TCanvas("c1", "U2 position");
t->Draw("CL_U2_pos", "abs(CL_U2_pos+0.2)<0.3");
c1->Update();
t->SetLineColor(kRed);
t->Draw("FL_U2_pos", "abs(FL_U2_pos+0.2)<0.3", "same");
t->SetLineColor(kBlack);
c1->Update();

c2 = new TCanvas("c2", "U2 slope");
t->Draw("CL_U2_slope", "abs(CL_U2_slope-0.8)<0.3"); 
c2->Update();
t->SetLineColor(kRed);
t->Draw("FL_U2_slope", "abs(FL_U2_slope-0.8)<0.3", "same");
t->SetLineColor(kBlack);
c2->Update();

c3 = new TCanvas("c3", "U2 pivot wire");
t->Draw("CL_U2_pivot", "abs(CL_U2_pivot-300)<100");
c3->Update();
t->SetLineColor(kRed);
t->Draw("FL_U2_pivot", "abs(FL_U2_pivot-300)<100", "same");
t->SetLineColor(kBlack);
c3->Update();
 break;


// 2d comparisons
 case 't':
 case 'T':
c9 = new TCanvas("c9", "New Pos v Slope");
t->Draw("CL_U2_slope:CL_U2_pos",
	"abs(CL_U2_pos+0.2)<0.3&&abs(CL_U2_slope-0.8)<0.3");
c10 = new TCanvas("c10", "Old Pos v Slope");
t->Draw("FL_U2_slope:FL_U2_pos",
	"abs(FL_U2_pos+0.2)<0.3&&abs(FL_U2_slope-0.8)<0.3");
 break;

// cross correlations
 case 'c':
 case 'C':

c20 = new TCanvas("c20", "New Pos v Old Pos");
t->Draw("FL_U2_pos:CL_U2_pos", "abs(FL_U2_pos+0.2)<0.3&&abs(CL_U2_pos+0.2)<0.3");
c20->Update();

c21 = new TCanvas("c21", "New Slope v Old Slope");
 t->Draw("FL_U2_slope:CL_U2_slope", 
	 "abs(FL_U2_slope-0.8)<0.3&&abs(CL_U2_slope-0.8)<0.3");
c21->Update();

c22 = new TCanvas("c22", "New Pivot v Old Pivot");
t->Draw("FL_U2_pivot:CL_U2_pivot", 
	"abs(FL_U2_pivot-300)<100&&abs(CL_U2_pivot-300)<100");
c22->Update();


 break;

// differences
 case 'd':
 case 'D':
c30 = new TCanvas("c30", "New Pos - Old Pos");
t->Draw("CL_U2_pos-FL_U2_pos", 
	"(CL_U2_pos-FL_U2_pos)>-0.005&&(CL_U2_pos-FL_U2_pos)<0.005");
TH1F *xdhist = (TH1F*)gPad->GetPrimitive("htemp");
xdhist->Fit("gaus");
 TF1 *xf = xdhist->GetFunction("gaus");
 cout<<"chi-squared         = "<<xf->GetChisquare()<<endl;
 cout<<"reduced chi-squared = "
     <<xf->GetChisquare()/(float)(xf->GetNDF())<<endl;
 cout<<"mean value          = "<<xdhist->GetMean()<<endl;
 cout<<"RMS value           = "<<xdhist->GetRMS()<<endl;
c30->Update();

c31 = new TCanvas("c31", "New Slope - Old Slope");
t->Draw("CL_U2_slope-FL_U2_slope", 
	"(CL_U2_slope-FL_U2_slope+0.2)>-0.2&&(CL_U2_slope-FL_U2_slope+0.2)<0.2");
TH1F *ydhist = (TH1F*)gPad->GetPrimitive("htemp");
ydhist->Fit("gaus");
 TF1 *yf = ydhist->GetFunction("gaus");
 cout<<"chi-squared         = "<<yf->GetChisquare()<<endl;
 cout<<"reduced chi-squared = "
     <<yf->GetChisquare()/(float)(yf->GetNDF())<<endl;
 cout<<"mean value          = "<<ydhist->GetMean()<<endl;
 cout<<"RMS value           = "<<ydhist->GetRMS()<<endl;
c31->Update();

c32 = new TCanvas("c32", "New Pivot - Old Pivot");
t->Draw("CL_U2_pivot-FL_U2_pivot", 
	"(CL_U2_pivot-FL_U2_pivot)>-3&&(CL_U2_pivot-FL_U2_pivot)<3");
/*
TH1F *vdhist = (TH1F*)gPad->GetPrimitive("htemp");
vdhist->Fit("gaus");
 TF1 *vf = xdhist->GetFunction("gaus");
 cout<<"chi-squared         = "<<vf->GetChisquare()<<endl;
 cout<<"reduced chi-squared = "
     <<xf->GetChisquare()/(float)(vf->GetNDF())<<endl;
 cout<<"mean value          = "<<vdhist->GetMean()<<endl;
 cout<<"RMS value           = "<<vdhist->GetRMS()<<endl;
*/

c33 = new TCanvas("c33", "New Pivot - Pos");
t->Draw("(CL_U2_pivot*-.0042426)-CL_U2_pos", 
	"(CL_U2_pivot*-.0042426)-CL_U2_pos+1.027>-0.01&&(CL_U2_pivot*-.0042426)-CL_U2_pos+1.027<0.01");
TH1F *npdhist = (TH1F*)gPad->GetPrimitive("htemp");
npdhist->Fit("gaus");
 TF1 *npf = xdhist->GetFunction("gaus");
 cout<<"chi-squared         = "<<npf->GetChisquare()<<endl;
 cout<<"reduced chi-squared = " 
     <<xf->GetChisquare()/(float)(npf->GetNDF())<<endl;
 cout<<"mean value          = "<<npdhist->GetMean()<<endl;
 cout<<"RMS value           = "<<npdhist->GetRMS()<<endl;
c33->Update();

c34 = new TCanvas("c34", "New Pivot - Pos");
t->Draw("(FL_U2_pivot*-.0042426)-FL_U2_pos", 
	"(FL_U2_pivot*-.0042426)-FL_U2_pos+1.032>-0.01&&(FL_U2_pivot*-.0042426)-FL_U2_pos+1.032<0.01");
TH1F *pdhist = (TH1F*)gPad->GetPrimitive("htemp");
pdhist->Fit("gaus");
 TF1 *pf = xdhist->GetFunction("gaus");
 cout<<"chi-squared         = "<<pf->GetChisquare()<<endl;
 cout<<"reduced chi-squared = " 
     <<xf->GetChisquare()/(float)(pf->GetNDF())<<endl;
 cout<<"mean value          = "<<pdhist->GetMean()<<endl;
 cout<<"RMS value           = "<<pdhist->GetRMS()<<endl;
c34->Update();
 break;
}
break;
}
break;

 case 'v':
 case 'V':

   switch(in_choose[1]) {

   case '1':
char input[kBUFLEN];
cout<<"Which graphs do you want to see?"<<endl;
cout<<"  (s)imple comparisons"<<endl;
cout<<"  (t)wo-d comparisons"<<endl;
cout<<"  (c)ross correlations"<<endl;
cout<<"  (d)ifferences"<<endl;

input[0] = '\0';
fgets(input, kBUFLEN, stdin);

switch(input[0])
{
// simple comparisons
 case 's':
 case 'S':
   
c1 = new TCanvas("c1", "V1 position");
t->Draw("CL_V1_pos", "abs(CL_V1_pos+0.3)<0.3");
c1->Update();
t->SetLineColor(kRed);
t->Draw("FL_V1_pos", "abs(FL_V1_pos+0.3)<0.3", "same");
t->SetLineColor(kBlack);
c1->Update();

c2 = new TCanvas("c2", "V1 slope");
t->Draw("CL_V1_slope", "abs(CL_V1_slope-0.8)<0.3"); 
c2->Update();
t->SetLineColor(kRed);
t->Draw("FL_V1_slope", "abs(FL_V1_slope-0.8)<0.3", "same");
t->SetLineColor(kBlack);
c2->Update();

c3 = new TCanvas("c3", "V1 pivot wire");
t->Draw("CL_V1_pivot", "abs(CL_V1_pivot-300)<100");
c3->Update();
t->SetLineColor(kRed);
t->Draw("FL_V1_pivot", "abs(FL_V1_pivot-300)<100", "same");
t->SetLineColor(kBlack);
c3->Update();

 break;


// 2d comparisons
 case 't':
 case 'T':
c9 = new TCanvas("c9", "New Pos v Slope");
t->Draw("CL_V1_slope:CL_V1_pos",
	"abs(CL_V1_pos+0.3)<0.3&&abs(CL_V1_slope-0.8)<0.3");
c10 = new TCanvas("c10", "Old Pos v Slope");
t->Draw("FL_V1_slope:FL_V1_pos",
	"abs(FL_V1_pos+0.3)<0.3&&abs(FL_V1_slope-0.8)<0.3");
 break;

// cross correlations
 case 'c':
 case 'C':

c20 = new TCanvas("c20", "New Pos v Old Pos");
t->Draw("FL_V1_pos:CL_V1_pos", "abs(FL_V1_pos+0.3)<0.3&&abs(CL_V1_pos+0.3)<0.3");
c20->Update();

c21 = new TCanvas("c21", "New Slope v Old Slope");
 t->Draw("FL_V1_slope:CL_V1_slope", 
	 "abs(FL_V1_slope-0.8)<0.3&&abs(CL_V1_slope-0.8)<0.3");
c21->Update();

c22 = new TCanvas("c22", "New Pivot v Old Pivot");
t->Draw("FL_V1_pivot:CL_V1_pivot", 
	"abs(FL_V1_pivot-300)<100&&abs(CL_V1_pivot-300)<100");
c22->Update();


 break;

// differences
 case 'd':
 case 'D':
c30 = new TCanvas("c30", "New Pos - Old Pos");
t->Draw("CL_V1_pos-FL_V1_pos", 
	"(CL_V1_pos-FL_V1_pos)>-0.005&&(CL_V1_pos-FL_V1_pos)<0.005");
TH1F *xdhist = (TH1F*)gPad->GetPrimitive("htemp");
xdhist->Fit("gaus");
 TF1 *xf = xdhist->GetFunction("gaus");
 cout<<"chi-squared         = "<<xf->GetChisquare()<<endl;
 cout<<"reduced chi-squared = "
     <<xf->GetChisquare()/(float)(xf->GetNDF())<<endl;
 cout<<"mean value          = "<<xdhist->GetMean()<<endl;
 cout<<"RMS value           = "<<xdhist->GetRMS()<<endl;
c30->Update();

c31 = new TCanvas("c31", "New Slope - Old Slope");
t->Draw("CL_V1_slope-FL_V1_slope", 
	"(CL_V1_slope-FL_V1_slope+0.2)>-0.2&&(CL_V1_slope-FL_V1_slope+0.2)<0.2");
TH1F *ydhist = (TH1F*)gPad->GetPrimitive("htemp");
ydhist->Fit("gaus");
 TF1 *yf = ydhist->GetFunction("gaus");
 cout<<"chi-squared         = "<<yf->GetChisquare()<<endl;
 cout<<"reduced chi-squared = "
     <<yf->GetChisquare()/(float)(yf->GetNDF())<<endl;
 cout<<"mean value          = "<<ydhist->GetMean()<<endl;
 cout<<"RMS value           = "<<ydhist->GetRMS()<<endl;
c31->Update();

c32 = new TCanvas("c32", "New Pivot - Old Pivot");
t->Draw("CL_V1_pivot-FL_V1_pivot", 
	"(CL_V1_pivot-FL_V1_pivot)>-3&&(CL_V1_pivot-FL_V1_pivot)<3");
/*
TH1F *vdhist = (TH1F*)gPad->GetPrimitive("htemp");
vdhist->Fit("gaus");
 TF1 *vf = xdhist->GetFunction("gaus");
 cout<<"chi-squared         = "<<vf->GetChisquare()<<endl;
 cout<<"reduced chi-squared = "
     <<xf->GetChisquare()/(float)(vf->GetNDF())<<endl;
 cout<<"mean value          = "<<vdhist->GetMean()<<endl;
 cout<<"RMS value           = "<<vdhist->GetRMS()<<endl;
*/

c33 = new TCanvas("c33", "New Pivot - Pos");
t->Draw("(CL_V1_pivot*-.0042426)-CL_V1_pos", 
	"(CL_V1_pivot*-.0042426)-CL_V1_pos+0.78>-0.01&&(CL_V1_pivot*-.0042426)-CL_V1_pos+0.78<0.01");
TH1F *npdhist = (TH1F*)gPad->GetPrimitive("htemp");
npdhist->Fit("gaus");
 TF1 *npf = xdhist->GetFunction("gaus");
 cout<<"chi-squared         = "<<npf->GetChisquare()<<endl;
 cout<<"reduced chi-squared = " 
     <<xf->GetChisquare()/(float)(npf->GetNDF())<<endl;
 cout<<"mean value          = "<<npdhist->GetMean()<<endl;
 cout<<"RMS value           = "<<npdhist->GetRMS()<<endl;
c33->Update();

c34 = new TCanvas("c34", "New Pivot - Pos");
t->Draw("(FL_V1_pivot*-.0042426)-FL_V1_pos", 
	"(FL_V1_pivot*-.0042426)-FL_V1_pos+0.78>-0.01&&(FL_V1_pivot*-.0042426)-FL_V1_pos+0.78<0.01");
TH1F *pdhist = (TH1F*)gPad->GetPrimitive("htemp");
pdhist->Fit("gaus");
 TF1 *pf = xdhist->GetFunction("gaus");
 cout<<"chi-squared         = "<<pf->GetChisquare()<<endl;
 cout<<"reduced chi-squared = " 
     <<xf->GetChisquare()/(float)(pf->GetNDF())<<endl;
 cout<<"mean value          = "<<pdhist->GetMean()<<endl;
 cout<<"RMS value           = "<<pdhist->GetRMS()<<endl;

 break;
 }

 break;

   case '2':
char input[kBUFLEN];
cout<<"Which graphs do you want to see?"<<endl;
cout<<"  (s)imple comparisons"<<endl;
cout<<"  (t)wo-d comparisons"<<endl;
cout<<"  (c)ross correlations"<<endl;
cout<<"  (d)ifferences"<<endl;

input[0] = '\0';
fgets(input, kBUFLEN, stdin);

switch(input[0])
{
// simple comparisons
 case 's':
 case 'S':
   
c1 = new TCanvas("c1", "V2 position");
t->Draw("CL_V2_pos", "abs(CL_V2_pos+0.17)<0.3");
c1->Update();
t->SetLineColor(kRed);
t->Draw("FL_V2_pos", "abs(FL_V2_pos+0.17)<0.3", "same");
t->SetLineColor(kBlack);
c1->Update();

c2 = new TCanvas("c2", "V2 slope");
t->Draw("CL_V2_slope", "abs(CL_V2_slope-0.8)<0.3"); 
c2->Update();
t->SetLineColor(kRed);
t->Draw("FL_V2_slope", "abs(FL_V2_slope-0.8)<0.3", "same");
t->SetLineColor(kBlack);
c2->Update();

c3 = new TCanvas("c3", "V2 pivot wire");
t->Draw("CL_V2_pivot", "abs(CL_V2_pivot-300)<100");
c3->Update();
t->SetLineColor(kRed);
t->Draw("FL_V2_pivot", "abs(FL_V2_pivot-300)<100", "same");
t->SetLineColor(kBlack);
c3->Update();
 break;


// 2d comparisons
 case 't':
 case 'T':
c9 = new TCanvas("c9", "New Pos v Slope");
t->Draw("CL_V2_slope:CL_V2_pos",
	"abs(CL_V2_pos+0.17)<0.3&&abs(CL_V2_slope-0.8)<0.3");
c10 = new TCanvas("c10", "Old Pos v Slope");
t->Draw("FL_V2_slope:FL_V2_pos",
	"abs(FL_V2_pos+0.17)<0.3&&abs(FL_V2_slope-0.8)<0.3");
 break;

// cross correlations
 case 'c':
 case 'C':

c20 = new TCanvas("c20", "New Pos v Old Pos");
t->Draw("FL_V2_pos:CL_V2_pos", "abs(FL_V2_pos+0.17)<0.3&&abs(CL_V2_pos+0.17)<0.3");
c20->Update();

c21 = new TCanvas("c21", "New Slope v Old Slope");
 t->Draw("FL_V2_slope:CL_V2_slope", 
	 "abs(FL_V2_slope-0.8)<0.3&&abs(CL_V2_slope-0.8)<0.3");
c21->Update();

c22 = new TCanvas("c22", "New Pivot v Old Pivot");
t->Draw("FL_V2_pivot:CL_V2_pivot", 
	"abs(FL_V2_pivot-300)<100&&abs(CL_V2_pivot-300)<100");
c22->Update();


 break;

// differences
 case 'd':
 case 'D':
c30 = new TCanvas("c30", "New Pos - Old Pos");
t->Draw("CL_V2_pos-FL_V2_pos", 
	"(CL_V2_pos-FL_V2_pos)>-0.005&&(CL_V2_pos-FL_V2_pos)<0.005");
TH1F *xdhist = (TH1F*)gPad->GetPrimitive("htemp");
xdhist->Fit("gaus");
 TF1 *xf = xdhist->GetFunction("gaus");
 cout<<"chi-squared         = "<<xf->GetChisquare()<<endl;
 cout<<"reduced chi-squared = "
     <<xf->GetChisquare()/(float)(xf->GetNDF())<<endl;
 cout<<"mean value          = "<<xdhist->GetMean()<<endl;
 cout<<"RMS value           = "<<xdhist->GetRMS()<<endl;
c30->Update();

c31 = new TCanvas("c31", "New Slope - Old Slope");
t->Draw("CL_V2_slope-FL_V2_slope", 
	"(CL_V2_slope-FL_V2_slope+0.2)>-0.2&&(CL_V2_slope-FL_V2_slope+0.2)<0.2");
TH1F *ydhist = (TH1F*)gPad->GetPrimitive("htemp");
ydhist->Fit("gaus");
 TF1 *yf = ydhist->GetFunction("gaus");
 cout<<"chi-squared         = "<<yf->GetChisquare()<<endl;
 cout<<"reduced chi-squared = "
     <<yf->GetChisquare()/(float)(yf->GetNDF())<<endl;
 cout<<"mean value          = "<<ydhist->GetMean()<<endl;
 cout<<"RMS value           = "<<ydhist->GetRMS()<<endl;
c31->Update();

c32 = new TCanvas("c32", "New Pivot - Old Pivot");
t->Draw("CL_V2_pivot-FL_V2_pivot", 
	"(CL_V2_pivot-FL_V2_pivot)>-3&&(CL_V2_pivot-FL_V2_pivot)<3");
/*
TH1F *vdhist = (TH1F*)gPad->GetPrimitive("htemp");
vdhist->Fit("gaus");
 TF1 *vf = xdhist->GetFunction("gaus");
 cout<<"chi-squared         = "<<vf->GetChisquare()<<endl;
 cout<<"reduced chi-squared = "
     <<xf->GetChisquare()/(float)(vf->GetNDF())<<endl;
 cout<<"mean value          = "<<vdhist->GetMean()<<endl;
 cout<<"RMS value           = "<<vdhist->GetRMS()<<endl;
*/

c33 = new TCanvas("c33", "New Pivot - Pos");
t->Draw("(CL_V2_pivot*-.0042426)-CL_V2_pos", 
	"(CL_V2_pivot*-.0042426)-CL_V2_pos+1.027>-0.01&&(CL_V2_pivot*-.0042426)-CL_V2_pos+1.027<0.01");
TH1F *npdhist = (TH1F*)gPad->GetPrimitive("htemp");
npdhist->Fit("gaus");
 TF1 *npf = xdhist->GetFunction("gaus");
 cout<<"chi-squared         = "<<npf->GetChisquare()<<endl;
 cout<<"reduced chi-squared = " 
     <<xf->GetChisquare()/(float)(npf->GetNDF())<<endl;
 cout<<"mean value          = "<<npdhist->GetMean()<<endl;
 cout<<"RMS value           = "<<npdhist->GetRMS()<<endl;
c33->Update();

c34 = new TCanvas("c34", "New Pivot - Pos");
t->Draw("(FL_V2_pivot*-.0042426)-FL_V2_pos", 
	"(FL_V2_pivot*-.0042426)-FL_V2_pos+1.032>-0.01&&(FL_V2_pivot*-.0042426)-FL_V2_pos+1.032<0.01");
TH1F *pdhist = (TH1F*)gPad->GetPrimitive("htemp");
pdhist->Fit("gaus");
 TF1 *pf = xdhist->GetFunction("gaus");
 cout<<"chi-squared         = "<<pf->GetChisquare()<<endl;
 cout<<"reduced chi-squared = " 
     <<xf->GetChisquare()/(float)(pf->GetNDF())<<endl;
 cout<<"mean value          = "<<pdhist->GetMean()<<endl;
 cout<<"RMS value           = "<<pdhist->GetRMS()<<endl;
c34->Update();
 break;
}
break;
}
}

// cleanup 
delete f;
}
