{
//
//  Steering script to check the bpm calibration using a bulls eye scan
//
// by Bodo Reitz, April 2003
//
// needed: ascii table, containing run numbers and results from
//         corresponding harp scans

// some variables:

char harpfname[255];
char expnr[25];
char exppath[200];
char codafname[255];
char buf[255];
char dummy[200];
char *filestatus;
int numofscans=0;

TVector hax(1);  // okay okay, that could be done much nicer
TVector hay(1);  // not giving a size here, but resizing 
TVector hbx(1);  // the vectors each time a new scan is read in
TVector hby(1);

TVector bax(1);
TVector bay(1);
TVector bbx(1);
TVector bby(1);

TH2F *a_xy = new TH2F("a_xy","BPMA x vs y",100,-0.01,0.01,100,-0.01,0.01 );
TH2F *b_xy = new TH2F("b_xy","BPMB x vs y",100,-0.01,0.01,100,-0.01,0.01 );

int dum1,dum2;
double dum3,dum4,dum5,dum6,dum7,dum8,dum9,dum10,dum11,dum12,dum13,dum14;
FILE *fi;

// Set up the equipment to be analyzed.


THaApparatus* BEAM1 = new THaUnRasteredBeam("urb","Beamline");
gHaApps->Add( BEAM1 );

// open table with results from harp runs


TCanvas* c1 = new TCanvas("c1","BPM CheckOut");
c1->Divide(2,3);


cout<<"Filename for Harp results: ";
cin>>harpfname;
cout<<"Experiment name: ";
cin>>expnr;
cout<<"Path for coda files: ";
cin>>exppath;
  
// Set up the event layout for the output file

THaEvent* event = new THaEvent;
  
fi=fopen(harpfname,"r");
  
filestatus=fgets( buf, 255, fi);
while (filestatus !=NULL) {
  sscanf(buf,"%d %d %lf %lf %lf %lf %lf %lf %lf %lf",
	 &dum1,&dum2,&dum3,&dum4,&dum5,&dum6,&dum7,&dum8,&dum9,&dum10);
  sprintf(codafname,"%s%s_%d.dat.0",exppath,expnr,dum1);
  
  hax->ResizeTo(numofscans+1);
  hbx->ResizeTo(numofscans+1);
  hay->ResizeTo(numofscans+1);
  hby->ResizeTo(numofscans+1);
  bax->ResizeTo(numofscans+1);
  bbx->ResizeTo(numofscans+1);
  bay->ResizeTo(numofscans+1);
  bby->ResizeTo(numofscans+1);


  hax(numofscans)=dum3*1.e-6;
  hay(numofscans)=dum5*1.e-6;
  hbx(numofscans)=dum7*1.e-6;
  hby(numofscans)=dum9*1.e-6;
    
  // Set up the analyzer - we use the standard one,
  // but this could be an experiment-specific one as well.
  THaAnalyzer* analyzer = new THaAnalyzer;
  
  // Define the run(s) that we want to analyze.
  // We just set up one, but this could be many.
  THaRun* run = new THaRun( codafname );
  
  // Define the analysis parameters
  
  analyzer->SetEvent( event );
  analyzer->SetOutFile( "Afile.root" );
  run->SetLastEvent( 5000 );
  
  analyzer->Process(*run);
  
  c1->cd(1);
  bpma_x->Draw();
  bpma_x->Fit("gaus");
  TF1* f1=bpma_x->GetFunction("gaus");
  bax(numofscans)=f1->GetParameter("Mean");
  c1->cd(2);
  bpma_y->Draw();
  bpma_y->Fit("gaus");
  TF1* f1=bpma_y->GetFunction("gaus");
  bay(numofscans)=f1->GetParameter("Mean");

  c1->cd(3);
  bpmb_x->Draw();
  bpmb_x->Fit("gaus");
  TF1* f1=bpmb_x->GetFunction("gaus");
  bbx(numofscans)=f1->GetParameter("Mean");
  c1->cd(4);
  bpmb_y->Draw();
  bpmb_y->Fit("gaus");
  TF1* f1=bpmb_y->GetFunction("gaus");
  bby(numofscans)=f1->GetParameter("Mean");

  TGraph *ga=new TGraph(1);
  TGraph *gb=new TGraph(1);

  ga->SetPoint(0,hax(numofscans),hay(numofscans));
  gb->SetPoint(0,hbx(numofscans),hby(numofscans));
  ga->SetMarkerColor(30);
  gb->SetMarkerColor(30);

  c1->cd(5);
  bpma_xy->Draw();
  ga->Draw("*");
  c1->cd(6);
  bpmb_xy->Draw();
  gb->Draw("*");

  a_xy.Add(bpma_xy);
  b_xy.Add(bpmb_xy);


  c1->Update();
  
  numofscans++;
  filestatus=fgets( buf, 255, fi);

  //  cout<<"press any key and enter"<<endl;
  //  cin>>dummy;

}

TGraph *ga=new TGraph(hax,hay);
TGraph *gb=new TGraph(hbx,hby);

TGraph *ga_y=new TGraph(hax,bax);
TGraph *ga_x=new TGraph(hay,bay);
TGraph *gb_y=new TGraph(hbx,bbx);
TGraph *gb_x=new TGraph(hby,bby);

ga->SetMarkerColor(30);
gb->SetMarkerColor(30);

c1->Clear();
c1->Divide(2,3);

c1->cd(1);
ga_x->Draw("a*");
ga_x->Fit("pol1");
c1->cd(2);
ga_y->Draw("a*");
ga_y->Fit("pol1");

c1->cd(3);
gb_x->Draw("a*");
gb_x->Fit("pol1");
c1->cd(4);
gb_y->Draw("a*");
gb_y->Fit("pol1");


c1->cd(5);
a_xy->Draw();
ga->Draw("*");
c1->cd(6);
b_xy->Draw();
gb->Draw("*");
fclose(fi);

}


