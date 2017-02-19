///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCPlane                                                               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Here:                                                                     //
//        Units of measurements:                                             //
//        For cluster position (center) and size  -  wires;                  //
//        For X, Y, and Z coordinates of track    -  meters;                 //
//        For Theta and Phi angles of track       -  in tangents.            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaVDC.h"
#include "THaVDCPlane.h"
#include "THaVDCWire.h"
#include "THaVDCUVPlane.h"
#include "THaVDCCluster.h"
#include "THaVDCHit.h"
#include "THaDetMap.h"
#include "THaVDCAnalyticTTDConv.h"
#include "THaEvData.h"
#include "TString.h"
#include "TClass.h"
#include "TMath.h"
#include "VarDef.h"
#include "THaApparatus.h"
#include "THaTriggerTime.h"

#include <cstring>
#include <vector>
#include <iostream>

using namespace std;

// Defaults for typical VDC operation. Can be overridden via set functions.
// Configurable via database in version 1.6 and later.

static const Int_t kDefaultNMaxGap = 1;
static const Int_t kDefaultMinTime = 800;
static const Int_t kDefaultMaxTime = 2200;
static const Double_t kDefaultTDCRes = 5.0e-10;  // 0.5 ns/chan = 5e-10 s /chan

//_____________________________________________________________________________
THaVDCPlane::THaVDCPlane( const char* name, const char* description,
			  THaDetectorBase* parent )
  : THaSubDetector(name,description,parent), fNWiresHit(0),
    fNMaxGap(kDefaultNMaxGap), fMinTime(kDefaultMinTime),
    fMaxTime(kDefaultMaxTime), fFlags(0), fTDCRes(kDefaultTDCRes),
    /*fTable(NULL),*/ fTTDConv(0), fVDC(0), fglTrg(0)
{
  // Constructor

  // Since TCloneArrays can resize, the size here is fairly unimportant
  fHits     = new TClonesArray("THaVDCHit", 20 );
  fClusters = new TClonesArray("THaVDCCluster", 5 );
  fWires    = new TClonesArray("THaVDCWire", 368 );

  fVDC = GetMainDetector();
}

//_____________________________________________________________________________
void THaVDCPlane::MakePrefix()
{
  // Special treatment of the prefix for VDC planes: We don't want variable
  // names such as "R.vdc.uv1.u.x", but rather "R.vdc.u1.x".

  TString basename;
  THaDetectorBase* uv_plane = GetParent();

  if( fVDC )
    basename = fVDC->GetPrefix();
  if( fName.Contains("u") )
    basename.Append("u");
  else if ( fName.Contains("v") )
    basename.Append("v");
  if( uv_plane && strstr( uv_plane->GetName(), "uv1" ))
    basename.Append("1.");
  else if( uv_plane && strstr( uv_plane->GetName(), "uv2" ))
    basename.Append("2.");
  if( basename.Length() == 0 )
    basename = fName + ".";

  delete [] fPrefix;
  fPrefix = new char[ basename.Length()+1 ];
  strcpy( fPrefix, basename.Data() );
}

//_____________________________________________________________________________
Int_t THaVDCPlane::DoReadDatabase( FILE* file, const TDatime& /* date */ )
{
  // Allocate TClonesArray objects and load plane parameters from database

  const char* const here = "ReadDatabase";
  const int LEN = 100;
  char buff[LEN];

  // Build the search tag and find it in the file. Search tags
  // are of form [ <prefix> ], e.g. [ R.vdc.u1 ].
  TString tag(fPrefix); tag.Chop(); // delete trailing dot of prefix
  tag.Prepend("["); tag.Append("]");
  TString line;
  bool found = false;
  while( !found && fgets(buff, LEN, file) ) {
    char* buf = ::Compress(buff);  //strip blanks
    line = buf;
    delete [] buf;
    if( line.EndsWith("\n") ) line.Chop();  //delete trailing newline
    if ( tag == line )
      found = true;
  }
  if( !found ) {
    Error(Here(here), "Database entry \"%s\" not found!", tag.Data() );
    return kInitError;
  }

  //Found the entry for this plane, so read data
  Int_t nWires = 0;    // Number of wires to create
  Int_t prev_first = 0, prev_nwires = 0;
  // Set up the detector map
  fDetMap->Clear();
  do {
    fgets( buff, LEN, file );
    // bad kludge to allow a variable number of detector map lines
    if( strchr(buff, '.') ) // any floating point number indicates end of map
      break;
    // Get crate, slot, low channel and high channel from file
    Int_t crate, slot, lo, hi;
    if( sscanf( buff, "%6d %6d %6d %6d", &crate, &slot, &lo, &hi ) != 4 ) {
      if( *buff ) buff[strlen(buff)-1] = 0; //delete trailing newline
      Error( Here(here), "Error reading detector map line: %s", buff );
      return kInitError;
    }
    Int_t first = prev_first + prev_nwires;
    // Add module to the detector map
    fDetMap->AddModule(crate, slot, lo, hi, first);
    prev_first = first;
    prev_nwires = (hi - lo + 1);
    nWires += prev_nwires;
  } while( *buff );  // sanity escape

  // Load z, wire beginning postion, wire spacing, and wire angle
  Int_t n =
    sscanf( buff, "%15lf %15lf %15lf %15lf", &fZ, &fWBeg, &fWSpac, &fWAngle );
  if( n != 4 ) return ErrPrint(file,here);
  fWAngle *= TMath::Pi()/180.0; // Convert to radians

  // Load drift velocity (will be used to initialize Crude Time to Distance
  // converter)
  n = fscanf(file, "%15lf", &fDriftVel);
  if( n != 1 ) return ErrPrint(file,here);
  fgets(buff, LEN, file); // Read to end of line
  fgets(buff, LEN, file); // Skip line

  fNWiresHit = 0;

  // first read in the time offsets for the wires
  vector<Double_t> wire_offsets(nWires);
  vector<Int_t> wire_nums(nWires);

  for (int i = 0; i < nWires; i++) {
    Int_t wnum = 0;
    Double_t offset = 0.0;
    n = fscanf(file, " %6d %15lf", &wnum, &offset);
    if( n != 2 ) return ErrPrint(file,here);
    wire_nums[i] = wnum-1; // Wire numbers in file start at 1
    wire_offsets[i] = offset;
  }

  // now read in the time-to-drift-distance lookup table
  // data (if it exists)
//   fgets(buff, LEN, file); // read to the end of line
//   fgets(buff, LEN, file);
//   if(strncmp(buff, "TTD Lookup Table", 16) == 0) {
//     // if it exists, read the data in
//     fscanf(file, "%le", &fT0);
//     fscanf(file, "%d", &fNumBins);

//     // this object is responsible for the memory management
//     // of the lookup table
//     delete [] fTable;
//     fTable = new Float_t[fNumBins];
//     for(int i=0; i<fNumBins; i++) {
//       fscanf(file, "%e", &(fTable[i]));
//     }
//   } else {
//     // if not, set some reasonable defaults and rewind the file
//     fT0 = 0.0;
//     fNumBins = 0;
//     fTable = NULL;
//     cout<<"Could not find lookup table header: "<<buff<<endl;
//     fseek(file, -strlen(buff), SEEK_CUR);
//   }

  // Define time-to-drift-distance converter
  // Currently, we use the analytic converter.
  // FIXME: Let user choose this via a parameter
  delete fTTDConv;
  fTTDConv = new THaVDCAnalyticTTDConv(fDriftVel);

  //THaVDCLookupTTDConv* ttdConv = new THaVDCLookupTTDConv(fT0, fNumBins, fTable);

  // Now initialize wires (those wires... too lazy to initialize themselves!)
  // Caution: This may not correspond at all to actual wire channels!
  for (int i = 0; i < nWires; i++) {
    THaVDCWire* wire = new((*fWires)[i])
      THaVDCWire( i, fWBeg+i*fWSpac, wire_offsets[i], fTTDConv );
    if( wire_nums[i] < 0 )
      wire->SetFlag(1);
  }

  fOrigin.SetXYZ( 0.0, 0.0, fZ );

  THaDetectorBase *sdet = GetParent();
  if( sdet )
    fOrigin += sdet->GetOrigin();

  // finally, find the timing-offset to apply on an event-by-event basis
  //FIXME: time offset handling should go into the enclosing apparatus -
  //since not doing so leads to exactly this kind of mess:
  THaApparatus* app = GetApparatus();
  const char* nm = "trg"; // inside an apparatus, the apparatus name is assumed
  if( !app ||
      !(fglTrg = dynamic_cast<THaTriggerTime*>(app->GetDetector(nm))) ) {
    Warning(Here(here),"Trigger-time detector \"%s\" not found. "
	    "Event-by-event time offsets will NOT be used!!",nm);
  }

  fIsInit = true;
  return kOK;
}

//_____________________________________________________________________________
Int_t THaVDCPlane::ReadDatabase( const TDatime& date )
{
  // Wrapper around actual database reader. Using a wrapper makes it much
  // to close the input file in case of an error.

  FILE* fi = OpenFile( date );
  if( !fi ) return kFileError;

  Int_t ret = DoReadDatabase( fi, date );

  fclose(fi);
  return ret;
}

//_____________________________________________________________________________
Int_t THaVDCPlane::DefineVariables( EMode mode )
{
  // initialize global variables

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  // Register variables in global list

  RVarDef vars[] = {
    { "nhit",   "Number of hits",             "GetNHits()" },
    { "wire",   "Active wire numbers",        "fHits.THaVDCHit.GetWireNum()" },
    { "rawtime","Raw TDC values of wires",    "fHits.THaVDCHit.fRawTime" },
    { "time",   "TDC values of active wires", "fHits.THaVDCHit.fTime" },
    { "dist",   "Drift distances",            "fHits.THaVDCHit.fDist" },
    { "ddist",  "Drft dist uncertainty",      "fHits.THaVDCHit.fdDist" },
    { "trdist", "Dist. from track",           "fHits.THaVDCHit.ftrDist" },
    { "nclust", "Number of clusters",         "GetNClusters()" },
    { "clsiz",  "Cluster sizes",              "fClusters.THaVDCCluster.fSize" },
    { "clpivot","Cluster pivot wire num",     "fClusters.THaVDCCluster.GetPivotWireNum()" },
    { "clpos",  "Cluster intercepts (m)",     "fClusters.THaVDCCluster.fInt" },
    { "slope",  "Cluster best slope",         "fClusters.THaVDCCluster.fSlope" },
    { "lslope", "Cluster local (fitted) slope","fClusters.THaVDCCluster.fLocalSlope" },
    { "t0",     "Timing offset (s)",          "fClusters.THaVDCCluster.fT0" },
    { "sigsl",  "Cluster slope error",        "fClusters.THaVDCCluster.fSigmaSlope" },
    { "sigpos", "Cluster position error (m)", "fClusters.THaVDCCluster.fSigmaInt" },
    { "sigt0",  "Timing offset error (s)",    "fClusters.THaVDCCluster.fSigmaT0" },
    { "clchi2", "Cluster chi2",               "fClusters.THaVDCCluster.fChi2" },
    { "clndof", "Cluster NDoF",               "fClusters.THaVDCCluster.fNDoF" },
    { "cltcor", "Cluster Time correction",    "fClusters.THaVDCCluster.fTimeCorrection" },
    { 0 }
  };
  return DefineVarsFromList( vars, mode );

}

//_____________________________________________________________________________
THaVDCPlane::~THaVDCPlane()
{
  // Destructor.

  if( fIsSetup )
    RemoveVariables();
  delete fWires;
  delete fHits;
  delete fClusters;
  delete fTTDConv;
//   delete [] fTable;
}

//_____________________________________________________________________________
void THaVDCPlane::Clear( Option_t* )
{    
  // Clears the contents of the and hits and clusters
  fNWiresHit = 0;
  fHits->Clear();
  fClusters->Clear();
}

//_____________________________________________________________________________
Int_t THaVDCPlane::Decode( const THaEvData& evData)
{    
  // Converts the raw data into hit information
  // Assumes channels & wires  are numbered in order
  // TODO: Make sure the wires are numbered in order, even if the channels
  //       aren't
              
  if (!evData.IsPhysicsTrigger()) return -1;

  // the event's T0-shift, due to the trigger-type
  // only an issue when adding in un-retimed trigger types
  Double_t evtT0=0;
  if ( fglTrg && fglTrg->Decode(evData)==kOK ) evtT0 = fglTrg->TimeOffset();
  
  Int_t nextHit = 0;

  bool only_fastest_hit, no_negative;
  if( fVDC ) {
    // If true, add only the first (earliest) hit for each wire
    only_fastest_hit = fVDC->TestBit(THaVDC::kOnlyFastest);
    // If true, ignore negativ drift times completely
    no_negative      = fVDC->TestBit(THaVDC::kIgnoreNegDrift);
  } else
    only_fastest_hit = no_negative = false;

  // Loop over all detector modules for this wire plane
  for (Int_t i = 0; i < fDetMap->GetSize(); i++) {
    THaDetMap::Module * d = fDetMap->GetModule(i);
    
    // Get number of channels with hits
    Int_t nChan = evData.GetNumChan(d->crate, d->slot);
    for (Int_t chNdx = 0; chNdx < nChan; chNdx++) {
      // Use channel index to loop through channels that have hits

      Int_t chan = evData.GetNextChan(d->crate, d->slot, chNdx);
      if (chan < d->lo || chan > d->hi) 
	continue; //Not part of this detector

      // Wire numbers count up in the order in which channels are defined
      // in the detector map. That order may be forward or reverse, e.g.
      // forward: lo hi first = 0  95 1 --> channels 0..95 -> wire# = 1...96
      // reverse: lo hi first = 95  0 1 --> channels 0..95 -> wire# = 96...1
      Int_t wireNum  = d->first + ((d->reverse) ? d->hi - chan : chan - d->lo);
      THaVDCWire* wire = GetWire(wireNum);
      if( !wire || wire->GetFlag() != 0 ) continue;

      // Get number of hits for this channel and loop through hits
      Int_t nHits = evData.GetNumHits(d->crate, d->slot, chan);
   
      Int_t max_data = -1;
      Double_t toff = wire->GetTOffset();

      for (Int_t hit = 0; hit < nHits; hit++) {
	
	// Now get the TDC data for this hit
	Int_t data = evData.GetData(d->crate, d->slot, chan, hit);

	// Convert the TDC value to the drift time.
	// Being perfectionist, we apply a 1/2 channel correction to the raw 
	// TDC data to compensate for the fact that the TDC truncates, not
	// rounds, the data.
	Double_t xdata = static_cast<Double_t>(data) + 0.5;
	Double_t time = fTDCRes * (toff - xdata) - evtT0;

	// If requested, ignore hits with negative drift times 
	// (due to noise or miscalibration). Use with care.
	// If only fastest hit requested, find maximum TDC value and record the
	// hit after the hit loop is done (see below). 
	// Otherwise just record all hits.
	if( !no_negative || time > 0.0 ) {	  
	  if( only_fastest_hit ) {
	    if( data > max_data )
	      max_data = data;
	  } else
	    new( (*fHits)[nextHit++] )  THaVDCHit( wire, data, time );
	}
	  
      } // End hit loop

      // If we are only interested in the hit with the largest TDC value 
      // (shortest drift time), it is recorded here.
      if( only_fastest_hit && max_data>0 ) {
	Double_t xdata = static_cast<Double_t>(max_data) + 0.5;
	Double_t time = fTDCRes * (toff - xdata) - evtT0;
	new( (*fHits)[nextHit++] ) THaVDCHit( wire, max_data, time );
      }
    } // End channel index loop
  } // End slot loop

  // Sort the hits in order of increasing wire number and (for the same wire
  // number) increasing time (NOT rawtime)

  fHits->Sort();

  if ( fDebug > 3 ) {
    printf("\nVDC %s:\n",GetPrefix());
    int ncol=4;
    for (int i=0; i<ncol; i++) {
      printf("     Wire    TDC  ");
    }
    printf("\n");
    
    for (int i=0; i<(nextHit+ncol-1)/ncol; i++ ) {
      for (int c=0; c<ncol; c++) {
	int ind = c*nextHit/ncol+i;
	if (ind < nextHit) {
	  THaVDCHit* hit = static_cast<THaVDCHit*>(fHits->At(ind));
	  printf("     %3d    %5d ",hit->GetWireNum(),hit->GetRawTime());
	} else {
	  //	  printf("\n");
	  break;
	}
      }
      printf("\n");
    }
  }

  return 0;

}


//_____________________________________________________________________________
Int_t THaVDCPlane::FindClusters()
{
  // Reconstruct clusters in a VDC plane
  // Assumes that the wires are numbered such that increasing wire numbers
  // correspond to decreasing physical position.
  // Ignores possibility of overlapping clusters

  bool hard_cut = false, soft_cut = false;
  if( fVDC ) {
    hard_cut = fVDC->TestBit(THaVDC::kHardTDCcut);
    soft_cut = fVDC->TestBit(THaVDC::kSoftTDCcut);
  }
  Double_t maxdist = 0.0;
  if( soft_cut ) {
    maxdist = 0.5*static_cast<THaVDCUVPlane*>(GetParent())->GetSpacing();
    if( maxdist == 0.0 )
      soft_cut = false;
  }

  Int_t pwireNum = -10;         // Previous wire number
  Int_t wireNum  =   0;         // Current wire number
  Int_t ndif     =   0;         // Difference between wire numbers
  Int_t nHits    = GetNHits();  // Number of hits in the plane
  THaVDCCluster* clust = NULL;  // Current cluster
  THaVDCHit* hit;               // current hit

//    Int_t minTime = 0;        // Smallest TDC time for a given cluster
//    THaVDCHit * minHit = NULL; // Hit with the smallest TDC time for 
                             // a given cluster
  //  const Double_t sqrt2 = 0.707106781186547462;

  Int_t nextClust = GetNClusters();  // Should be zero

  for ( int i = 0; i < nHits; i++ ) {
    //Loop through all TDC  hits
    if( !(hit = GetHit(i)))
      continue;

    // Time within sanity cuts?
    if( hard_cut ) {
      Double_t rawtime = hit->GetRawTime();
      if( rawtime < fMinTime || rawtime > fMaxTime) 
	continue;
    }
    if( soft_cut ) {
      Double_t ratio = hit->GetTime() * fDriftVel / maxdist;
      if( ratio < -0.5 || ratio > 1.5 )
	continue;
    }

    wireNum = hit->GetWire()->GetNum();  

    // Ignore multiple hits per wire
    if ( wireNum == pwireNum )
      continue;

    // Keep track of how many wire were hit
    fNWiresHit++;
    ndif = wireNum - pwireNum;
    if (ndif < 0) {
      // Scream Bloody Murder
      Error(Here("FindCluster"),"Wire ordering error at wire numbers %d %d. "
	    "Call expert.", pwireNum, wireNum );
      fClusters->Remove(clust);
      return GetNClusters();
    }

    pwireNum = wireNum;
    if ( ndif > fNMaxGap+1 ) {
      // Found a new cluster
      if (clust) 
	// Estimate the track parameters for this cluster
	// (Pivot, intercept, and slope)
	clust->EstTrackParameters();

      // Make a new THaVDCCluster (using space from fCluster array)  
      clust = new ( (*fClusters)[nextClust++] ) THaVDCCluster(this);
    } 
    //Add hit to the cluster
    clust->AddHit(hit);

  } // End looping through hits

  // Estimate track parameters for the last cluster found
  if (clust)
    clust->EstTrackParameters(); 

  return GetNClusters();  // return the number of clusters found
}

//_____________________________________________________________________________
Int_t THaVDCPlane::FitTracks()
{    
  // Fit tracks to cluster positions and drift distances.
  
  THaVDCCluster* clust;
  Int_t nClust = GetNClusters();
  for (int i = 0; i < nClust; i++) {
    if( !(clust = static_cast<THaVDCCluster*>( (*fClusters)[i] )))
      continue;

    // Convert drift times to distances. 
    // The conversion algorithm is determined at wire initialization time,
    // i.e. currently in the ReadDatabase() function of this class.
    // Current best estimates of the track parameters will be passed to
    // the converter.
    clust->ConvertTimeToDist();

    // Fit drift distances to get intercept, slope.
    clust->FitTrack();
  }
  return 0;
}

//_____________________________________________________________________________
void THaVDCPlane::SetNMaxGap( Int_t val )
{
  fNMaxGap = val;
}

//_____________________________________________________________________________
void THaVDCPlane::SetMinTime( Int_t val )
{
  fMinTime = val;
}

//_____________________________________________________________________________
void THaVDCPlane::SetMaxTime( Int_t val )
{
  fMaxTime = val;
}

//_____________________________________________________________________________
void THaVDCPlane::SetTDCRes( Double_t val )
{
  fTDCRes = val;
}


///////////////////////////////////////////////////////////////////////////////
ClassImp(THaVDCPlane)
