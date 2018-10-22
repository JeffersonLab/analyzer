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
#include "THaVDCPlane.h"
#include "THaVDCChamber.h"
#include "THaVDCPoint.h"
#include "THaVDCCluster.h"
#include "THaVDCTrackID.h"
#include "THaVDCPointPair.h"
#include "THaVDCHit.h"
#include "THaScintillator.h"
#include "THaSpectrometer.h"
#include "TMath.h"
#include "TClonesArray.h"
#include "TList.h"
#include "VarDef.h"
#include "TROOT.h"
#include "THaString.h"

//#include <algorithm>
#include <map>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cctype>
#include <sstream>

#ifdef WITH_DEBUG
#include <iostream>
#endif


using namespace std;
using namespace VDC;

// Helper structure for parsing tensor data
typedef vector<THaVDC::THaMatrixElement> MEvec_t;
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
THaVDC::THaVDC( const char* name, const char* description,
		THaApparatus* apparatus ) :
  THaTrackingDetector(name,description,apparatus),
  fNtracks(0), fEvNum(0),
  fVDCAngle(-TMath::PiOver4()), fSin_vdc(-0.5*TMath::Sqrt2()),
  fCos_vdc(0.5*TMath::Sqrt2()), fTan_vdc(-1.0),
  fSpacing(0.33), fCentralDist(0.),
  fNumIter(1), fErrorCutoff(1e9), fCoordType(kRotatingTransport)
{
  // Constructor

  // Create objects for the upper and lower chamber
  fLower = new THaVDCChamber( "uv1", "Lower VDC chamber", this );
  fUpper = new THaVDCChamber( "uv2", "Upper VDC chamber", this );
  if( !fLower || !fUpper || fLower->IsZombie() || fUpper->IsZombie() ) {
    Error( Here("THaVDC()"), "Failed to create subdetectors." );
    MakeZombie();
  }

  fLUpairs = new TClonesArray( "THaVDCPointPair", 20 );

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

  // Get the spacing between the VDC chambers
  fSpacing = fUpper->GetUPlane()->GetZ() - fLower->GetUPlane()->GetZ();

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

  const char* const here = "THaVDC::ParseMatrixElements";

  istringstream ist(MEstring.c_str());
  string word, w;
  bool findnext = true, findpowers = true;
  Int_t powers_left = 0;
  map<string,MEdef_t>::iterator cur = matrix_map.end();
  THaVDC::THaMatrixElement ME;
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
	  if( ME.poly.size() >= THaVDC::kPORDER )
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
	      THaVDC::THaMatrixElement& m = (*mat)[cur->second.fpidx];
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
Int_t THaVDC::ReadDatabase( const TDatime& date )
{
  // Read VDC database

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
  fTan_vdc  = fFPMatrixElems[T000].poly[0];
  fVDCAngle = TMath::ATan(fTan_vdc);
  fSin_vdc  = TMath::Sin(fVDCAngle);
  fCos_vdc  = TMath::Cos(fVDCAngle);

  // Define the VDC coordinate axes in the "TRANSPORT system" (z along particle
  // direction at central momentum)
  DefineAxes(fVDCAngle);

  // Read configuration parameters
  fErrorCutoff = 1e9;
  fNumIter = 1;
  fCoordType = kRotatingTransport;
  Int_t disable_tracking = 0, disable_finetrack = 0, only_fastest_hit = 1;
  Int_t do_tdc_hardcut = 1, do_tdc_softcut = 0, ignore_negdrift = 0;
#ifdef MCDATA
  Int_t mc_data = 0;
#endif
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
#ifdef MCDATA
    { "MCdata",            &mc_data,           kInt,    0, 1 },
#endif
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
#ifdef MCDATA
  SetBit( kMCdata,          mc_data );
#endif
  SetBit( kDecodeOnly,      disable_tracking );
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
#ifdef MCDATA
    Info( Here(here), "VDC flags fastest/hardcut/softcut/noneg/mcdata/"
	  "decode/coarse = %d/%d/%d/%d/%d/%d/%d", TestBit(kOnlyFastest),
	  TestBit(kHardTDCcut), TestBit(kSoftTDCcut), TestBit(kIgnoreNegDrift),
	  TestBit(kMCdata), TestBit(kDecodeOnly), TestBit(kCoarseOnly) );
#else
    Info( Here(here), "VDC flags fastest/hardcut/softcut/noneg/"
	  "decode/coarse = %d/%d/%d/%d/%d/%d", TestBit(kOnlyFastest),
	  TestBit(kHardTDCcut), TestBit(kSoftTDCcut), TestBit(kIgnoreNegDrift),
	  TestBit(kDecodeOnly), TestBit(kCoarseOnly) );
#endif
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
THaVDC::~THaVDC()
{
  // Destructor. Delete subdetectors.

  delete fLower;
  delete fUpper;
  delete fLUpairs;
}

//_____________________________________________________________________________
Int_t THaVDC::ConstructTracks( TClonesArray* tracks, Int_t mode )
{
  // Construct tracks from pairs of upper and lower points and add
  // them to 'tracks'

  // TODO:
  //   Only make combos whose projections are close
  //   do a real 3D fit, not just compute 3D chi2?

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

  fLUpairs->Clear();

  Int_t nUpper = fUpper->GetNPoints();
  Int_t nLower = fLower->GetNPoints();

#ifdef WITH_DEBUG
  if( fDebug>1 )
    cout << "nUpper/nLower = " << nUpper << "  " << nLower << endl;
#endif

  // One plane has no tracks, the other does
  // -> maybe recoverable with loss of precision
  // FIXME: Only do this if missing cluster recovery flag set
  if( (nUpper == 0) xor (nLower == 0) ) {
    //FIXME: Put missing cluster recovery code here
    //For now, do nothing
#ifdef WITH_DEBUG
    if( fDebug>1 )
      cout << "missing cluster " << nLower << " " << nUpper << endl;
#endif
  }

  Int_t nPairs  = 0;  // Number of point pairs to consider

  for( int i = 0; i < nLower; i++ ) {
    THaVDCPoint* lowerPoint = fLower->GetPoint(i);
    assert(lowerPoint);

    for( int j = 0; j < nUpper; j++ ) {
      THaVDCPoint* upperPoint = fUpper->GetPoint(j);
      assert(upperPoint);

      // Compute projection error of the selected pair of points
      // i.e., how well the two points point at each other.
      // Don't bother with pairs that are obviously mismatched
      Double_t error =
	THaVDCPointPair::CalcError( lowerPoint, upperPoint, fSpacing );

      // Don't create pairs whose matching error is too big
      if( error >= fErrorCutoff )
	continue;

      // Create new point pair
      THaVDCPointPair* thePair = new( (*fLUpairs)[nPairs++] )
	THaVDCPointPair( lowerPoint, upperPoint, fSpacing );

      // Explicitly mark these points as unpartnered
      lowerPoint->SetPartner( 0 );
      upperPoint->SetPartner( 0 );

      // Further analyze this pair
      //TODO: Several things come to mind, to be tested:
      // - calculate global slope
      // - recompute drift distances using global slope
      // - refit cluster using new distances
      // - calculate global chi2
      // - sort pairs by this global chi2 later?
      // - could do all of this before deciding to keep this pair
      thePair->Analyze();
    }
  }

  // Initialize counters
  int n_exist = 0, n_mod = 0;
#ifdef WITH_DEBUG
  int n_oops = 0;
#endif
  // How many tracks already exist in the global track array?
  if( tracks )
    n_exist = tracks->GetLast()+1;

  // Sort pairs in order of ascending matching error
  if( nPairs > 1 )
    fLUpairs->Sort();

#ifdef WITH_DEBUG
  if( fDebug>1 ) {
    cout << nPairs << " pairs.\n";
    fLUpairs->Print();
  }
#endif

 Int_t nTracks = 0;  // Number of reconstructed tracks

  // Mark pairs as partners, starting with the best matches,
  // until all tracks are marked.
  for( int i = 0; i < nPairs; i++ ) {
    THaVDCPointPair* thePair = static_cast<THaVDCPointPair*>( fLUpairs->At(i) );
    assert( thePair );
    assert( thePair->GetError() < fErrorCutoff );

#ifdef WITH_DEBUG
    if( fDebug>1 ) {
      cout << "Pair " << i << ":  ";
      thePair->Print("NOHEAD");
    }
#endif

    // Skip pairs where any of the points already has at least one used cluster
    if( thePair->HasUsedCluster() ) {
#ifdef WITH_DEBUG
      if( fDebug>1 )
	cout << " ... skipped (cluster already used)." << endl;
#endif
      continue;
    }
#ifdef WITH_DEBUG
    if( fDebug>1 )
      cout << " ... good." << endl;
#endif

    // Get the points of the pair
    THaVDCPoint* lowerPoint = thePair->GetLower();
    THaVDCPoint* upperPoint = thePair->GetUpper();
    assert( lowerPoint && upperPoint );

    // All partnered pairs must have a used cluster and hence never get here,
    // else there is a bug in the underlying logic
    assert( lowerPoint->GetPartner() == 0 && upperPoint->GetPartner() == 0 );

    // Use the pair. This partners the points, marks its clusters as used
    // and calculates global slopes
    thePair->Use();
    nTracks++;

#ifdef WITH_DEBUG
    if( fDebug>2 ) {
      thePair->Print("TRACKP");
    }
#endif

    // If the 'tracks' array was given, add THaTracks to it
    // (or modify existing ones).
    if (tracks) {

      // Decide whether this is a new track or an old track
      // that is being updated
      THaVDCTrackID* thisID = new THaVDCTrackID(lowerPoint,upperPoint);
      THaTrack* theTrack = 0;
      bool found = false;
      int t;
      for( t = 0; t < n_exist; t++ ) {
	theTrack = static_cast<THaTrack*>( tracks->At(t) );
	// This test is true if an existing track has exactly the same clusters
	// as the current one (defined by lowerPoint/upperPoint)
	if( theTrack && theTrack->GetCreator() == this &&
	    *thisID == *theTrack->GetID() ) {
	  found = true;
	  break;
	}
#ifdef WITH_DEBUG
	// FIXME: for debugging
	n_oops++;
#endif
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
	theTrack->AddCluster( lowerPoint );
	theTrack->AddCluster( upperPoint );
	assert( tracks->IndexOf(theTrack) >= 0 );
	theTrack->SetTrkNum( tracks->IndexOf(theTrack)+1 );
	thePair->Associate( theTrack );
	if( theStage == kFine )
	  flag |= kReassigned;
      }

      theTrack->SetD(lowerPoint->GetX(), lowerPoint->GetY(),
		     lowerPoint->GetTheta(), lowerPoint->GetPhi());
      theTrack->SetFlag( flag );

      // Calculate the TRANSPORT coordinates
      CalcFocalPlaneCoords(theTrack);

      // Calculate the chi2 of the track from the distances to the wires
      // in the associated clusters
      chi2_t chi2 = thePair->CalcChi2();
      theTrack->SetChi2( chi2.first, chi2.second - 4 );

#ifdef WITH_DEBUG
      if( fDebug>2 ) {
	Double_t chisq = chi2.first;
	Int_t nhits = chi2.second;
	cout << " chi2/ndof = " << chisq << "/" << nhits-4;
	if( nhits > 4 )
	  cout << " = " << chisq/(nhits-4);
	cout << endl;
      }
#endif
    }
  }

#ifdef WITH_DEBUG
  if( fDebug>1 )
    cout << nTracks << " good tracks.\n";
#endif

  // Delete tracks that were not updated
  if( tracks && n_exist > n_mod ) {
    //    bool modified = false;
    for( int i = 0; i < tracks->GetLast()+1; i++ ) {
      THaTrack* theTrack = static_cast<THaTrack*>( tracks->At(i) );
      // Track created by this class and not updated?
      if( (theTrack->GetCreator() == this) &&
	  ((theTrack->GetFlag() & kStageMask) != theStage ) ) {
	// First, release clusters pointing to this track
	for( int j = 0; j < nPairs; j++ ) {
	  THaVDCPointPair* thePair
	    = static_cast<THaVDCPointPair*>( fLUpairs->At(i) );
	  assert(thePair);
	  if( thePair->GetTrack() == theTrack ) {
	    thePair->Associate(0);
	    break;
	  }
	}
	// Then, remove the track
	tracks->RemoveAt(i);
	//	modified = true;
#ifdef WITH_DEBUG
	if( fDebug>1 )
	  cout << "Track " << i << " deleted.\n";
#endif
      }
    }
    // Get rid of empty slots - they may cause trouble in the Event class and
    // with global variables.
    // Note that the PIDinfo and vertices arrays are not reordered.
    // Therefore, PID and vertex information must always be retrieved from the
    // track objects, not from the PID and vertex TClonesArrays.
    // FIXME: Is this really what we want?
    // FIXME: invalidates some or all the track numbers in the clusters
//     if( modified )
//       tracks->Compress();
  }

  // Assign index to each track (0 = first/"best", 1 = second, etc.)
  if( tracks ) {
    for( int i = 0; i < tracks->GetLast()+1; i++ ) {
      THaTrack* theTrack = static_cast<THaTrack*>( tracks->At(i) );
      assert( theTrack );
      theTrack->SetIndex(i);
    }
  }

  return nTracks;
}

//_____________________________________________________________________________
void THaVDC::Clear( Option_t* opt )
{
  // Clear event-by-event data

  THaTrackingDetector::Clear(opt);
  fLower->Clear(opt);
  fUpper->Clear(opt);
}

//_____________________________________________________________________________
Int_t THaVDC::Decode( const THaEvData& evdata )
{
  // Decode data from VDC planes

#ifdef WITH_DEBUG
  // Save current event number for diagnostics
  fEvNum = evdata.GetEvNum();
  if( fDebug>1 ) {
    cout << "=========================================\n";
    cout << "Event: " << fEvNum << endl;
  }
#endif

  fLower->Decode(evdata);
  fUpper->Decode(evdata);

  return 0;
}

//_____________________________________________________________________________
Int_t THaVDC::CoarseTrack( TClonesArray& tracks )
{
  // Coarse Tracking

  if( TestBit(kDecodeOnly) )
    return 0;

  fLower->CoarseTrack();
  fUpper->CoarseTrack();

  // Build tracks and mark them as level 1
  fNtracks = ConstructTracks( &tracks, 1 );

  return 0;
}

//_____________________________________________________________________________
Int_t THaVDC::FineTrack( TClonesArray& /* tracks */ )
{
  // Calculate exact track position and angle using drift time information.
  // Assumes that CoarseTrack has been called (ie - clusters are matched)

  if( TestBit(kDecodeOnly) || TestBit(kCoarseOnly) )
    return 0;

  fLower->FineTrack();
  fUpper->FineTrack();

  //FindBadTracks(tracks);
  //CorrectTimeOfFlight(tracks);

//   for (Int_t i = 0; i < fNumIter; i++) {
//     ConstructTracks();
//     fLower->FineTrack();
//     fUpper->FineTrack();
//   }

//   fNtracks = ConstructTracks( &tracks, 2 );

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
    CalcTargetCoords(theTrack);
  }

  return 0;
}

#if 0
//_____________________________________________________________________________
void THaVDC::DetToTrackTransportCoords( Double_t& x, Double_t& y,
					Double_t& theta, Double_t& phi ) const
{
  // Convert given Transport coordinates from detector (VDC) frame to
  // track (TRANSPORT) frame.

  // This algorithm gives the exact same results as the explicit formulas
  // in CalcFocalPlaneCoords if DetToTrackCoords is a simple rotation
  // about y by fVDCAngle. Because the VDC frame is assumed to be the
  // track reference frame, this is always true, so we don't need the full
  // generality of the coordinate transformation, and the explicit formulas
  // are more efficient.

  // Define two points along the track in VDC coordinates
  TVector3 x0( x, y, 0. );
  TVector3 x1( x0 + TVector3(theta, phi, 1.) );
  // Convert these two points to the TRANSPORT frame
  x0 = DetToTrackCoord(x0);
  x1 = DetToTrackCoord(x1);
  // Calculate the new direction vector
  TVector3 t = x1 - x0;
  // Normalize it to z=1, and we have the projected TRANSPORT angles
  Double_t invz = 1./t.Z();
  theta = t.X()*invz;
  phi   = t.Y()*invz;
  // Project one of the points back to z=0 along the direction vector
  // to get the TRANSPORT positions
  x     = x0.X() - x0.Z()*theta;
  y     = x0.Y() - x0.Z()*phi;
}
#endif

//_____________________________________________________________________________
void THaVDC::CalcFocalPlaneCoords( THaTrack* track )
{
  // calculates focal plane coordinates from detector coordinates

  // first calculate the transport frame coordinates
  // (see DetToTrackTransportCoords above for the general algorithm)
  Double_t theta = (track->GetDTheta()+fTan_vdc) /
    (1.0-track->GetDTheta()*fTan_vdc);
  Double_t x = track->GetDX() * (fCos_vdc + theta * fSin_vdc);
  Double_t phi = track->GetDPhi() / (fCos_vdc - track->GetDTheta() * fSin_vdc);
  Double_t y = track->GetDY() + fSin_vdc*phi*track->GetDX();

  // then calculate the rotating transport frame coordinates
  Double_t r_x = x;

  // calculate the focal-plane matrix elements
  CalcMatrix( r_x, fFPMatrixElems );

  Double_t r_y = y - fFPMatrixElems[Y000].v;  // Y000

  // Calculate now the tan(rho) and cos(rho) of the local rotation angle.
  Double_t tan_rho_loc = fFPMatrixElems[T000].v;   // T000
  Double_t cos_rho_loc = 1.0/sqrt(1.0+tan_rho_loc*tan_rho_loc);

  Double_t r_phi = (track->GetDPhi() - fFPMatrixElems[P000].v /* P000 */ ) /
    (1.0-track->GetDTheta()*tan_rho_loc) / cos_rho_loc;
  Double_t r_theta = (track->GetDTheta()+tan_rho_loc) /
    (1.0-track->GetDTheta()*tan_rho_loc);

  // set the values we calculated
  track->Set(x, y, theta, phi);
  track->SetR(r_x, r_y, r_theta, r_phi);
}

//_____________________________________________________________________________
void THaVDC::CalcTargetCoords( THaTrack* track )
{
  // calculates target coordinates from focal plane coordinates

  const Int_t kNUM_PRECOMP_POW = 10;

  Double_t x_fp, y_fp, th_fp, ph_fp;
  Double_t powers[kNUM_PRECOMP_POW][5];  // {(x), th, y, ph, abs(th) }
  Double_t x, y, theta, phi, dp, p, pathl;

  // first select the coords to use
  if( fCoordType == kTransport ) {
    x_fp = track->GetX();
    y_fp = track->GetY();
    th_fp = track->GetTheta();
    ph_fp = track->GetPhi();
  } else {  // kRotatingTransport
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
       it!=matrix.end(); ++it ) {
    it->v = 0.0;

    if(it->order > 0) {
      for(int i=it->order-1; i>=1; --i)
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
       it!=matrix.end(); ++it )
    if(it->v != 0.0) {
      v = it->v;
      unsigned int np = it->pw.size(); // generalize for extra matrix elems.
      for (unsigned int i=0; i<np; ++i)
	v *= powers[it->pw[i]][i+1];
      retval += v;
  //      retval += it->v * powers[it->pw[0]][1]
  //		      * powers[it->pw[1]][2]
  //		      * powers[it->pw[2]][3];
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
       it!=matrix.end(); ++it )
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

  if( (s1 == 0) || (s2 == 0) )
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
      THaVDCPoint* the_point =
	static_cast<THaVDCPoint*>( track->GetCluster(i) );
      if( !the_point )
	continue;

      the_point->GetUCluster()->SetTimeCorrection(tdelta);
      the_point->GetVCluster()->SetTimeCorrection(tdelta);
    }
  }
}

//_____________________________________________________________________________
void THaVDC::FindBadTracks(TClonesArray& tracks)
{
  // Flag tracks that don't intercept S2 scintillator as bad

  THaScintillator* s2 = static_cast<THaScintillator*>
    ( GetApparatus()->GetDetector("s2") );

  if(s2 == 0) {
    //cerr<<"Could not find s2 plane!!"<<endl;
    return;
  }

  Int_t n_exist = tracks.GetLast()+1;
  for( Int_t t = 0; t < n_exist; t++ ) {
    THaTrack* track = static_cast<THaTrack*>( tracks.At(t) );

    // project the current x and y positions into the s2 plane
    // if the tracks go out of the bounds of the s2 plane,
    // toss the track out
    Double_t x2, y2;
    if( !s2->CalcInterceptCoords(track, x2, y2) || !s2->IsInActiveArea(x2,y2) ) {

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
void THaVDC::Print(const Option_t* opt) const
{
  THaTrackingDetector::Print(opt);

  TString sopt(opt);
  sopt.ToUpper();
  if( sopt.Contains("ME") || sopt.Contains("MATRIX") ) {
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
  }

  return;
}

//_____________________________________________________________________________
Int_t THaVDC::ReadGeometry( FILE* file, const TDatime& date, Bool_t )
{
  // Special VDC geometry reader. Reads only the optional "size" parameters and
  // ignores "position" and "angle".
  // Since the VDC defines the origin of the TRANSPORT coordinate system, we
  // always have fOrigin = (0,0,0). The VDC angle is the T000 focal plane matrix
  // element and so should not be read separately here.

  const char* const here = "ReadGeometry";

  fOrigin.SetXYZ(0,0,0); // by definition

  vector<double> size;
  DBRequest request[] = {
    { "size", &size, kDoubleV, 0, 1, 0, "\"size\" (detector size [m])" },
    { 0 }
  };
  Int_t err = LoadDB( file, date, request );
  if( err )
    return kInitError;

  if( !size.empty() ) {
    if( size.size() != 3 ) {
      Error( Here(here), "Incorrect number of values = %u for "
	     "detector size. Must be exactly 3. Fix database.",
	     static_cast<unsigned int>(size.size()) );
      return 2;
    }
    if( size[0] == 0 || size[1] == 0 || size[2] == 0 ) {
      Error( Here(here), "Illegal zero detector dimension. Fix database." );
      return 3;
    }
    if( size[0] < 0 || size[1] < 0 || size[2] < 0 ) {
      Warning( Here(here), "Illegal negative value for detector dimension. "
	       "Taking absolute. Check database." );
    }
    fSize[0] = 0.5 * TMath::Abs(size[0]);
    fSize[1] = 0.5 * TMath::Abs(size[1]);
    fSize[2] = TMath::Abs(size[2]);
  }
  else
    fSize[0] = fSize[1] = fSize[2] = kBig;

  return 0;
}

//_____________________________________________________________________________
void THaVDC::SetDebug( Int_t level )
{
  // Set debug level of the VDC and the chamber/plane subdetectors

  THaTrackingDetector::SetDebug(level);
  fLower->SetDebug(level);
  fUpper->SetDebug(level);
}

////////////////////////////////////////////////////////////////////////////////
ClassImp(THaVDC)
