{
//
//  script to extract transformation between raster currents 
//   and beam position
//

// by Bodo Reitz in April 2003
//

// Set up the equipment to be analyzed.

THaApparatus* BEAM1 = new THaUnRasteredBeam("urb","Beamline");
gHaApps->Add( BEAM1 );
THaApparatus* BEAM2 = new THaRasteredBeam("rb","Beamline");
gHaApps->Add( BEAM2 );



// Set up the event layout for the output file

THaEvent* event = new THaEvent;

// Set up the analyzer - we use the standard one,
// but this could be an experiment-specific one as well.

THaAnalyzer* analyzer = new THaAnalyzer;

// Define the run that we want to analyze.

cout<<"Enter Filename (including path) coda file:"<<endl;

char filename[255];
cin>>filename;

// e.g. "/data/reitz/misc/raw/e01012_1442.dat"

THaRun* run = new THaRun( filename );

// Define the analysis parameters

analyzer->SetEvent( event );
analyzer->SetOutFile( "Afile.root" );
run->SetLastEvent( -1 );

analyzer->Process(*run);

TCanvas* c1 = new TCanvas("c1","Raster Calibration");

c1->Clear();
c1->Divide(2,4);

c1->cd(1);
rastraw1->Draw();
Double_t rax=rastraw1->GetMean();
Double_t drax=rastraw1->GetRMS();
c1->cd(2);
rastraw2->Draw();
Double_t ray=rastraw2->GetMean();
Double_t dray=rastraw2->GetRMS();

c1->cd(3);
bpma_x->Draw();
Double_t bax=bpma_x->GetMean();
Double_t dbax=bpma_x->GetRMS();
c1->cd(4);
bpma_y->Draw();
Double_t bay=bpma_y->GetMean();
Double_t dbay=bpma_y->GetRMS();

c1->cd(5);
bpmb_x->Draw();
Double_t bbx=bpmb_x->GetMean();
Double_t dbbx=bpmb_x->GetRMS();
c1->cd(6);
bpmb_y->Draw();
Double_t bby=bpmb_y->GetMean();
Double_t dbby=bpmb_y->GetRMS();

c1->cd(5);
bpmb_x->Draw();
Double_t bbx=bpmb_x->GetMean();
Double_t dbbx=bpmb_x->GetRMS();
c1->cd(6);
bpmb_y->Draw();
Double_t bby=bpmb_y->GetMean();
Double_t dbby=bpmb_y->GetRMS();

c1->cd(7);
beam_x->Draw();
Double_t tax=beam_x->GetMean();
Double_t dtax=beam_x->GetRMS();
c1->cd(8);
beam_y->Draw();
Double_t tay=beam_y->GetMean();
Double_t dtay=beam_y->GetRMS();

Double_t bpma_ofx= bax-rax*dbax/drax ; 
Double_t bpma_slx= dbax/drax ; 
Double_t bpma_ofy= bay-ray*dbay/dray; 
Double_t bpma_sly= dbay/dray; 

Double_t bpmb_ofx= bbx-rax*dbbx/drax ; 
Double_t bpmb_slx= dbbx/drax ; 
Double_t bpmb_ofy= bby-ray*dbby/dray; 
Double_t bpmb_sly= dbby/dray; 

Double_t tar_ofx= tax-rax*dtax/drax ; 
Double_t tar_slx= dtax/drax ; 
Double_t tar_ofy= tay-ray*dtay/dray; 
Double_t tar_sly= dtay/dray; 

cout<<" Raster Constants: (please modify database accordingly)"<<endl;
cout<<bpma_ofx<<" "<<bpma_ofy<<" "<<bpma_slx<<" "<<bpma_sly<<" 0. 0."<<endl;
cout<<bpmb_ofx<<" "<<bpmb_ofy<<" "<<bpmb_slx<<" "<<bpmb_sly<<" 0. 0."<<endl;
cout<<tar_ofx<<" "<<tar_ofy<<" "<<tar_slx<<" "<<tar_sly<<" 0. 0."<<endl;


}
