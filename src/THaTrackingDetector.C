//*-- Author :    Ole Hansen   7-Sep-00

//////////////////////////////////////////////////////////////////////////
//
// THaTrackingDetector
//
// Abstract base class for a generic Hall A tracking detector. 
//
// This is a special THaSpectrometerDetector that is capable of
// finding particle tracks used for momentum analysis and target 
// reconstruction. This is usually a VDC.
//
// Note: The FPP is NOT a "tracking detector" in this scheme because,
// with the present FPP algorithms, it is not used for calculating the 
// fp coordinates. 
//
//////////////////////////////////////////////////////////////////////////

#include "TClonesArray.h"
#include "THaTrackingDetector.h"
#include "THaPIDinfo.h"
//#include "THaVertex.h"
#include "THaTrack.h"
#include "THaSpectrometer.h"
#include "TError.h"

ClassImp(THaTrackingDetector)

//_____________________________________________________________________________
THaTrackingDetector::THaTrackingDetector( const char* name, 
					  const char* description,
					  THaApparatus* apparatus )
  : THaSpectrometerDetector(name,description,apparatus)
{
  // Constructor

}

//_____________________________________________________________________________
THaTrackingDetector::~THaTrackingDetector()
{
  // Destructor

}

//_____________________________________________________________________________
THaTrack* THaTrackingDetector::AddTrack( TClonesArray& tracks,
					 Double_t p, Double_t theta, 
					 Double_t phi, Double_t x, Double_t y,
					 const TClonesArray* clusters )
{
  // Add a track with the given parameters to the array of tracks
  // Returns the index of the created track in the TClonesArray.

  Int_t i = tracks.GetLast()+1;
  THaPIDinfo* pid = 0;
  THaVertex*  vertex = 0;
  THaSpectrometer* spectro = static_cast<THaSpectrometer*>( fApparatus );

  if( spectro ) {

    // Create new PIDinfo and vertex objects if necessary

    UInt_t ndet      = spectro->GetNpidDetectors();
    UInt_t npart     = spectro->GetNpidParticles();
    TClonesArray& c  = *spectro->GetTrackPID();

    // caution: c[i] creates an empty object at i if slot i was empty,
    // so use At(i) to check.

    if( !c.At(i) )  new( c[i] ) THaPIDinfo( ndet, npart );
    pid = static_cast<THaPIDinfo*>( c.At(i) );
  
    //  TClonesArray& c2 = *spectro->GetVertices();
    //  if( !c2.At(i) )  new( c2[i] ) THaVertex();
    //vertex = static_cast<THaVertex*>( c2.At(i) );

  } else {
    ::Warning("THaTrackingDetector::AddTrack", "No spectrometer defined for "
	      "detector %s. Track has no PID and vertex info.", GetName());
  }

  // Create the track

  return new( tracks[i] ) THaTrack( p, theta, phi, x, y, 
				    spectro, clusters, pid, vertex );

}

