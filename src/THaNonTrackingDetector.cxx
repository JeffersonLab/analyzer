//*-- Author :    Ole Hansen   7-Sep-00

//////////////////////////////////////////////////////////////////////////
//
// THaNonTrackingDetector
//
// Abstract base class for a generic non-tracking spectrometer detector.
//
// This is a special THaSpectrometerDetector -- any detector that
// is not a tracking detector.  This includes PID detectors.
//
//////////////////////////////////////////////////////////////////////////

#include "THaNonTrackingDetector.h"
#include "TClonesArray.h"
#include "THaTrack.h"
#include "THaTrackProj.h"
#include <cassert>

//______________________________________________________________________________
THaNonTrackingDetector::THaNonTrackingDetector( const char* name,
						const char* description,
						THaApparatus* apparatus )
  : THaSpectrometerDetector(name,description,apparatus)
{
  // Normal constructor with name and description

  fTrackProj = new TClonesArray( "THaTrackProj", 5 );
}

//______________________________________________________________________________
THaNonTrackingDetector::THaNonTrackingDetector()
  : THaSpectrometerDetector(), fTrackProj(0)
{
  // for ROOT I/O only
}

//______________________________________________________________________________
THaNonTrackingDetector::~THaNonTrackingDetector()
{
  // Destructor

  delete fTrackProj; fTrackProj = 0;
}

//_____________________________________________________________________________
void THaNonTrackingDetector::Clear( Option_t* opt )
{
  // Reset per-event data.

  THaSpectrometerDetector::Clear(opt);

  fTrackProj->Clear();
}

//_____________________________________________________________________________
Int_t THaNonTrackingDetector::GetNTracks() const
{
  // Return number of tracks found to be crossing this detector

  Int_t n_cross = 0;
  for( Int_t i=0; i<fTrackProj->GetLast()+1; i++ ) {
    THaTrackProj* proj = static_cast<THaTrackProj*>( fTrackProj->At(i) );
    assert( proj ); // else logic error in CalcTrackProj
    if( proj->IsOK() )
      ++n_cross;
  }
  return n_cross;
}

//_____________________________________________________________________________
Int_t THaNonTrackingDetector::CalcTrackProj( TClonesArray& tracks )
{
  // Calculate projections of all given tracks onto detector reference plane.
  //
  // fTrackProj will hold as many entries as the given track array. Entries
  // corresponding to non-crossing tracks will have fIsOK = false
  //
  // Does not calculate position residual or channel number. This is detector-
  // specific and should be done in the detector code after calling this
  // function.
  //
  // Returns number of tracks found to cross the detector plane.

  Int_t n_track = tracks.GetLast()+1;   // Number of tracks

  fTrackProj->Clear();  // already done in Clear(), but do for safety
  Int_t n_cross = 0;
  for( Int_t i=0; i<n_track; i++ ) {
    THaTrack* theTrack = static_cast<THaTrack*>( tracks.At(i) );
    assert( theTrack );  // else logic error in tracking detector

    Double_t xc = kBig, yc = kBig, pathl = kBig;
    Bool_t found = CalcTrackIntercept( theTrack, pathl, xc, yc );
    THaTrackProj* proj = new ( (*fTrackProj)[i] ) THaTrackProj(xc,yc,pathl);
    if( found && !IsInActiveArea(xc,yc) )
      found = false;
    proj->SetOK(found);
    if( found )
      ++n_cross;
  }
  return n_cross;
}

//_____________________________________________________________________________
ClassImp(THaNonTrackingDetector)
