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
THaApparatus* DECDAT = new THaDecData("Misc. Decoder Data");
gHaApps->Add( DECDAT );

THaScaler* scaler;
scaler = new THaScaler("Left");
gHaScalers->Add( scaler );
scaler = new THaScaler("Right");
gHaScalers->Add( scaler );

// Set up the event layout for the output file

THaEvent* event = new THaEvent;

// Set up the analyzer - we use the standard one,
// but this could be an experiment-specific one as well.

THaAnalyzer* analyzer = new THaAnalyzer;


// Define the run(s) that we want to analyze.
// We just set up one, but this could be many.

THaRun* run = new THaRun( "run.dat" );


// Define the analysis parameters

analyzer->SetEvent( event );
analyzer->SetOutFile( "Afile.root" );
run->SetLastEvent( 10000 );

}
