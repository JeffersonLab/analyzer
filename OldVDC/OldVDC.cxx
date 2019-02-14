///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// OldVDC                                                                    //
//                                                                           //
// High precision wire chamber class.                                        //
//                                                                           //
// Units used:                                                               //
//        For X, Y, and Z coordinates of track    -  meters                  //
//        For Theta and Phi angles of track       -  tan(angle)              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "OldVDC.h"
#include "THaGlobals.h"
#include "THaEvData.h"
#include "THaDetMap.h"
#include "THaTrack.h"
#include "OldVDCUVPlane.h"
#include "OldVDCUVTrack.h"
#include "OldVDCCluster.h"
#include "OldVDCTrackID.h"
#include "OldVDCTrackPair.h"
#include "OldVDCHit.h"
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
#include <sstream>

#ifdef WITH_DEBUG
#include <iostream>
#include <limits>
#endif

using namespace std;
using THaString::Split;

// Helper structure for parsing tensor data
typedef vector<OldVDC::OldVDCMatrixElement> MEvec_t;
struct MEdef_t {
  MEdef_t() : npow(0), elems(0), isfp(false), fpidx(0) {}
  MEdef_t( Int_t npw, MEvec_t* elemp, Bool_t is_fp = false, Int_t fp_idx = 0 )
    : npow(npw), elems(elemp), isfp(is_fp), fpidx(fp_idx) {}
  MEvec_t::size_type npow; // Number of exponents for this element type
  MEvec_t* elems;          // Pointer to member variable holding data
  Bool_t isfp;             // This defines a focal plane matrix element
  Int_t fpidx;             // Index into fFPMatrixElems
};

//_____________________________________________________________________________
OldVDC::OldVDC( const char* name, const char* description,
		THaApparatus* apparatus ) :
  THaTrackingDetector(name,description,apparatus),
  fVDCAngle(-TMath::PiOver4()), fSin_vdc(-0.5*TMath::Sqrt2()),
  fCos_vdc(0.5*TMath::Sqrt2()), fTan_vdc(-1.0),
  fUSpacing(0.33), fVSpacing(0.33), fNtracks(0), fNumIter(1),
  fErrorCutoff(1e9), fCoordType(kRotatingTransport), fCentralDist(0.)
{
  // Constructor

  // Create Upper and Lower UV planes
  fLower   = new OldVDCUVPlane( "uv1", "Lower UV Plane", this );
  fUpper   = new OldVDCUVPlane( "uv2", "Upper UV Plane", this );
  if( !fLower || !fUpper || fLower->IsZombie() || fUpper->IsZombie() ) {
    Error( Here("OldVDC()"), "Failed to create subdetectors." );
    MakeZombie();
  }

  fUVpairs = new TClonesArray( "OldVDCTrackPair", 20 );

  // Default behavior for now
  SetBit( kOnlyFastest | kHardTDCcut );

}

//_____________________________________________________________________________
THaAnalysisObject::EStatus OldVDC::Init( const TDatime& date )
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
static Int_t ParseMatrixElements( const string& MEstring,
				  map<string,MEdef_t>& matrix_map,
				  const char* prefix )
{
  // Parse the contents of MEstring (from the database) into the local
  // member variables holding the matrix elements

  //FIXME: move to HRS

  const char* const here = "OldVDC::ParseMatrixElements";

  istringstream ist(MEstring.c_str());
  string word, w;
  bool findnext = true, findpowers = true;
  Int_t powers_left = 0;
  map<string,MEdef_t>::iterator cur = matrix_map.end();
  OldVDC::OldVDCMatrixElement ME;
  while( ist >> word ) {
    if( !findnext ) {
      assert( cur != matrix_map.end() );
      bool havenext = isalpha(word[0]);
      if( findpowers ) {
	assert( powers_left > 0 );
	if( havenext || word.find_first_not_of("0123456789") != string::npos ||
	    atoi(word.c_str()) > 9 ) {
	  Error( Here(here,prefix), "Bad exponent = %s for matrix element \"%s\". "
		 "Must be integer between 0 and 9. Fix database.",
		 word.c_str(), w.c_str() );
	  return THaAnalysisObject::kInitError;
	}
	ME.pw.push_back( atoi(word.c_str()) );
	if( --powers_left == 0 ) {
	  // Read all exponents
	  if( cur->second.isfp ) {
	    // Currently only the "000" focal plane matrix elements are supported
	    if( ME.pw[0] != 0 || ME.pw[1] != 0 || ME.pw[2] != 0 ) {
	      Error( Here(here,prefix), "Bad coefficients of focal plane matrix "
		    "element %s = %d %d %d. Fix database.",
		    w.c_str(), ME.pw[0], ME.pw[1], ME.pw[2] );
	      return THaAnalysisObject::kInitError;
	    } else {
	      findpowers = false;
	    }
	  }
	  else {
	    // Check if this element already exists, if so, skip
	    MEvec_t* mat = cur->second.elems;
	    assert(mat);
	    bool match = false;
	    for( MEvec_t::iterator it = mat->begin();
		 it != mat->end() && !(match = it->match(ME)); ++it ) {}
	    if( match ) {
	      Warning( Here(here,prefix), "Duplicate definition of matrix element %s. "
		      "Using first definition.", cur->first.c_str() );
	      findnext = true;
	    } else {
	      findpowers = false;
	    }
	  }
	}
      } else {
	if( !havenext ) {
	  if( ME.poly.size() >= OldVDC::kPORDER )
	    havenext = true;
	  else {
	    ME.poly.push_back( atof(word.c_str()) );
	    if( ME.poly.back() != 0.0 ) {
	      ME.iszero = false;
	      ME.order = ME.poly.size();
	    }
	  }
	}
	if( havenext || ist.eof() ) {
	  if( ME.poly.empty() ) {
	    // No data read at all?
	    Error( Here(here,prefix), "Could not read in Matrix Element %s%d%d%d!",
		  w.c_str(), ME.pw[0], ME.pw[1], ME.pw[2]);
	    return THaAnalysisObject::kInitError;
	  }
	  if( !ME.iszero ) {
	    MEvec_t* mat = cur->second.elems;
	    assert(mat);
	    // The focal plane matrix elements are stored in a single vector
	    if( cur->second.isfp ) {
	      OldVDC::OldVDCMatrixElement& m = (*mat)[cur->second.fpidx];
	      if( m.order > 0 ) {
		Warning( Here(here,prefix), "Duplicate definition of focal plane "
			"matrix element %s. Using first definition.",
			w.c_str() );
	      } else
		m = ME;
	    } else
	      mat->push_back(ME);
	  }
	  findnext = true;
	  if( !havenext )
	    continue;
	} // if (havenext)
      } // if (findpowers) else
    } // if (!findnext)
    if( findnext ) {
      cur = matrix_map.find(word);
      if( cur == matrix_map.end() ) {
	// Error( Here(here,prefix), "Unknown matrix element type %s. Fix database.",
	// 	 word.c_str() );
	// return THaAnalysisObject::kInitError;
	continue;
      }
      ME.clear();
      findnext = false; findpowers = true;
      powers_left = cur->second.npow;
      w = word;
    }
  } // while(word)

  return THaAnalysisObject::kOK;
}

//_____________________________________________________________________________
Int_t OldVDC::ReadDatabase( const TDatime& date )
{
  // Read global VDC parameters from the VDC database

  const char* const here = "ReadDatabase";

  FILE* file = OpenFile( date );
  if( !file ) return kFileError;

  // Read fOrigin and fSize (currently unused)
  Int_t err = ReadGeometry( file, date );
  if( err ) {
    fclose(file);
    return err;
  }

  // Read TRANSPORT matrices
  //FIXME: move to HRS
  fTMatrixElems.clear();
  fDMatrixElems.clear();
  fPMatrixElems.clear();
  fPTAMatrixElems.clear();
  fYMatrixElems.clear();
  fYTAMatrixElems.clear();
  fLMatrixElems.clear();

  fFPMatrixElems.clear();
  fFPMatrixElems.resize(3);

  map<string,MEdef_t> matrix_map;

  // TRANSPORT to focal-plane tensors
  matrix_map["t"]   = MEdef_t( 3, &fFPMatrixElems, true, 0 );
  matrix_map["y"]   = MEdef_t( 3, &fFPMatrixElems, true, 1 );
  matrix_map["p"]   = MEdef_t( 3, &fFPMatrixElems, true, 2 );
  // Standard focal-plane to target matrix elements (D=delta, T=theta, Y=y, P=phi)
  matrix_map["D"]   = MEdef_t( 3, &fDMatrixElems );
  matrix_map["T"]   = MEdef_t( 3, &fTMatrixElems );
  matrix_map["Y"]   = MEdef_t( 3, &fYMatrixElems );
  matrix_map["P"]   = MEdef_t( 3, &fPMatrixElems );
  // Additional matrix elements describing the dependence of y-target and
  // phi-target on the /absolute value/ of theta, found necessary in optimizing
  // the septum magnet optics (R. Feuerbach, March 1, 2005)
  matrix_map["YTA"] = MEdef_t( 4, &fYTAMatrixElems );
  matrix_map["PTA"] = MEdef_t( 4, &fPTAMatrixElems );
  // Matrix for calculating pathlength from z=0 (target) to focal plane (meters)
  // (R. Feuerbach, October 16, 2003)
  matrix_map["L"]   = MEdef_t( 4, &fLMatrixElems );

  string MEstring;
  DBRequest request1[] = {
    { "matrixelem",  &MEstring, kString },
    { 0 }
  };
  err = LoadDB( file, date, request1, fPrefix );
  if( err ) {
    fclose(file);
    return err;
  }
  if( MEstring.empty() ) {
    Error( Here(here), "No matrix elements defined. Set \"maxtrixelem\" in database." );
    fclose(file);
    return kInitError;
  }
  // Parse the matrix elements
  err = ParseMatrixElements( MEstring, matrix_map, fPrefix );
  if( err ) {
    fclose(file);
    return err;
  }
  MEstring.clear();

  // Ensure that we have all three focal plane matrix elements, else we cannot
  // do anything sensible with the tracks
  if( fFPMatrixElems[T000].order == 0 ) {
    Error( Here(here), "Missing FP matrix element t000. Fix database." );
    err = kInitError;
  }
  if( fFPMatrixElems[Y000].order == 0 ) {
    Error( Here(here), "Missing FP matrix element y000. Fix database." );
    err = kInitError;
  }
  if( fFPMatrixElems[P000].order == 0 ) {
    Error( Here(here), "Missing FP matrix element p000. Fix database." );
    err = kInitError;
  }
  if( err ) {
    fclose(file);
    return err;
  }

  // Compute derived geometry quantities
  //FIXME: get VDC angle from parent HRS spectrometer?
  const Double_t degrad = TMath::Pi()/180.0;
  fTan_vdc  = fFPMatrixElems[T000].poly[0];
  fVDCAngle = TMath::ATan(fTan_vdc);
  fSin_vdc  = TMath::Sin(fVDCAngle);
  fCos_vdc  = TMath::Cos(fVDCAngle);

  // Define the VDC coordinate axes in the "detector system". By definition,
  // the detector system is identical to the VDC origin in the Hall A HRS.
  DefineAxes(0.0*degrad);

  // Read configuration parameters
  fErrorCutoff = 1e9;
  fNumIter = 1;
  fCoordType = kRotatingTransport;
  Int_t disable_tracking = 0, disable_finetrack = 0, only_fastest_hit = 1;
  Int_t do_tdc_hardcut = 1, do_tdc_softcut = 0, ignore_negdrift = 0;
  string coord_type;

  DBRequest request[] = {
    { "max_matcherr",      &fErrorCutoff,      kDouble, 0, 1 },
    { "num_iter",          &fNumIter,          kInt,    0, 1 },
    { "coord_type",        &coord_type,        kString, 0, 1 },
    { "disable_tracking",  &disable_tracking,  kInt,    0, 1 },
    { "disable_finetrack", &disable_finetrack, kInt,    0, 1 },
    { "only_fastest_hit",  &only_fastest_hit,  kInt,    0, 1 },
    { "do_tdc_hardcut",    &do_tdc_hardcut,    kInt,    0, 1 },
    { "do_tdc_softcut",    &do_tdc_softcut,    kInt,    0, 1 },
    { "ignore_negdrift",   &ignore_negdrift,   kInt,    0, 1 },
    { 0 }
  };

  err = LoadDB( file, date, request, fPrefix );
  fclose(file);
  if( err )
    return err;

  // Analysis control flags
  SetBit( kOnlyFastest,     only_fastest_hit );
  SetBit( kHardTDCcut,      do_tdc_hardcut );
  SetBit( kSoftTDCcut,      do_tdc_softcut );
  SetBit( kIgnoreNegDrift,  ignore_negdrift );
  SetBit( kCoarseOnly,      !disable_tracking && disable_finetrack );

  if( !coord_type.empty() ) {
    if( THaString::CmpNoCase(coord_type, "Transport") == 0 )
      fCoordType = kTransport;
    else if( THaString::CmpNoCase(coord_type, "RotatingTransport") == 0 )
      fCoordType = kRotatingTransport;
    else {
      Error( Here(here), "Invalid coordinate type coord_type = %s. "
	     "Must be \"Transport\" or \"RotatingTransport\". Fix database.",
	     coord_type.c_str() );
      return kInitError;
    }
  }

  // Sanity checks of parameters
  if( fErrorCutoff < 0.0 ) {
    Warning( Here(here), "Negative max_matcherr = %6.2lf makes no sense, "
	     "taking absolute.", fErrorCutoff );
    fErrorCutoff = -fErrorCutoff;
  } else if( fErrorCutoff == 0.0 ) {
    Error( Here(here), "Illegal parameter max_matcherr = 0.0. Must be > 0. "
	   "Fix database." );
    return kInitError;
  }
  if( fNumIter < 0) {
    Warning( Here(here), "Negative num_iter = %d makes no sense, "
	     "taking absolute.", fNumIter );
    fNumIter = -fNumIter;
  } else if( fNumIter > 10 ) {
    Error( Here(here), "Illegal parameter num_iter = %d. Must be <= 10. "
	   "Fix database.", fNumIter );
    return kInitError;
  }

  if( fDebug > 0 ) {
    Info( Here(here), "VDC flags fastest/hardcut/softcut/noneg/"
	  "coarse = %d/%d/%d/%d/%d", TestBit(kOnlyFastest),
	  TestBit(kHardTDCcut), TestBit(kSoftTDCcut), TestBit(kIgnoreNegDrift),
	  TestBit(kCoarseOnly) );
  }

  // figure out the track length from the origin to the s1 plane
  // since we take the VDC to be the origin of the coordinate
  // space, this is actually pretty simple
  const THaDetector* s1 = 0;
  if( GetApparatus() )
    // TODO: neeed? if so, change to HRS reference detector
    s1 = GetApparatus()->GetDetector("s1");
  if(s1 == 0)
    fCentralDist = 0;
  else
    fCentralDist = s1->GetOrigin().Z();

  CalcMatrix(1.,fLMatrixElems); // tensor without explicit polynomial in x_fp

  fIsInit = true;
  return kOK;
}

//_____________________________________________________________________________
OldVDC::~OldVDC()
{
  // Destructor. Delete subdetectors.

  delete fLower;
  delete fUpper;
  delete fUVpairs;
}

//_____________________________________________________________________________
Int_t OldVDC::ConstructTracks( TClonesArray* tracks, Int_t mode )
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

  OldVDCUVTrack *track, *partner;
  OldVDCTrackPair *thePair;

  for( int i = 0; i < nLowerTracks; i++ ) {
    track = fLower->GetUVTrack(i);
    if( !track ) 
      continue;

    for( int j = 0; j < nUpperTracks; j++ ) {
      partner = fUpper->GetUVTrack(j);
      if( !partner ) 
	continue;

      // Create new UV track pair.
      thePair = new( (*fUVpairs)[nPairs++] ) OldVDCTrackPair( track, partner );

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
    if( !(thePair = static_cast<OldVDCTrackPair*>( fUVpairs->At(i) )) )
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

    OldVDCCluster 
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
      OldVDCTrackID* thisID = new OldVDCTrackID(track,partner);
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
      CalcFocalPlaneCoords(theTrack, fCoordType);
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
void OldVDC::Clear( Option_t* opt )
{ 
  // Clear event-by-event data

  THaTrackingDetector::Clear(opt);
  fLower->Clear(opt);
  fUpper->Clear(opt);
}

//_____________________________________________________________________________
Int_t OldVDC::Decode( const THaEvData& evdata )
{
  // Decode data from VDC planes
  
  fLower->Decode(evdata); 
  fUpper->Decode(evdata);

  return 0;
}

//_____________________________________________________________________________
Int_t OldVDC::CoarseTrack( TClonesArray& tracks )
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
Int_t OldVDC::FineTrack( TClonesArray& tracks )
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
  static char c = '\0';
  static bool first = true;
  if( fDebug>1 ) {
    if( first ) {
      cin.ignore(numeric_limits<streamsize>::max(), '\n');
      first = false;
    }
    cin.clear();
    while( !cin.eof() && cin.get(c) && c != '\n') {}
  }
#endif

  return 0;
}

//_____________________________________________________________________________
Int_t OldVDC::FindVertices( TClonesArray& tracks )
{
  // Calculate the target location and momentum at the target.
  // Assumes that CoarseTrack() and FineTrack() have both been called.

  Int_t n_exist = tracks.GetLast()+1;
  for( Int_t t = 0; t < n_exist; t++ ) {
    THaTrack* theTrack = static_cast<THaTrack*>( tracks.At(t) );
    CalcTargetCoords(theTrack, fCoordType);
  }

  return 0;
}

//_____________________________________________________________________________
void OldVDC::CalcFocalPlaneCoords( THaTrack* track, const ECoordTypes mode )
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
void OldVDC::CalcTargetCoords(THaTrack *track, const ECoordTypes mode)
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
void OldVDC::CalcMatrix( const Double_t x, vector<OldVDCMatrixElement>& matrix )
{
  // calculates the values of the matrix elements for a given location
  // by evaluating a polynomial in x of order it->order with 
  // coefficients given by it->poly

  for( vector<OldVDCMatrixElement>::iterator it=matrix.begin();
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
Double_t OldVDC::CalcTargetVar(const vector<OldVDCMatrixElement>& matrix,
			       const Double_t powers[][5])
{
  // calculates the value of a variable at the target
  // the x-dependence is already in the matrix, so only 1-3 (or np) used
  Double_t retval=0.0;
  Double_t v=0;
  for( vector<OldVDCMatrixElement>::const_iterator it=matrix.begin();
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
Double_t OldVDC::CalcTarget2FPLen(const vector<OldVDCMatrixElement>& matrix,
				  const Double_t powers[][5])
{
  // calculates distance from the nominal target position (z=0)
  // to the transport plane

  Double_t retval=0.0;
  for( vector<OldVDCMatrixElement>::const_iterator it=matrix.begin();
       it!=matrix.end(); it++ ) 
    if(it->v != 0.0)
      retval += it->v * powers[it->pw[0]][0]
	              * powers[it->pw[1]][1]
	              * powers[it->pw[2]][2]
	              * powers[it->pw[3]][3];

  return retval;
}

//_____________________________________________________________________________
void OldVDC::CorrectTimeOfFlight(TClonesArray& tracks)
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
      OldVDCUVTrack* the_uvtrack = 
	static_cast<OldVDCUVTrack*>( track->GetCluster(i) );
      if( !the_uvtrack )
	continue;
      
      //FIXME: clusters guaranteed to be nonzero?
      the_uvtrack->GetUCluster()->SetTimeCorrection(tdelta);
      the_uvtrack->GetVCluster()->SetTimeCorrection(tdelta);
    }
  }
}

//_____________________________________________________________________________
void OldVDC::FindBadTracks(TClonesArray& tracks)
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
void OldVDC::Print(const Option_t* opt) const {
  THaTrackingDetector::Print(opt);
  // Print out the optics matrices, to verify they make sense
  printf("Matrix FP (t000, y000, p000)\n");
  typedef vector<OldVDCMatrixElement>::size_type vsiz_t;
  for (vsiz_t i=0; i<fFPMatrixElems.size(); i++) {
    const OldVDCMatrixElement& m = fFPMatrixElems[i];
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
    const OldVDCMatrixElement& m = fDMatrixElems[i];
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
    const OldVDCMatrixElement& m = fTMatrixElems[i];
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
    const OldVDCMatrixElement& m = fYMatrixElems[i];
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
    const OldVDCMatrixElement& m = fYTAMatrixElems[i];
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
    const OldVDCMatrixElement& m = fPMatrixElems[i];
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
    const OldVDCMatrixElement& m = fPTAMatrixElems[i];
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
    const OldVDCMatrixElement& m = fLMatrixElems[i];
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
bool OldVDC::OldVDCMatrixElement::match(const OldVDCMatrixElement& rhs) const
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

//_____________________________________________________________________________
void OldVDC::SetNMaxGap( Int_t val )
{
  if( val < 0 || val > 2 ) {
    Error( Here("SetNMaxGap"),
	   "Invalid max_gap = %d, must be betwwen 0 and 2.", val );
    return;
  }
  fUpper->SetNMaxGap(val);
  fLower->SetNMaxGap(val);
}

//_____________________________________________________________________________
void OldVDC::SetMinTime( Int_t val )
{
  if( val < 0 || val > 4095 ) {
    Error( Here("SetMinTime"),
	   "Invalid min_time = %d, must be betwwen 0 and 4095.", val );
    return;
  }
  fUpper->SetMinTime(val);
  fLower->SetMinTime(val);
}

//_____________________________________________________________________________
void OldVDC::SetMaxTime( Int_t val )
{
  if( val < 1 || val > 4096 ) {
    Error( Here("SetMaxTime"),
	   "Invalid max_time = %d. Must be between 1 and 4096.", val );
    return;
  }
  fUpper->SetMaxTime(val);
  fLower->SetMaxTime(val);
}

//_____________________________________________________________________________
void OldVDC::SetTDCRes( Double_t val )
{
  if( val < 0 || val > 1e-6 ) {
    Error( Here("SetTDCRes"),
	   "Nonsense TDC resolution = %8.1le s/channel.", val );
    return;
  }
  fUpper->SetTDCRes(val);
  fLower->SetTDCRes(val);
}

//_____________________________________________________________________________
void OldVDC::SetDebug( Int_t level )
{
  // Set debug level of the VDC and the chamber/plane subdetectors

  THaTrackingDetector::SetDebug(level);
  fLower->SetDebug(level);
  fUpper->SetDebug(level);
}

////////////////////////////////////////////////////////////////////////////////
ClassImp(OldVDC)
