{
  //
  //  Hall A analyzer demo script
  //

  // Set up the equipment to be analyzed.

  // Right arm HRS spectrometer with the "standard" configuration
  // (VDC planes, S1, and S2) and additional detectors
  THaApparatus* RHRS = new THaHRS("R", "Right arm HRS");
  RHRS->AddDetector( new THaVDC("vdc", "RHRS Vertical drift chamber" ));
  RHRS->AddDetector( new THaScintillator("s1", "RHRS S1 scintillator" ));
  RHRS->AddDetector( new THaScintillator("s2", "RHRS S2 scintillator" ));
  RHRS->AddDetector( new THaCherenkov("cer", "RHRS Gas Cherenkov counter" ));
  RHRS->AddDetector( new THaShower("ps", "RHRS Preshower counter"));
  RHRS->AddDetector( new THaShower("sh", "RHRS Shower counter"));
  gHaApps->Add( RHRS );

  // Left arm HRS spectrometer with the "standard" configuration
  // (VDC planes, S1, and S2)
  THaApparatus* LHRS = new THaHRS("L", "Left arm HRS");
  LHRS->AddDetector( new THaVDC("vdc", "LHRS Vertical drift chamber" ));
  LHRS->AddDetector( new THaScintillator("s1", "LHRS S1 scintillator" ));
  LHRS->AddDetector( new THaScintillator("s2", "LHRS S2 scintillator" ));
  gHaApps->Add( LHRS );

  // Unrastered, ideally positioned and directed electron beam (needed for
  // kinematics calculations below). The "ideal beam" apparatus requires
  // virtually no processing time. For more precise kinematics results,
  // one would use THaUnRasteredBeam or THaRasteredBeam here.
  THaApparatus* BEAM = new THaIdealBeam("Beam", "Simple ideal beamline");
  gHaApps->Add( BEAM );

  // Collect information about an easily modified random set of channels
  // (see DB_DIR/*/db_D.dat).
  THaApparatus* DECDAT = new THaDecData("D", "Misc. Decoder Data");
  gHaApps->Add( DECDAT );

  // Prepare the physics modules to use

  // Watch the electron kinematics
  Double_t amu = 931.494*1.e-3;  // amu to GeV
  Double_t mass_he3 = 2.809413;  // in GeV
  Double_t mass_C12 = 12.0*amu;  // AMU to GeV
  Double_t mass_tg  = mass_C12;  // chosen for Carbon elastic calibration runs

  // Plain, uncorrected Electron kinematics
  THaPhysicsModule* EK_r = new THaElectronKine("EK_R",
                                               "Electron kinematics in HRS-R",
                                               "R", mass_tg);
  THaPhysicsModule* EK_l = new THaElectronKine("EK_L",
                                               "Electron kinematics in HRS-L",
                                               "L", mass_tg);

  gHaPhysics->Add( EK_r );
  gHaPhysics->Add( EK_l );

  // Use the reaction vertex to correct for an extended target.
  // Cheating for now, and assuming a perfect, unrastered electron beam.
  THaPhysicsModule *Rpt_r = new THaReactionPoint("ReactPt_R", "Reaction vertex for Right",
                                                 "R", "Beam");

  THaPhysicsModule *Rpt_l = new THaReactionPoint("ReactPt_L", "Reaction vertex for Left",
                                                 "L", "Beam");

  gHaPhysics->Add( Rpt_r );
  gHaPhysics->Add( Rpt_l );

  // Correct for using an Extended target.
  // This needs information about the Point of interaction (hence a THaVertexModule).
  THaPhysicsModule* TgC_r = new THaExtTarCor("ExTgtCor_R",
                                             "Corrected for extended target, HRS-R",
                                             "R", "ReactPt_R");

  THaPhysicsModule* TgC_l = new THaExtTarCor("ExTgtCor_L",
                                             "Corrected for extended target, HRS-L",
                                             "L", "ReactPt_L");

  gHaPhysics->Add( TgC_r );
  gHaPhysics->Add( TgC_l );

  // Finally, the CORRECTED Electron kinematics (note change of "spectrometer")
  THaPhysicsModule* EKxc_r = new THaElectronKine("EKxc_R", "Electron kinematics in HRS-R",
                                                 "ExTgtCor_R", mass_tg);
  THaPhysicsModule* EKxc_l = new THaElectronKine("EKxc_L", "Electron kinematics in HRS-L",
                                                 "ExTgtCor_L", mass_tg);

  gHaPhysics->Add( EKxc_r );
  gHaPhysics->Add( EKxc_l );


  // Set up the analyzer - we use the standard one,
  // but this could be an experiment-specific one as well.
  // The Analyzer controls the reading of the data, executes
  // tests/cuts, loops over Apparatus's and PhysicsModules,
  // and executes the output routines.
  THaAnalyzer* analyzer = new THaAnalyzer;

  // Define the run(s) that we want to analyze.
  // We just set up one, but this could be many.
  THaRun* run = new THaRun( "runR.dat" );

  // Define the analysis parameters
  analyzer->SetOutFile("$BIGDISK/runR.root");
  analyzer->SetOdefFile("output_example.def");
  analyzer->SetCutFile("cuts_example.def");        // optional

  // File to record cuts accounting information
  analyzer->SetSummaryFile("summary_example.log"); // optional

  //analyzer->SetCompressionLevel(0); // turn off compression

  // Start the actual analysis.
  // This is commented out here so that this script can be run out
  // of the box without any of the input files actually being present.
  // Instead, we print instructions how to go from here.
  cout << "Example setup completed. Type" << endl
       << "  gHaApps->Print()" << endl
       << "to see the defined apparatuses and" << endl
       << "  gHaApps->Print(\"DETS\")" << endl
       << "to see the appratuses and the detectors they contain." << endl
       << endl
       << "To proceed to a real analysis, edit the detector configurations and" << endl
       << "input file names in this script as appropriate. Then, set up a database" << endl
       << "and point $DB_DIR to it (see documentation). Re-run this script and do" << endl
       << "  analyzer->Init(run)" << endl
       << "to test the database. When ready, do" << endl
       << "  analyzer->Process(run)" << endl
       << "to start the actual analysis." << endl;

  //analyzer->Process(run);  // uncomment when ready
}
