//*-- Author :    Ole Hansen   2-Oct-01

//////////////////////////////////////////////////////////////////////////
//
// THaHRS
//
// The standard Hall A High Resolution Spectrometers (HRS).
// Contains three standard detectors,
//    VDC
//    Scintillator S1
//    Scintillator S2
//
// The usual name of this object is either "R" or "L", for Left 
// and Right HRS, respectively.
//
// Defines the functions FindVertices() and TrackCalc(), which are common
// to both the LeftHRS and the RightHRS.
//
// Special configurations of the HRS (e.g. more detectors, different 
// detectors) can be supported in on e of three ways:
//
//   1. Use the AddDetector() method to include a new detector
//      in this apparatus.  The detector will be decoded properly,
//      and its variables will be available for cuts and histograms.
//      Its processing methods will also be called by the generic Reconstruct()
//      algorithm implemented in THaSpectrometer::Reconstruct() and should
//      be correctly handled if the detector class follows the standard 
//      interface design.
//
//   2. Write a derived class that creates the detector in the
//      constructor.  Write a new Reconstruct() method or extend the existing
//      one if necessary.
//
//   3. Write a new class inheriting from THaSpectrometer, using this
//      class as an example.  This is appropriate if your HRS 
//      configuration has fewer or different detectors than the 
//      standard HRS. (It might not be sensible to provide a RemoveDetector() 
//      method since Reconstruct() relies on the presence of the 
//      standard detectors to some extent.)
//
//////////////////////////////////////////////////////////////////////////

#include "THaHRS.h"
#include "THaTrackingDetector.h"
#include "THaTrack.h"
#include "THaScintillator.h"
#include "THaVDC.h"

#ifdef WITH_DEBUG
#include <iostream>
#endif

using namespace std;

//_____________________________________________________________________________
THaHRS::THaHRS( const char* name, const char* description ) :
  THaSpectrometer( name, description )
{
  // Constructor. Defines the standard detectors for the HRS.

  AddDetector( new THaVDC("vdc", "Vertical Drift Chamber"));
  AddDetector( new THaScintillator("s1", "S1 scintillator"));
  AddDetector( new THaScintillator("s2", "S2 scintillator"));
}

//_____________________________________________________________________________
THaHRS::~THaHRS()
{
  // Destructor
}

//_____________________________________________________________________________
Int_t THaHRS::FindVertices( TClonesArray& tracks )
{
  // Reconstruct target coordinates for all tracks found in the focal plane.

  TIter nextTrack( fTrackingDetectors );

  nextTrack.Reset();
  while( THaTrackingDetector* theTrackDetector =
	 static_cast<THaTrackingDetector*>( nextTrack() )) {
#ifdef WITH_DEBUG
    if( fDebug>0 ) cout << "Call FineTrack() for " 
			<< theTrackDetector->GetName() << "... ";
#endif
    theTrackDetector->FindVertices( tracks );
#ifdef WITH_DEBUG
    if( fDebug>0 ) cout << "done.\n";
#endif
  }

  return 0;
}

//_____________________________________________________________________________
Int_t THaHRS::TrackCalc()
{
  // Find the "Golden Track". 

  if( GetNTracks() > 0 ) {
    //FIXME: quick and dirty hack to get started ...
    fGoldenTrack = static_cast<THaTrack*>( fTracks->At(0) );
    fTrkIfo      = *fGoldenTrack;
    fTrk         = fGoldenTrack;
  } else
    fGoldenTrack = NULL;

  return 0;
}

//_____________________________________________________________________________
ClassImp(THaHRS)
