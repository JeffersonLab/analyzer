{
//
//  Steering script to extract the bpm calibration out of a bulls eye scan
//
// by Bodo Reitz, April 2003
//
// needed: ascii table, containing run numbers and results from
//         corresponding harp scans
//         always adjust pedestals before analyzing bulls eye scans
//         never change pedestals without reanalyzing it
//         do not worry, if you dont have new pedestals
//         since the bpm calibration here corrects for them
//         anyhow

// not perfect yet, feel free to improve ;)
// currently the fits put an equal weight on each data point
// one could use a weighted fit instead

char harpfname[255];
char expnr[25];
char exppath[200];
char codafname[255];
char buf[255];
char *filestatus;
int numofscans=0;

TVector hax(1);
TVector hay(1);
TVector hbx(1);
TVector hby(1);

TVector bax(1);
TVector bay(1);
TVector bbx(1);
TVector bby(1);


int dum1,dum2;
double dum3,dum4,dum5,dum6,dum7,dum8,dum9,dum10,dum11,dum12,dum13,dum14;
FILE *fi;
  

// Set up the equipment to be analyzed.

THaApparatus* BEAM1 = new THaUnRasteredBeam("urb","Beamline");
gHaApps->Add( BEAM1 );



TCanvas* c1 = new TCanvas("c1","BPM Pedestals");
c1->Divide(2,2);
  

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
  bpmarot1->Draw();
  bpmarot1->Fit("gaus");
  TF1* f1=bpmarot1->GetFunction("gaus");
  bax(numofscans)=f1->GetParameter("Mean");
  c1->cd(2);
  bpmarot2->Draw();
  bpmarot2->Fit("gaus");
  TF1* f1=bpmarot2->GetFunction("gaus");
  bay(numofscans)=f1->GetParameter("Mean");
    
  c1->cd(3);
  bpmbrot1->Draw();
  bpmbrot1->Fit("gaus");
  TF1* f1=bpmbrot1->GetFunction("gaus");
  bbx(numofscans)=f1->GetParameter("Mean");
  c1->cd(4);
  bpmbrot2->Draw();
  bpmbrot2->Fit("gaus");
  TF1* f1=bpmbrot2->GetFunction("gaus");
  bby(numofscans)=f1->GetParameter("Mean");
  
  c1->Update();
  
  numofscans++;
  filestatus=fgets( buf, 255, fi);
}
  
fclose(fi);

Double_t x11=0.;
Double_t x12=0.; 
Double_t x1yx=0.; 
Double_t x1yy=0.;
Double_t x21=0.; 
Double_t x22=0.; 
Double_t x2yx=0.; 
Double_t x2yy=0.;
Double_t x1=0.; 
Double_t x2=0.; 
Double_t yx=0.; 
Double_t yy=0.;

for(Int_t j=0;j<numofscans;j++) {
  x11=x11+bax(j)*bax(j);
  x12=x12+bax(j)*bay(j);
  x1yx=x1yx+bax(j)*hax(j);
  x1yy=x1yy+bax(j)*hay(j);
  x1=x1+bax(j);
  x2=x2+bay(j);
  yx=yx+hax(j);
  yy=yy+hay(j);
  x21=x21+bay(j)*bax(j);
  x22=x22+bay(j)*bay(j);
  x2yx=x2yx+bay(j)*hax(j);
  x2yy=x2yy+bay(j)*hay(j);
}

TMatrix trafo(3,3);
TVector inhomo(3);

trafo(0,0)=x11;
trafo(0,1)=x12;
trafo(0,2)=x1;
inhomo(0)=x1yx;

trafo(1,0)=x21;
trafo(1,1)=x22;
trafo(1,2)=x2;
inhomo(1)=x2yx;

trafo(2,0)=x1;
trafo(2,1)=x2;
trafo(2,2)=numofscans;
inhomo(2)=yx;

TMatrix itrafo(trafo);
itrafo.Invert();

TVector solu1(inhomo);
solu1*=itrafo;
  
inhomo(0)=x1yy;
inhomo(1)=x2yy;
inhomo(2)=yy;
TVector solu2(inhomo);
solu2*=itrafo;


x11=0.;
x12=0.; 
x1yx=0.; 
x1yy=0.;
x21=0.; 
x22=0.; 
x2yx=0.; 
x2yy=0.;
x1=0.; 
x2=0.; 
yx=0.; 
yy=0.;

for(Int_t j=0;j<numofscans;j++) {
  x11=x11+bbx(j)*bbx(j);
  x12=x12+bbx(j)*bby(j);
  x1yx=x1yx+bbx(j)*hbx(j);
  x1yy=x1yy+bbx(j)*hby(j);
  x1=x1+bbx(j);
  x2=x2+bby(j);
  yx=yx+hbx(j);
  yy=yy+hby(j);
  x21=x21+bby(j)*bbx(j);
  x22=x22+bby(j)*bby(j);
  x2yx=x2yx+bby(j)*hbx(j);
  x2yy=x2yy+bby(j)*hby(j);
}

trafo(0,0)=x11;
trafo(0,1)=x12;
trafo(0,2)=x1;
inhomo(0)=x1yx;

trafo(1,0)=x21;
trafo(1,1)=x22;
trafo(1,2)=x2;
inhomo(1)=x2yx;

trafo(2,0)=x1;
trafo(2,1)=x2;
trafo(2,2)=numofscans;
inhomo(2)=yx;

TMatrix itrafo(trafo);
itrafo.Invert();


TVector solu3(inhomo);
solu3*=itrafo;
  
inhomo(0)=x1yy;
inhomo(1)=x2yy;
inhomo(2)=yy;
TVector solu4(inhomo);
solu4*=itrafo;


cout<<"Please change the BPMA constants to:"<<endl;

cout<<solu1(0)<<" "<<solu1(1)<<" "<<solu2(0)<<" "<<solu2(1)<<" "<<solu1(2)<<" "<<solu2(2)<<endl;

cout<<"Please change the BPMB constants to:"<<endl;

cout<<solu3(0)<<" "<<solu3(1)<<" "<<solu4(0)<<" "<<solu4(1)<<" "<<solu3(2)<<" "<<solu4(2)<<endl;

}


