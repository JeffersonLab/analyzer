
{
  //
  //  Steering script for Hall A analyzer demo
  //
  
  // Set up the equipment to be analyzed.
  
  // add the two spectrometers with the "standard" configuration
  // (VDC planes, S1, and S2)
  THaApparatus* HRSR = new THaHRS("R","Right arm HRS");
  gHaApps->Add( HRSR );

  // Add additional detectors to the Right HRS
  HRSR->AddDetector( new THaCherenkov("cer", "Gas Cherenkov counter" ));
  HRSR->AddDetector( new THaShower("ps", "Preshower counter"));
  HRSR->AddDetector( new THaShower("sh", "Shower counter"));


  THaApparatus* HRSL = new THaHRS("L","Left arm HRS");
  gHaApps->Add( HRSL );
  
  // Add an unrastered, ideally positioned and directed electron beam
  THaApparatus* BEAM = new THaIdealBeam("Beam","Simple ideal beamline");
  gHaApps->Add( BEAM );
  
  // Collect information about a easily modified random set of channels
  // (see DB_DIR/*/db_D.dat)
  THaApparatus* DECDAT = new THaDecData("D","Misc. Decoder Data");
  gHaApps->Add( DECDAT );
  
  // Predefined scaler groups, according to the scaler.map file
  THaScalerGroup* scaler;
  scaler = new THaScalerGroup("Left");
  gHaScalers->Add( scaler );
  scaler = new THaScalerGroup("Right");
  gHaScalers->Add( scaler );

  // Prepare the physics modules to use

  // Watch the electron kinematics
  Double_t amu = 931.494*1.e-3;  // amu to GeV
  Double_t mass_he3 = 2.809413; // in GeV
  Double_t mass_C12 = 12.0*amu; // AMU to GeV
  Double_t mass_tg  = mass_C12;  // chosen for Carbon elastic calibration runs

  // Plain, uncorrected Electron kinematics
  THaPhysicsModule* EK_r = new THaElectronKine("EK_R",
					       "Electron kinematics in HRS-R",
					       "R",mass_tg);
  THaPhysicsModule* EK_l = new THaElectronKine("EK_L",
					       "Electron kinematics in HRS-L",
					       "L",mass_tg);
  
  gHaPhysics->Add( EK_r );
  gHaPhysics->Add( EK_l );

  // Use the reaction vertex to correct for an extended target
  // Cheating for now, and assuming a perfect, unrastered electron beam
  THaPhysicsModule *Rpt_r = new THaReactionPoint("ReactPt_R","Reaction vertex for Right",
						"R","Beam");
  
  THaPhysicsModule *Rpt_l = new THaReactionPoint("ReactPt_L","Reaction vertex for Left",
						"L","Beam");
  
  gHaPhysics->Add( Rpt_r );
  gHaPhysics->Add( Rpt_l );

  // Correct for using an Extended target
  // This needs information about the Point of interaction (hence a THaVertexModule)
  THaPhysicsModule* TgC_r = new THaExtTarCor("ExTgtCor_R",
					     "Corrected for extended target, HRS-R",
					     "R","ReactPt_R");

  THaPhysicsModule* TgC_l = new THaExtTarCor("ExTgtCor_L",
					     "Corrected for extended target, HRS-L",
					     "L","ReactPt_L");
  
  gHaPhysics->Add( TgC_r );
  gHaPhysics->Add( TgC_l );

  // Finally, the CORRECTED Electron kinematics (note change of "spectrometer")
  THaPhysicsModule* EKxc_r = new THaElectronKine("EKxc_R","Electron kinematics in HRS-R",
  						"ExTgtCor_R",mass_tg);
  THaPhysicsModule* EKxc_l = new THaElectronKine("EKxc_L","Electron kinematics in HRS-L",
  						"ExTgtCor_L",mass_tg);
  
  gHaPhysics->Add( EKxc_r );
  gHaPhysics->Add( EKxc_l );


  // Set up the analyzer - we use the standard one,
  // but this could be an experiment-specific one as well.
  // The Analyzer controls the reading of the data, executes
  // tests/cuts, loops over Apparatus's and PhysicsModules,
  // and executes the output routines.
  THaAnalyzer* analyzer = new THaAnalyzer;
  

  // A simple event class to be output to the resulting tree.
  // Creating your own descendant of THaEvent is one way of
  // defining and controlling the output.
  THaEvent* event = new THaEvent;
  
  // Define the run(s) that we want to analyze.
  // We just set up one, but this could be many.
  THaRun* run = new THaRun( "runR.dat" );
  
  // Define the analysis parameters
  analyzer->SetEvent( event );
  analyzer->SetOutFile( "Afile.root" );
  analyzer->SetOdefFile("output_example.def");
  analyzer->SetCutFile("cuts_example.def");        // optional
  
  // File to record cuts accounting information
  analyzer->SetSummaryFile("summary_example.log"); // optional
  
  //analyzer->SetCompressionLevel(0); // turn off compression
  //  analyzer->Process(run);     // start the actual analysis
}
