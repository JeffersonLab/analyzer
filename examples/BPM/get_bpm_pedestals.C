{
//
//  script to extract BPM pedestals using the Hall A analyzer
//

// by Bodo Reitz in April 2003
//
// other files needed
// - analyzer
// - bpm pedestal run:
//   this is a run taken under special run conditions:
//   1.   MCC hast to follow the following procedure
//   1.1    Open BPM window "BPM Diagnostics - SEE"
//   1.2    From there pull down the "Expert Screens" and open "Gain Control"
//   1.3    For IOCSE10, use the pull down window and change from "auto gain"
//          to "forced gain". Then change the Forced Gain values x and y to zero.
//   2.   CODA has to run in pedestal mode
//   the files have usually a distinct filename


// Set up the equipment to be analyzed.

THaApparatus* BEAM1 = new THaUnRasteredBeam("urb","Beamline");
gHaApps->Add( BEAM1 );

// Set up the event layout for the output file

THaEvent* event = new THaEvent;

// Set up the analyzer - we use the standard one,
// but this could be an experiment-specific one as well.

THaAnalyzer* analyzer = new THaAnalyzer;

// Define the run that we want to analyze.

cout<<"Enter Filename (including path) of bpm pedestal data file:"<<endl;

char filename[255];
cin>>filename;

// e.g. "/data/reitz/misc/raw/e01012_ped_1532.dat"

THaRun* run = new THaRun( filename );

// Define the analysis parameters

analyzer->SetEvent( event );
analyzer->SetOutFile( "Afile.root" );
run->SetLastEvent( -1 );

analyzer->Process(*run);

TCanvas* c1 = new TCanvas("c1","BPM Pedestals");
TVector pedestal(8);

c1->Clear();
c1->Divide(2,2);

// would be nice to do the following in a loop, but dont know how to create arrays of
// histograms using the outputdef file from the analyzer

c1->cd(1);
bpmaraw1->Draw();
bpmaraw1->Fit("gaus");
TF1* f1=bpmaraw1->GetFunction("gaus");
pedestal(0)=f1->GetParameter("Mean");

c1->cd(2);
bpmaraw2->Draw();
bpmaraw2->Fit("gaus");
TF1* f1=bpmaraw2->GetFunction("gaus");
pedestal(1)=f1->GetParameter("Mean");

c1->cd(3);
bpmaraw3->Draw();
bpmaraw3->Fit("gaus");
TF1* f1=bpmaraw3->GetFunction("gaus");
pedestal(2)=f1->GetParameter("Mean");

c1->cd(4);
bpmaraw4->Draw();
bpmaraw4->Fit("gaus");
TF1* f1=bpmaraw4->GetFunction("gaus");
pedestal(3)=f1->GetParameter("Mean");

cout<<" Please change the pedestals for BPMA to:"<<endl;
for (Int_t i=0; i<4; i++) {
  cout<<" "<<(Int_t)pedestal(i)<<" ";
}

c1->Update();

cout<<endl<<"Press Any Key and Enter to continue"<<endl;
char dummy[255];
cin>>dummy;

c1->Clear();
c1->Divide(2,2);

// would be nice to do the following in a loop, but dont know how to create arrays of
// histograms using the outputdef file from the analyzer

c1->cd(1);
bpmbraw1->Draw();
bpmbraw1->Fit("gaus");
TF1* f1=bpmbraw1->GetFunction("gaus");
pedestal(0)=f1->GetParameter("Mean");

c1->cd(2);
bpmbraw2->Draw();
bpmbraw2->Fit("gaus");
TF1* f1=bpmbraw2->GetFunction("gaus");
pedestal(1)=f1->GetParameter("Mean");

c1->cd(3);
bpmbraw3->Draw();
bpmbraw3->Fit("gaus");
TF1* f1=bpmbraw3->GetFunction("gaus");
pedestal(2)=f1->GetParameter("Mean");

c1->cd(4);
bpmbraw4->Draw();
bpmbraw4->Fit("gaus");
TF1* f1=bpmbraw4->GetFunction("gaus");
pedestal(3)=f1->GetParameter("Mean");

cout<<" Please change the pedestals for BPMB to:"<<endl;
for (Int_t i=0; i<4; i++) {
  cout<<" "<<(Int_t)pedestal(i)<<" ";
}


cout<<endl<<"Looking forward to working with you again, good bye"<<endl;

}
