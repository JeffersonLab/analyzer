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
#include "THaVDCHit.h"
#include "THaScintillator.h"
#include "THaSpectrometer.h"
#include "TMath.h"
#include "TClonesArray.h"
#include "TList.h"
#include "VarDef.h"
//#include <algorithm>
#include "TROOT.h"

#ifdef WITH_DEBUG
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
    Error( Here("THaVDC()"), "Failed to create subdetectors." );
    MakeZombie();
  }

  fUVpairs = new TClonesArray( "THaVDCTrackPair", 20 );

  // Default behavior for now
  SetBit( kOnlyFastest | kHardTDCcut );

}

//_____________________________________________________________________________
THaAnalysisObject::EStatus THaVDC::Init( const TDatime& date )
{
  // Initialize VDC. Calls standard Init(), then initializes subdetectors.

  if( IsZombie() || !fUpper || !fLower )
    return fStatus = kInitError;

  EStatus status;
  if( (status = THaTrackingDetector::Init( date )) ||
      (status = fLower->Init( date )) ||
      (status = fUpper->Init( date )) )
    return fStatus = status;

  return fStatus = kOK;
}

//_____________________________________________________________________________
Int_t THaVDC::ReadDatabase( FILE* file, const TDatime& date )
{
  // load global VDC parameters
  static const char* const here = "ReadDatabase()";
  const int LEN = 100;
  char buff[LEN];
  
  // Build the search tag and find it in the file. Search tags
  // are of form [ <prefix> ], e.g. [ R.vdc.u1 ].
  TString tag(fPrefix);
  Ssiz_t pos = tag.Index("."); 
  if( pos != kNPOS )
    tag = tag(0,pos+1);
  else
    tag.Append(".");
  tag.Prepend("[");
  tag.Append("global]"); 
  TString line, tag2(tag);
  tag.ToLower();

  bool found = false;
  while (!found && fgets (buff, LEN, file) != NULL) {
    char* buf = ::Compress(buff);  //strip blanks
    //cout<<buf;

    if( strlen(buf) > 0 && buf[ strlen(buf)-1 ] == '\n' )
      buf[ strlen(buf)-1 ] = 0;    //delete trailing newline
    line = buf; line.ToLower();
 
    //cout<<line.Data()<<endl;
    if ( tag == line ) 
      found = true;
    delete [] buf;
  }
  if( !found ) {
    Error(Here(here), "Database entry %s not found!", tag2.Data() );
    return kInitError;
  }

  // We found the entry, now read the data

  // read in some basic constants first
  fscanf(file, "%lf", &fSpacing);
  fgets(buff, LEN, file); // Skip rest of line
  fgets(buff, LEN, file); // Skip line

  // Read in the focal plane transfer elements
  // NOTE: the matrix elements should be stored such that
  // the 3 focal plane matrix elements come first, followed by
  // the other backwards elements, starting with D000
  THaMatrixElement ME;
  char w;
  bool good = false;

  // read in t000 and verify it
  ME.iszero = true;  ME.order = 0;
  // Read matrix element signature
  if( fscanf(file, "%c %d %d %d", &w, &ME.pw[0], &ME.pw[1], &ME.pw[2]) == 4) {
    if( w == 't' && ME.pw[0] == 0 && ME.pw[1] == 0 && ME.pw[2] == 0 ) {
      good = true;
      for(int p_cnt=0; p_cnt<kPORDER; p_cnt++) {
	if(!fscanf(file, "%le", &ME.poly[p_cnt])) {
	  good = false;
	  break;
	}
	if(ME.poly[p_cnt] != 0.0) {
	  ME.iszero = false;
	  ME.order = p_cnt+1;
	}
      }
    }
  }
  if( good ) 
    fFPMatrixElems.push_back(ME);
  else {
    Error(Here(here), "Could not read in Matrix Element T000!");
    return kInitError;
  }
  fscanf(file, "%*c");

  // read in y000 and verify it
  ME.iszero = true;  ME.order = 0; good = false;
  // Read matrix element signature
  if( fscanf(file, "%c %d %d %d", &w, &ME.pw[0], &ME.pw[1], &ME.pw[2]) == 4) {
    if( w == 'y' && ME.pw[0] == 0 && ME.pw[1] == 0 && ME.pw[2] == 0 ) {
      good = true;
      for(int p_cnt=0; p_cnt<kPORDER; p_cnt++) {
	if(!fscanf(file, "%le", &ME.poly[p_cnt])) {
	  good = false;
	  break;
	}
	if(ME.poly[p_cnt] != 0.0) {
	  ME.iszero = false;
	  ME.order = p_cnt+1;
	}
      }
    }
  }
  if( good )
    fFPMatrixElems.push_back(ME);
  else {
    Error(Here(here), "Could not read in Matrix Element Y000!");
    return kInitError;
  }
  fscanf(file, "%*c");

  // read in p000 and verify it
  ME.iszero = true;  ME.order = 0; good = false;
  if( fscanf(file, "%c %d %d %d", &w, &ME.pw[0], &ME.pw[1], &ME.pw[2]) == 4) {
    if( w == 'p' && ME.pw[0] == 0 && ME.pw[1] == 0 && ME.pw[2] == 0 ) {
      good = true;
      for(int p_cnt=0; p_cnt<kPORDER; p_cnt++) {
	if(!fscanf(file, "%le", &ME.poly[p_cnt])) {
	  good = false;
	  break;
	}
	if(ME.poly[p_cnt] != 0.0) {
	  ME.iszero = false;
	  ME.order = p_cnt+1;
	}
      }
    }
  }
  if( good )
    fFPMatrixElems.push_back(ME);
  else {
    Error(Here(here), "Could not read in Matrix Element P000!");
    return kInitError;
  }
  fscanf(file, "%*c");

  // Read in as many of the matrix elements as there are
  while(fscanf(file, "%c %d %d %d", &w, &ME.pw[0], 
	       &ME.pw[1], &ME.pw[2]) == 4) {

    ME.iszero = true;  ME.order = 0;
    for(int p_cnt=0; p_cnt<kPORDER; p_cnt++) {
      if(!fscanf(file, "%le", &ME.poly[p_cnt])) {
	Error(Here(here), "Could not read in Matrix Element %c%d%d%d!",
	      w, ME.pw[0], ME.pw[1], ME.pw[2]);
	return kInitError;
      }

      if(ME.poly[p_cnt] != 0.0) {
	ME.iszero = false;
	ME.order = p_cnt+1;
      }
    }
    
    // Add this matrix element to the appropriate array
    switch(w) {
    case 'D': fDMatrixElems.push_back(ME); break;
    case 'T': fTMatrixElems.push_back(ME); break;
    case 'Y': fYMatrixElems.push_back(ME); break;
    case 'P': fPMatrixElems.push_back(ME); break;
    default:
      Error(Here(here), "Invalid Matrix Element specifier: %c!", w);
      break;
    }

    fscanf(file, "%*c");

    if(feof(file) || ferror(file))
      break;
  }

  // Compute derived quantities and set some hardcoded parameters
  const Float_t degrad = TMath::Pi()/180.0;
  fTan_vdc  = fFPMatrixElems[T000].poly[0];
  fVDCAngle = TMath::ATan(fTan_vdc);
  fSin_vdc  = TMath::Sin(fVDCAngle);
  fCos_vdc  = TMath::Cos(fVDCAngle);

  DefineAxes((90.0 - fVDCAngle)*degrad);

  fNumIter = 1;      // Number of iterations for FineTrack()
  fErrorCutoff = 1e100;

  // figure out the track length from the origin to the s1 plane

  // since we take the VDC to be the origin of the coordinate
  // space, this is actually pretty simple
  const THaDetector* s1 = fApparatus->GetDetector("s1");
  if(s1 == NULL)
    fCentralDist = 0;
  else
    fCentralDist = s1->GetOrigin().Z();

  // FIXME: Set geometry data (fOrigin). Currently fOrigin = (0,0,0).

  fIsInit = true;
  return kOK;
}

//_____________________________________________________________________________
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

//_____________________________________________________________________________
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
    // Replace local cluster slopes with global ones, 
    // which have higher precision.

    THaVDCCluster 
      *tu = track->GetUCluster(), 
      *tv = track->GetVCluster(), 
      *pu = partner->GetUCluster(),
      *pv = partner->GetVCluster();

    Double_t du = pu->GetIntercept() - tu->GetIntercept();
    Double_t dv = pv->GetIntercept() - tv->GetIntercept();
    Double_t mu = du / fSpacing;
    Double_t mv = dv / fSpacing;

    tu->SetSlope(mu);
    tv->SetSlope(mv);
    pu->SetSlope(mu);
    pv->SetSlope(mv);

    // Recalculate the UV track's detector coordinates using the global
    // U,V slopes.
    track->CalcDetCoords();
    partner->CalcDetCoords();

#ifdef WITH_DEBUG
    if( fDebug>2 )
      cout << "Global track parameters: " 
	   << mu << " " << mv << " " 
	   << track->GetTheta() << " " << track->GetPhi()
	   << endl;
#endif

    // If the 'tracks' array was given, add THaTracks to it 
    // (or modify existing ones).
    if (tracks) {

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
        delete thisID;
        ++n_mod;
      } else {
#ifdef WITH_DEBUG
	if( fDebug>0 )
	  cout << "Track " << tracks->GetLast()+1 << " added.\n";
#endif
	theTrack = AddTrack(*tracks, 0.0, 0.0, 0.0, 0.0, thisID );
	//	theTrack->SetID( thisID );
	//	theTrack->SetCreator( this );
	theTrack->AddCluster( track );
	theTrack->AddCluster( partner );
	if( theStage == kFine ) 
	  flag |= kReassigned;
      }

      theTrack->SetD(track->GetX(), track->GetY(), track->GetTheta(), 
		     track->GetPhi());
      theTrack->SetFlag( flag );

      // calculate the TRANSPORT coordinates
      CalcFocalPlaneCoords(theTrack, kRotatingTransport);
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

//_____________________________________________________________________________
Int_t THaVDC::Decode( const THaEvData& evdata )
{
  // Decode data from VDC planes
  
  Clear();
  fLower->Decode(evdata); 
  fUpper->Decode(evdata);

  return 0;
}

//_____________________________________________________________________________
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

//_____________________________________________________________________________
Int_t THaVDC::FineTrack( TClonesArray& tracks )
{
  // Calculate exact track position and angle using drift time information.
  // Assumes that CoarseTrack has been called (ie - clusters are matched)
  
  if( TestBit(kCoarseOnly) )
    return 0;

  fLower->FineTrack();
  fUpper->FineTrack();

  //FindBadTracks(tracks);
  //CorrectTimeOfFlight(tracks);

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

//_____________________________________________________________________________
Int_t THaVDC::FindVertices( TClonesArray& tracks )
{
  // Calculate the target location and momentum at the target.
  // Assumes that CoarseTrack() and FineTrack() have both been called.

  Int_t n_exist = tracks.GetLast()+1;
  for( Int_t t = 0; t < n_exist; t++ ) {
    THaTrack* theTrack = static_cast<THaTrack*>( tracks.At(t) );
    CalcTargetCoords(theTrack, kRotatingTransport);
  }

  return 0;
}

//_____________________________________________________________________________
void THaVDC::CalcFocalPlaneCoords( THaTrack* track, const ECoordTypes mode )
{
  // calculates focal plane coordinates from detector coordinates

  Double_t tan_rho, cos_rho, tan_rho_loc, cos_rho_loc;
  // TRANSPORT coordinates (projected to z=0)
  Double_t x, y, theta, phi;
  // Rotating TRANSPORT coordinates
  Double_t r_x, r_y, r_theta, r_phi;
  
  // tan rho (for the central ray) is stored as a matrix element 
  tan_rho = fFPMatrixElems[T000].poly[0];
  cos_rho = 1.0/sqrt(1.0+tan_rho*tan_rho);

  // first calculate the transport frame coordinates
  theta = (track->GetDTheta()+tan_rho) /
    (1.0-track->GetDTheta()*tan_rho);
  x = track->GetDX() * cos_rho * (1.0 + theta * tan_rho);
  phi = track->GetDPhi() / (1.0-track->GetDTheta()*tan_rho) / cos_rho;
  y = track->GetDY() + tan_rho*phi*track->GetDX()*cos_rho;
  
  // then calculate the rotating transport frame coordinates
  r_x = x;

  // calculate the focal-plane matrix elements
  if(mode == kTransport)
    CalcMatrix( x, fFPMatrixElems );
  else if (mode == kRotatingTransport)
    CalcMatrix( r_x, fFPMatrixElems );

  r_y = y - fFPMatrixElems[Y000].v;  // Y000

  // Calculate now the tan(rho) and cos(rho) of the local rotation angle.
  tan_rho_loc = fFPMatrixElems[T000].v;   // T000
  cos_rho_loc = 1.0/sqrt(1.0+tan_rho_loc*tan_rho_loc);
  
  r_phi = (track->GetDPhi() - fFPMatrixElems[P000].v /* P000 */ ) / 
    (1.0-track->GetDTheta()*tan_rho_loc) / cos_rho_loc;
  r_theta = (track->GetDTheta()+tan_rho_loc) /
    (1.0-track->GetDTheta()*tan_rho_loc);

  // set the values we calculated
  track->Set(x, y, theta, phi);
  track->SetR(r_x, r_y, r_theta, r_phi);

}

//_____________________________________________________________________________
void THaVDC::CalcTargetCoords(THaTrack *track, const ECoordTypes mode)
{
  // calculates target coordinates from focal plane coordinates

  const Int_t kNUM_PRECOMP_POW = 10;

  Double_t x_fp, y_fp, th_fp, ph_fp;
  Double_t powers[kNUM_PRECOMP_POW][3];  // {th, y, ph}
  Double_t x, y, theta, phi, dp, p;

  // first select the coords to use
  if(mode == kTransport) {
    x_fp = track->GetX();
    y_fp = track->GetY();
    th_fp = track->GetTheta();
    ph_fp = track->GetPhi();
  } else if(mode == kRotatingTransport) {
    x_fp = track->GetRX();
    y_fp = track->GetRY();
    th_fp = track->GetRTheta();
    ph_fp = track->GetRPhi();  
  }

  // calculate the powers we need
  for(int i=0; i<kNUM_PRECOMP_POW; i++) {
    powers[i][0] = pow(th_fp, i);
    powers[i][1] = pow(y_fp, i);
    powers[i][2] = pow(ph_fp, i);
  }

  // calculate the matrices we need
  CalcMatrix(x_fp, fDMatrixElems);
  CalcMatrix(x_fp, fTMatrixElems);
  CalcMatrix(x_fp, fYMatrixElems);
  CalcMatrix(x_fp, fPMatrixElems);

  // calculate the coordinates at the target
  theta = CalcTargetVar(fTMatrixElems, powers);
  phi = CalcTargetVar(fPMatrixElems, powers);
  y = CalcTargetVar(fYMatrixElems, powers);

  // calculate momentum
  dp = CalcTargetVar(fDMatrixElems, powers);
  p  = static_cast<THaSpectrometer*>(fApparatus)->GetPcentral() * (1.0+dp);

  //FIXME: estimate x ??
  x = 0.0;

  // Save the target quantities with the tracks
  track->SetTarget(x, y, theta, phi);
  track->SetDp(dp);
  track->SetMomentum(p);
  static_cast<THaSpectrometer*>(fApparatus)->
    TransportToLab( p, theta, phi, track->GetPvect() );

}


//_____________________________________________________________________________
void THaVDC::CalcMatrix( const Double_t x, vector<THaMatrixElement>& matrix )
{
  // calculates the values of the matrix elements for a given location
  // by evaluating a polynomial in x of order it->order with 
  // coefficients given by it->poly

  for( vector<THaMatrixElement>::iterator it=matrix.begin();
       it!=matrix.end(); it++ ) {
    it->v = 0.0;

    if(it->iszero == false) {
      for(int i=it->order-1; i>=1; i--)
	it->v = x * (it->v + it->poly[i]);
      it->v += it->poly[0];
    }
  }
}

//_____________________________________________________________________________
Double_t THaVDC::CalcTargetVar(const vector<THaMatrixElement>& matrix,
			       const Double_t powers[][3])
{
  // calculates the value of a variable at the target

  Double_t retval=0.0;
  for( vector<THaMatrixElement>::const_iterator it=matrix.begin();
       it!=matrix.end(); it++ ) 
    if(it->v != 0.0)
      retval += it->v * powers[it->pw[0]][0]
	              * powers[it->pw[1]][1]
	              * powers[it->pw[2]][2];

  return retval;
}

//_____________________________________________________________________________
void THaVDC::CorrectTimeOfFlight(TClonesArray& tracks)
{
  const static Double_t v = 3.0e-8;   // for now, assume that everything travels at c

  // get scintillator planes
  THaDetector* s1 = fApparatus->GetDetector("s1");
  THaDetector* s2 = fApparatus->GetDetector("s2");

  if( (s1 == NULL) || (s2 == NULL) )
    return;

  // adjusts caluculated times so that the time of flight to S1
  // is the same as a track going through the middle of the VDC
  // (i.e. x_det = 0) at a 45 deg angle (theta_t and phi_t = 0)
  // assumes that at least the coarse tracking has been performed

  Int_t n_exist = tracks.GetLast()+1;
  //cerr<<"num tracks: "<<n_exist<<endl;
  for( Int_t t = 0; t < n_exist; t++ ) {
    THaTrack* track = static_cast<THaTrack*>( tracks.At(t) );
    TList* clusters = track->GetClusters();
    
    // calculate the correction, since it's on a per track basis
    Double_t s1_dist, vdc_dist, dist, tdelta;
    if(!s1->CalcPathLen(track, s1_dist))
      s1_dist = 0.0;
    if(!CalcPathLen(track, vdc_dist))
      vdc_dist = 0.0;

    // since the z=0 of the transport coords is inclined with respect
    // to the VDC plane, the VDC correction depends on the location of
    // the track
    if( track->GetX() < 0 )
      dist = s1_dist + vdc_dist;
    else
      dist = s1_dist - vdc_dist;
    
    tdelta = ( fCentralDist - dist) / v;
    //cout<<"time correction: "<<tdelta<<endl;

    // apply the correction
    Int_t n_clust = clusters->GetSize();
    for( Int_t i = 0; i < n_clust; i++ ) {
      THaVDCUVTrack* the_uvtrack = 
	static_cast<THaVDCUVTrack*>( clusters->At(i) );
      if(the_uvtrack == NULL)
	continue;
      
      //FIXME: clusters guaranteed to be nonzero?
      the_uvtrack->GetUCluster()->SetTimeCorrection(tdelta);
      the_uvtrack->GetVCluster()->SetTimeCorrection(tdelta);
    }
  }
}

//_____________________________________________________________________________
void THaVDC::FindBadTracks(TClonesArray& tracks)
{
  // Flag tracks that don't intercept S2 scintillator as bad

  THaDetector* s2 = fApparatus->GetDetector("s2");

  if(s2 == NULL) {
    //cerr<<"Could not find s2 plane!!"<<endl;
    return;
  }

  Int_t n_exist = tracks.GetLast()+1;
  for( Int_t t = 0; t < n_exist; t++ ) {
    THaTrack* track = static_cast<THaTrack*>( tracks.At(t) );
    Double_t x2, y2;

    // project the current x and y positions into the s2 plane
    if(!s2->CalcInterceptCoords(track, x2, y2)) {
      x2 = 0.0;
      y2 = 0.0;
    } 

    // if the tracks go out of the bounds of the s2 plane,
    // toss the track out
    if( (TMath::Abs(x2 - s2->GetOrigin().X()) > s2->GetSize()[0]) ||
	(TMath::Abs(y2 - s2->GetOrigin().Y()) > s2->GetSize()[1]) ) {

      // for now, we just flag the tracks as bad
      track->SetFlag( track->GetFlag() | kBadTrack );

      //tracks.RemoveAt(t);
#ifdef WITH_DEBUG
      //cout << "Track " << t << " deleted.\n";
#endif  
    }
  }

  // get rid of the slots for the deleted tracks
  //tracks.Compress();
}


////////////////////////////////////////////////////////////////////////////////
