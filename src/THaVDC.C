///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDC                                                                    //
//                                                                           //
// High precision wire chamber class.                                        //
//                                                                           //
// Units used:                                                               //
//        For X, Y, and Z coordinates of track    -  meters                  //
//        For Theta and Phi angles of track       -  tan(angle)              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaVDC.h"
#include "THaGlobals.h"
#include "THaEvData.h"
#include "THaDetMap.h"
#include "THaTrack.h"
#include "THaVDCUVPlane.h"
#include "THaVDCUVTrack.h"
#include "THaVDCCluster.h"
#include "THaVDCTrackID.h"
#include "THaVDCTrackPair.h"
#include "TMath.h"
#include "TClonesArray.h"
#include "TList.h"
#include "VarDef.h"

#ifdef WITH_DEBUG
#include "THaGlobals.h"
#include "THaVarList.h"
#include <iostream>
#endif

ClassImp(THaVDC)

//_____________________________________________________________________________
THaVDC::THaVDC( const char* name, const char* description,
		THaApparatus* apparatus ) :
  THaTrackingDetector(name,description,apparatus), fNtracks(0)
{
  // Constructor

  // Create Upper and Lower UV planes
  fLower   = new THaVDCUVPlane( "uv1", "Lower UV Plane", this );
  fUpper   = new THaVDCUVPlane( "uv2", "Upper UV Plane", this );
  if( !fLower || !fUpper || fLower->IsZombie() || fUpper->IsZombie() ) {
    Error( Here("THaVDC()"), "Failed to created subdetectors." );
    MakeZombie();
  }

  fUVpairs = new TClonesArray( "THaVDCTrackPair", 20 );

  // Default behavior for now
  SetBit( kOnlyFastest | kHardTDCcut );
}

//_____________________________________________________________________________
THaDetectorBase::EStatus THaVDC::Init( const TDatime& date )
{
  // Initialize VDC. Calls its own Init(), then initializes subdetectors.

  if( IsZombie() || !fUpper || !fLower )
    return fStatus = kInitError;

  EStatus status;
  if( (status = THaTrackingDetector::Init( date )) ||
      (status = fLower->Init( date )) ||
      (status = fUpper->Init( date )) )
    return fStatus = status;

  return fStatus;
}


//_____________________________________________________________________________
Int_t THaVDC::SetupDetector( const TDatime& date )
{

  if( fIsSetup ) return kOK;
  fIsSetup = true;

  // FIXME: Get these values from database file
  const Double_t degrad = TMath::Pi()/180.0;
  fVDCAngle = -45.0887 *  degrad;  //Convert to radians
  fSin_vdc = TMath::Sin(fVDCAngle);
  fCos_vdc = TMath::Cos(fVDCAngle);
  fTan_vdc = TMath::Tan(fVDCAngle);
  fSpacing = 0.3348; // Dist between similar wire planes (eg u1->u2) (m)

  fNumIter = 1;      // Number of iterations for FineTrack()
  fErrorCutoff = 1e100;

  // FIXME: Set geometry data (fOrigin). Currently fOrigin = (0,0,0).

  fIsInit = true;
  return kOK;
}

//______________________________________________________________________________
THaVDC::~THaVDC()
{
  // Destructor. Delete subdetectors and remove variables from global list.

  if( fIsSetup )
    RemoveVariables();

  delete fLower;
  delete fUpper;
  //  fUVpairs->Delete();
  delete fUVpairs;
}

//______________________________________________________________________________
Int_t THaVDC::ConstructTracks( TClonesArray* tracks, Int_t mode )
{
  // Construct tracks from pairs of upper and lower UV tracks and add 
  // them to 'tracks'

#ifdef WITH_DEBUG
  if( fDebug>0 ) {
    cout << "-----------------------------------------------\n";
    cout << "ConstructTracks: ";
    if( mode == 0 )
      cout << "iterating";
    if( mode == 1 )
      cout << "coarse tracking";
    if( mode == 2 )
      cout << "fine tracking";
    cout << endl;
  }
#endif
  UInt_t theStage = ( mode == 1 ) ? kCoarse : kFine;

  fUVpairs->Clear();

  Int_t nUpperTracks = fUpper->GetNUVTracks();
  Int_t nLowerTracks = fLower->GetNUVTracks();

#ifdef WITH_DEBUG
  if( fDebug>0 )
    cout << "nUpper/nLower = " << nUpperTracks << "  " << nLowerTracks << endl;
#endif

  // No tracks at all -> can't have any tracks
  if( nUpperTracks == 0 && nLowerTracks == 0 ) {
#ifdef WITH_DEBUG
    if( fDebug>0 )
      cout << "No tracks.\n";
#endif
    return 0;
  }

  Int_t nTracks = 0;  // Number of valid particle tracks through the detector
  Int_t nPairs  = 0;  // Number of UV track pairs to consider

  // One plane has no tracks, the other does 
  // -> maybe recoverable with loss of precision
  // FIXME: Only do this if missing cluster recovery flag set
  if( nUpperTracks == 0 || nLowerTracks == 0 ) {
    //FIXME: Put missing cluster recovery code here
    //For now, do nothing
#ifdef WITH_DEBUG
    if( fDebug>0 ) 
      cout << "missing cluster " << nUpperTracks << " " << nUpperTracks << endl;
#endif
    return 0;
  }

  THaVDCUVTrack *track, *partner;
  THaVDCTrackPair *thePair;

  for( int i = 0; i < nLowerTracks; i++ ) {
    track = fLower->GetUVTrack(i);
    if( !track ) 
      continue;

    for( int j = 0; j < nUpperTracks; j++ ) {
      partner = fUpper->GetUVTrack(j);
      if( !partner ) 
	continue;

      // Create new UV track pair.
      thePair = new( (*fUVpairs)[nPairs++] ) THaVDCTrackPair( track, partner );

      // Explicitly mark these UV tracks as unpartnered
      track->SetPartner( NULL );
      partner->SetPartner( NULL );

      // Compute goodness of match parameter
      thePair->Analyze( fSpacing );
    }
  }
      
#ifdef WITH_DEBUG
  if( fDebug>0 )
    cout << nPairs << " pairs.\n";
#endif

  // Initialize some counters
  int n_exist, n_mod = 0;
  int n_oops=0;
  // How many tracks already exist in the global track array?
  if( tracks )
    n_exist = tracks->GetLast()+1;

  // Sort pairs in order of ascending goodness of match
  if( nPairs > 1 )
    fUVpairs->Sort();

  // Mark pairs as partners, starting with the best matches,
  // until all tracks are marked.
  for( int i = 0; i < nPairs; i++ ) {
    if( !(thePair = static_cast<THaVDCTrackPair*>( fUVpairs->At(i) )) )
      continue;

#ifdef WITH_DEBUG
    if( fDebug>0 ) {
      cout << "Pair " << i << ":  " 
	   << thePair->GetUpper()->GetUCluster()->GetPivotWireNum() << " "
	   << thePair->GetUpper()->GetVCluster()->GetPivotWireNum() << " "
	   << thePair->GetLower()->GetUCluster()->GetPivotWireNum() << " "
	   << thePair->GetLower()->GetVCluster()->GetPivotWireNum() << " "
	   << thePair->GetError() << endl;
    }
#endif
    // Stop if track matching error too big
    if( thePair->GetError() > fErrorCutoff )
      break;

    // Get the tracks of the pair
    track   = thePair->GetLower();
    partner = thePair->GetUpper();
    if( !track || !partner ) 
      continue;

    //FIXME: debug
#ifdef WITH_DEBUG
    if( fDebug>0 ) {
      cout << "dUpper/dLower = " 
	   << thePair->GetProjectedDistance( track,partner,fSpacing) << "  "
	   << thePair->GetProjectedDistance( partner,track,-fSpacing);
    }
#endif

    // Skip pairs where any of the tracks already has a partner
    if( track->GetPartner() || partner->GetPartner() ) {
#ifdef WITH_DEBUG
      if( fDebug>0 )
	cout << " ... skipped.\n";
#endif
      continue;
    }
#ifdef WITH_DEBUG
    if( fDebug>0 )
      cout << " ... good.\n";
#endif

    // Make the tracks of this pair each other's partners. This prevents
    // tracks from being associated with more than one valid pair.
    track->SetPartner( partner );
    partner->SetPartner( track );
    thePair->SetStatus(1);

    nTracks++;

    // Compute global track values and get TRANSPORT coordinates for tracks.
    // Replace local slopes and angles with global ones.

    // Slopes
    Double_t du, dv, mu, mv;
    THaVDCCluster 
      *tu = track->GetUCluster(), 
      *tv = track->GetVCluster(), 
      *pu = partner->GetUCluster(),
      *pv = partner->GetVCluster();

    du = pu->GetIntercept() - tu->GetIntercept();
    dv = pv->GetIntercept() - tv->GetIntercept();
    mu = du / fSpacing;
    mv = dv / fSpacing;

    tu->SetSlope(mu);
    tv->SetSlope(mv);
    pu->SetSlope(mu);
    pv->SetSlope(mv);

    // Angles
    Double_t dx = partner->GetX() - track->GetX();
    Double_t dy = partner->GetY() - track->GetY();
    Double_t detTheta = dx / fSpacing;
    Double_t detPhi   = dy / fSpacing;

    track->SetTheta(detTheta);
    track->SetPhi(detPhi);
    partner->SetTheta(detTheta);
    partner->SetPhi(detPhi);

#ifdef WITH_DEBUG
    if( fDebug>2 )
      cout << "Global track parameters: " 
	   << mu << " " << mv << " " 
	   << detTheta << " " << detPhi 
	   << endl;
#endif

    // If the 'tracks' array was given, add THaTracks to it 
    // (or modify existing ones).
    if (tracks) {
      // Calculate Transport coordinates from detector coordinates

      // Note: If not in the focal plane of the spectrometer, transX and transZ
      // need to include a term for the detector z position
      Double_t transX = track->GetX() * fCos_vdc;
      Double_t transY = track->GetY();
      Double_t transZ = -track->GetX() * fSin_vdc;

      Double_t transTheta = (detTheta + fTan_vdc) / (1.0 - detTheta * fTan_vdc);
      Double_t transPhi = detPhi / (fCos_vdc - detTheta * fSin_vdc);

#ifdef WITH_DEBUG
      if( fDebug>2 )
	cout << "Detector coordinates: "
	     << transX << " " << transY << " " << transZ << " "
	     << transTheta << " " << transPhi << endl;
#endif
      // Project these results into the transport plane (transZ = 0)
      ProjToTransPlane(transX, transY, transZ, transTheta, transPhi);
#ifdef WITH_DEBUG
      if( fDebug>2 )
	cout << "Projected coordinates: "
	     << transX << " " << transY << " " << transZ << " "
	     << transTheta << " " << transPhi << endl;
#endif

      // Decide whether this is a new track or an old track 
      // that is being updated
      THaVDCTrackID* thisID = new THaVDCTrackID(track,partner);
      THaTrack* theTrack = NULL;
      bool found = false;
      int t;
      for( t = 0; t < n_exist; t++ ) {
	theTrack = static_cast<THaTrack*>( tracks->At(t) );
	// This test is true if an existing track has exactly the same clusters
	// as the current one (defined by track/partner)
	if( theTrack && theTrack->GetCreator() == this &&
	    *thisID == *theTrack->GetID() ) {
	  found = true;
	  break;
	}
	// FIXME: for debugging
	n_oops++;
      }

      UInt_t flag = theStage;
      if( nPairs > 1 )
	flag |= kMultiTrack;

      if( found ) {
#ifdef WITH_DEBUG
	if( fDebug>0 )
	  cout << "Track " << t << " modified.\n";
#endif
	theTrack->Set( 0.0, transTheta, transPhi, transX, transY );
	delete thisID;
	++n_mod;
      } else {
#ifdef WITH_DEBUG
	if( fDebug>0 )
	  cout << "Track " << tracks->GetLast()+1 << " added.\n";
#endif
	theTrack = 
	  AddTrack(*tracks, 0.0, transTheta, transPhi, transX, transY);
	theTrack->SetID( thisID );
	theTrack->SetCreator( this );
	theTrack->AddCluster( track );
	theTrack->AddCluster( partner );
	flag |= kReassigned;
      }
      theTrack->SetFlag( flag );
    }
  }

#ifdef WITH_DEBUG
  if( fDebug>0 )
    cout << nTracks << " good tracks.\n";
#endif

  // Delete tracks that were not updated
  if( tracks && n_exist > n_mod ) {
    bool modified = false;
    for( int i = 0; i < tracks->GetLast()+1; i++ ) {
      THaTrack* theTrack = static_cast<THaTrack*>( tracks->At(i) );
      // Track created by this class and not updated?
      if( (theTrack->GetCreator() == this) &&
	  ((theTrack->GetFlag() & kStageMask) != theStage ) ) {
#ifdef WITH_DEBUG
	if( fDebug>0 )
	  cout << "Track " << i << " deleted.\n";
#endif
	tracks->RemoveAt(i);
	modified = true;
      }
    }
    // Get rid of empty slots - they may cause trouble in the Event class and
    // with global variables.
    // Note that the PIDinfo and vertices arrays are not reordered.
    // Therefore, PID and vertex information must always be retrieved from the
    // track objects, not from the PID and vertex TClonesArrays.
    // FIXME: Is this really what we want?
    if( modified )
      tracks->Compress();
  }

  return nTracks;
}

//______________________________________________________________________________
void THaVDC::ProjToTransPlane(Double_t& x, Double_t& y, Double_t& z, 
			      Double_t& th, Double_t& ph)
{
  // Project coordinates in the transport coordinate system to the
  // transport (transZ = 0) plane
  Double_t detX = x / fCos_vdc;

  x += fSin_vdc * th * detX;
  y += fSin_vdc * ph * detX;
  z = 0;

}

//______________________________________________________________________________
Int_t THaVDC::Decode( const THaEvData& evdata )
{
  // Decode data from VDC planes
  
  Clear();
  fLower->Decode(evdata); 
  fUpper->Decode(evdata);

  return 0;
}

//______________________________________________________________________________
Int_t THaVDC::CoarseTrack( TClonesArray& tracks )
{
 
#ifdef WITH_DEBUG
  static int nev = 0;
  if( fDebug>0 ) {
    nev++;
    cout << "=========================================\n";
    cout << "Event: " << nev << endl;
  }
#endif

  // Quickly calculate tracks in upper and lower UV planes
  fLower->CoarseTrack();
  fUpper->CoarseTrack();

  // Build tracks and mark them as level 1
  fNtracks = ConstructTracks( &tracks, 1 );

  return 0;
}

//______________________________________________________________________________
Int_t THaVDC::FineTrack( TClonesArray& tracks )
{
  // Calculate exact track position and angle using drift time information.
  // Assumes that CoarseTrack has been called (ie - clusters are matched)
  
  if( TestBit(kCoarseOnly) )
    return 0;

  fLower->FineTrack();
  fUpper->FineTrack();

  // FIXME: Is angle information given to T2D converter?
  for (Int_t i = 0; i < fNumIter; i++) {
    ConstructTracks();
    fLower->FineTrack();
    fUpper->FineTrack();
  }

  fNtracks = ConstructTracks( &tracks, 2 );

#ifdef WITH_DEBUG
  // Wait for user to hit Return
  static char c;
  if( fDebug>1 ) {
    cin.clear();
    while( !cin.eof() && cin.get(c) && c != '\n');
  }
#endif

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
