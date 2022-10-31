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
#include "Helper.h"

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

//_____________________________________________________________________________
THaVDCPlane::THaVDCPlane( const char* name, const char* description,
                          THaDetectorBase* parent ) :
  THaSubDetector(name, description, parent),
  // Since TCloneArrays can resize, the size here is fairly unimportant
  fWires{new TClonesArray("THaVDCWire", 368)},
  fHits{new TClonesArray("THaVDCHit", 20)},
  fClusters{new TClonesArray("THaVDCCluster", 5)},
  fNHits(0), fNWiresHit(0), fNpass(0), fMinClustSize(0),
  fMaxClustSpan(kMaxInt), fNMaxGap(0), fMinTime(0), fMaxTime(kMaxInt),
  fMaxThits(0), fMinTdiff(0), fMaxTdiff(kBig), fTDCRes(0), fDriftVel(0),
  fT0Resolution(0), fOnlyFastestHit(false), fNoNegativeTime(false),
  fWBeg(0), fWSpac(0), fWAngle(0), fSinWAngle(0),
  fCosWAngle(1), /*fTable(0),*/ fTTDConv(nullptr),
  fVDC{dynamic_cast<THaVDC*>( GetMainDetector() )},
  fMaxData(kMaxUInt), fNextHit(0), fPrevWire(nullptr)
{
  // Constructor
}

//_____________________________________________________________________________
THaVDCPlane::~THaVDCPlane()
{
  // Destructor.

  RemoveVariables();
  delete fWires;
  delete fHits;
  delete fClusters;
  delete fTTDConv;
//   delete [] fTable;
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

  FILE* file = OpenFile(date);
  if( !file ) return kFileError;

  // Read fCenter and fSize
  Int_t err = ReadGeometry(file, date);
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
    { "nwires",         &fNelem,         kInt,     0, false, -1 },
    { "wire.start",     &fWBeg,          kDouble },
    { "wire.spacing",   &fWSpac,         kDouble,  0, false, -1 },
    { "wire.angle",     &fWAngle,        kDouble },
    { "wire.badlist",   &bad_wirelist,   kIntV,    0, true },
    { "driftvel",       &fDriftVel,      kDouble,  0, false, -1 },
    { "tdc.min",        &fMinTime,       kInt,     0, true, -1 },
    { "tdc.max",        &fMaxTime,       kInt,     0, true, -1 },
    { "tdc.hits"     ,  &fMaxThits,      kUInt,    0, true, -1 },
    { "tdc.res",        &fTDCRes,        kDouble,  0, false, -1 },
    { "tdc.offsets",    &tdc_offsets,    kFloatV },
    { "ttd.converter",  &ttd_conv,       kTString, 0, true, -1 },
    { "ttd.param",      &ttd_param,      kDoubleV, 0, false, -1 },
    { "t0.res",         &fT0Resolution,  kDouble,  0, true, -1 },
    { "clust.minsize",  &fMinClustSize,  kInt,     0, true, -1 },
    { "clust.maxspan",  &fMaxClustSpan,  kInt,     0, true, -1 },
    { "maxgap",         &fNMaxGap,       kInt,     0, true, -1 },
    { "tdiff.min",      &fMinTdiff,      kDouble,  0, true, -1 },
    { "tdiff.max",      &fMaxTdiff,      kDouble,  0, true, -1 },
    { "description",    &fTitle,         kTString, 0, true },
    { nullptr }
  };

  err = LoadDB(file, date, request, fPrefix);
  fclose(file);
  if( err )
    return err;

  if( FillDetMap(detmap, THaDetMap::kFillLogicalChannel, here) <= 0 )
    return kInitError; // Error already printed by FillDetMap

  // All our frontend modules are common stop TDCs
  UInt_t nmodules = fDetMap->GetSize();
  for( UInt_t i = 0; i < nmodules; i++ ) {
    THaDetMap::Module* d = fDetMap->GetModule(i);
    d->MakeTDC();
    d->SetTDCMode(false);
  }

  err = ReadDatabaseErrcheck(tdc_offsets, here);
  if( err )
    return err;

  // Derived geometry quantities
  fWAngle *= TMath::DegToRad();
  fSinWAngle = TMath::Sin(fWAngle);
  fCosWAngle = TMath::Cos(fWAngle);

  if( fVDC ) {
    // The parent VDC should be initialized at this point
    assert(fVDC->Status() == kOK);

    DefineAxes(fVDC->GetVDCAngle());

    // If true, add only the first (earliest) hit for each wire
    fOnlyFastestHit = fVDC->TestBit(THaVDC::kOnlyFastest);
    // If true, ignore negative drift times completely
    fNoNegativeTime = fVDC->TestBit(THaVDC::kIgnoreNegDrift);
  } else
    DefineAxes(0);

  // Searchable list of bad wire numbers, if any (usually none :-) )
  set<Int_t> bad_wires(ALL(bad_wirelist));
  bad_wirelist.clear();
  bad_wirelist.shrink_to_fit();

  // Create time-to-distance converter
  if( !ttd_conv.Contains("::") )
    ttd_conv.Prepend("VDC::");
  err = CreateTTDConv(ttd_conv, ttd_param, here);
  if( err )
    return err;

  // Initialize wires
  for( int i = 0; i < fNelem; i++ ) {
    auto* wire = new((*fWires)[i])
      THaVDCWire(i, fWBeg + i * fWSpac, tdc_offsets[i], fTTDConv);
    if( bad_wires.find(i) != bad_wires.end() )
      wire->SetFlag(1);
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
      { nullptr }
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

  vector<Double_t> position, size;
  DBRequest request[] = {
    { "position", &position, kDoubleV, 0, false, 0, "\"position\" (detector position [m])" },
    { "size",     &size,     kDoubleV, 0, true, 0, "\"size\" (detector size [m])" },
    { nullptr }
  };
  Int_t err = LoadDB( file, date, request );
  if( err )
    return kInitError;

  assert( !position.empty() );  // else LoadDB didn't honor "required" flag
  err = ReadGeometryErrcheck(position, size, here);
  if( err )
    return err;

  fCenter.SetXYZ( position[0], position[1], position[2] );

  if( !size.empty() ) {
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
Int_t THaVDCPlane::ReadDatabaseErrcheck( const vector<Float_t>& tdc_offsets,
                                         const char* here ) {
  // Sanity checks
  if( fNelem <= 0 ) {
    Error(Here(here), "Invalid number of wires: %d", fNelem);
    return kInitError;
  }
  UInt_t nwires = fNelem;
  UInt_t nchan = fDetMap->GetTotNumChan();
  if( nchan != nwires ) {
    Error(Here(here),
          "Number of detector map channels (%u) disagrees with "
          "number of wires (%u)", nchan, nwires);
    return kInitError;
  }
  nchan = tdc_offsets.size();
  if( nchan != nwires ) {
    Error(Here(here),
          "Number of TDC offset values (%u) disagrees with "
          "number of wires (%u)", nchan, nwires);
    return kInitError;
  }

  if( fMinClustSize < 1 || fMinClustSize > 6 ) {
    Error(Here(here),
          "Invalid min_clust_size = %d, must be between 1 and 6. "
          "Fix database.", fMinClustSize);
    return kInitError;
  }
  if( fMaxClustSpan < 2 || fMaxClustSpan > 12 ) {
    Error(Here(here),
          "Invalid max_clust_span = %d, must be between 1 and 12. "
          "Fix database.", fMaxClustSpan);
    return kInitError;
  }
  if( fNMaxGap < 0 || fNMaxGap > 2 ) {
    Error(Here(here),
          "Invalid max_gap = %d, must be between 0 and 2. Fix database.",
          fNMaxGap);
    return kInitError;
  }
  if( fMinTime < 0 || fMinTime > 4095 ) {
    Error(Here(here),
          "Invalid min_time = %d, must be between 0 and 4095. Fix database.",
          fMinTime);
    return kInitError;
  }
  if( fMaxTime < 1 || fMaxTime > 4096 || fMinTime >= fMaxTime ) {
    Error(Here(here),
          "Invalid max_time = %d. Must be between 1 and 4096 "
          "and >= min_time = %d. Fix database.", fMaxTime, fMinTime);
    return kInitError;
  }
  return kOK;
}

//_____________________________________________________________________________
Int_t THaVDCPlane::ReadGeometryErrcheck( const vector<Double_t>& position,
                                         const vector<Double_t>& size,
                                         const char* const here )
{
  if( position.size() != 3 ) {
    Error( Here(here), "Incorrect number of values = %u for "
                       "detector position. Must be exactly 3. Fix database.",
           static_cast<unsigned int>(position.size()) );
    return kInitError;
  }
  if( !size.empty() ) {
    if( size.size() != 3 ) {
      Error(Here(here), "Incorrect number of values = %u for "
                        "detector size. Must be exactly 3. Fix database.",
            static_cast<unsigned int>(size.size()));
      return kInitError;
    }
    if( size[0] == 0 || size[1] == 0 || size[2] == 0 ) {
      Error(Here(here), "Illegal zero detector dimension. Fix database.");
      return kInitError;
    }
    if( size[0] < 0 || size[1] < 0 || size[2] < 0 ) {
      Warning(Here(here), "Illegal negative value for detector dimension. "
                          "Taking absolute. Check database.");
    }
  }
  return kOK;
}

//_____________________________________________________________________________
Int_t THaVDCPlane::CreateTTDConv( const char* classname,
                                  const vector<Double_t>& ttd_param,
                                  const char* here )
{
  TClass* cl = TClass::GetClass(classname);
  if( !cl ) {
    Error(Here(here),
          "Drift time-to-distance converter \"%s\" not available. "
          "Load library or fix database.", classname ? classname : "");
    return kInitError;
  }
  if( !cl->InheritsFrom(VDC::TimeToDistConv::Class()) ) {
    Error(Here(here),
          "Class \"%s\" is not a drift time-to-distance "
          "converter. Fix database.", classname);
    return kInitError;
  }
  fTTDConv = static_cast<VDC::TimeToDistConv*>( cl->New() );
  if( !fTTDConv ) {
    Error(Here(here),
          "Unexpected error creating drift time-to-distance converter "
          "object \"%s\". Call expert.", classname);
    return kInitError;
  }
  // Set the converters parameters
  fTTDConv->SetDriftVel(fDriftVel);
  if( fTTDConv->SetParameters(ttd_param) != 0 ) {
    Error(Here(here),
          "Error initializing drift time-to-distance converter "
          "\"%s\". Check ttd.param in database.", classname);
    return kInitError;
  }
  return kOK;
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
    { nullptr }
  };
  return DefineVarsFromList( vars, mode );

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
Int_t THaVDCPlane::StoreHit( const DigitizerHitInfo_t& hitinfo, UInt_t data )
{

  assert( hitinfo.nhit > 0 );

  Int_t wireNum = hitinfo.lchan;
  auto* wire = GetWire(wireNum);
  if( !wire || wire->GetFlag() != 0 )
    // Wire is disabled (maybe noisy)
    return -1;

  // Count hits
  ++fNHits;
  if( wire != fPrevWire ) {
    fMaxData = kMaxUInt;
    ++fNWiresHit;
  }
  // Bugcheck: identical wires -> multihit
  assert(wire != fPrevWire || hitinfo.nhit > 1);
  fPrevWire = wire;

  // Keep maximum data value seen for this series of hits
  if( data > fMaxData || fMaxData == kMaxUInt )
    fMaxData = data;

  // Convert the TDC value to the drift time.
  // Being perfectionist, we apply a 1/2 channel correction to the raw
  // TDC data to compensate for the fact that the TDC truncates, not
  // rounds, the data.
  Double_t toff  = wire->GetTOffset();
  Double_t xdata = static_cast<Double_t>(data) + 0.5;
  Double_t time  = fTDCRes * (toff - xdata);

  // If requested, ignore hits with negative drift times
  // (due to noise or miscalibration). Use with care.
  if( fNoNegativeTime && time <= 0.0 )
    return -2;

  // If there are multiple hits on this channel and only the fastest hit is
  // requested, find maximum TDC value and record the corresponding hit at
  // the end of the hit loop.
  if( hitinfo.nhit > 1 && fOnlyFastestHit ) {
    if( hitinfo.hit + 1 < hitinfo.nhit || fMaxData == kMaxUInt )
      // Keep going. More hits to come (or nothing valid found)
      return 0;

    // Got all the hits on this channel. Record the max TDC value.
    if( data != fMaxData ) {
      assert(data < fMaxData);
      data = fMaxData;
      xdata = static_cast<Double_t>(data) + 0.5;
      time = fTDCRes * (toff - xdata);
    }
  }

  // Record hit on this wire
  new((*fHits)[fNextHit++])  THaVDCHit(wire, data, time, hitinfo.nhit);

  return 0;
}

//_____________________________________________________________________________
Int_t THaVDCPlane::Decode( const THaEvData& evData )
{
  // Converts the raw data into hit information
  // Logical wire numbers a defined by the detector map. Wire number 0
  // corresponds to the first defined channel, etc.

  if (!evData.IsPhysicsTrigger()) return -1;

  const char* const here = "Decode";
  bool has_warning = false;

  fNextHit = 0;
  fMaxData = -1;
  fPrevWire = nullptr;

  auto hitIter = fDetMap->MakeMultiHitIterator(evData);
  while( hitIter ) {
    const auto& hitinfo = *hitIter;

    // Get the TDC data for this hit
    OptUInt_t data = LoadData(evData, hitinfo);
    if( !data ) {
      // Data could not be retrieved (probably decoder bug)
      DataLoadWarning(hitinfo, here);
      has_warning = true;
      continue;
    }

    // Store hit data in fHits
    StoreHit(hitinfo, data.value());

    // Next active hit or channel
    ++hitIter;
  }

  // Sort the hits in order of increasing wire number and (for the same wire
  // number) increasing time (NOT rawtime)
  fHits->Sort();

  if( has_warning )
    ++fNEventsWithWarnings;

#ifdef WITH_DEBUG
  if ( fDebug > 3 )
    PrintDecodedData(evData);
#endif

  return GetNHits();
}

//_____________________________________________________________________________
void THaVDCPlane::PrintDecodedData( const THaEvData& /*evdata*/ ) const
{
#ifdef WITH_DEBUG
  if( fDebug > 3 ) {
    Int_t nhits = GetNHits();
    cout << endl
         << "VDC plane " << GetPrefixName() << ": " << nhits << " hits" << endl;
    const int ncol = 6;
    for( int i = 0; i < TMath::Min(ncol, nhits); i++ )
      cout << "  Wire  TDC ";
    cout << endl;

    for( int i = 0; i < (nhits+ncol-1)/ncol; i++ ) {
      for( int c = ncol*i; c < ncol*(i+1); c++ ) {
        if( c < nhits ) {
          auto* hit = dynamic_cast<THaVDCHit*>(fHits->At(c));
          if( hit )
            cout << right
                 << "  " << setw(3) << hit->GetWireNum()
                 << "  " << setw(5) << hit->GetRawTime()
                 << left;
          else {
            assert(false); // else bug in Decode()
            cout << right << setw(12) << "<null>" << left;
          }
        } else {
          break;
        }
      }
      cout << endl;
    }
  }
#endif
}

//_____________________________________________________________________________
Int_t THaVDCPlane::ApplyTimeCorrection()
{
  // Correct drift times of all hits by GetTimeCorrection() from the VDC
  // parent detector

  if( fVDC ) {
    auto r = fVDC->GetTimeCorrection();
    if( r.second ) {
      Double_t evtT0 = r.first;
      Int_t nHits = GetNHits();
      for( Int_t i = 0; i < nHits; ++i ) {
        auto* hit = GetHit(i);
        hit->SetTime(hit->GetTime() - evtT0);
      }
    }
  }
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
      const auto* chamber = dynamic_cast<THaVDCChamber*>(plane->GetParent());
      if( chamber )
        maxdist = 0.5 * chamber->GetSpacing();
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

  TimeCut timecut(fVDC, this);

  Int_t nHits = GetNHits();   // Number of hits in the plane
  Int_t nUsed = 0;                // Number of wires used in clustering
  Int_t nLastUsed = -1;
  Int_t nextClust = 0;            // Current cluster number
  assert(GetNClusters() == 0);

  vector<THaVDCHit*> clushits;
  clushits.reserve(nHits);

  fNpass = 0;

  //  Loop while we're making new clusters
  while( nLastUsed != nUsed ) {
    fNpass++;
    nLastUsed = nUsed;
    //Loop through all TDC hits
    for( Int_t i = 0; i < nHits; ) {
      clushits.clear();
      Bool_t falling = true;

      THaVDCHit* hit = GetHit(i);
      assert(hit);

      if( !timecut(hit) ) {
        ++i;
        continue;
      }
      if( hit->GetClsNum() != -1 ) {
        ++i;
        continue;
      }
      // Ensures we don't use this to try and start a new
      // cluster
      hit->SetClsNum(-3);

      // use this to skip bad signal (but do not want to break a possible cluster)
      Int_t nskip = 0;

      // Consider this hit the beginning of a potential new cluster.
      // Find the end of the cluster.
      Int_t span = 0;
      Int_t nwires = 1;
      while( ++i < nHits ) {

        THaVDCHit* nextHit = GetHit(i);
        assert(nextHit);    // should never happen, else bug in Decode
        if( !timecut(nextHit))
          continue;
        if( nextHit->GetClsNum() != -1  && // -1 is virgin
            nextHit->GetClsNum() != -3 )   // -3 was considered to start a cluster but is not in cluster
          continue;

        // if the hits per wire is more than the TDC set limit, it's just noise. Skip this wire but
        // continue the cluster searching
        if( nextHit->GetNthit() >= fMaxThits ) {
          nskip++;
          continue;
        }
        Int_t ndif = nextHit->GetWireNum() - hit->GetWireNum();
        // Do not consider adding hits from a wire that was already
        // added
        if( ndif == 0 )
          continue;
        assert(ndif >= 0);
        // The cluster ends when we encounter a gap in wire numbers.
        // TODO: cluster should also end if
        //  DONE (a) it is too big
        //  DONE (b) drift times decrease again after initial fall/rise (V-shape)
        //  DONE (c) Enforce reasonable changes in wire-to-wire V-shape

        // Times are sorted by earliest first when on same wire
        Double_t deltat = nextHit->GetTime() - hit->GetTime();

        span += ndif;
        if( ndif > fNMaxGap + 1 + nskip || span > fMaxClustSpan )
          break;

        // Make sure the time structure is sensible
        // If this cluster is rising, wire with falling time
        // should not be associated in the cluster
        if( !falling ) {
          if( deltat < fMinTdiff * ndif ||
              deltat > fMaxTdiff * ndif )
            continue;
        }

        if( falling ) {
          // Step is too big, can't be associated
          if( deltat < -fMaxTdiff * ndif )
            continue;
          if( deltat > 0.0 ) {
            // if rise is reasonable and we don't
            // have a monotonically increasing cluster
            if( deltat < fMaxTdiff * ndif && span > 1 ) {
              // now we're rising
              falling = false;
            } else {
              continue;
            }
          }
        }

        nwires++;
        if( clushits.empty() ) {
          clushits.push_back(hit);
          hit->SetClsNum(-2);
          nUsed++;
        }
        clushits.push_back(nextHit);
        nextHit->SetClsNum(-2);
        nUsed++;
        hit = nextHit;
      }
      assert(i <= nHits);
      // Make a new cluster if it is big enough
      // If not, the hits of this i-iteration are ignored
      // Also, make sure that we did indeed see the time
      // spectrum turn around at some point
      if( nwires >= fMinClustSize && !falling ) {
        auto* clust = new((*fClusters)[nextClust++]) THaVDCCluster(this);
        for( auto* clushit : clushits ) {
          clushit->SetClsNum(nextClust - 1);
          clust->AddHit(clushit);
        }

        assert(clust->GetSize() > 0 && clust->GetSize() >= nwires);
        // This is a good cluster candidate. Estimate its position/slope
        clust->EstTrackParameters();
      } //end new cluster

    } //end loop over hits

  } // end passes over hits

  assert(GetNClusters() == nextClust);

  return nextClust;  // return the number of clusters found
}

//_____________________________________________________________________________
Int_t THaVDCPlane::FitTracks()
{
  // Fit tracks to cluster positions and drift distances.

  Int_t nClust = GetNClusters();
  for (int i = 0; i < nClust; i++) {
    auto* clust = static_cast<THaVDCCluster*>( (*fClusters)[i] );
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
