///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// OldVDCPlane                                                               //
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

#include "OldVDC.h"
#include "OldVDCPlane.h"
#include "OldVDCWire.h"
#include "OldVDCUVPlane.h"
#include "OldVDCCluster.h"
#include "OldVDCHit.h"
#include "THaDetMap.h"
#include "OldVDCAnalyticTTDConv.h"
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
#include <iomanip>

using namespace std;

#define ALL(c) (c).begin(), (c).end()

// Defaults for typical VDC operation. Can be overridden via set functions.
// Configurable via database in version 1.6 and later.

static const Int_t kDefaultNMaxGap = 1;
static const Int_t kDefaultMinTime = 800;
static const Int_t kDefaultMaxTime = 2200;
static const Double_t kDefaultTDCRes = 5.0e-10;  // 0.5 ns/chan = 5e-10 s /chan

//_____________________________________________________________________________
OldVDCPlane::OldVDCPlane( const char* name, const char* description,
			  THaDetectorBase* parent )
  : THaSubDetector(name,description,parent), fNWiresHit(0),
    fNWiresHit(0), fNMaxGap(kDefaultNMaxGap), fMinTime(kDefaultMinTime),
    fMaxTime(kDefaultMaxTime), fFlags(0), fZ(0), fWBeg(0), fWSpac(0),
    fWAngle(0), fDriftVel(0), fTDCRes(kDefaultTDCRes),
    /*fTable(NULL),*/ fTTDConv(0), fglTrg(0)
{
  // Constructor

  // Since TCloneArrays can resize, the size here is fairly unimportant
  fHits     = new TClonesArray("OldVDCHit", 20 );
  fClusters = new TClonesArray("OldVDCCluster", 5 );
  fWires    = new TClonesArray("OldVDCWire", 368 );

  fVDC = GetMainDetector();

  ResetBit(kMaxGapSet);
  ResetBit(kMinTimeSet);
  ResetBit(kMaxTimeSet);
  ResetBit(kTDCResSet);
}

//_____________________________________________________________________________
void OldVDCPlane::MakePrefix()
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
Int_t OldVDCPlane::DoReadDatabase( FILE* file, const TDatime& date )
{
  // Allocate TClonesArray objects and load plane parameters from database

  const char* const here = "ReadDatabase";

  // Read fOrigin and fSize
  Int_t err = ReadGeometry( file, date );
  if( err ) {
    return err;
  }
  fZ = fOrigin.Z();

  // Read configuration parameters
  vector<Int_t> detmap, bad_wirelist;
  vector<Double_t> ttd_param;
  vector<Float_t> tdc_offsets;
  Int_t maxgap  = kDefaultNMaxGap;
  Int_t mintime = kDefaultMinTime;
  Int_t maxtime = kDefaultMaxTime;
  Double_t tdcres = kDefaultTDCRes;

  DBRequest request[] = {
    { "detmap",         &detmap,         kIntV },
    { "nwires",         &fNelem,         kInt,     0, 0, -1 },
    { "wire.start",     &fWBeg,          kDouble },
    { "wire.spacing",   &fWSpac,         kDouble,  0, 0, -1 },
    { "wire.angle",     &fWAngle,        kDouble,  0, 0 },
    { "wire.badlist",   &bad_wirelist,   kIntV,    0, 1 },
    { "driftvel",       &fDriftVel,      kDouble,  0, 0, -1 },
    { "maxgap",         &maxgap,         kInt,     0, 1, -1 },
    { "tdc.min",        &mintime,        kInt,     0, 1, -1 },
    { "tdc.max",        &maxtime,        kInt,     0, 1, -1 },
    { "tdc.res",        &tdcres,         kDouble,  0, 0, -1 },
    { "tdc.offsets",    &tdc_offsets,    kFloatV },
    { "ttd.param",      &ttd_param,      kDoubleV, 0, 0, -1 },
    { "description",    &fTitle,         kTString, 0, 1 },
    { 0 }
  };

  err = LoadDB( file, date, request, fPrefix );
  if( err )
    return err;

  if( FillDetMap(detmap, THaDetMap::kFillLogicalChannel, here) <= 0 )
    return kInitError; // Error already printed by FillDetMap

  fWAngle *= TMath::DegToRad(); // Convert to radians

  set<Int_t> bad_wires( ALL(bad_wirelist) );
  bad_wirelist.clear();

  // Sanity checks
  if( fNelem <= 0 ) {
    Error( Here(here), "Invalid number of wires: %d", fNelem );
    return kInitError;
  }

  Int_t nchan = fDetMap->GetTotNumChan();
  if( nchan != fNelem ) {
    Error( Here(here), "Number of detector map channels (%d) "
	   "disagrees with number of wires (%d)", nchan, fNelem );
    return kInitError;
  }
  nchan = tdc_offsets.size();
  if( nchan != fNelem ) {
    Error( Here(here), "Number of TDC offset values (%d) "
	   "disagrees with number of wires (%d)", nchan, fNelem );
    return kInitError;
  }

  // Values set via Set functions override database values
  if( !TestBit(kMaxGapSet) )
    fNMaxGap = maxgap;
  if( !TestBit(kMinTimeSet) )
    fMinTime = mintime;
  if( !TestBit(kMaxTimeSet) )
    fMaxTime = maxtime;
  if( !TestBit(kTDCResSet) )
    fTDCRes = tdcres;

  if( fNMaxGap < 0 || fNMaxGap > 2 ) {
    Error( Here(here), "Invalid max_gap = %d, must be betwwen 0 and 2. "
	   "Fix database.", fNMaxGap );
    return kInitError;
  }
  if( fMinTime < 0 || fMinTime > 4095 ) {
    Error( Here(here), "Invalid min_time = %d, must be betwwen 0 and 4095. "
	   "Fix database.", fMinTime );
    return kInitError;
  }
  if( fMaxTime < 1 || fMaxTime > 4096 || fMinTime >= fMaxTime ) {
    Error( Here(here), "Invalid max_time = %d. Must be between 1 and 4096 "
	   "and >= min_time = %d. Fix database.", fMaxTime, fMinTime );
    return kInitError;
  }
  if( fTDCRes < 0 || fTDCRes > 1e-6 ) {
    Error( Here(here), "Nonsense TDC resolution = %8.1le s/channel. "
	   "Fix database.", fTDCRes );
    return kInitError;
  }

  // Create time-to-distance converter
  delete fTTDConv;
  fTTDConv = new OldVDCAnalyticTTDConv(fDriftVel);
  
  // Initialize wires
  for (int i = 0; i < fNelem; i++) {
    OldVDCWire* wire = new((*fWires)[i])
      OldVDCWire( i, fWBeg+i*fWSpac, tdc_offsets[i], fTTDConv );
    if( bad_wires.find(i) != bad_wires.end() )
      wire->SetFlag(1);
  }

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
    // Warning(Here(here),"Trigger-time detector \"%s\" not found. "
    // 	    "Event-by-event time offsets will NOT be used!!",nm);
  }

#ifdef WITH_DEBUG
  if( fDebug > 2 ) {
    Double_t pos[3]; fOrigin.GetXYZ(pos);
    Double_t angle = fWAngle*TMath::RadToDeg();
    DBRequest list[] = {
      { "Number of wires",         &fNelem,     kInt       },
      { "Detector position",       pos,         kDouble, 3 },
      { "Detector size",           fSize,       kFloat,  3 },
      { "Wire angle (deg)",        &angle                  },
      { "Wire start pos (m)",      &fWBeg                  },
      { "Wire spacing (m)",        &fWSpac                 },
      { "TDC resolution (s/chan)", &fTDCRes                },
      { "Drift Velocity (m/s) ",   &fDriftVel              },
      { "Max gap in cluster",      &fNMaxGap,   kInt       },
      { "Min TDC raw time",        &fMinTime,   kInt       },
      { "Max TDC raw time",        &fMaxTime,   kInt       },
      { 0 }
    };
    DebugPrint( list );
  }
#endif

  fIsInit = true;
  return kOK;
}

//_____________________________________________________________________________
Int_t OldVDCPlane::ReadDatabase( const TDatime& date )
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
Int_t OldVDCPlane::DefineVariables( EMode mode )
{
  // initialize global variables

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  // Register variables in global list

  RVarDef vars[] = {
    { "nhit",   "Number of hits",             "GetNHits()" },
    { "wire",   "Active wire numbers",        "fHits.OldVDCHit.GetWireNum()" },
    { "rawtime","Raw TDC values of wires",    "fHits.OldVDCHit.fRawTime" },
    { "time",   "TDC values of active wires", "fHits.OldVDCHit.fTime" },
    { "dist",   "Drift distances",            "fHits.OldVDCHit.fDist" },
    { "ddist",  "Drft dist uncertainty",      "fHits.OldVDCHit.fdDist" },
    { "trdist", "Dist. from track",           "fHits.OldVDCHit.ftrDist" },
    { "nclust", "Number of clusters",         "GetNClusters()" },
    { "clsiz",  "Cluster sizes",              "fClusters.OldVDCCluster.fSize" },
    { "clpivot","Cluster pivot wire num",     "fClusters.OldVDCCluster.GetPivotWireNum()" },
    { "clpos",  "Cluster intercepts (m)",     "fClusters.OldVDCCluster.fInt" },
    { "slope",  "Cluster best slope",         "fClusters.OldVDCCluster.fSlope" },
    { "lslope", "Cluster local (fitted) slope","fClusters.OldVDCCluster.fLocalSlope" },
    { "t0",     "Timing offset (s)",          "fClusters.OldVDCCluster.fT0" },
    { "sigsl",  "Cluster slope error",        "fClusters.OldVDCCluster.fSigmaSlope" },
    { "sigpos", "Cluster position error (m)", "fClusters.OldVDCCluster.fSigmaInt" },
    { "sigt0",  "Timing offset error (s)",    "fClusters.OldVDCCluster.fSigmaT0" },
    { "clchi2", "Cluster chi2",               "fClusters.OldVDCCluster.fChi2" },
    { "clndof", "Cluster NDoF",               "fClusters.OldVDCCluster.fNDoF" },
    { "cltcor", "Cluster Time correction",    "fClusters.OldVDCCluster.fTimeCorrection" },
    { 0 }
  };
  return DefineVarsFromList( vars, mode );

}

//_____________________________________________________________________________
OldVDCPlane::~OldVDCPlane()
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
void OldVDCPlane::Clear( Option_t* opt )
{    
  // Clears the contents of the and hits and clusters
  THaSubDetector::Clear(opt);
  fNWiresHit = 0;
  fHits->Clear();
  fClusters->Clear();
}

//_____________________________________________________________________________
Int_t OldVDCPlane::Decode( const THaEvData& evData)
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
    only_fastest_hit = fVDC->TestBit(OldVDC::kOnlyFastest);
    // If true, ignore negativ drift times completely
    no_negative      = fVDC->TestBit(OldVDC::kIgnoreNegDrift);
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
      OldVDCWire* wire = GetWire(wireNum);
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
	    new( (*fHits)[nextHit++] )  OldVDCHit( wire, data, time );
	}
	  
      } // End hit loop

      // If we are only interested in the hit with the largest TDC value 
      // (shortest drift time), it is recorded here.
      if( only_fastest_hit && max_data>0 ) {
	Double_t xdata = static_cast<Double_t>(max_data) + 0.5;
	Double_t time = fTDCRes * (toff - xdata) - evtT0;
	new( (*fHits)[nextHit++] ) OldVDCHit( wire, max_data, time );
      }
    } // End channel index loop
  } // End slot loop

  // Sort the hits in order of increasing wire number and (for the same wire
  // number) increasing time (NOT rawtime)

  fHits->Sort();

#ifdef WITH_DEBUG
  if ( fDebug > 3 ) {
    cout << endl << "VDC plane " << GetPrefix() << endl;
    int ncol=4;
    for (int i=0; i<ncol; i++) {
      cout << "     Wire    TDC  ";
    }
    cout << endl;
    
    for (int i=0; i<(nextHit+ncol-1)/ncol; i++ ) {
      for (int c=0; c<ncol; c++) {
	int ind = c*nextHit/ncol+i;
	if (ind < nextHit) {
	  OldVDCHit* hit = static_cast<OldVDCHit*>(fHits->At(ind));
	  cout << "     " << setw(3) << hit->GetWireNum()
	       << "    "  << setw(5) << hit->GetRawTime() << " ";
	} else {
	  //	  cout << endl;
	  break;
	}
      }
      cout << endl;
    }
  }
#endif

  return 0;

}


//_____________________________________________________________________________
Int_t OldVDCPlane::FindClusters()
{
  // Reconstruct clusters in a VDC plane
  // Assumes that the wires are numbered such that increasing wire numbers
  // correspond to decreasing physical position.
  // Ignores possibility of overlapping clusters

  bool hard_cut = false, soft_cut = false;
  if( fVDC ) {
    hard_cut = fVDC->TestBit(OldVDC::kHardTDCcut);
    soft_cut = fVDC->TestBit(OldVDC::kSoftTDCcut);
  }
  Double_t maxdist = 0.0;
  if( soft_cut ) {
    maxdist = 0.5*static_cast<OldVDCUVPlane*>(GetParent())->GetSpacing();
    if( maxdist == 0.0 )
      soft_cut = false;
  }

  Int_t pwireNum = -10;         // Previous wire number
  Int_t wireNum  =   0;         // Current wire number
  Int_t ndif     =   0;         // Difference between wire numbers
  Int_t nHits    = GetNHits();  // Number of hits in the plane
  OldVDCCluster* clust = NULL;  // Current cluster
  OldVDCHit* hit;               // current hit

//    Int_t minTime = 0;        // Smallest TDC time for a given cluster
//    OldVDCHit * minHit = NULL; // Hit with the smallest TDC time for 
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

      // Make a new OldVDCCluster (using space from fCluster array)  
      clust = new ( (*fClusters)[nextClust++] ) OldVDCCluster(this);
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
Int_t OldVDCPlane::FitTracks()
{    
  // Fit tracks to cluster positions and drift distances.
  
  OldVDCCluster* clust;
  Int_t nClust = GetNClusters();
  for (int i = 0; i < nClust; i++) {
    if( !(clust = static_cast<OldVDCCluster*>( (*fClusters)[i] )))
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
void OldVDCPlane::SetNMaxGap( Int_t val )
{
  if( val < 0 || val > 2 ) {
    Error( Here("SetNMaxGap"),
	   "Invalid max_gap = %d, must be betwwen 0 and 2.", val );
    return;
  }
  fNMaxGap = val;
  SetBit(kMaxGapSet);
}

//_____________________________________________________________________________
void OldVDCPlane::SetMinTime( Int_t val )
{
  if( val < 0 || val > 4095 ) {
    Error( Here("SetMinTime"),
	   "Invalid min_time = %d, must be betwwen 0 and 4095.", val );
    return;
  }
  fMinTime = val;
  SetBit(kMinTimeSet);
}

//_____________________________________________________________________________
void OldVDCPlane::SetMaxTime( Int_t val )
{
  if( val < 1 || val > 4096 ) {
    Error( Here("SetMaxTime"),
	   "Invalid max_time = %d. Must be between 1 and 4096.", val );
    return;
  }
  fMaxTime = val;
  SetBit(kMaxTimeSet);
}

//_____________________________________________________________________________
void OldVDCPlane::SetTDCRes( Double_t val )
{
  if( val < 0 || val > 1e-6 ) {
    Error( Here("SetTDCRes"),
	   "Nonsense TDC resolution = %8.1le s/channel.", val );
    return;
  }
  fTDCRes = val;
  SetBit(kTDCResSet);
}


///////////////////////////////////////////////////////////////////////////////
ClassImp(OldVDCPlane)
