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
//#include <algorithm>

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
Int_t THaVDC::ReadDatabase( FILE* file, const TDatime& date )
{
  // load global VDC parameters
  static const char* const here = "ReadDatabase()";
  const int LEN = 100;
  char buff[LEN];
  
  // Build the search tag and find it in the file. Search tags
  // are of form [ <prefix> ], e.g. [ R.vdc.u1 ].
  TString tag(fPrefix); tag.Resize(2); tag.Append("global"); 
  tag.Prepend("["); tag.Append("]"); 
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

  // we found the entry we needed, so we need to read in the data

  // read in some basic constants first
  fscanf(file, "%lf", &fSpacing);
  fgets(buff, LEN, file); // Skip rest of line

  cout<<"VDC Angle: "<<fVDCAngle<<"  plane spacing: "<<fSpacing<<endl;

  fgets(buff, LEN, file); // Skip line

  // now, read in the focal plane transfer elements
  // NOTE: the matrix elements should be stored such that
  // T000, Y000, and P000 are first, in that order, then come the
  // rest of them beginning with D000
  /** FIXME: need to simplify input code **/
  char w;
  int i,j,k;
  vector<double> poly(kPORDER);
  THaMatrixElement temp_elem;
  
  // read in T000 and verify it
  if( (fscanf(file, "%c %d %d %d", &w, &i, &j, &k) != 4) ||
      (w!='T') || (i!=0) || (j!=0) || (k!=0) ) {
    Error(Here(here), "Matrix Element T000 not found!");
    return kInitError;
  }
  for(int p_cnt=0; p_cnt<kPORDER; p_cnt++)
    if(!fscanf(file, "%le", &poly[p_cnt])) {
      Error(Here(here), "Could not read in Matrix Element T000!");
      return kInitError;
    }
  // assign data
  temp_elem.pw[0] = i; temp_elem.pw[1] = j; temp_elem.pw[2] = k;
  temp_elem.poly = poly;
    for(int j=0; j<kPORDER; j++)
      if(poly[j] != 0.0) {
	temp_elem.iszero = false;
	temp_elem.order = j;
      }
  fFPMatrixElems.push_back(temp_elem);

  // read in Y000 and verify it
  fscanf(file, "%*c");
  if( (fscanf(file, "%c %d %d %d", &w, &i, &j, &k) != 4) ||
      (w!='Y') || (i!=0) || (j!=0) || (k!=0) ) {
    Error(Here(here), "Matrix Element Y000 not found!");
    return kInitError;
  }
  for(int p_cnt=0; p_cnt<kPORDER; p_cnt++)
    if(!fscanf(file, "%le", &poly[p_cnt])) {
      Error(Here(here), "Could not read in Matrix Element Y000!");
      return kInitError;
    }
  // assign data
  temp_elem.pw[0] = i; temp_elem.pw[1] = j; temp_elem.pw[2] = k;
  temp_elem.poly = poly;
    for(int j=0; j<kPORDER; j++)
      if(poly[j] != 0.0) {
	temp_elem.iszero = false;
	temp_elem.order = j;
      }
  fFPMatrixElems.push_back(temp_elem);


  // read in P000 and verify it
  fscanf(file, "%*c");
  if( (fscanf(file, "%c %d %d %d", &w, &i, &j, &k) != 4) ||
      (w!='P') || (i!=0) || (j!=0) || (k!=0) ) {
    Error(Here(here), "Matrix Element P000 not found!");
    return kInitError;
  }
  for(int p_cnt=0; p_cnt<kPORDER; p_cnt++)
    if(!fscanf(file, "%le", &poly[p_cnt])) {
      Error(Here(here), "Could not read in Matrix Element P000!");
      return kInitError;
    }
  // assign data
  temp_elem.pw[0] = i; temp_elem.pw[1] = j; temp_elem.pw[2] = k;
  temp_elem.poly = poly;
    for(int j=0; j<kPORDER; j++)
      if(poly[j] != 0.0) {
	temp_elem.iszero = false;
	temp_elem.order = j;
      }
  fFPMatrixElems.push_back(temp_elem);
  
  // now, read in as many of the matrix elements as there are
  fscanf(file, "%*c");
  while(fscanf(file, "%c %d %d %d", &w, &i, &j, &k) == 4) {

    for(int p_cnt=0; p_cnt<kPORDER; p_cnt++)
      if(!fscanf(file, "%le", &poly[p_cnt])) {
	Error(Here(here), "Could not read in Matrix Element %c%d%d%d!",
	      w, i, j, k);
	return kInitError;
      }
    
    temp_elem.pw[0] = i;
    temp_elem.pw[1] = j;
    temp_elem.pw[2] = k;
    temp_elem.poly = poly;

    cout<<w<<" "<<i<<" "<<j<<" "<<k<<" ";
    for(int i=0; i<kPORDER; i++)
      cout<<poly[i]<<" ";
    cout<<endl;

    // calculate some fields
    for(int j=0; j<kPORDER; j++)
      if(poly[j] != 0.0) {
	temp_elem.iszero = false;
	temp_elem.order = j;
      }

    /* decide which list of matrix elements to add it to */
    switch(w) {
    case 'D': fDMatrixElems.push_back(temp_elem); break;
    case 'T': fTMatrixElems.push_back(temp_elem); break;
    case 'Y': fYMatrixElems.push_back(temp_elem); break;
    case 'P': fPMatrixElems.push_back(temp_elem); break;
    default:
     	Error(Here(here), "Invalid Matrix Element specifier: %c!", w);
	break;
    }

    fscanf(file, "%*c");

    if(feof(file) || ferror(file))
      break;
  }
  return kOK;
}

//_____________________________________________________________________________
Int_t THaVDC::SetupDetector( const TDatime& date )
{

  if( fIsSetup ) return kOK;
  fIsSetup = true;

  // FIXME: Get these values from database file
  //const Double_t degrad = TMath::Pi()/180.0;
  //fVDCAngle *= degrad;
  //fVDCAngle = -45.0887 *  degrad;  //Convert to radians
  //fTan_vdc  = fT000.poly[0];   // tan(angle) stored as matrix element
  fTan_vdc = fFPMatrixElems[0].poly[0];
  fVDCAngle = TMath::ATan(fTan_vdc);
  fSin_vdc  = TMath::Sin(fVDCAngle);
  fCos_vdc  = TMath::Cos(fVDCAngle);
  //fTan_vdc = TMath::Tan(fVDCAngle);
  //fSpacing = 0.3348; // Dist between similar wire planes (eg u1->u2) (m)

  cout<<"Calculated VDC Angle: "<<(fVDCAngle*180.0)/TMath::Pi()<<endl;

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
      /*
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
      */

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

      /*
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
      */
      if(!found) {
#ifdef WITH_DEBUG
	if( fDebug>0 )
	  cout << "Track " << tracks->GetLast()+1 << " added.\n";
#endif
	theTrack = 
	  //	  AddTrack(*tracks, 0.0, transTheta, transPhi, transX, transY);
	  AddTrack(*tracks, 0.0, 0.0, 0.0, 0.0, 0.0);
	theTrack->SetID( thisID );
	theTrack->SetCreator( this );
	theTrack->AddCluster( track );
	theTrack->AddCluster( partner );
	flag |= kReassigned;
      }

      // calculate the transport coordinates
      CalcFocalPlaneCoords(track, theTrack, kRotatingTransport);

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

//______________________________________________________________________________
Int_t THaVDC::FindVertices( TClonesArray& tracks )
{
  // Calculate the target location and momentum at the target
  // assumes that CoarseTrack() and FineTrack() have both been called

  int n_exist = tracks.GetLast()+1;
  for(int t = 0; t < n_exist; t++ ) {
    THaTrack *theTrack = static_cast<THaTrack*>( tracks.At(t) );
    CalcTargetCoords(theTrack, kRotatingTransport);
  }

  return 0;
}

//_____________________________________________________________________________
void THaVDC::CalcFocalPlaneCoords(const THaVDCUVTrack *the_track, 
				  THaTrack *new_track, const ECoordTypes mode)
{
  // calculates focal plane coordinates from detector coordinates

  double tan_rho, cos_rho, tan_rho_loc, cos_rho_loc;
  double x, y, theta, phi;
  double r_x, r_y, r_theta, r_phi;
  
  // tan rho (for the central ray) is stored as a matrix element 
  tan_rho = fFPMatrixElems[0].poly[0];
  cos_rho = 1.0/sqrt(1.0+tan_rho*tan_rho);

  // first calculate the transport frame coordinates
  theta = (the_track->GetTheta()+tan_rho) /
          (1.0-the_track->GetTheta()*tan_rho);
  x = the_track->GetX() * cos_rho * (1.0 + theta * tan_rho);
  phi = the_track->GetPhi() / (1.0-the_track->GetTheta()*tan_rho) / cos_rho;
  y = the_track->GetY() + tan_rho*phi*the_track->GetX()*cos_rho;
  
  // then calculate the rotating transport frame coordinates
  //r_x = the_track->GetX() * cos_rho * (1.0 + theta*tan_rho);
  r_x = x;

  // calculate the focal-plane matrix elements
  if(mode == kTransport)
    CalcMatrix(x, fFPMatrixElems);
  else if (mode == kRotatingTransport)
    CalcMatrix(r_x, fFPMatrixElems);

  r_y = y - fFPMatrixElems[1].v;  // Y000

  // Calculate now the tan(rho) and cos(rho) of the local rotation angle.
  tan_rho_loc = fFPMatrixElems[0].v;   // T000
  cos_rho_loc = 1.0/sqrt(1.0+tan_rho_loc*tan_rho_loc);
  
  r_phi = (the_track->GetPhi() - fFPMatrixElems[2].v /* P000 */ ) / 
          (1.0-the_track->GetTheta()*tan_rho_loc) / cos_rho_loc;
  r_theta = (the_track->GetTheta()+tan_rho_loc) /
		       (1.0-the_track->GetTheta()*tan_rho_loc);

  // set the values we calculated
  new_track->Set(0.0, theta, phi, x, y);
  new_track->SetR(r_x, r_y, r_theta, r_phi);

}

//_____________________________________________________________________________
void THaVDC::CalcTargetCoords(THaTrack *the_track, const ECoordTypes mode)
{
  // calculates target coordinates from focal plane coordinates

  const int kNUM_PRECOMP_POW = 10;

  double x_fp, y_fp, th_fp, ph_fp;
  double powers[kNUM_PRECOMP_POW][3];  // {th, y, ph}
  double x, y, theta, phi, dp, p;

  // first select the coords to use
  if(mode == kTransport) {
    x_fp = the_track->GetX();
    y_fp = the_track->GetY();
    th_fp = the_track->GetTheta();
    ph_fp = the_track->GetPhi();
  } else if(mode == kRotatingTransport) {
    x_fp = the_track->GetRX();
    y_fp = the_track->GetRY();
    th_fp = the_track->GetRTheta();
    ph_fp = the_track->GetRPhi();  
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
  /*p = center_momentum * (1+dp); */ p = 0.0;

  // estimate x ??
  x = 0.0;

  // set the values we just calculated
  the_track->SetTarget(x, y, theta, phi);
  the_track->SetDp(dp);
  the_track->SetMomentum(p, the_track->GetTheta(), the_track->GetPhi());
}


//_____________________________________________________________________________
void THaVDC::CalcMatrix(const double x, vector<THaMatrixElement> &matrix)
{
  // calculates the values of the matrix elements for a given location
  // by evaluating a polynomial in x of order it->order with 
  // coefficients given by it->poly

  for(vector<THaMatrixElement>::iterator it=matrix.begin();
      it!=matrix.end(); it++) {
    it->v = 0.0;

    if(it->iszero == false) {
      for(int i=it->order-1; i>=1; i--)
	it->v = x * (it->v + it->poly[i]);
      it->v += it->poly[0];
    }
  }
}

//_____________________________________________________________________________
double THaVDC::CalcTargetVar(const vector<THaMatrixElement> &matrix,
			     const double powers[][3])
{
  // calculates the value of a variable at the target

  double retval=0.0;
  for(vector<THaMatrixElement>::const_iterator it=matrix.begin();
      it!=matrix.end(); it++) 
    if(it->v != 0.0)
      retval += it->v * powers[it->pw[0]][0]
	              * powers[it->pw[1]][1]
	              * powers[it->pw[2]][2];

  return retval;
}

////////////////////////////////////////////////////////////////////////////////
