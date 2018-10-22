//*-- Author :    Ole Hansen   2-Oct-01

//////////////////////////////////////////////////////////////////////////
//
// THaHRS
//
// The standard Hall A High Resolution Spectrometers (HRS).
//
// The usual name of this object is either "R" or "L", for Left 
// and Right HRS, respectively.
//
// Defines the functions FindVertices() and TrackCalc(), which are common
// to both the LeftHRS and the RightHRS.
//
// By default, the THaHRS class does not define any detectors. (This is new
// in analyzer 1.6 and later.) In this way, the user has full control over
// the detector configuration in the analysis script.
// Detectors should be added with AddDetector() as usual.
//
// However: To maintain backward compatibility with old scripts, the THaHRS
// will auto-create the previous set of standard detectors, "vdc", "s1" and
// "s2", if no "vdc" detector is defined at Init() time.
// This can be turned off by calling AutoStandardDetectors(kFALSE).
//
// For timing calculations, one can specify a reference detector via SetRefDet
// (usually a scintillator) as the detector at the 'reference distance',
// corresponding to the pathlength correction matrix.
//
//////////////////////////////////////////////////////////////////////////

#include "THaHRS.h"
#include "THaTrackingDetector.h"
#include "THaTrack.h"
#include "THaScintillator.h"  // includes THaNonTrackingDetector
#include "THaVDC.h"
#include "THaTrackProj.h"
#include "THaTriggerTime.h"
#include "TMath.h"
#include "TList.h"
#include <cassert>

#ifdef WITH_DEBUG
#include <iostream>
#endif


using namespace std;

//_____________________________________________________________________________
THaHRS::THaHRS( const char* name, const char* description ) :
  THaSpectrometer( name, description ), fRefDet(0)
{
  // Constructor

  SetTrSorting(kFALSE);
  AutoStandardDetectors(kTRUE); // for backward compatibility
}

//_____________________________________________________________________________
THaHRS::~THaHRS()
{
  // Destructor
}

//_____________________________________________________________________________
Bool_t THaHRS::SetTrSorting( Bool_t set )
{
  Bool_t oldset = TestBit(kSortTracks);
  SetBit( kSortTracks, set );
  return oldset;
}

//_____________________________________________________________________________
Bool_t THaHRS::GetTrSorting() const
{
  return TestBit(kSortTracks);
}
 
//_____________________________________________________________________________
Bool_t THaHRS::AutoStandardDetectors( Bool_t set )
{
  Bool_t oldset = TestBit(kAutoStdDets);
  SetBit( kAutoStdDets, set );
  return oldset;
}

//_____________________________________________________________________________
THaAnalysisObject::EStatus THaHRS::Init( const TDatime& run_time )
{
  // Special HRS Init method for approximate backward compatibility with old
  // scripts. If no "vdc" detector has been defined by the user, assume we
  // are being run from an old script and create the old set of "standard
  // detectors" ("vdc", "s1", "s2") at the beginning of the detector list.
  // Note that the old script may have defined non-standard detectors, e.g.
  // Cherenkov, Shower, FPP etc.
  // This behavior can be turned off by calling AutoStandardDetectors(kFALSE).

  if( TestBit(kAutoStdDets) ) {
    THaDetector* pdet = static_cast<THaDetector*>( fDetectors->FindObject("vdc") );
    if( !pdet ) {
      AddDetector( new THaScintillator("s2", "S2 scintillator"), true, true );
      AddDetector( new THaScintillator("s1", "S1 scintillator"), true, true );
#ifndef NDEBUG
      Int_t ret =
#endif
	AddDetector( new THaVDC("vdc", "Vertical Drift Chamber"), true, true );
      assert(ret==0);  // else "vdc" was already defined after all
      assert( fDetectors->GetSize() >= 3 );
    }
  }

  // If the reference detector hasn't been defined yet (via SetRefDet),
  // use "s1", if it exists.
  if( !fRefDet )
    fRefDet = static_cast<THaScintillator*>( GetDetector("s1") );

  // Continue with standard initialization as before
  return THaSpectrometer::Init(run_time);
}

//_____________________________________________________________________________
Int_t THaHRS::SetRefDet( const char* name )
{
  // Set reference detector for TrackTimes calculation to the detector
  // with the given name (typically a scintillator)

  const char* const here = "SetRefDet";

  if( !name || !*name ) {
    Error( Here(here), "Invalid detector name" );
    return 1;
  }

  fRefDet = static_cast<THaNonTrackingDetector*>
    ( fNonTrackingDetectors->FindObject(name) );

  if( !fRefDet ) {
    Error( Here(here), "Can't find detector \"%s\"", name );
    return 1;
  }

  return 0;
}

//_____________________________________________________________________________
Int_t THaHRS::SetRefDet( const THaNonTrackingDetector* obj )
{
  const char* const here = "SetRefDet";

  if( !obj ) {
    Error( Here(here), "Invalid detector pointer" );
    return 1;
  }

  fRefDet = static_cast<THaNonTrackingDetector*>
    ( fNonTrackingDetectors->FindObject(obj) );

  if( !fRefDet ) {
    Error( Here(here), "Can't find given detector. "
	   "Is it a THaNonTrackingDetector?");
    return 1;
  }

  return 0;
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
    if( fDebug>1 ) cout << "Call FineTrack() for " 
			<< theTrackDetector->GetName() << "... ";
#endif
    theTrackDetector->FindVertices( tracks );
#ifdef WITH_DEBUG
    if( fDebug>1 ) cout << "done.\n";
#endif
  }

  // If enabled, sort the tracks by chi2/ndof
  if( GetTrSorting() ) {
    fTracks->Sort();
    // Reassign track indexes. Sorting may have changed the order
    for( int i = 0; i < fTracks->GetLast()+1; i++ ) {
      THaTrack* theTrack = static_cast<THaTrack*>( fTracks->At(i) );
      assert( theTrack );
      theTrack->SetIndex(i);
    }
  }

  // Find the "Golden Track". 
  if( GetNTracks() > 0 ) {
    // Select first track in the array. If there is more than one track
    // and track sorting is enabled, then this is the best fit track
    // (smallest chi2/ndof).  Otherwise, it is the track with the best
    // geometrical match (smallest residuals) between the U/V clusters
    // in the upper and lower VDCs (old behavior).
    // 
    // Chi2/dof is a well-defined quantity, and the track selected in this
    // way is immediately physically meaningful. The geometrical match
    // criterion is mathematically less well defined and not usually used
    // in track reconstruction. Hence, chi2 sortiing is preferable, albeit
    // obviously slower.

    fGoldenTrack = static_cast<THaTrack*>( fTracks->At(0) );
    fTrkIfo      = *fGoldenTrack;
    fTrk         = fGoldenTrack;
  } else
    fGoldenTrack = NULL;

  return 0;
}

//_____________________________________________________________________________
Int_t THaHRS::TrackCalc()
{
  // Additioal track calculations. At present, we only calculate beta here.

  return TrackTimes( fTracks );
}

//_____________________________________________________________________________
Int_t THaHRS::TrackTimes( TClonesArray* Tracks )
{
  // Do the actual track-timing (beta) calculation.
  // Use multiple scintillators to average together and get "best" time at S1.
  //
  // To be useful, a meaningful timing resolution should be assigned
  // to each Scintillator object (part of the database).
  
  if ( !Tracks || !fRefDet ) return -1;
  
  Int_t ntrack = GetNTracks();

  // linear regression to:  t = t0 + pathl/(beta*c)
  //   where t0 is the time of the track at the reference plane (fRefDet).
  //   t0 and beta are solved for.
  //
  for ( Int_t i=0; i < ntrack; i++ ) {
    THaTrack* track = static_cast<THaTrack*>(Tracks->At(i));
    THaTrackProj* tr_ref = static_cast<THaTrackProj*>
      (fRefDet->GetTrackHits()->At(i));
    
    Double_t pathlref = tr_ref->GetPathLen();
    
    Double_t wgt_sum=0.,wx2=0.,wx=0.,wxy=0.,wy=0.;
    Int_t ncnt=0;
    
    // linear regression to get beta and time at ref.
    TIter nextSc( fNonTrackingDetectors );
    THaNonTrackingDetector *det;
    while ( ( det = static_cast<THaNonTrackingDetector*>(nextSc()) ) ) {
      THaScintillator *sc = dynamic_cast<THaScintillator*>(det);
      if ( !sc ) continue;

      const THaTrackProj *trh = static_cast<THaTrackProj*>(sc->GetTrackHits()->At(i));
      
      Int_t pad = trh->GetChannel();
      if (pad<0) continue;
      Double_t pathl = (trh->GetPathLen()-pathlref);
      Double_t time = (sc->GetTimes())[pad];
      Double_t wgt = (sc->GetTuncer())[pad];
      
      if (pathl>.5*kBig || time>.5*kBig) continue;
      if (wgt>0) wgt = 1./(wgt*wgt);
      else continue;
      
      wgt_sum += wgt;
      wx2 += wgt*pathl*pathl;
      wx  += wgt*pathl;
      wxy += wgt*pathl*time;
      wy  += wgt*time;
      ncnt++;
    }

    Double_t beta = kBig;
    Double_t dbeta = kBig;
    Double_t time = kBig;
    Double_t dt = kBig;
    
    Double_t delta = wgt_sum*wx2-wx*wx;
    
    if (delta != 0.) {
      time = (wx2*wy-wx*wxy)/delta;
      dt = TMath::Sqrt(wx2/delta);
      Double_t invbeta = (wgt_sum*wxy-wx*wy)/delta;
      if (invbeta != 0.) {
#if ROOT_VERSION_CODE >= ROOT_VERSION(3,4,0)
	Double_t c = TMath::C();
#else
	Double_t c = 2.99792458e8;
#endif
	beta = 1./(c*invbeta);
	dbeta = TMath::Sqrt(wgt_sum/delta)/(c*invbeta*invbeta);
      }
    } 

    track->SetBeta(beta);
    track->SetdBeta(dbeta);
    track->SetTime(time);
    track->SetdTime(dt);
  }
  return 0;
}

//_____________________________________________________________________________
ClassImp(THaHRS)
