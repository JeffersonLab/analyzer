///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaShower                                                                 //
//                                                                           //
// Shower counter class, describing a generic segmented shower detector      //
// (preshower or shower).                                                    //
// Currently, only the "main" cluster, i.e. cluster with the largest energy  //
// deposition is considered. Units of measurements are MeV for energy of     //
// shower and centimeters for coordinates.                                   //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaShower.h"
#include "THaGlobals.h"
#include "THaEvData.h"
#include "THaDetMap.h"
#include "VarDef.h"
#include "VarType.h"
#include "THaTrack.h"
#include "TClonesArray.h"
#include "TDatime.h"
#include "TMath.h"

#include <cassert>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#define OSSTREAM ostringstream

using namespace std;

//_____________________________________________________________________________
THaShower::THaShower( const char* name, const char* description,
		      THaApparatus* apparatus ) :
  THaPidDetector(name,description,apparatus),
  fNclublk(0), fNrows(0), fBlockX(0), fBlockY(0), fPed(0), fGain(0),
  fNhits(0), fA(0), fA_p(0), fA_c(0), fNblk(0), fEblk(0)
{
  // Constructor
}

//_____________________________________________________________________________
THaShower::THaShower() :
  THaPidDetector(),
  fNclublk(0), fNrows(0), fBlockX(0), fBlockY(0), fPed(0), fGain(0),
  fNhits(0), fA(0), fA_p(0), fA_c(0), fNblk(0), fEblk(0)
{
  // Default constructor (for ROOT I/O)
}

//_____________________________________________________________________________
Int_t THaShower::ReadDatabase( const TDatime& date )
{
  // Read parameters from the database.
  // 'date' contains the date/time of the run being analyzed.

  const char* const here = "ReadDatabase";

  // Read database

  FILE* file = OpenFile( date );
  if( !file ) return kFileError;

  // Read fOrigin and fSize (required!)
  Int_t err = ReadGeometry( file, date, true );
  if( err ) {
    fclose(file);
    return err;
  }

  vector<Int_t> detmap, chanmap;
  vector<Double_t> xy, dxy;
  Int_t ncols, nrows;

  // Read mapping/geometry/configuration parameters
  DBRequest config_request[] = {
    { "detmap",       &detmap,  kIntV },
    { "chanmap",      &chanmap, kIntV,    0, 1 },
    { "ncols",        &ncols,   kInt },
    { "nrows",        &nrows,   kInt },
    { "xy",           &xy,      kDoubleV, 2 },  // center pos of block 1
    { "dxdy",         &dxy,     kDoubleV, 2 },  // dx and dy block spacings
    { "emin",         &fEmin,   kDouble },
    { 0 }
  };
  err = LoadDB( file, date, config_request, fPrefix );

  // Sanity checks
  if( !err && (nrows <= 0 || ncols <= 0) ) {
    Error( Here(here), "Illegal number of rows or columns: %d %d. Must be > 0. "
	   "Fix database.", nrows, ncols );
    err = kInitError;
  }

  Int_t nelem = ncols * nrows; 
  Int_t nclbl = TMath::Min( 3, nrows ) * TMath::Min( 3, ncols );

  // Reinitialization only possible for same basic configuration
  if( !err ) {
    if( fIsInit && nelem != fNelem ) {
      Error( Here(here), "Cannot re-initalize with different number of blocks or "
	     "blocks per cluster (was: %d, now: %d). Detector not re-initialized.",
	     fNelem, nelem );
      err = kInitError;
    } else {
      fNelem = nelem;
      fNrows = nrows;
      fNclublk = nclbl;
    }
  }

  if( !err ) {
    // Clear out the old channel map before reading a new one
    fChanMap.clear();
    if( FillDetMap(detmap, 0, here) <= 0 ) {
      err = kInitError;  // Error already printed by FillDetMap
    } else if( (nelem = fDetMap->GetTotNumChan()) != fNelem ) {
      Error( Here(here), "Number of detector map channels (%d) "
	     "inconsistent with number of blocks (%d)", nelem, fNelem );
      err = kInitError;
    }
  }
  if( !err ) {
    if( !chanmap.empty() ) {
      // If a map is found in the database, ensure it has the correct size
      Int_t cmapsize = chanmap.size();
      if( cmapsize != fNelem ) {
	Error( Here(here), "Channel map size (%d) and number of detector "
	       "channels (%d) must be equal. Fix database.", cmapsize, fNelem );
	err = kInitError;
      }
    }
    if( !err ) {
      // Set up the new channel map
      Int_t nmodules = fDetMap->GetSize();
      assert( nmodules > 0 );
      fChanMap.resize(nmodules);
      for( Int_t i=0, k=0; i < nmodules && !err; i++ ) {
	THaDetMap::Module* module = fDetMap->GetModule(i);
	Int_t nchan = module->hi - module->lo + 1;
	if( nchan > 0 ) {
	  fChanMap.at(i).resize(nchan);
	  for( Int_t j=0; j<nchan; ++j ) {
	    assert( k < fNelem );
	    fChanMap.at(i).at(j) = chanmap.empty() ? k+1 : chanmap[k];
	    ++k;
	  }
	} else {
	  Error( Here(here), "No channels defined for module %d.", i);
	  fChanMap.clear();
	  err = kInitError;
	}
      }
    }
  }

  if( err ) {
    fclose(file);
    return err;
  }

  // Dimension arrays
  //FIXME: use a structure!
  UInt_t nval = fNelem;
  if( !fIsInit ) {
    // Geometry
    fBlockX = new Float_t[ nval ];
    fBlockY = new Float_t[ nval ];

    // Calibrations
    fPed    = new Float_t[ nval ];
    fGain   = new Float_t[ nval ];

    // Per-event data
    fA    = new Float_t[ nval ];
    fA_p  = new Float_t[ nval ];
    fA_c  = new Float_t[ nval ];
    fNblk = new Int_t[ fNclublk ];
    fEblk = new Float_t[ fNclublk ];

    fIsInit = true;
  }

  // Compute block positions
  for( int c=0; c<ncols; c++ ) {
    for( int r=0; r<nrows; r++ ) {
      int k = nrows*c + r;
      // Units are meters
      fBlockX[k] = xy[0] + r*dxy[0];
      fBlockY[k] = xy[1] + c*dxy[1];
    }
  }

  // Read calibration parameters

  // Set DEFAULT values here
  // Default ADC pedestals (0) and ADC gains (1)
  memset( fPed, 0, nval*sizeof(fPed[0]) );
  for( UInt_t i=0; i<nval; ++i ) { fGain[i] = 1.0; }

  // Read ADC pedestals and gains (in order of logical channel number)
  DBRequest calib_request[] = {
    { "pedestals",    fPed,   kFloat, nval, 1 },
    { "gains",        fGain,  kFloat, nval, 1 },
    { 0 }
  };
  err = LoadDB( file, date, calib_request, fPrefix );
  fclose(file);
  if( err )
    return err;

#ifdef WITH_DEBUG
  // Debug printout
  if ( fDebug > 2 ) {
    const UInt_t N = static_cast<UInt_t>(fNelem);
    Double_t pos[3]; fOrigin.GetXYZ(pos);
    DBRequest list[] = {
      { "Number of blocks",       &fNelem,     kInt        },
      { "Detector center",        pos,         kDouble, 3  },
      { "Detector size",          fSize,       kDouble, 3  },
      { "Channel map",            &chanmap[0], kInt,    N  },
      { "Position of block 1",    &xy,         kDoubleV    },
      { "Block x/y spacings",     &dxy,        kDoubleV    },
      { "Minimum cluster energy", &fEmin,      kFloat,  1  },
      { "ADC pedestals",          fPed,        kFloat,  N  },
      { "ADC pedestals",          fPed,        kFloat,  N  },
      { "ADC gains",              fGain,       kFloat,  N  },
      { 0 }
    };
    DebugPrint( list );
  }
#endif

  return kOK;
}

//_____________________________________________________________________________
Int_t THaShower::DefineVariables( EMode mode )
{
  // Initialize global variables

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  // Register variables in global list

  RVarDef vars[] = {
    { "nhit",   "Number of hits",                     "fNhits" },
    { "a",      "Raw ADC amplitudes",                 "fA" },
    { "a_p",    "Ped-subtracted ADC amplitudes",      "fA_p" },
    { "a_c",    "Calibrated ADC amplitudes",          "fA_c" },
    { "asum_p", "Sum of ped-subtracted ADCs",         "fAsum_p" },
    { "asum_c", "Sum of calibrated ADCs",             "fAsum_c" },
    { "nclust", "Number of clusters",                 "fNclust" },
    { "e",      "Energy (MeV) of largest cluster",    "fE" },
    { "x",      "x-position (cm) of largest cluster", "fX" },
    { "y",      "y-position (cm) of largest cluster", "fY" },
    { "mult",   "Multiplicity of largest cluster",    "fMult" },
    { "nblk",   "Numbers of blocks in main cluster",  "fNblk" },
    { "eblk",   "Energies of blocks in main cluster", "fEblk" },
    { "trx",    "x-position of track in det plane",   "fTrackProj.THaTrackProj.fX" },
    { "try",    "y-position of track in det plane",   "fTrackProj.THaTrackProj.fY" },
    { "trpath", "TRCS pathlen of track to det plane", "fTrackProj.THaTrackProj.fPathl" },
    { 0 }
  };
  return DefineVarsFromList( vars, mode );
}

//_____________________________________________________________________________
THaShower::~THaShower()
{
  // Destructor. Removes internal arrays and global variables.

  if( fIsSetup )
    RemoveVariables();
  if( fIsInit )
    DeleteArrays();
}

//_____________________________________________________________________________
void THaShower::DeleteArrays()
{
  // Delete member arrays. Internal function used by destructor.

  fChanMap.clear();
  delete [] fBlockX;  fBlockX  = 0;
  delete [] fBlockY;  fBlockY  = 0;
  delete [] fPed;     fPed     = 0;
  delete [] fGain;    fGain    = 0;
  delete [] fA;       fA       = 0;
  delete [] fA_p;     fA_p     = 0;
  delete [] fA_c;     fA_c     = 0;
  delete [] fNblk;    fNblk    = 0;
  delete [] fEblk;    fEblk    = 0;
}

//_____________________________________________________________________________
void THaShower::Clear( Option_t* opt )
{
  // Clear event data

  THaPidDetector::Clear(opt);
  fNhits = fNclust = fMult = 0;
  assert(fIsInit);
  for( Int_t i=0; i<fNelem; ++i ) {
    fA[i] = fA_p[i] = fA_c[i] = kBig;
  }
  fAsum_p = fAsum_c = 0.0;
  fE = fX = fY = kBig;
  memset( fNblk, 0, fNclublk*sizeof(fNblk[0]) );
  for( Int_t i=0; i<fNclublk; ++i ) {
    fEblk[i] = kBig;
  }
}

//_____________________________________________________________________________
Int_t THaShower::Decode( const THaEvData& evdata )
{
  // Decode shower data, scale the data to energy deposition
  // ( in MeV ), and copy the data into the following local data structure:
  //
  // fNhits           -  Number of hits on shower;
  // fA[]             -  Array of ADC values of shower blocks;
  // fA_p[]           -  Array of ADC minus ped values of shower blocks;
  // fA_c[]           -  Array of corrected ADC values of shower blocks;
  // fAsum_p          -  Sum of shower blocks ADC minus pedestal values;
  // fAsum_c          -  Sum of shower blocks corrected ADC values;

  const char* const here = "Decode";

  // Loop over all modules defined for shower detector
  bool has_warning = false;
  Int_t nmodules = fDetMap->GetSize();
  for( Int_t i = 0; i < nmodules; i++ ) {
    THaDetMap::Module* d = fDetMap->GetModule( i );

    // Loop over all channels that have a hit.
    for( Int_t j = 0; j < evdata.GetNumChan( d->crate, d->slot ); j++) {

      Int_t chan = evdata.GetNextChan( d->crate, d->slot, j );
      if( chan > d->hi || chan < d->lo ) continue;    // Not one of my channels.

      Int_t nhit = evdata.GetNumHits(d->crate, d->slot, chan);
      if( nhit > 1 || nhit == 0 ) {
	OSSTREAM msg;
	msg << nhit << " hits on " << "ADC channel "
	    << d->crate << "/" << d->slot << "/" << chan;
	++fMessages[msg.str()];
	has_warning = true;
	if( nhit == 0 ) {
	  msg << ". Should never happen. Decoder bug. Call expert.";
	  Warning( Here(here), "Event %d: %s", evdata.GetEvNum(),
		   msg.str().c_str() );
	  continue;
	}
#ifdef WITH_DEBUG
	if( fDebug>0 ) {
	  Warning( Here(here), "Event %d: %s", evdata.GetEvNum(),
		   msg.str().c_str() );
	}
#endif
      }
      // Get the data. If multiple hits on a channel, take the first (ADC)
      Int_t data = evdata.GetData( d->crate, d->slot, chan, 0 );

      Int_t jchan = (d->reverse) ? d->hi - chan : chan-d->lo;
      if( jchan<0 || jchan>=fNelem ) {
	Error( Here(here), "Illegal detector channel: %d", jchan );
	continue;
      }
#ifdef NDEBUG
      Int_t k = fChanMap[i][jchan] - 1;
#else
      Int_t k = fChanMap.at(i).at(jchan) - 1;
#endif
      if( k<0 || k>=fNelem ) {
	Error( Here(here), "Bad array index: %d. Your channel map is "
	       "invalid. Data skipped.", k );
	continue;
      }

      // Copy the data and apply calibrations
      fA[k]   = data;                   // ADC value
      fA_p[k] = data - fPed[k];         // ADC minus ped
      fA_c[k] = fA_p[k] * fGain[k];     // ADC corrected
      if( fA_p[k] > 0.0 )
	fAsum_p += fA_p[k];             // Sum of ADC minus ped
      if( fA_c[k] > 0.0 )
	fAsum_c += fA_c[k];             // Sum of ADC corrected
      fNhits++;
    }
  }

  if( has_warning )
    ++fNEventsWithWarnings;

#ifdef WITH_DEBUG
  if ( fDebug > 3 ) {
    cout << endl << "Shower Detector " << GetPrefix() << ":" << endl;
    int ncol=3;
    for (int i=0; i<ncol; i++) {
      cout << "  Block  ADC  ADC_p  ";
    }
    cout << endl;

    for (int i=0; i<(fNelem+ncol-1)/ncol; i++ ) {
      for (int c=0; c<ncol; c++) {
	int ind = c*fNelem/ncol+i;
	if (ind < fNelem) {
	  cout << "  " << setw(3) << ind+1;
	  cout << "  "; WriteValue(fA[ind]);
	  cout << "  "; WriteValue(fA_p[ind]);
	  cout << "  ";
	} else {
	  //	  cout << endl;
	  break;
	}
      }
      cout << endl;
    }
  }
#endif

  return fNhits;
}

//_____________________________________________________________________________
Int_t THaShower::CoarseProcess( TClonesArray& tracks )
{
  // Reconstruct Clusters in shower detector and copy the data
  // into the following local data structure:
  //
  // fNclust        -  Number of clusters in shower;
  // fE             -  Energy (in MeV) of the "main" cluster;
  // fX             -  X-coordinate (in cm) of the cluster;
  // fY             -  Y-coordinate (in cm) of the cluster;
  // fMult          -  Number of blocks in the cluster;
  // fNblk[0]...[5] -  Numbers of blocks composing the cluster;
  // fEblk[0]...[5] -  Energies in blocks composing the cluster;
  //
  // Only one ("main") cluster, i.e. the cluster with the largest energy
  // deposition is considered. Units are MeV for energies and cm for
  // coordinates.

  fNclust = 0;
  int nmax = -1;
  double  emax = fEmin;                     // Min threshold of energy in center
  for ( int  i = 0; i < fNelem; i++) {      // Find the block with max energy:
    double  ei = fA_c[i];                   // Energy in next block
    if ( ei > emax) {
      nmax = i;                             // Number of block with max energy
      emax = ei;                            // Max energy per a blocks
    }
  }
  if ( nmax >= 0 ) {
    short mult = 0, nr, nc, ir, ic, dr, dc;
    double  sxe = 0, sye = 0;               // Sums of xi*ei and yi*ei
    nc = nmax/fNrows;                       // Column of the block with max engy
    nr = nmax%fNrows;                       // Row of the block with max energy
                                            // Add the block to cluster (center)
    fNblk[mult]   = nmax;                   // Add number of the block (center)
    fEblk[mult++] = emax;                   // Add energy in the block (center)
    sxe = emax * fBlockX[nmax];             // Sum of xi*ei
    sye = emax * fBlockY[nmax];             // Sum of yi*ei
    for ( int  i = 0; i < fNelem; i++) {    // Detach surround blocks:
      double  ei = fA_c[i];                 // Energy in next block
      if ( ei>0 && i!=nmax ) {              // Some energy out of cluster center
	ic = i/fNrows;                      // Column of next block
	ir = i%fNrows;                      // Row of next block
	dr = nr - ir;                       // Distance on row up to center
	dc = nc - ic;                       // Distance on column up to center
	if ( -2<dr&&dr<2&&-2<dc&&dc<2 ) {   // Surround block:
	                                    // Add block to cluster (surround)
	  fNblk[mult]   = i;                // Add number of block (surround)
	  fEblk[mult++] = ei;               // Add energy of block (surround)
	  sxe  += ei * fBlockX[i];          // Sum of xi*ei of cluster blocks
	  sye  += ei * fBlockY[i];          // Sum of yi*ei of cluster blocks
	  emax += ei;                       // Sum of energies in cluster blocks
	}
      }
    }
    fNclust = 1;                            // One ("main") cluster detected
    fE      = emax;                         // Energy (MeV) in "main" cluster
    fX      = sxe/emax;                     // X coordinate (cm) of the cluster
    fY      = sye/emax;                     // Y coordinate (cm) of the cluster
    fMult   = mult;                         // Number of blocks in "main" clust.
  }

  // Calculate track projections onto shower plane

  CalcTrackProj( tracks );

  return 0;
}

//_____________________________________________________________________________
Int_t THaShower::FineProcess( TClonesArray& tracks )
{
  // Fine Shower processing.

  // Redo the track-matching, since tracks might have been thrown out
  // during the FineTracking stage.

  CalcTrackProj( tracks );

  return 0;
}

//_____________________________________________________________________________
ClassImp(THaShower)
