
{
  // Script to execute the analysis process
  // This is what the user would probably type by hand:

  // when done by hand, just do ".x setup.C"
  gROOT->ProcessLine(".x setup.C");

  // analyzer and run are defined inside setup.C
  // This is NOT good C++, but instead a cheat ROOT
  // permits us to do.
  // Variables defined within nameless macros
  // are visible from the top process.
  run->SetLastEvent( 50 );          // Number of physics events to process
  analyzer->Process(run);             // Start the actual analysis

  // From here is supposed to be valid standard C++
  // gDirectory is a global pointer to the current ROOT directory
  TCanvas* c1 = new TCanvas("c1");
  
  c1->Divide(2,1,0.001,0.001); // Break canvas into 2-by-1 (like 'zone 2 1')

  c1->cd(1); // Select the first 'pad'
  c1->Draw();
  
  // Get and draw a histogram defined by the Output-definition file
  TH2F* kin_diff = (TH2F*)gDirectory->Get("deltaNu");
  if (kin_diff) {
    kin_diff->GetXaxis()->SetTitle("#nu (GeV)");
    kin_diff->GetYaxis()->SetTitle("#nu_{cor}-#nu (GeV)");
    kin_diff->Draw();
  }

  c1->cd(2); // Select the second pad

  // get a pointer to the current pad
  gPad;
  gPad->SetLogy();  // logorithmic Y-axis

  // Get the Tree from the file
  TTree* T = (TTree*)gDirectory->Get("T");
  if (T) {
    T->Draw("R.tr.n");  // Draw the number of tracks in the Right arm
  }

  cout << "\t ** Suggestion: Use the mouse to Zoom-In on the axes of the graphs **"
       << endl;

}
