{
//
//  Steering script for Hall A analyzer demo
//

// Set up the equipment to be analyzed.

THaApparatus* HRSR = new THaRightHRS("Right arm HRS");
gHaApps->Add( HRSR );
THaApparatus* HRSL = new THaLeftHRS("Left arm HRS");
gHaApps->Add( HRSL );
THaApparatus* BEAM = new THaBeam("Beamline");
gHaApps->Add( BEAM );

// Set up the event layout for the output file

THaEvent* event = new THaRawEvent;

// Set up the analyzer - we use the standard one,
// but this could be an experiment-specific one as well.

THaAnalyzer* analyzer = new THaAnalyzer;


// Define the run(s) that we want to analyze.
// We just set up one, but this could be many.

THaRun* run = new THaRun( "demo.dat" );


// Define the analysis parameters

analyzer->SetEvent( event );
analyzer->SetOutFile( "Afile.root" );
run->SetLastEvent( 10000 );

}
