//*-- Author :    Ole Hansen   07-Sep-00

//////////////////////////////////////////////////////////////////////////
//
// THaSpectrometer
//
// Abstract base class for a spectrometer.
//
// It implements a generic Reconstruct() method that should be
// useful for most situations. See specific description below.
//
// It also provides support for lists of different detector types
// (e.g. tracking/non-tracking/PID) for efficient processing.
//
//////////////////////////////////////////////////////////////////////////

#include "TClonesArray.h"
#include "THaSpectrometer.h"
#include "THaParticleInfo.h"
#include "THaTrackingDetector.h"
#include "THaNonTrackingDetector.h"
#include "THaPidDetector.h"
#include "THaPIDinfo.h"
#include "THaTrack.h"
//#include "THaVertex.h"
#include "TClass.h"
#include "VarDef.h"

#ifdef WITH_DEBUG
#include <iostream>
#endif

ClassImp(THaSpectrometer)

//_____________________________________________________________________________
THaSpectrometer::THaSpectrometer( const char* name, const char* desc ) : 
  THaApparatus( name,desc ), fListInit(kFALSE)
{
  // Constructor.
  // Protected. Can only be called by derived classes.

  fTracks     = new TClonesArray( "THaTrack",   kInitTrackMultiplicity );
  fTrackPID   = new TClonesArray( "THaPIDinfo", kInitTrackMultiplicity );
  //  fVertices   = new TClonesArray( "THaVertex",  kInitTrackMultiplicity );
  fTrackingDetectors    = new TList;
  fNonTrackingDetectors = new TList;
  fPidDetectors         = new TObjArray;
  fPidParticles         = new TObjArray;

  Clear();
  DefinePidParticles();
}

//_____________________________________________________________________________
THaSpectrometer::~THaSpectrometer()
{
  // Destructor. Delete the lists of specialized detectors.
  // FIXME: delete tracks, pid, vertices too?

  fPidParticles->Delete();   //delete all THaParticleInfo objects

  delete fPidParticles;          fPidParticles = 0;
  delete fPidDetectors;          fPidDetectors = 0;
  delete fNonTrackingDetectors;  fNonTrackingDetectors = 0;
  delete fTrackingDetectors;     fTrackingDetectors = 0;
  //  delete fVertices;              fVertices = 0;
  delete fTrackPID;              fTrackPID = 0;
  delete fTracks;                fTracks = 0;

  DefineVariables( kDelete );
}

//_____________________________________________________________________________
Int_t THaSpectrometer::AddDetector( THaDetector* pdet )
{
  // Add a detector to the internal lists of spectrometer detectors.
  // This method is useful for quick testing of a new detector class that 
  // one doesn't want to include permanently in an Apparatus yet.
  // The detector object must be allocated and deleted by the caller.
  // Duplicate detector names are not allowed.

  if( !pdet || !pdet->IsA()->InheritsFrom("THaSpectrometerDetector")) {
    Error("AddDetector", "Detector is not a THaSpectrometerDetector. "
	  "Detector not added.");
    return -1;
  }
  THaSpectrometerDetector* sdet = 
    static_cast<THaSpectrometerDetector*>( pdet );

  Int_t status = THaApparatus::AddDetector( sdet );
  if( status != 0 ) return status;

  if( sdet->IsTracking() )
    fTrackingDetectors->Add( sdet );
  else
    fNonTrackingDetectors->Add( sdet );

  if( sdet->IsPid() )
    fPidDetectors->Add( sdet );

  return 0;
}

//_____________________________________________________________________________
Int_t THaSpectrometer::AddPidParticle( const char* shortname, 
				       const char* name, 
				       Double_t mass, Int_t charge )
{
  // Add a particle type to the list of particles for which we want PID.
  // The "charge" argument is optional.
  
  fPidParticles->Add( new THaParticleInfo( shortname, name, mass, charge ));
  return 0;
}

//_____________________________________________________________________________
Int_t THaSpectrometer::CalcPID()
{
  // Combine the PID information from all detectors into an overall PID
  // for each track.  The actual work is done in the THaPIDinfo class.
  // This is just a loop over all tracks.
  // Called by Reconstruct().

  THaTrack* theTrack;
  THaPIDinfo* pid;

  for( int i = 0; i < fTracks->GetLast()+1; i++ ) {
    if( (theTrack = static_cast<THaTrack*>( fTracks->At(i) ))) 
      if( (pid = theTrack->GetPIDinfo() )) {
	pid->CombinePID();
    }
  }    
  return 0;
}

//_____________________________________________________________________________
void THaSpectrometer::Clear( Option_t* opt )
{
  // Delete contents of internal even-by-event arrays

  fTracks->Delete();
}

//_____________________________________________________________________________
void THaSpectrometer::DefinePidParticles()
{
  // Define the default set of PID particles:
  //  pion, kaon, proton

  fPidParticles->Delete();    //make sure array is empty

  AddPidParticle( "pi", "pion",   0.139, 0 );
  AddPidParticle( "k",  "kaon",   0.440, 0 );
  AddPidParticle( "p",  "proton", 0.938, 1 );
}

//_____________________________________________________________________________
Int_t THaSpectrometer::DefineVariables( EMode mode )
{
  // Define/delete standard variables for a spectrometer (tracks etc.)
  // Can be overridden or extended by derived (actual) apparatuses

  RVarDef vars[] = {
    { "tr.n",    "Number of tracks",             "GetNTracks()" },
    { "tr.x",    "Track x coordinate (m)",       "fTracks.THaTrack.fX" },
    { "tr.y",    "Track x coordinate (m)",       "fTracks.THaTrack.fY" },
    { "tr.th",   "Tangent of track theta angle", "fTracks.THaTrack.fTheta" },
    { "tr.ph",   "Tangent of track phi angle",   "fTracks.THaTrack.fPhi" },
    { "tr.p",    "Track momentum (GeV)",         "fTracks.THaTrack.fP" },
    { "tr.flag", "Track status flag",            "fTracks.THaTrack.fFlag" },
    { 0 }
  };

  return DefineVarsFromList( vars, mode );
}

//_____________________________________________________________________________
void THaSpectrometer::ListInit()
{
  // Initialize lists of specialized detectors. 
  // Private function called by Reconstruct().

  fTrackingDetectors->Clear();
  fNonTrackingDetectors->Clear();
  fPidDetectors->Clear();

  TIter next(fDetectors);
  while( THaSpectrometerDetector* theDetector = 
	 static_cast<THaSpectrometerDetector*>( next() )) {

    if( theDetector->IsTracking() )
      fTrackingDetectors->Add( theDetector );
    else
      fNonTrackingDetectors->Add( theDetector );

    if( theDetector->IsPid() )
      fPidDetectors->Add( theDetector );
  }

  // Set up PIDinfo and vertex objects that can be associated with tracks

  UInt_t ndet  = GetNpidDetectors();
  UInt_t npart = GetNpidParticles();
  TClonesArray& pid  = *fTrackPID;
  //  TClonesArray& vert = *fVertices;

  for( int i = 0; i < kInitTrackMultiplicity; i++ ) {
    new( pid[i] )  THaPIDinfo( ndet, npart );
    // new( vert[i] ) THaVertex();
  }
  
  fListInit = kTRUE;
}

//_____________________________________________________________________________
Int_t THaSpectrometer::Reconstruct()
{
  // This method implements a fairly generic algorithm for spectrometer
  // reconstruction which should be useful for most situations.
  // Additional, equipment-specific processing can be done in
  // a derived class that calls this method first.
  //
  // The algorithm is as follows:
  //
  // For all tracking detectors:
  //   CoarseTrack()
  // --evaluate test block--
  // For all non-tracking detectors:
  //   CoarseProcess()
  // --evaluate test block--
  // For all tracking detectors:
  //   FineTrack()
  // --evaluate test block--
  // For all non-tracking detectors:
  //   FineProcess()
  // --evaluate test block--
  // Compute additional attributes of tracks (e.g. momentum, beta)
  // Find "Golden Track"
  // --evaluate test block--
  // Combine all PID detectors to get overall PID for each track
  // --evaluate test block--
  // Reconstruct tracks to target
  // --evaluate test block--
  //
  // Test blocks can be used to quit processing when appropriate.
  //

  if( !fListInit ) ListInit();

  TIter nextTrack( fTrackingDetectors );
  TIter nextNonTrack( fNonTrackingDetectors );
  TClonesArray& tracks = *fTracks;
  Clear();

  // 1st step: Coarse tracking.  This should be quick and dirty.
  // Any tracks found are put in the fTrack array.

  while( THaTrackingDetector* theTrackDetector =
	 static_cast<THaTrackingDetector*>( nextTrack() )) {
#ifdef WITH_DEBUG
    if( fDebug>0 ) cout << "Call CoarseTrack() for " 
			<< theTrackDetector->GetName() << "... ";
#endif
    theTrackDetector->CoarseTrack( tracks );
#ifdef WITH_DEBUG
    if( fDebug>0 ) cout << "done.\n";
#endif
  }

  // --evaluate test block--

  // 2nd step: Coarse processing.  Pass the coarse tracks to the remaining
  // detectors for any processing that can be done at this stage.
  // This may include clustering and preliminary PID.
  // PID information is tacked onto the tracks as a THaPIDinfo object.

  while( THaNonTrackingDetector* theNonTrackDetector =
	 static_cast<THaNonTrackingDetector*>( nextNonTrack() )) {
#ifdef WITH_DEBUG
    if( fDebug>0 ) cout << "Call CoarseProcess() for " 
			<< theNonTrackDetector->GetName() << "... ";
#endif
    theNonTrackDetector->CoarseProcess( tracks );
#ifdef WITH_DEBUG
    if( fDebug>0 ) cout << "done.\n";
#endif
  }

  // --evaluate test block--

  // 3rd step: Fine tracking.  Compute the tracks with high precision.
  // If coarse tracking was done, this step should simply "refine" the
  // tracks found earlier, not add new tracks to fTrack.

  nextTrack.Reset();
  while( THaTrackingDetector* theTrackDetector =
	 static_cast<THaTrackingDetector*>( nextTrack() )) {
#ifdef WITH_DEBUG
    if( fDebug>0 ) cout << "Call FineTrack() for " 
			<< theTrackDetector->GetName() << "... ";
#endif
    theTrackDetector->FineTrack( tracks );
#ifdef WITH_DEBUG
    if( fDebug>0 ) cout << "done.\n";
#endif
  }

  // --evaluate test block--

  // 4th step: Fine processing.  Pass the precise tracks to the
  // remaining detectors for any precision processing.
  // PID likelihoods should be calculated here.

  nextNonTrack.Reset();
  while( THaNonTrackingDetector* theNonTrackDetector =
	 static_cast<THaNonTrackingDetector*>( nextNonTrack() )) {
#ifdef WITH_DEBUG
    if( fDebug>0 ) cout << "Call FineProcess() for " 
			<< theNonTrackDetector->GetName() << "... ";
#endif
    theNonTrackDetector->FineProcess( tracks );
#ifdef WITH_DEBUG
    if( fDebug>0 ) cout << "done.\n";
#endif
  }

  // Compute additional track properties (e.g. momentum, beta)
  // Find "Golden Track" if appropriate.

  TrackCalc();

  // --evaluate test block--


  // Compute combined PID

  if( fPID ) CalcPID();
      
  // --evaluate test block--


  // Reconstruct tracks to target/vertex

  FindVertices();

  // --evaluate test block--




  return 0;
}


