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


//_____________________________________________________________________________
THaVDCPlane::THaVDCPlane( const char* name, const char* description,
			  THaDetectorBase* parent )
  : THaSubDetector(name,description,parent), fTable(NULL), fTTDConv(NULL),
    fVDC(NULL), fglTrg(NULL)
{
  // Constructor

  // Since TCloneArrays can resize, the size here is fairly unimportant
  fHits     = new TClonesArray("THaVDCHit", 20 );
  fClusters = new TClonesArray("THaVDCCluster", 5 );
  fWires    = new TClonesArray("THaVDCWire", 368 );

  THaDetectorBase *det = GetDetector();
  if( det )
    fVDC = static_cast<THaSubDetector*>(det)->GetDetector();
}

//_____________________________________________________________________________
void THaVDCPlane::MakePrefix()
{
  // Special treatment of the prefix for VDC planes: We don't want variable
  // names such as "R.vdc.uv1.u.x", but rather "R.vdc.u1.x".

  TString basename;
  THaDetectorBase
    *parent = NULL,
    *det = GetDetector();

  if( det && det->IsA()->InheritsFrom("THaSubDetector") )
    parent = static_cast<THaSubDetector*>(det)->GetDetector();
  if( parent )
    basename = parent->GetPrefix();
  if( fName.Contains("u") )
    basename.Append("u");
  else if ( fName.Contains("v") )
    basename.Append("v");
  if( det && strstr( det->GetName(), "uv1" ))
    basename.Append("1.");
  else if( det && strstr( det->GetName(), "uv2" ))
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
  // Allocate TClonesArray objects and load plane parameters from database

  FILE* file = OpenFile( date );
  if( !file ) return kFileError;

  // Use default values until ready to read from database
  
  static const char* const here = "ReadDatabase";
  const int LEN = 100;
  char buff[LEN];
  
  // Build the search tag and find it in the file. Search tags
  // are of form [ <prefix> ], e.g. [ R.vdc.u1 ].
  TString tag(fPrefix); tag.Prepend("["); tag.Append("]"); 
  tag.Replace( tag.Length()-2, 1, "" );  // delete trailing dot of prefix
  TString line, tag2(tag);
  tag.ToLower();
  bool found = false;
  while (!found && fgets (buff, LEN, file) != NULL) {
    char* buf = ::Compress(buff);  //strip blanks
    line = buf;
    delete [] buf;
    if( line.EndsWith("\n") ) line.Chop();  //delete trailing newline
    line.ToLower();
    if ( tag == line ) 
      found = true;
  }
  if( !found ) {
    Error(Here(here), "Database entry \"%s\" not found!", tag2.Data() );
    fclose(file);
    return kInitError;
  }
  
  //Found the entry for this plane, so read data
  Int_t nWires = 0;    // Number of wires to create
  Int_t prev_first = 0, prev_nwires = 0;
  // Set up the detector map
  fDetMap->Clear();
  for(int i=0; i<4; i++) {
    Int_t crate, slot, lo, hi;
    // Get crate, slot, low channel and high channel from file
    fgets( buff, LEN, file );
    if( sscanf( buff, "%d%d%d%d", &crate, &slot, &lo, &hi ) != 4 ) {
      if( *buff ) buff[strlen(buff)-1] = 0; //delete trailing newline
      Error( Here(here), "Error reading detector map line: %s", buff );
      fclose(file);
      return kInitError;
    }
    Int_t first = prev_first + prev_nwires;
    // Add module to the detector map
    fDetMap->AddModule(crate, slot, lo, hi, first);
    prev_first = first;
    prev_nwires = (hi - lo + 1);
    nWires += prev_nwires;
  }
  // Load z, wire beginning postion, wire spacing, and wire angle
  fscanf (file, "%lf%lf%lf%lf", &fZ, &fWBeg, &fWSpac, &fWAngle);
  fgets(buff, LEN, file); // Read to end of line
  fWAngle *= TMath::Pi()/180.0; // Convert to radians
  // FIXME: Read from file
  fTDCRes = 5.0e-10;  // 0.5 ns/chan = 5e-10 s /chan

  // Load drift velocity (will be used to initialize Crude Time to Distance
  // converter)
  fscanf(file, "%lf", &fDriftVel);
  fgets(buff, LEN, file); // Read to end of line
  fgets(buff, LEN, file); // Skip line

  fNWiresHit = 0;
  
  // Values are the same for each plane
  fNMaxGap = 1;
  fMinTime = 800;
  fMaxTime = 2200;

  // first read in the time offsets for the wires
  float* wire_offsets = new float[nWires];
  int*   wire_nums    = new int[nWires];

  for (int i = 0; i < nWires; i++) {
    int wnum = 0;
    float offset = 0.0;
    fscanf(file, " %d %f", &wnum, &offset);
    wire_nums[i] = wnum-1; // Wire numbers in file start at 1
    wire_offsets[i] = offset;
  }

  // now read in the time-to-drift-distance lookup table
  // data (if it exists)
  fgets(buff, LEN, file); // read to the end of line
  fgets(buff, LEN, file);
  if(strncmp(buff, "TTD Lookup Table", 16) == 0) {
    // if it exists, read the data in
    fscanf(file, "%le", &fT0);
    fscanf(file, "%d", &fNumBins);
    
    // this object is responsible for the memory management 
    // of the lookup table
    delete [] fTable;
    fTable = new Float_t[fNumBins];
    for(int i=0; i<fNumBins; i++) {
      fscanf(file, "%e", &(fTable[i]));
    }
  } else {
    // if not, set some reasonable defaults and rewind the file
    fT0 = 0.0;
    fNumBins = 0;
    fTable = NULL;
    cout<<"Could not find lookup table header: "<<buff<<endl;
    fseek(file, -strlen(buff), SEEK_CUR);
  }

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
  delete [] wire_offsets;
  delete [] wire_nums;

  fOrigin.SetXYZ( 0.0, 0.0, fZ );

  THaDetectorBase *sdet = GetDetector();
  if( sdet )
    fOrigin += sdet->GetOrigin();

  fIsInit = true;
  fclose(file);

  // finally, find the timing-offset to apply on an event-by-event basis
  // How do I find my way to the parent apparatus?
  while ( sdet && dynamic_cast<THaSubDetector*>(sdet) ) 
    sdet = static_cast<THaSubDetector*>(sdet)->GetDetector();
  // so, sdet should be a 'THADetector' now, and we can find the apparatus
  THaApparatus* app = (sdet ? static_cast<THaDetector*>(sdet)->GetApparatus() : 0);
  if (!app) Error(Here(here),"Subdet->Det->App chain is incorrect!");

  TString nm = "trg";  // inside an apparatus, the apparatus name is assumed
  fglTrg = dynamic_cast<THaTriggerTime*>(app->GetDetector(nm.Data()));
  if (!fglTrg) Warning(Here(here),"Expected %s to be prepared before VDCs. Event-dependent time offsets NOT used!!",nm.Data());
  
  return kOK;
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
  delete [] fTable;
}

//_____________________________________________________________________________
inline
void THaVDCPlane::Clear( Option_t* opt )
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
              
  Clear();  //Clear the last event

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

      // Wire numbers and channels go in the same order ... 
      Int_t wireNum  = d->first + chan - d->lo;
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
	  THaVDCHit* hit = static_cast<THaVDCHit*>((*fHits)[ind]);
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

  //FIXME: Requires ROOT 3.02. Below is a non-equivalent workaround.
  //  bool hard_cut = fVDC->TestBits(THaVDC::kTDCbits) == kHardTDCcut;
  //  bool soft_cut = fVDC->TestBits(kTDCbits) == kSoftTDCcut;
  bool hard_cut, soft_cut;
  if( fVDC ) {
    hard_cut = fVDC->TestBit(THaVDC::kHardTDCcut);
    soft_cut = fVDC->TestBit(THaVDC::kSoftTDCcut);
  } else
    hard_cut = soft_cut = false;
  Double_t maxdist = 0.0;
  if( soft_cut ) {
    maxdist = static_cast<THaVDCUVPlane*>(GetDetector())->GetSpacing() / 2.0;
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
      printf("THaVDCPlane::FindCluster()-Wire Ordering Error\n");
      return 0;
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

///////////////////////////////////////////////////////////////////////////////
ClassImp(THaVDCPlane)
