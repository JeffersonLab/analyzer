///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDC                                                                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaVDC.h"
#include "THaGlobals.h"
#include "THaEvData.h"
#include "THaDetMap.h"
#include "THaTrack.h"
#include "TMath.h"

#include "THaVDCUVPlane.h"
#include "THaVDCUVTrack.h"
#include "THaVDCCluster.h"
#include "THaVDCTrackID.h"
#include "VarDef.h"

// FIXME: Debug
#include "THaGlobals.h"
#include "THaVarList.h"

ClassImp(THaVDC)

//_____________________________________________________________________________
THaVDC::THaVDC( const char* name, const char* description,
		THaApparatus* apparatus ) :
  THaTrackingDetector(name,description,apparatus), fNtracks(0)
{
  // Constructor

  // Create Upper and Lower UV planes
  fLower = new THaVDCUVPlane( "uv1", "Lower UV Plane", this );
  fUpper = new THaVDCUVPlane( "uv2", "Upper UV Plane", this );
  if( !fLower || !fUpper || fLower->IsZombie() || fUpper->IsZombie() ) {
    Error( Here("THaVDC()"), "Failed to created subdetectors." );
    MakeZombie();
  }
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
 
  fIsInit = true;


  // initialize global variables

  RVarDef vars[] = {
    { "ntracks", "Number of tracks", "fNtracks" },
    { 0 }
  };
  DefineVariables( vars );

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
}

//______________________________________________________________________________
Int_t THaVDC::MatchUVTracks( Int_t flag )
{
  // Match UV tracks from upper UV plane with ones from lower UV plane.
  // Have lower plane find partners

  //FIXME: Improve algorithm

  Int_t nUpperTracks = fUpper->GetNUVTracks();
  Int_t nLowerTracks = fLower->GetNUVTracks();

  if (nUpperTracks == 0 || nLowerTracks == 0)
    return 0;  // Can't have any tracks

  Int_t nTracks = 0;  // Number of particle tracks through the detector
  //  printf("\tfinding partners for lower tracks...\n");
  for (int i = 0; i < nLowerTracks; i++)
    fLower->GetUVTrack(i)->FindPartner(*fUpper->GetUVTracks(), nUpperTracks);

  // Have upper plane find partners, and compare results with lower plane
  //  printf("\tfinding partners for upper tracks...\n");
  for (int i = 0; i < nUpperTracks; i++) {
    THaVDCUVTrack* track = fUpper->GetUVTrack(i);

    if (!track)
      printf ("!-Error Matching UV Tracks!  No Upper Track %d\n", i);

    THaVDCUVTrack* partner = track->FindPartner(*fLower->GetUVTracks(), 
						nLowerTracks);
    if (!partner)
      printf("!-Error Matching UV Tracks! No partner for track %d\n", i);

    if (track == partner->GetPartner()) {
      nTracks++;
    } else {
      // FIXME: Handle special cases where tracks don't "agree" on who their
      // partners ought to be
      // FIXME: Debug
//        THaVar* v = gHaVars->Find("nev");
//        UInt_t nev = (UInt_t) v->GetValue();
//        Warning( Here("MatchUVTracks()"), "Tracks do not partner (%d %d %d) "
//  	       "Event %d, Stage: %s",
//  	       i, nUpperTracks, nLowerTracks, nev, 
//  	       (flag == 1) ? "Coarse" : "Fine"  );
    }
  }   

  return nTracks;
}

//______________________________________________________________________________
Int_t THaVDC::ConstructTracks( TClonesArray * tracks, Int_t flag )
{
  // Construct tracks from pairs of upper and lower UV tracks and add 
  // them to 'tracks'

  int nTracks = MatchUVTracks( flag );

  // How many tracks already there?
  int n_exist, n_mod = 0;
  int n_oops=0;
  if( tracks )
    n_exist = tracks->GetLast()+1;

  // Now compute global track values and get transport coordinates for tracks

  THaVDCUVTrack *track, *partner;
  // Note that i is a counter for the number of successfully matched UV tracks
  // and j is a counter for ALL UV tracks, including ones which were not 
  // successfully matched
  for (int i = 0, j = 0; i < nTracks; i++,j++) {
    track = fLower->GetUVTrack(j);
    if (!track)
      continue; // No track?  Bail!
    partner = track->GetPartner();
    if(!partner)
      continue; //No partner? Bail!

    // For now, a valid track is one with two UV tracks pointing at each other
    while (track != partner->GetPartner()) {
      j++;
      track = fLower->GetUVTrack(j);
      partner = track->GetPartner();
    }
 
    // Replace local slopes and angles with global ones

    // First slopes
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

    // Now update angles
    Double_t dx = partner->GetX() - track->GetX();
    Double_t dy = partner->GetY() - track->GetY();
    Double_t detTheta = dx / fSpacing;
    Double_t detPhi   = dy / fSpacing;

    track->SetTheta(detTheta);
    track->SetPhi(detPhi);
    partner->SetTheta(detTheta);
    partner->SetPhi(detPhi);

    if (tracks) {
      // Calculate Transport coordinates from detector coordinates

      // Note: If not in the focal plane of the spectrometer, transX and transZ
      // need to include a term for the detector z position
      Double_t transX = track->GetX() * fCos_vdc;
      Double_t transY = track->GetY();
      Double_t transZ = -track->GetX() * fSin_vdc;

      Double_t transTheta = (detTheta + fTan_vdc) / (1.0 - detTheta * fTan_vdc);
      Double_t transPhi = detPhi / (fCos_vdc - detTheta * fSin_vdc);

      // Project these results into the transport plane (transZ = 0)
      ProjToTransPlane(transX, transY, transZ, transTheta, transPhi);

      // Decide whether this is a new track or an old track 
      // that is being updated
      THaVDCTrackID* thisID = new THaVDCTrackID(track,partner);
      THaTrack* theTrack = NULL;
      bool found = false;
      for( int t = 0; t < n_exist; t++ ) {
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
      if( found ) {
	theTrack->Set( 0.0, transTheta, transPhi, transX, transY );
	delete thisID;
	++n_mod;
      } else {
	theTrack = 
	  AddTrack(*tracks, 0.0, transTheta, transPhi, transX, transY);
	theTrack->SetID( thisID );
	theTrack->SetCreator( this );
      }
      theTrack->SetFlag(flag);
    }
  }

  // Delete tracks that were not updated
  if( tracks && n_exist > n_mod ) {
    bool modified = false;
    for( int i = 0; i < tracks->GetLast()+1; i++ ) {
      THaTrack* theTrack = static_cast<THaTrack*>( tracks->At(i) );
      // Track created by this class and not updated?
      if( (theTrack->GetCreator() == this) && (theTrack->GetFlag() < flag) ) {
	tracks->RemoveAt(i);
	modified = true;
      }
    }
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
  
  fLower->FineTrack();
  fUpper->FineTrack();

  // FIXME: Is angle information given to T2D converter?
  for (Int_t i = 0; i < fNumIter; i++) {
    ConstructTracks();
    fLower->FineTrack();
    fUpper->FineTrack();
  }

  fNtracks = ConstructTracks( &tracks, 2 );

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
