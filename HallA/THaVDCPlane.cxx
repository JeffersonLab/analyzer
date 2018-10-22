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

//#define CLUST_RAWDATA_HACK

#include "THaVDC.h"
#include "THaVDCPlane.h"
#include "THaVDCWire.h"
#include "THaVDCChamber.h"
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
#include <cassert>
#include <stdexcept>
#include <set>
#include <iomanip>

#ifdef CLUST_RAWDATA_HACK
#include <fstream>
#endif

using namespace std;

#define ALL(c) (c).begin(), (c).end()

//_____________________________________________________________________________
THaVDCPlane::THaVDCPlane( const char* name, const char* description,
			  THaDetectorBase* parent )
  : THaSubDetector(name,description,parent),
    fNHits(0), fNWiresHit(0), fNpass(0), fMinClustSize(0),
    fMaxClustSpan(kMaxInt), fNMaxGap(0), fMinTime(0), fMaxTime(kMaxInt),
    fMaxThits(0), fMinTdiff(0), fMaxTdiff(kBig), fTDCRes(0), fDriftVel(0),
    fT0Resolution(0), fWBeg(0), fWSpac(0), fWAngle(0), fSinWAngle(0),
    fCosWAngle(1), /*fTable(0),*/ fTTDConv(0), fglTrg(0)
{
  // Constructor

  // Since TCloneArrays can resize, the size here is fairly unimportant
  fWires    = new TClonesArray("THaVDCWire", 368 );
  fHits     = new TClonesArray("THaVDCHit", 20 );
  fClusters = new TClonesArray("THaVDCCluster", 5 );

  fVDC = dynamic_cast<THaVDC*>( GetMainDetector() );
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
Int_t THaVDCPlane::ReadDatabase( const TDatime& date )
{
  // Load VDCPlane parameters from database

  const char* const here = "ReadDatabase";

  FILE* file = OpenFile( date );
  if( !file ) return kFileError;

  // Read fCenter and fSize
  Int_t err = ReadGeometry( file, date );
  if( err ) {
    fclose(file);
    return err;
  }

  // Read configuration parameters
  vector<Int_t> detmap, bad_wirelist;
  vector<Double_t> ttd_param;
  vector<Float_t> tdc_offsets;
  TString ttd_conv = "AnalyticTTDConv";
  // Default values for optional parameters
  fTDCRes = 5.0e-10;  // 0.5 ns/chan = 5e-10 s /chan
  fT0Resolution = 6e-8; // 60 ns --- crude guess
  fMinClustSize = 3;
  fMaxClustSpan = 7;
  fNMaxGap = 1;
  fMinTime = 800;
  fMaxTime = 2200;
  fMinTdiff = 3e-8;   // 30ns  -> ~20 deg track angle
  fMaxTdiff = 2.0e-7; // 200ns -> ~67 deg track angle
  fMaxThits = 6;      // current TDC setting is to record only the last 6 hits 
  DBRequest request[] = {
    { "detmap",         &detmap,         kIntV },
    { "nwires",         &fNelem,         kInt,     0, 0, -1 },
    { "wire.start",     &fWBeg,          kDouble },
    { "wire.spacing",   &fWSpac,         kDouble,  0, 0, -1 },
    { "wire.angle",     &fWAngle,        kDouble },
    { "wire.badlist",   &bad_wirelist,   kIntV,    0, 1 },
    { "driftvel",       &fDriftVel,      kDouble,  0, 0, -1 },
    { "tdc.min",        &fMinTime,       kInt,     0, 1, -1 },
    { "tdc.max",        &fMaxTime,       kInt,     0, 1, -1 },
    { "tdc.hits"     ,  &fMaxThits,      kInt,     0, 1, -1 },
    { "tdc.res",        &fTDCRes,        kDouble,  0, 0, -1 },
    { "tdc.offsets",    &tdc_offsets,    kFloatV },
    { "ttd.converter",  &ttd_conv,       kTString, 0, 1, -1 },
    { "ttd.param",      &ttd_param,      kDoubleV, 0, 0, -1 },
    { "t0.res",         &fT0Resolution,  kDouble,  0, 1, -1 },
    { "clust.minsize",  &fMinClustSize,  kInt,     0, 1, -1 },
    { "clust.maxspan",  &fMaxClustSpan,  kInt,     0, 1, -1 },
    { "maxgap",         &fNMaxGap,       kInt,     0, 1, -1 },
    { "tdiff.min",      &fMinTdiff,      kDouble,  0, 1, -1 },
    { "tdiff.max",      &fMaxTdiff,      kDouble,  0, 1, -1 },
    { "description",    &fTitle,         kTString, 0, 1 },
    { 0 }
  };

  err = LoadDB( file, date, request, fPrefix );
  fclose(file);
  if( err )
    return err;

  if( FillDetMap(detmap, THaDetMap::kFillLogicalChannel, here) <= 0 )
    return kInitError; // Error already printed by FillDetMap

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

  if( fMinClustSize < 1 || fMinClustSize > 6 ) {
    Error( Here(here), "Invalid min_clust_size = %d, must be betwwen 1 and "
	   "6. Fix database.", fMinClustSize );
    return kInitError;
  }
  if( fMaxClustSpan < 2 || fMaxClustSpan > 12 ) {
    Error( Here(here), "Invalid max_clust_span = %d, must be betwwen 1 and "
	   "12. Fix database.", fMaxClustSpan );
    return kInitError;
  }
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

  // Derived geometry quatities
  fWAngle *= TMath::DegToRad();
  fSinWAngle = TMath::Sin( fWAngle );
  fCosWAngle = TMath::Cos( fWAngle );

  if( fVDC )
    DefineAxes( fVDC->GetVDCAngle() );
  else
    DefineAxes( 0 );

  // Searchable list of bad wire numbers, if any (usually none :-) )
  set<Int_t> bad_wires( ALL(bad_wirelist) );
  bad_wirelist.clear();

  // Create time-to-distance converter
  if( !ttd_conv.Contains("::") )
    ttd_conv.Prepend("VDC::");
  const char* s = ttd_conv.Data();
  TClass* cl = TClass::GetClass( s );
  if( !cl ) {
    Error( Here(here), "Drift time-to-distance converter \"%s\" not "
	   "available. Load library or fix database.", s?s:"" );
    return kInitError;
  }
  if( !cl->InheritsFrom( VDC::TimeToDistConv::Class() )) {
    Error( Here(here), "Class \"%s\" is not a drift time-to-distance "
	   "converter. Fix database.", s );
    return kInitError;
  }
  fTTDConv = static_cast<VDC::TimeToDistConv*>( cl->New() );
  if( !fTTDConv ) {
    Error( Here(here), "Unexpected error creating drift time-to-distance "
	   "converter object \"%s\". Call expert.", s );
    return kInitError;
  }
  // Set the converters parameters
  fTTDConv->SetDriftVel( fDriftVel );
  if( fTTDConv->SetParameters( ttd_param ) != 0 ) {
    Error( Here(here), "Error initializing drift time-to-distance converter "
	   "\"%s\". Check ttd.param in database.", s );
    return kInitError;
  }

  // Initialize wires
  for (int i = 0; i < fNelem; i++) {
    THaVDCWire* wire = new((*fWires)[i])
      THaVDCWire( i, fWBeg+i*fWSpac, tdc_offsets[i], fTTDConv );
    if( bad_wires.find(i) != bad_wires.end() )
      wire->SetFlag(1);
  }

  // finally, find the timing-offset to apply on an event-by-event basis
  //FIXME: time offset handling should go into the enclosing apparatus -
  //since not doing so leads to exactly this kind of mess:
  THaApparatus* app = GetApparatus();
  const char* nm = "trg"; // inside an apparatus, the apparatus name is assumed
  if( !app ||
      !(fglTrg = dynamic_cast<THaTriggerTime*>(app->GetDetector(nm))) ) {
    // Warning(Here(here),"Trigger-time detector \"%s\" not found. "
    //	    "Event-by-event time offsets will NOT be used!!",nm);
  }

#ifdef WITH_DEBUG
  if( fDebug > 2 ) {
    Double_t org[3]; fOrigin.GetXYZ(org);
    Double_t pos[3]; fCenter.GetXYZ(pos);
    Double_t angle = fWAngle*TMath::RadToDeg();
    DBRequest list[] = {
      { "Number of wires",         &fNelem,     kInt       },
      { "Detector origin",         org,         kDouble, 3 },
      { "Detector pos VDC coord",  pos,         kDouble, 3 },
      { "Detector size",           fSize,       kDouble, 3 },
      { "Wire angle (deg)",        &angle                  },
      { "Wire start pos (m)",      &fWBeg                  },
      { "Wire spacing (m)",        &fWSpac                 },
      { "TDC resolution (s/chan)", &fTDCRes                },
      { "Drift Velocity (m/s) ",   &fDriftVel              },
      { "Min TDC raw time",        &fMinTime,      kInt    },
      { "Max TDC raw time",        &fMaxTime,      kInt    },
      { "Min adj wire tdiff (s)",  &fMinTdiff              },
      { "Max adj wire tdiff (s)",  &fMaxTdiff              },
      { "Time-to-dist conv param", &ttd_param, kDoubleV    },
      { "Max gap in cluster",      &fNMaxGap,      kInt    },
      { "Min cluster size",        &fMinClustSize, kInt    },
      { "Max cluster span",        &fMaxClustSpan, kInt    },
      { "t0 resolution (s)",       &fT0Resolution          },
      { 0 }
    };
    DebugPrint( list );
  }
#endif

  fIsInit = true;
  return kOK;
}

//_____________________________________________________________________________
Int_t THaVDCPlane::ReadGeometry( FILE* file, const TDatime& date, Bool_t )
{
  // Custom geometry reader for VDC planes.
  //
  // Reads:
  //  "position": Plane center in VDC coordinate system (m) -> fCenter
  //              This parameter is required because we need Z
  //  "size":     full x/y/z size of active area (m) -> fSize
  //              This parameter is optional, but recommended
  //              If not given, defaults for the HRS VDCs are used

  const char* const here = "ReadGeometry";

  vector<double> position, size;
  DBRequest request[] = {
    { "position", &position, kDoubleV, 0, 0, 0, "\"position\" (detector position [m])" },
    { "size",     &size,     kDoubleV, 0, 1, 0, "\"size\" (detector size [m])" },
    { 0 }
  };
  Int_t err = LoadDB( file, date, request );
  if( err )
    return kInitError;

  assert( !position.empty() );  // else LoadDB didn't honor "required" flag
  if( position.size() != 3 ) {
    Error( Here(here), "Incorrect number of values = %u for "
	   "detector position. Must be exactly 3. Fix database.",
	   static_cast<unsigned int>(position.size()) );
    return 1;
  }
  fCenter.SetXYZ( position[0], position[1], position[2] );

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
  else {
    // Conservative defaults for HRS VDCs
    // NB: these are used by the IsInActiveArea cut in THaVDCChamber::
    // MatchUVClusters. Too small values lead to loss of acceptance.
    fSize[0] = 1.2; // half size
    fSize[1] = 0.2; // half size
    fSize[2] = 0.026;
  }

  return 0;
}

//_____________________________________________________________________________
void THaVDCPlane::UpdateGeometry( Double_t x, Double_t y, bool force )
{
  // Set the xy coordinates of this plane's center unless already set.
  // Then set fOrigin to fCenter of this plane, rotated by the VDC angle

  if( force ||
      (TMath::Abs(fCenter.X()) < 1e-6 && TMath::Abs(fCenter.Y()) < 1e-6) ) {
    fCenter.SetX(x);
    fCenter.SetY(y);
  }
  if( fVDC )
    fOrigin = fVDC->DetToTrackCoord( fCenter );
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
    { "nthit",  "tdc hits per channel",       "fHits.THaVDCHit.fNthit" },
    { "dist",   "Drift distances",            "fHits.THaVDCHit.fDist" },
    { "ddist",  "Drft dist uncertainty",      "fHits.THaVDCHit.fdDist" },
    { "trdist", "Dist. from track",           "fHits.THaVDCHit.ftrDist" },
    { "ltrdist","Dist. from local track",     "fHits.THaVDCHit.fltrDist" },
    { "trknum", "Track number (0=unused)",    "fHits.THaVDCHit.fTrkNum" },
    { "clsnum", "Cluster number (-1=unused)", "fHits.THaVDCHit.fClsNum" },
    { "nclust", "Number of clusters",         "GetNClusters()" },
    { "clsiz",  "Cluster sizes",              "fClusters.THaVDCCluster.GetSize()" },
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
    { "cltridx","Idx of track assoc w/cluster", "fClusters.THaVDCCluster.GetTrackIndex()" },
    { "cltrknum", "Cluster track number (0=unused)", "fClusters.THaVDCCluster.fTrkNum" },
    { "clbeg", "Cluster start wire",          "fClusters.THaVDCCluster.fClsBeg" },
    { "clend", "Cluster end wire",            "fClusters.THaVDCCluster.fClsEnd" },
    { "npass", "Number of hit passes for cluster", "fNpass" },
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
void THaVDCPlane::Clear( Option_t* opt )
{
  // Clears the contents of the and hits and clusters
  THaSubDetector::Clear(opt);
  fNHits = fNWiresHit = 0;
  fHits->Clear();
  fClusters->Delete();
}

//_____________________________________________________________________________
Int_t THaVDCPlane::Decode( const THaEvData& evData )
{
  // Converts the raw data into hit information
  // Logical wire numbers a defined by the detector map. Wire number 0
  // corresponds to the first defined channel, etc.

  // TODO: Support "reversed" detector map modules a la MWDC

  if (!evData.IsPhysicsTrigger()) return -1;

  // the event's T0-shift, due to the trigger-type
  // only an issue when adding in un-retimed trigger types
  Double_t evtT0=0;
  if ( fglTrg && fglTrg->Decode(evData)==kOK ) evtT0 = fglTrg->TimeOffset();

  Int_t nextHit = 0;

  bool only_fastest_hit = false, no_negative = false;
  if( fVDC ) {
    // If true, add only the first (earliest) hit for each wire
    only_fastest_hit = fVDC->TestBit(THaVDC::kOnlyFastest);
    // If true, ignore negative drift times completely
    no_negative      = fVDC->TestBit(THaVDC::kIgnoreNegDrift);
  }

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
	    new( (*fHits)[nextHit++] )  THaVDCHit( wire, data, time, nHits );
	}

    // Count all hits and wires with hits
    //    fNWiresHit++;

      } // End hit loop

      // If we are only interested in the hit with the largest TDC value
      // (shortest drift time), it is recorded here.
      if( only_fastest_hit && max_data>0 ) {
	Double_t xdata = static_cast<Double_t>(max_data) + 0.5;
	Double_t time = fTDCRes * (toff - xdata) - evtT0;
	new( (*fHits)[nextHit++] ) THaVDCHit( wire, max_data, time, nHits );
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
	  THaVDCHit* hit = static_cast<THaVDCHit*>(fHits->At(ind));
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
class TimeCut {
public:
  TimeCut( THaVDC* vdc, THaVDCPlane* _plane )
    : hard_cut(false), soft_cut(false), maxdist(0.0), plane(_plane)
  {
    assert(vdc);
    assert(plane);
    if( vdc ) {
      hard_cut = vdc->TestBit(THaVDC::kHardTDCcut);
      soft_cut = vdc->TestBit(THaVDC::kSoftTDCcut);
    }
    if( soft_cut ) {
      maxdist =
	0.5*static_cast<THaVDCChamber*>(plane->GetParent())->GetSpacing();
      if( maxdist == 0.0 )
	soft_cut = false;
    }
  }
  bool operator() ( const THaVDCHit* hit )
  {
    // Only keep hits whose drift times are within sanity cuts
    if( hard_cut ) {
      Double_t rawtime = hit->GetRawTime();
      if( rawtime < plane->GetMinTime() || rawtime > plane->GetMaxTime())
	return false;
    }
    if( soft_cut ) {
      Double_t ratio = hit->GetTime() * plane->GetDriftVel() / maxdist;
      if( ratio < -0.5 || ratio > 1.5 )
	return false;
    }
    return true;
  }
private:
  bool          hard_cut;
  bool          soft_cut;
  double        maxdist;
  THaVDCPlane*  plane;
};

//_____________________________________________________________________________
Int_t THaVDCPlane::FindClusters()
{
  // Reconstruct clusters in a VDC plane
  // Assumes that the wires are numbered such that increasing wire numbers
  // correspond to decreasing physical position.
  // Ignores possibility of overlapping clusters

  TimeCut timecut(fVDC,this);

// #ifndef NDEBUG
//   // bugcheck
//   bool only_fastest_hit = false;
//   if( fVDC )
//     only_fastest_hit = fVDC->TestBit(THaVDC::kOnlyFastest);
// #endif

  Int_t nHits     = GetNHits();   // Number of hits in the plane
  Int_t nUsed = 0;                // Number of wires used in clustering
  Int_t nLastUsed = -1;
  Int_t nextClust = 0;            // Current cluster number
  assert( GetNClusters() == 0 );

  vector <THaVDCHit *> clushits;
  Double_t deltat;
  Bool_t falling;

  fNpass = 0;

  Int_t nwires, span;
  UInt_t j;
  Int_t nskip = 0; // use this to skip bad signal (but do not want to break a possible cluster)

  //  Loop while we're making new clusters
  while( nLastUsed != nUsed ){
     fNpass++;
     nLastUsed = nUsed;
     //Loop through all TDC hits
     for( Int_t i = 0; i < nHits; ) {
       clushits.clear();
       falling = kTRUE;

       THaVDCHit* hit = GetHit(i);
       assert(hit);

       if( !timecut(hit) ) {
	       ++i;
	       continue;
       }
       if( hit->GetClsNum() != -1 )
	  { ++i; continue; }
       // Ensures we don't use this to try and start a new
       // cluster
       hit->SetClsNum(-3);

       // Consider this hit the beginning of a potential new cluster.
       // Find the end of the cluster.
       span = 0;
       nwires = 1;
       while( ++i < nHits ) {

	  THaVDCHit* nextHit = GetHit(i);
	  assert( nextHit );    // should never happen, else bug in Decode
	  if( !timecut(nextHit) )
		  continue;
	  if(    nextHit->GetClsNum() != -1   // -1 is virgin
	      && nextHit->GetClsNum() != -3 ) // -3 was considered to start
					      //a clus but is not in cluster
		  continue;

    // if the hits per wire is more than the TDC set limit, it's just noise. Skip this wire but continue the cluster searching
    if (nextHit->GetNthit() >= fMaxThits){
          nskip++;
          continue;
    }
	  Int_t ndif = nextHit->GetWireNum() - hit->GetWireNum();
	  // Do not consider adding hits from a wire that was already
	  // added
	  if( ndif == 0 ) { continue; }
	  assert( ndif >= 0 );
	  // The cluster ends when we encounter a gap in wire numbers.
	  // TODO: cluster should also end if
	  //  DONE (a) it is too big
	  //  DONE (b) drift times decrease again after initial fall/rise (V-shape)
	  //  DONE (c) Enforce reasonable changes in wire-to-wire V-shape

	  // Times are sorted by earliest first when on same wire
	  deltat = nextHit->GetTime() - hit->GetTime();

	  span += ndif;
	  if( ndif > fNMaxGap+1+nskip || span > fMaxClustSpan ){
      nskip = 0;
		  break;
	  }

	  // Make sure the time structure is sensible
	  // If this cluster is rising, wire with falling time
	  // should not be associated in the cluster
	  if( !falling ){
		  if( deltat < fMinTdiff*ndif ||
		      deltat > fMaxTdiff*ndif )
		  { continue; }
	  }

	  if( falling ){
		  // Step is too big, can't be associated
		  if( deltat < -fMaxTdiff*ndif ){ continue; }
		  if( deltat > 0.0 ){
			  // if rise is reasonable and we don't
			  // have a monotonically increasing cluster
			  if( deltat < fMaxTdiff*ndif && span > 1 ){
				  // now we're rising
				  falling = kFALSE;
			  } else {
				  continue;
			  }
		  }
	  }

	  nwires++;
	  if( clushits.size() == 0 ){
		  clushits.push_back(hit);
		  hit->SetClsNum(-2);
		  nUsed++;
	  }
	  clushits.push_back(nextHit);
	  nextHit->SetClsNum(-2);
	  nUsed++;
	  hit = nextHit;
       }
       assert( i <= nHits );
       // Make a new cluster if it is big enough
       // If not, the hits of this i-iteration are ignored
       // Also, make sure that we did indeed see the time
       // spectrum turn around at some point
       if( nwires >= fMinClustSize && !falling ) {
	  THaVDCCluster* clust =
	     new ( (*fClusters)[nextClust++] ) THaVDCCluster(this);

	  for( j = 0; j < clushits.size(); j++ ){
	     clushits[j]->SetClsNum(nextClust-1);
	     clust->AddHit( clushits[j] );
	  }

	  assert( clust->GetSize() > 0 && clust->GetSize() >= nwires );
	  // This is a good cluster candidate. Estimate its position/slope
	  clust->EstTrackParameters();
       } //end new cluster

     } //end loop over hits

  } // end passes over hits

  assert( GetNClusters() == nextClust );

  return nextClust;  // return the number of clusters found
}

//_____________________________________________________________________________
Int_t THaVDCPlane::FitTracks()
{
  // Fit tracks to cluster positions and drift distances.

  Int_t nClust = GetNClusters();
  for (int i = 0; i < nClust; i++) {
    THaVDCCluster* clust = static_cast<THaVDCCluster*>( (*fClusters)[i] );
    if( !clust ) continue;

    // Convert drift times to distances.
    // The conversion algorithm is determined at wire initialization time,
    // i.e. currently in the ReadDatabase() function of this class.
    // The conversion is done with the current value of fSlope in the
    // clusters, i.e. either the rough guess from
    // THaVDCCluster::EstTrackParameters or the global slope from
    // THaVDC::ConstructTracks
    clust->ConvertTimeToDist();

    // Fit drift distances to get intercept, slope.
    clust->FitTrack();

#ifdef CLUST_RAWDATA_HACK
    // HACK: write out cluster info for small-t0 clusters in u1
    if( fName == "u" && !strcmp(GetParent()->GetName(),"uv1") &&
	TMath::Abs(clust->GetT0()) < fT0Resolution/3. &&
	clust->GetSize() <= 6 ) {
      ofstream outp;
      outp.open("u1_cluster_data.out",ios_base::app);
      outp << clust->GetSize() << endl;
      for( int i=clust->GetSize()-1; i>=0; i-- ) {
	outp << clust->GetHit(i)->GetPos() << " "
	     << clust->GetHit(i)->GetDist()
	     << endl;
      }
      outp << 1./clust->GetSlope() << " "
	   << clust->GetIntercept()
	   << endl;
      outp.close();
    }
#endif
  }

  return 0;
}

//_____________________________________________________________________________
Bool_t THaVDCPlane::IsInActiveArea( Double_t x, Double_t y ) const
{
  // Check if given (x,y) coordinates are inside this plane's active area
  // (defined by fCenter and fSize).
  // x and y must be given in the VDC coordinate system.

  return ( TMath::Abs(x-fCenter.X()) <= fSize[0] &&
	   TMath::Abs(y-fCenter.Y()) <= fSize[1] );
}

///////////////////////////////////////////////////////////////////////////////
ClassImp(THaVDCPlane)
