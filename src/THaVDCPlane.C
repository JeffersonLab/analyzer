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

#include "THaVDCPlane.h"
#include "THaVDCWire.h"
#include "THaVDCUVPlane.h"
#include "THaVDCCluster.h"
#include "THaVDCHit.h"
#include "THaDetMap.h"
#include "THaVDCAnalyticTTDConv.h"
#include "TString.h"
#include "TClass.h"
#include "TMath.h"

#include <cstring>

ClassImp(THaVDCPlane)


//_____________________________________________________________________________
THaVDCPlane::THaVDCPlane( const char* name, const char* description,
			  THaDetectorBase* parent )
  : THaSubDetector(name,description,parent)
{
  // Constructor

  // Since TCloneArrays can resize, the size here is fairly unimportant
  fHits  = new TClonesArray("THaVDCHit", 50);
  fClusters = new TClonesArray("THaVDCCluster", 10);

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
  //TODO: Read from file
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

  // Now initialize wires (those wires... too lazy to initialize themselves!)
  // Caution: This may not correspond at all to actual wire channels!

  THaVDCAnalyticTTDConv* ttdConv = new THaVDCAnalyticTTDConv(driftVel);

  float offset = 0;
  for (int i = 0; i < nWires; i++) {

    // Constuct the new THaVDCWire (using space in the TClonesArray obj)
    THaVDCWire * wire = new((*fWires)[i]) THaVDCWire();

    int junk;

    wire->SetNum(i);                   //Set number of wire
    wire->SetPos(fWBeg + i * fWSpac);  //Set position of wire
    fscanf(file, " %d %f", &junk, &offset);

    wire->SetTOffset(offset);          //Use TOffset from file
    //Set a Time to Distance converter
    wire->SetTTDConv(ttdConv);
  }

  fIsInit = true;
  return kOK;
}

//_____________________________________________________________________________
Int_t THaVDCPlane::SetupDetector( const TDatime& date )
{
  // initialize global variables

  if( fIsSetup ) return kOK;
  fIsSetup = true;



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
void THaVDCPlane::Clear()
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
  
  Int_t wireOffset = 0;

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
      Int_t wireNum = wireOffset + chan;

      // Get number of hits for this channel and loop through hits
      Int_t nHits = evData.GetNumHits(d->crate, d->slot, chan);
   
      for (Int_t hit = 0; hit < nHits; hit++) {
	
	// Now get the TDC data for this hit
	Int_t        data    = evData.GetData(d->crate, d->slot, chan, hit);
	THaVDCWire*  wire    = GetWire(wireNum);
	Int_t        nextHit = GetNHits();
	THaVDCHit*   newHit  = new( (*fHits)[nextHit] ) 
	  THaVDCHit( wire, data, fTDCRes * (wire->GetTOffset() - data));

      } // End hit loop

    } // End channel index loop
    wireOffset += (d->hi - d->lo + 1);
  } // End slot loop

  // Sort the hits in order of increasing wire number and (for the same wire
  // number) increasing time (NOT rawtime)

  fHits->Sort();

  return 0;

}


//_____________________________________________________________________________
Int_t THaVDCPlane::FindClusters(  )
{
  // Reconstruct clusters in a VDC plane
  // Assumes that the wires are numbered such that increasing wire numbers
  //   correspond to decreasing physical position
  // Ignores possibility of overlapping clusters

  Int_t pwireNum = -10;      // Previous wire number
  Int_t wireNum  = 0;        // Current wire number
  Int_t ndif  = 0;           // Difference between wire numbers
  Int_t time = 0;            // TDC time for a wire hit
//    Int_t minTime = 0;         // Smallest TDC time for a given cluster
  Int_t nHits = GetNHits();  // Number of hits in the plane
  THaVDCCluster * clust = NULL; // Current cluster
  THaVDCHit * hit = NULL;    // current hit
//    THaVDCHit * minHit = NULL; // Hit with the smallest TDC time for 
                             // a given cluster
  //  const Double_t sqrt2 = 0.707106781186547462;

  for ( int i = 0; i < nHits; i++ ) {
    //Loop through all TDC  hits

    // Get a hit 
    hit = GetHit(i);

    time = hit->GetRawTime();
    if ( fMinTime < time && time < fMaxTime  ) {
      // If within time range
      
      wireNum = hit->GetWire()->GetNum();  

      if ( wireNum != pwireNum ) {
	// Ignore multiple hits per wire

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
	  Int_t nextClust = GetNClusters();
	  clust = new ( (*fClusters)[nextClust] ) THaVDCCluster();
	  clust->SetPlane(this);
	  
	} 

	clust->AddHit(hit);  //Add hit to the cluster
      } 

    }      
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
  // Find where tracks cross the plane by determining the center of each 
  // cluster
  
  THaVDCCluster * clust = NULL;
  Int_t nClust = GetNClusters();
  for (int i = 0; i < nClust; i++) {
    clust = (THaVDCCluster*)(*fClusters)[i];

    // Make an estimate at drift distances, intercept, and slope
    //  printf("Cluster %d\n", i);

    clust->ConvertTimeToDist();
    clust->FitTrack();
    

  }

  return 0;

}


///////////////////////////////////////////////////////////////////////////////
