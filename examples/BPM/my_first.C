{
//
//  Steering script for Hall A analyzer demo
//

// Set up the equipment to be analyzed.


THaApparatus* BEAM2 = new THaRasteredBeam("rb","Beamline");
gHaApps->Add( BEAM2 );
THaApparatus* BEAM1 = new THaUnRasteredBeam("urb","Beamline");
gHaApps->Add( BEAM1 );



// Set up the event layout for the output file

THaEvent* event = new THaEvent;



// Set up the analyzer - we use the standard one,
// but this could be an experiment-specific one as well.

THaAnalyzer* analyzer = new THaAnalyzer;


// Define the run(s) that we want to analyze.
// We just set up one, but this could be many.

THaRun* run = new THaRun( "/data/reitz/misc/raw/e01012_ped_1532.dat" );

// Define the analysis parameters

analyzer->SetEvent( event );
analyzer->SetOutFile( "Afile.root" );
run->SetLastEvent( 10000 );


analyzer->Process(*run);

}
