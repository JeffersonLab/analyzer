///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaVDCPlane                                                               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Here:                                                                     //
//        Units of measurements:                                             //
//        For cluster position (center) and size  -  wires;                  //
//        For X, Y, and Z coordinates of track    -  centimeters;            //
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
#include "THaVDCLookupTTDConv.h"
#include "TString.h"
#include "TClass.h"
#include "TMath.h"
#include "VarDef.h"

#include <cstring>
#include <vector>
#include <stdio.h>

ClassImp(THaVDCPlane)


//_____________________________________________________________________________
THaVDCPlane::THaVDCPlane( const char* name, const char* description,
			  THaDetectorBase* parent )
  : THaSubDetector(name,description,parent), fVDC(NULL)
{
  // Constructor

  // Since TCloneArrays can resize, the size here is fairly unimportant
  fHits     = new TClonesArray("THaVDCHit", 20 );
  fClusters = new TClonesArray("THaVDCCluster", 5 );

  if( fDetector )
    fVDC = static_cast<THaSubDetector*>(fDetector)->GetDetector();
}

//_____________________________________________________________________________
void THaVDCPlane::MakePrefix()
{
  // Special treatment of the prefix for VDC planes: We don't want variable
  // names such as "R.vdc.uv1.u.x", but rather "R.vdc.u1.x".

  TString basename;
  THaDetectorBase* parent = NULL;
  if( fDetector && fDetector->IsA()->InheritsFrom("THaSubDetector") )
    parent = static_cast<THaSubDetector*>(fDetector)->GetDetector();
  if( parent )
    basename = parent->GetPrefix();
  if( fName.Contains("u") )
    basename.Append("u");
  else if ( fName.Contains("v") )
    basename.Append("v");
  if( fDetector && strstr( fDetector->GetName(), "uv1" ))
    basename.Append("1.");
  else if( fDetector && strstr( fDetector->GetName(), "uv2" ))
    basename.Append("2.");
  if( basename.Length() == 0 )
    basename = fName + ".";

  delete [] fPrefix;
  fPrefix = new char[ basename.Length()+1 ];
  strcpy( fPrefix, basename.Data() );
}

//_____________________________________________________________________________
Int_t THaVDCPlane::ReadDatabase( FILE* file, const TDatime& date )
{
  // Allocate TClonesArray objects and load plane parameters from database
  
  // Use default values until ready to read from database
  
  static const char* const here = "ReadDatabase()";
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
    if( strlen(buf) > 0 && buf[ strlen(buf)-1 ] == '\n' )
      buf[ strlen(buf)-1 ] = 0;    //delete trailing newline
    line = buf; line.ToLower();
    if ( tag == line ) 
      found = true;
    delete [] buf;
  }
  if( !found ) {
    Error(Here(here), "Database entry \"%s\" not found!", tag2.Data() );
    return kInitError;
  }
  
  //Found the entry for this plane, so read data
  Int_t nWires = 0;    // Number of wires to create
  Int_t crate, slot, lo, hi;
  // Set up the detector map
  for (int i = 0; i < 4; i++) {
    // Get crate, slot, low channel and high channel from file
    fscanf(file, "%d%d%d%d", &crate, &slot, &lo, &hi);
    // Add module to the detector map
    fDetMap->AddModule(crate, slot, lo, hi);
    nWires += (hi - lo + 1); //More wires
    fgets(buff, LEN, file);  //Read to end of line
  }
  // Load z, wire beginning postion, wire spacing, and wire angle
  const Double_t degrad = TMath::Pi()/180.0;
  float z, wbeg, wspac, wangle;
  fscanf (file, "%f%f%f%f", &z, &wbeg, &wspac, &wangle);
  fgets(buff, LEN, file);  //Read to end of line
  fZ = z;
  fWBeg = wbeg;     
  fWSpac = wspac;
  fWAngle = wangle * degrad; // Convert to radians
  // FIXME: Read from file
  fTDCRes = 5.0e-10;  // 0.5 ns/chan = 5e-10 s /chan

  // Load drift velocity (will be used to initialize Crude Time to Distance
  // converter)
  float driftVel;
  fscanf(file, "%f", &driftVel);
  fgets(buff, LEN, file); // Read to end of line
  fgets(buff, LEN, file); // Skip line

  fDriftVel = driftVel;

  fNWiresHit = 0;
  
  // Values are the same for each plane
  fNMaxGap = 1;
  fMinTime = 800;
  fMaxTime = 2200;

  // Create TClonesArray objects for wires, hits,  and clusters
  fWires = new TClonesArray("THaVDCWire", nWires);

  // first read in the time offsets for the wires
  float wire_offsets[nWires];

  for (int i = 0; i < nWires; i++) {
    float offset = 0.0;
    fscanf(file, " %*d %f", &offset);
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
  THaVDCAnalyticTTDConv* ttdConv = new THaVDCAnalyticTTDConv(driftVel);

  //THaVDCLookupTTDConv* ttdConv = new THaVDCLookupTTDConv(fT0, fNumBins, fTable);

  // Now initialize wires (those wires... too lazy to initialize themselves!)
  // Caution: This may not correspond at all to actual wire channels!
  for (int i = 0; i < nWires; i++)
    new((*fWires)[i]) THaVDCWire( i, fWBeg+i*fWSpac, wire_offsets[i], 
				  ttdConv );

  /*
  for (int i = 0; i < nWires; i++) {

    float offset = 0.0;
    fscanf(file, " %*d %f", &offset);

    // Construct the new THaVDCWire (using space in the TClonesArray obj)
    new((*fWires)[i]) THaVDCWire( i, fWBeg+i*fWSpac, offset, ttdConv );
  }
  */

  // Read in TTD Lookup Table
  
  fOrigin.SetXYZ( 0.0, 0.0, fZ );
  if( fDetector )
    fOrigin += fDetector->GetOrigin();

  fIsInit = true;
  return kOK;
}

//_____________________________________________________________________________
Int_t THaVDCPlane::SetupDetector( const TDatime& date )
{
  // initialize global variables

  if( fIsSetup ) return kOK;
  fIsSetup = true;

  // Register variables in global list

  RVarDef vars[] = {
    { "nhit",   "Number of hits",             "GetNHits()" },
    { "wire",   "Active wire numbers",        "fHits.THaVDCHit.GetWireNum()" },
    { "rawtime","Raw TDC values of wires",    "fHits.THaVDCHit.fRawTime" },
    { "time",   "TDC values of active wires", "fHits.THaVDCHit.fTime" },
    { "dist",   "Drift distances",            "fHits.THaVDCHit.fDist" },
    { "nclust", "Number of clusters",         "GetNClusters()" },
    { "clsiz",  "Cluster sizes",              "fClusters.THaVDCCluster.fSize" },
    { "clpivot","Cluster pivot wire num",     "fClusters.THaVDCCluster.GetPivotWireNum()" },
    { "clpos",  "Cluster intercepts",         "fClusters.THaVDCCluster.fInt" },
    { "slope",  "Cluster slopes",             "fClusters.THaVDCCluster.fSlope" },
    { "sigsl",  "Cluster slope sigmas",       "fClusters.THaVDCCluster.fSigmaSlope" },
    { "sigpos", "Cluster position sigmas",    "fClusters.THaVDCCluster.fSigmaInt" },
    { 0 }
  };
  DefineVariables( vars );

  return kOK;
}

//_____________________________________________________________________________
THaVDCPlane::~THaVDCPlane()
{
  // Destructor.

  if( fIsSetup )
    RemoveVariables();
  fWires->Delete();
  fHits->Clear();
  fClusters->Delete();
  delete fWires;
  delete fHits;
  delete fClusters;
}

//_____________________________________________________________________________
void THaVDCPlane::Clear( Option_t* opt )
{    
  // Clears the contents of the and hits and clusters
  fNWiresHit = 0;
  fHits->Clear();
  fClusters->Delete();
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
  
  Int_t nextHit = 0;
  Int_t wireOffset = 0;

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
      if (chan < d->lo || chan > d->hi) {
	continue; //Not part of this detector
      } 
      // Wire numbers and channels go in the same order ... 
      Int_t wireNum    = wireOffset + chan;
      THaVDCWire* wire = GetWire(wireNum);

      // Get number of hits for this channel and loop through hits
      Int_t nHits = evData.GetNumHits(d->crate, d->slot, chan);
   
      Int_t max_data = -1;
      for (Int_t hit = 0; hit < nHits; hit++) {
	
	// Now get the TDC data for this hit
	Int_t data = evData.GetData(d->crate, d->slot, chan, hit);
	if( only_fastest_hit ) {
	  if( data > max_data )
	    max_data = data;
	} else {
	  Double_t time = fTDCRes * (wire->GetTOffset() - data);
	  if( !no_negative || time > 0.0 )
	    new( (*fHits)[nextHit++] )  THaVDCHit( wire, data, time );
	}
	  
      } // End hit loop

      if( only_fastest_hit ) {
	Double_t time = fTDCRes * (wire->GetTOffset() - max_data);
	// FIXME: This is tricky ... could be that the next slower hit was good
	if( !no_negative || time > 0.0 )
	  new( (*fHits)[nextHit++] ) 
	    THaVDCHit( wire, max_data, time );
      }
    } // End channel index loop
    wireOffset += (d->hi - d->lo + 1);
  } // End slot loop

  // Sort the hits in order of increasing wire number and (for the same wire
  // number) increasing time (NOT rawtime)

  fHits->Sort();

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
    maxdist = static_cast<THaVDCUVPlane*>(fDetector)->GetSpacing() / 2.0;
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
      Int_t rawtime = hit->GetRawTime();
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
      if (clust) {
	// Estimate the track parameters for this cluster
	// (Pivot, intercept, and slope)
	clust->EstTrackParameters();
      }
      // Make a new THaVDCCluster (using space from fCluster array)  
      clust = new ( (*fClusters)[nextClust++] ) THaVDCCluster(this);
    } 
    //Add hit to the cluster
    clust->AddHit(hit);

  } // End looping through hits

  // Estimate track parameters for the last cluster found
  if (clust) {
    clust->EstTrackParameters(); 
  }

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
