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
#include "THaString.h"
#include <map>
#include <cstdio>
#include <cstdlib>

#ifdef WITH_DEBUG
#include <iostream>
#endif

using namespace std;
using THaString::Split;

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

  // Get the spacing between the VDC planes from the planes' geometry
  fUSpacing = fUpper->GetUPlane()->GetZ() - fLower->GetUPlane()->GetZ();
  fVSpacing = fUpper->GetVPlane()->GetZ() - fLower->GetVPlane()->GetZ();

  return fStatus = kOK;
}

//_____________________________________________________________________________
Int_t THaVDC::ReadDatabase( const TDatime& date )
{
  // Read VDC database

  FILE* file = OpenFile( date );
  if( !file ) return kFileError;

  // load global VDC parameters
  static const char* const here = "ReadDatabase";
  const int LEN = 200;
  char buff[LEN];
  
  // Look for the section [<prefix>.global] in the file, e.g. [ R.global ]
  TString tag(fPrefix);
  Ssiz_t pos = tag.Index("."); 
  if( pos != kNPOS )
    tag = tag(0,pos+1);
  else
    tag.Append(".");
  tag.Prepend("[");
  tag.Append("global]"); 

  TString line;
  bool found = false;
  while (!found && fgets (buff, LEN, file) != NULL) {
    char* buf = ::Compress(buff);  //strip blanks
    line = buf;
    delete [] buf;
    if( line.EndsWith("\n") ) line.Chop();
    if ( tag == line ) 
      found = true;
  }
  if( !found ) {
    Error(Here(here), "Database section %s not found!", tag.Data() );
    fclose(file);
    return kInitError;
  }

  // We found the section, now read the data

  // read in some basic constants first
  //  fscanf(file, "%lf", &fSpacing);
  // fSpacing is now calculated from the actual z-positions in Init(),
  // so skip the first line after [ global ] completely:
  fgets(buff, LEN, file); // Skip rest of line
 
  // Read in the focal plane transfer elements
  // For fine-tuning of these data, we seek to a matching time stamp, or
  // if no time stamp found, to a "configuration" section. Examples:
  // 
  // [ 2002-10-10 15:30:00 ]
  // comment line goes here
  // t 0 0 0  ...
  // y 0 0 0  ... etc.
  //
  // or
  // 
  // [ config=highmom ]
  // comment line
  // t 0 0 0  ... etc.
  //
  if( (found = SeekDBdate( file, date )) == 0 && !fConfig.IsNull() && 
      (found = SeekDBconfig( file, fConfig.Data() )) == 0 ) {
    // Print warning if a requested (non-empty) config not found
    Warning( Here(here), "Requested configuration section \"%s\" not "
	     "found in database. Using default (first) section.", 
	     fConfig.Data() );
  }

  // Second line after [ global ] or first line after a found tag.
  // After a found tag, it must be the comment line. If not found, then it 
  // can be either the comment or a non-found tag before the comment...
  fgets(buff, LEN, file);  // Skip line
  if( !found && IsTag(buff) )
    // Skip one more line if this one was a non-found tag
    fgets(buff, LEN, file);

  fTMatrixElems.clear();
  fDMatrixElems.clear();
  fPMatrixElems.clear();
  fPTAMatrixElems.clear();
  fYMatrixElems.clear();
  fYTAMatrixElems.clear();
  fLMatrixElems.clear();
  
  fFPMatrixElems.clear();
  fFPMatrixElems.resize(3);

  typedef vector<string>::size_type vsiz_t;
  map<string,vsiz_t> power;
  power["t"] = 3;  // transport to focal-plane tensors
  power["y"] = 3;
  power["p"] = 3;
  power["D"] = 3;  // focal-plane to target tensors
  power["T"] = 3;
  power["Y"] = 3;
  power["YTA"] = 4;
  power["P"] = 3;
  power["PTA"] = 4;
  power["L"] = 4;  // pathlength from z=0 (target) to focal plane (meters)
  power["XF"] = 5; // forward: target to focal-plane (I think)
  power["TF"] = 5;
  power["PF"] = 5;
  power["YF"] = 5;
  
  map<string,vector<THaMatrixElement>*> matrix_map;
  matrix_map["t"] = &fFPMatrixElems;
  matrix_map["y"] = &fFPMatrixElems;
  matrix_map["p"] = &fFPMatrixElems;
  matrix_map["D"] = &fDMatrixElems;
  matrix_map["T"] = &fTMatrixElems;
  matrix_map["Y"] = &fYMatrixElems;
  matrix_map["YTA"] = &fYTAMatrixElems;
  matrix_map["P"] = &fPMatrixElems;
  matrix_map["PTA"] = &fPTAMatrixElems;
  matrix_map["L"] = &fLMatrixElems;

  map <string,int> fp_map;
  fp_map["t"] = 0;
  fp_map["y"] = 1;
  fp_map["p"] = 2;

  // Read in as many of the matrix elements as there are.
  // Read in line-by-line, so as to be able to handle tensors of
  // different orders.
  while( fgets(buff, LEN, file) ) {
    string line(buff);
    // Erase trailing newline
    if( line.size() > 0 && line[line.size()-1] == '\n' ) {
      buff[line.size()-1] = 0;
      line.erase(line.size()-1,1);
    }
    // Split the line into whitespace-separated fields    
    vector<string> line_spl = Split(line);

    // Stop if the line does not start with a string referring to
    // a known type of matrix element. In particular, this will
    // stop on a subsequent timestamp or configuration tag starting with "["
    if(line_spl.empty())
      continue; //ignore empty lines
    const char* w = line_spl[0].c_str();
    vsiz_t npow = power[w];
    if( npow == 0 ) 
      break;

    // Looks like a good line, go parse it.
    THaMatrixElement ME;
    ME.pw.resize(npow);
    ME.iszero = true;  ME.order = 0;
    vsiz_t pos;
    for (pos=1; pos<=npow && pos<line_spl.size(); pos++) {
      ME.pw[pos-1] = atoi(line_spl[pos].c_str());
    }
    vsiz_t p_cnt;
    for ( p_cnt=0; pos<line_spl.size() && p_cnt<kPORDER && pos<=npow+kPORDER;
	  pos++,p_cnt++ ) {
      ME.poly[p_cnt] = atof(line_spl[pos].c_str());
      if (ME.poly[p_cnt] != 0.0) {
	ME.iszero = false;
	ME.order = p_cnt+1;
      }
    }
    if (p_cnt < 1) {
	Error(Here(here), "Could not read in Matrix Element %s%d%d%d!",
	      w, ME.pw[0], ME.pw[1], ME.pw[2]);
	Error(Here(here), "Line looks like: %s",line.c_str());
	fclose(file);
	return kInitError;
    }
    // Don't bother with all-zero matrix elements
    if( ME.iszero )
      continue;
    
    // Add this matrix element to the appropriate array
    vector<THaMatrixElement> *mat = matrix_map[w];
    if (mat) {
      // Special checks for focal plane matrix elements
      if( mat == &fFPMatrixElems ) {
	if( ME.pw[0] == 0 && ME.pw[1] == 0 && ME.pw[2] == 0 ) {
	  THaMatrixElement& m = (*mat)[fp_map[w]];
	  if( m.order > 0 ) {
	    Warning(Here(here), "Duplicate definition of focal plane "
		    "matrix element: %s. Using first definition.", buff);
	  } else
	    m = ME;
	} else
	  Warning(Here(here), "Bad coefficients of focal plane matrix "
		  "element %s", buff);
      } 
      else {
	// All other matrix elements are just appended to the respective array
	// but ensure that they are defined only once!
	bool match = false;
	for( vector<THaMatrixElement>::iterator it = mat->begin();
	     it != mat->end() && !(match = it->match(ME)); it++ ) {}
	if( match ) {
	  Warning(Here(here), "Duplicate definition of "
		  "matrix element: %s. Using first definition.", buff);
	} else
	  mat->push_back(ME);
      }
    }
    else if ( fDebug > 0 )
      Warning(Here(here), "Not storing matrix for: %s !", w);
    
  } //while(fgets)

  // Compute derived quantities and set some hardcoded parameters
  const Double_t degrad = TMath::Pi()/180.0;
  fTan_vdc  = fFPMatrixElems[T000].poly[0];
  fVDCAngle = TMath::ATan(fTan_vdc);
  fSin_vdc  = TMath::Sin(fVDCAngle);
  fCos_vdc  = TMath::Cos(fVDCAngle);

  // Define the VDC coordinate axes in the "detector system". By definition,
  // the detector system is identical to the VDC origin in the Hall A HRS.
  DefineAxes(0.0*degrad);

  fNumIter = 1;      // Number of iterations for FineTrack()
  fErrorCutoff = 1e100;

  // figure out the track length from the origin to the s1 plane

  // since we take the VDC to be the origin of the coordinate
  // space, this is actually pretty simple
  const THaDetector* s1 = 0;
  if( GetApparatus() )
    s1 = GetApparatus()->GetDetector("s1");
  if(s1 == NULL)
    fCentralDist = 0;
  else
    fCentralDist = s1->GetOrigin().Z();

  CalcMatrix(1.,fLMatrixElems); // tensor without explicit polynomial in x_fp
  
  // FIXME: Set geometry data (fOrigin). Currently fOrigin = (0,0,0).

  fIsInit = true;
  fclose(file);
  return kOK;
}

//_____________________________________________________________________________
THaVDC::~THaVDC()
{
  // Destructor. Delete subdetectors.

  delete fLower;
  delete fUpper;
  delete fUVpairs;
}

//_____________________________________________________________________________
Int_t THaVDC::ConstructTracks( TClonesArray* tracks, Int_t mode )
{
  // Construct tracks from pairs of upper and lower UV tracks and add 
  // them to 'tracks'

#ifdef WITH_DEBUG
  if( fDebug>1 ) {
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
  if( fDebug>1 )
    cout << "nUpper/nLower = " << nUpperTracks << "  " << nLowerTracks << endl;
#endif

  // No tracks at all -> can't have any tracks
  if( nUpperTracks == 0 && nLowerTracks == 0 ) {
#ifdef WITH_DEBUG
    if( fDebug>1 )
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
    if( fDebug>1 ) 
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
      thePair->Analyze( fUSpacing );
    }
  }
      
#ifdef WITH_DEBUG
  if( fDebug>1 )
    cout << nPairs << " pairs.\n";
#endif

  // Initialize some counters
  int n_exist = 0, n_mod = 0;
  int n_oops = 0;
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
    if( fDebug>1 ) {
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
    if( fDebug>1 ) {
      cout << "dUpper/dLower = " 
	   << thePair->GetProjectedDistance( track,partner,fUSpacing) << "  "
	   << thePair->GetProjectedDistance( partner,track,-fUSpacing);
    }
#endif

    // Skip pairs where any of the tracks already has a partner
    if( track->GetPartner() || partner->GetPartner() ) {
#ifdef WITH_DEBUG
      if( fDebug>1 )
	cout << " ... skipped.\n";
#endif
      continue;
    }
#ifdef WITH_DEBUG
    if( fDebug>1 )
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
    Double_t mu = du / fUSpacing;
    Double_t mv = dv / fVSpacing;

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
        if( fDebug>1 )
          cout << "Track " << t << " modified.\n";
#endif
        delete thisID;
        ++n_mod;
      } else {
#ifdef WITH_DEBUG
	if( fDebug>1 )
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
      
      Double_t chi2=0;
      Int_t nhits=0;
      track->CalcChisquare(chi2,nhits);
      partner->CalcChisquare(chi2,nhits);
      theTrack->SetChi2(chi2,nhits-4); // Nconstraints - Nparameters

      // calculate the TRANSPORT coordinates
      CalcFocalPlaneCoords(theTrack, kRotatingTransport);
    }
  }

#ifdef WITH_DEBUG
  if( fDebug>1 )
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
	if( fDebug>1 )
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
void THaVDC::Clear( Option_t* opt )
{ 
  // Clear event-by-event data

  fLower->Clear(opt);
  fUpper->Clear(opt);
}

//_____________________________________________________________________________
Int_t THaVDC::Decode( const THaEvData& evdata )
{
  // Decode data from VDC planes
  
  fLower->Decode(evdata); 
  fUpper->Decode(evdata);

  return 0;
}

//_____________________________________________________________________________
Int_t THaVDC::CoarseTrack( TClonesArray& tracks )
{
 
#ifdef WITH_DEBUG
  static int nev = 0;
  if( fDebug>1 ) {
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
  Double_t powers[kNUM_PRECOMP_POW][5];  // {(x), th, y, ph, abs(th) }
  Double_t x, y, theta, phi, dp, p, pathl;

  // first select the coords to use
  if(mode == kTransport) {
    x_fp = track->GetX();
    y_fp = track->GetY();
    th_fp = track->GetTheta();
    ph_fp = track->GetPhi();
  } else {//if(mode == kRotatingTransport) {
    x_fp = track->GetRX();
    y_fp = track->GetRY();
    th_fp = track->GetRTheta();
    ph_fp = track->GetRPhi();  
  }

  // calculate the powers we need
  for(int i=0; i<kNUM_PRECOMP_POW; i++) {
    powers[i][0] = pow(x_fp, i);
    powers[i][1] = pow(th_fp, i);
    powers[i][2] = pow(y_fp, i);
    powers[i][3] = pow(ph_fp, i);
    powers[i][4] = pow(TMath::Abs(th_fp),i);
  }

  // calculate the matrices we need
  CalcMatrix(x_fp, fDMatrixElems);
  CalcMatrix(x_fp, fTMatrixElems);
  CalcMatrix(x_fp, fYMatrixElems);
  CalcMatrix(x_fp, fYTAMatrixElems);
  CalcMatrix(x_fp, fPMatrixElems);
  CalcMatrix(x_fp, fPTAMatrixElems);

  // calculate the coordinates at the target
  theta = CalcTargetVar(fTMatrixElems, powers);
  phi = CalcTargetVar(fPMatrixElems, powers)+CalcTargetVar(fPTAMatrixElems,powers);
  y = CalcTargetVar(fYMatrixElems, powers)+CalcTargetVar(fYTAMatrixElems,powers);

  THaSpectrometer *app = static_cast<THaSpectrometer*>(GetApparatus());
  // calculate momentum
  dp = CalcTargetVar(fDMatrixElems, powers);
  p  = app->GetPcentral() * (1.0+dp);

  // pathlength matrix is for the Transport coord plane
  pathl = CalcTarget2FPLen(fLMatrixElems, powers);

  //FIXME: estimate x ??
  x = 0.0;

  // Save the target quantities with the tracks
  track->SetTarget(x, y, theta, phi);
  track->SetDp(dp);
  track->SetMomentum(p);
  track->SetPathLen(pathl);
  
  app->TransportToLab( p, theta, phi, track->GetPvect() );

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

    if(it->order > 0) {
      for(int i=it->order-1; i>=1; i--)
	it->v = x * (it->v + it->poly[i]);
      it->v += it->poly[0];
    }
  }
}

//_____________________________________________________________________________
Double_t THaVDC::CalcTargetVar(const vector<THaMatrixElement>& matrix,
			       const Double_t powers[][5])
{
  // calculates the value of a variable at the target
  // the x-dependence is already in the matrix, so only 1-3 (or np) used
  Double_t retval=0.0;
  Double_t v=0;
  for( vector<THaMatrixElement>::const_iterator it=matrix.begin();
       it!=matrix.end(); it++ ) 
    if(it->v != 0.0) {
      v = it->v;
      unsigned int np = it->pw.size(); // generalize for extra matrix elems.
      for (unsigned int i=0; i<np; i++)
	v *= powers[it->pw[i]][i+1];
      retval += v;
  //      retval += it->v * powers[it->pw[0]][1] 
  //	              * powers[it->pw[1]][2]
  //	              * powers[it->pw[2]][3];
    }

  return retval;
}

//_____________________________________________________________________________
Double_t THaVDC::CalcTarget2FPLen(const vector<THaMatrixElement>& matrix,
				  const Double_t powers[][5])
{
  // calculates distance from the nominal target position (z=0)
  // to the transport plane

  Double_t retval=0.0;
  for( vector<THaMatrixElement>::const_iterator it=matrix.begin();
       it!=matrix.end(); it++ ) 
    if(it->v != 0.0)
      retval += it->v * powers[it->pw[0]][0]
	              * powers[it->pw[1]][1]
	              * powers[it->pw[2]][2]
	              * powers[it->pw[3]][3];

  return retval;
}

//_____________________________________________________________________________
void THaVDC::CorrectTimeOfFlight(TClonesArray& tracks)
{
  const static Double_t v = 3.0e-8;   // for now, assume that everything travels at c

  // get scintillator planes
  THaScintillator* s1 = static_cast<THaScintillator*>
    ( GetApparatus()->GetDetector("s1") );
  THaScintillator* s2 = static_cast<THaScintillator*>
    ( GetApparatus()->GetDetector("s2") );

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
    Int_t n_clust = track->GetNclusters();
    for( Int_t i = 0; i < n_clust; i++ ) {
      THaVDCUVTrack* the_uvtrack = 
	static_cast<THaVDCUVTrack*>( track->GetCluster(i) );
      if( !the_uvtrack )
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

  THaScintillator* s2 = static_cast<THaScintillator*>
    ( GetApparatus()->GetDetector("s2") );

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

//_____________________________________________________________________________
void THaVDC::Print(const Option_t* opt) const {
  THaTrackingDetector::Print(opt);
  // Print out the optics matrices, to verify they make sense
  printf("Matrix FP (t000, y000, p000)\n");
  typedef vector<THaMatrixElement>::size_type vsiz_t;
  for (vsiz_t i=0; i<fFPMatrixElems.size(); i++) {
    const THaMatrixElement& m = fFPMatrixElems[i];
    for (vsiz_t j=0; j<m.pw.size(); j++) {
      printf("  %2d",m.pw[j]);
    }
    for (int j=0; j<m.order; j++) {
      printf("  %g",m.poly[j]);
    }
    printf("\n");
  }

  printf("Transport Matrix:  D-terms\n");
  for (vsiz_t i=0; i<fDMatrixElems.size(); i++) {
    const THaMatrixElement& m = fDMatrixElems[i];
    for (vsiz_t j=0; j<m.pw.size(); j++) {
      printf("  %2d",m.pw[j]);
    }
    for (int j=0; j<m.order; j++) {
      printf("  %g",m.poly[j]);
    }
    printf("\n");
  }

  printf("Transport Matrix:  T-terms\n");
  for (vsiz_t i=0; i<fTMatrixElems.size(); i++) {
    const THaMatrixElement& m = fTMatrixElems[i];
    for (vsiz_t j=0; j<m.pw.size(); j++) {
      printf("  %2d",m.pw[j]);
    }
    for (int j=0; j<m.order; j++) {
      printf("  %g",m.poly[j]);
    }
    printf("\n");
  }

  printf("Transport Matrix:  Y-terms\n");
  for (vsiz_t i=0; i<fYMatrixElems.size(); i++) {
    const THaMatrixElement& m = fYMatrixElems[i];
    for (vsiz_t j=0; j<m.pw.size(); j++) {
      printf("  %2d",m.pw[j]);
    }
    for (int j=0; j<m.order; j++) {
      printf("  %g",m.poly[j]);
    }
    printf("\n");
  }

  printf("Transport Matrix:  YTA-terms (abs(theta))\n");
  for (vsiz_t i=0; i<fYTAMatrixElems.size(); i++) {
    const THaMatrixElement& m = fYTAMatrixElems[i];
    for (vsiz_t j=0; j<m.pw.size(); j++) {
      printf("  %2d",m.pw[j]);
    }
    for (int j=0; j<m.order; j++) {
      printf("  %g",m.poly[j]);
    }
    printf("\n");
  }

  printf("Transport Matrix:  P-terms\n");
  for (vsiz_t i=0; i<fPMatrixElems.size(); i++) {
    const THaMatrixElement& m = fPMatrixElems[i];
    for (vsiz_t j=0; j<m.pw.size(); j++) {
      printf("  %2d",m.pw[j]);
    }
    for (int j=0; j<m.order; j++) {
      printf("  %g",m.poly[j]);
    }
    printf("\n");
  }

  printf("Transport Matrix:  PTA-terms\n");
  for (vsiz_t i=0; i<fPTAMatrixElems.size(); i++) {
    const THaMatrixElement& m = fPTAMatrixElems[i];
    for (vsiz_t j=0; j<m.pw.size(); j++) {
      printf("  %2d",m.pw[j]);
    }
    for (int j=0; j<m.order; j++) {
      printf("  %g",m.poly[j]);
    }
    printf("\n");
  }

  printf("Matrix L\n");
  for (vsiz_t i=0; i<fLMatrixElems.size(); i++) {
    const THaMatrixElement& m = fLMatrixElems[i];
    for (vsiz_t j=0; j<m.pw.size(); j++) {
      printf("  %2d",m.pw[j]);
    }
    for (int j=0; j<m.order; j++) {
      printf("  %g",m.poly[j]);
    }
    printf("\n");
  }

  return;
}

//_____________________________________________________________________________
bool THaVDC::THaMatrixElement::match(const THaMatrixElement& rhs) const
{
  // Compare coefficients of this matrix element to another

  if( pw.size() != rhs.pw.size() )
    return false;
  for( vector<int>::size_type i=0; i<pw.size(); i++ ) {
    if( pw[i] != rhs.pw[i] )
      return false;
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////
ClassImp(THaVDC)
