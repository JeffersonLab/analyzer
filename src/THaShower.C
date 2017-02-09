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

#include <cstring>
#include <iostream>
#include <cassert>

using namespace std;

//_____________________________________________________________________________
THaShower::THaShower( const char* name, const char* description,
		      THaApparatus* apparatus )
  : THaPidDetector(name,description,apparatus),
    fNChan(0), fChanMap(0), fNclublk(0), fNrows(0),
    fBlockX(0), fBlockY(0), fPed(0), fGain(0),
    fNhits(0), fA(0), fA_p(0), fA_c(0),
    fNclust(0), fMult(0), fNblk(0), fEblk(0),
    tan_angle(0), sin_angle(0), cos_angle(1.)
{
  // Constructor.

}

//_____________________________________________________________________________
Int_t THaShower::DoReadDatabase( FILE* fi, const TDatime& date )
{
  // Read this detector's parameters from the database file 'fi'.
  // This function is called by THaDetectorBase::Init() once at the
  // beginning of the analysis.
  // 'date' contains the date/time of the run being analyzed.

  const char* const here = "ReadDatabase";
  const int LEN = 100;
  char buf[LEN];
  Int_t flags = kErrOnTooManyValues; // TODO: make configurable
  bool old_format = false;

  // Blocks, rows, max blocks per cluster
  Int_t nclbl, ncols, nrows, ivals[3];
  Int_t err = ReadBlock(fi,ivals,2,here,kRequireGreaterZero|kQuietOnTooMany);
  if( err ) {
    if( err != kTooManyValues )
      return kInitError;
    // > 2 values - may be an old-style DB with 3 numbers on first line
    if( ReadBlock(fi,ivals,3,here,kRequireGreaterZero) )
      return kInitError;
    if( ivals[0] % ivals[1] ) {
      Error( Here(here), "Total number of blocks = %d does not divide evenly into "
	     "number of columns = %d", ivals[0], ivals[1] );
      return kInitError;
    }
    ncols = ivals[0] / ivals[1];
    nrows = ivals[1];
    nclbl = ivals[2];
    //    old_format = true;  //FIXME: require consistency with block geometry later?
  } else {
    ncols = ivals[0];
    nrows = ivals[1];
    nclbl = TMath::Min( 3, nrows ) * TMath::Min( 3, ncols );
  }
  Int_t nelem = ncols * nrows;

  // Reinitialization only possible for same basic configuration
  if( fIsInit && (nelem != fNelem || nclbl != fNclublk) ) {
    Error( Here(here), "Cannot re-initalize with different number of blocks or "
	   "blocks per cluster. Detector not re-initialized." );
    return kInitError;
  }
  fNelem = nelem;
  fNrows = nrows;
  fNclublk = nclbl;

  if( fIsInit )
    ClearEvent();

  // Clear out the old detector and channel map before reading new ones
  Int_t mapsize = fDetMap->GetSize();
  delete [] fNChan; fNChan = 0;
  if( fChanMap ) {
    for( Int_t i = 0; i<mapsize; i++ )
      delete [] fChanMap[i];
  }
  delete [] fChanMap; fChanMap = 0;
  fDetMap->Clear();

  // Read detector map
  while( ReadComment(fi,buf,LEN) ) {}
  while (1) {
    Int_t crate, slot, first, last;
    Int_t n = fscanf( fi,"%6d %6d %6d %6d", &crate, &slot, &first, &last );
    fgets ( buf, LEN, fi );
    if( crate < 0 ) break;
    if( n < 4 ) return ErrPrint(fi,here);
    if( fDetMap->AddModule( crate, slot, first, last ) < 0 ) {
      Error( Here(here), "Too many DetMap modules (maximum allowed - %d).",
	     THaDetMap::kDetMapSize);
      return kInitError;
    }
  }
  if( fDetMap->GetTotNumChan() != fNelem ) {
    Error( Here(here), "Database inconsistency.\n Defined %d channels in detector map, "
	   "but have %d total channels (%d blocks with 1 ADC each)",
	   fDetMap->GetTotNumChan(), fNelem, fNelem );
    return kInitError;
  }

  // Read channel map
  vector<int> chanmap;
  chanmap.resize(fNelem);
  if( ReadBlock(fi,&chanmap[0],fNelem,here,flags|kRequireGreaterZero) )
    return kInitError;

  // Consistency check
  for( Int_t i = 0; i < fNelem; ++i ) {
    if( (Int_t)chanmap[i] > fNelem ) {
      Error( Here(here), "Illegal logical channel number %d. "
	     "Must be between 1 and %d.", chanmap[i], fNelem );
      return kInitError;
    }
  }

  // Copy channel map to the per-module array
  // Set up the new channel map
  mapsize = fDetMap->GetSize();
  assert( mapsize > 0 );
  fNChan = new UShort_t[ mapsize ];
  fChanMap = new UShort_t*[ mapsize ];
  for( Int_t i=0, k=0; i < mapsize; i++ ) {
    THaDetMap::Module* module = fDetMap->GetModule(i);
    fNChan[i] = module->hi - module->lo + 1;
    if( fNChan[i] > 0 ) {
      fChanMap[i] = new UShort_t[ fNChan[i] ];
      for( Int_t j=0; j<fNChan[i]; ++j ) {
	assert( k < fNelem );
	fChanMap[i][j] = chanmap[k];
	++k;
      }
    } else {
      Error( Here(here), "No channels defined for module %d.", i );
      delete [] fNChan; fNChan = 0;
      for( Int_t j=0; j<i; j++ )
	delete [] fChanMap[j];
      delete [] fChanMap; fChanMap = 0;
      return kInitError;
    }
  }

  // Read geometry
  Double_t dvals[3], angle;
  if( ReadBlock(fi,dvals,3,here,flags) )                   // Detector's X,Y,Z coord
    return kInitError;
  fOrigin.SetXYZ( dvals[0], dvals[1], dvals[2] );

  if( ReadBlock(fi,fSize,3,here,flags|kNoNegativeValues) ) // Size in X,Y,Z
    return kInitError;

  if( ReadBlock(fi,&angle,1,here,flags) )                  // Rotation angle of det
    return kInitError;

  const Double_t degrad = TMath::Pi()/180.0;
  tan_angle = TMath::Tan(angle*degrad);
  sin_angle = TMath::Sin(angle*degrad);
  cos_angle = TMath::Cos(angle*degrad);
  DefineAxes(angle*degrad);

  // Dimension arrays if initializing for the first time
  if( !fIsInit ) {
    // Geometry (block positions)
    fBlockX = new Float_t[ fNelem ];
    fBlockY = new Float_t[ fNelem ];

    // Calibrations
    fPed    = new Float_t[ fNelem ];
    fGain   = new Float_t[ fNelem ];

    // Per-event data
    fA      = new Float_t[ fNelem ];
    fA_p    = new Float_t[ fNelem ];
    fA_c    = new Float_t[ fNelem ];
    fNblk   = new Int_t  [ fNclublk ];
    fEblk   = new Float_t[ fNclublk ];

    fIsInit = true;
    ClearEvent();
  }

  // Try new format: two values here
  Double_t xy[2], dxy[2];
  err = ReadBlock(fi,xy,2,here,flags|kQuietOnTooMany);    // Block 1 center position
  if( err ) {
    if( err != kTooManyValues )
      return kInitError;
    old_format = true;
  }

  if( old_format ) {
    const Double_t u = 1e-2;   // Old database values are in cm
    const Double_t eps = 1e-4; // Rounding error tolerance for block spacings
    Double_t dx = 0, dy = 0, dx1, dy1;
    Double_t* xpos = new Double_t[ fNelem ];
    Double_t* ypos = new Double_t[ fNelem ];

    // Convert old cm units to meters
    fOrigin *= u;
    for( Int_t i = 0; i < 3; ++i )
      fSize[i] *= u;

    if( (err = ReadBlock(fi,xpos,fNelem,here,flags)) )     // Block x positions
      goto err;
    if( (err = ReadBlock(fi,ypos,fNelem,here,flags)) )     // Block y positions
      goto err;

    xy[0] = u*xpos[0];
    xy[1] = u*ypos[0];

    // Determine the block spacings. Irregular spacings are not supported.
    for( Int_t i = 1; i < fNelem; ++i ) {
      dx1 = xpos[i] - xpos[i-1];
      dy1 = ypos[i] - ypos[i-1];
      if( dx == 0 ) {
	if( dx1 != 0 )
	  dx = dx1;
      } else if( dx1 != 0 && dx*dx1 > 0 && TMath::Abs(dx1-dx) > eps ) {
	Error( Here(here), "Irregular x block positions not supported, "
	       "dx = %lf, dx1 = %lf", dx, dx1 );
	err = -1;
	goto err;
      }
      if( dy == 0 ) {
	if( dy1 != 0 )
	  dy = dy1;
      } else if( dy1 != 0 && dy*dy1 > 0 && TMath::Abs(dy1-dy) > eps ) {
	Error( Here(here), "Irregular y block positions not supported, "
	       "dy = %lf, dy1 = %lf", dy, dy1 );
	err = -1;
	goto err;
      }
    }
    dxy[0] = u*dx;
    dxy[1] = u*dy;
  err:
    delete [] xpos;
    delete [] ypos;
    if( err )
      return kInitError;
  }
  else {
    // New format
    if( ReadBlock(fi,dxy,2,here,flags) )                    // Block spacings in x and y
      return kInitError;

    if( ReadBlock(fi,&fEmin,1,here,flags|kNoNegativeValues) ) // Emin thresh for center
      return kInitError;
  }

  // Compute block positions
  for( Int_t c=0; c<ncols; c++ ) {
    for( Int_t r=0; r<fNrows; r++ ) {
      Int_t k = fNrows*c + r;
      // Units are meters
      fBlockX[k] = xy[0] + r*dxy[0];
      fBlockY[k] = xy[1] + c*dxy[1];
    }
  }

  if( !old_format ) {
    // Search for optional time stamp or configuration section
    TDatime datime(date);
    if( SeekDBdate( fi, datime ) == 0 && fConfig.Length() > 0 &&
	SeekDBconfig( fi, fConfig.Data() )) {}
  }

  // Read calibrations
  // ADC pedestals (in order of logical channel number)
  if( ReadBlock(fi,fPed,fNelem,here,flags) )
    return kInitError;
  // ADC gains
  if( ReadBlock(fi,fGain,fNelem,here,flags|kNoNegativeValues) )
    return kInitError;

  if( old_format ) {
    // Emin thresh for center (occurs earlier in new format, see above)
    if( ReadBlock(fi,&fEmin,1,here,flags|kNoNegativeValues) )
      return kInitError;
  }

  // Debug printout
  if ( fDebug > 1 ) {
    const UInt_t N = static_cast<UInt_t>(fNelem);
    Double_t pos[3]; fOrigin.GetXYZ(pos);
    DBRequest list[] = {
      { "Number of blocks",       &fNelem,     kInt        },
      { "Detector center",        pos,         kDouble, 3  },
      { "Detector size",          fSize,       kFloat,  3  },
      { "Detector angle",         &angle,                  },
      { "Channel map",            &chanmap[0], kInt,    N  },
      { "Position of block 1",    xy,          kDouble, 2  },
      { "Block x/y spacings",     dxy,         kDouble, 2  },
      { "Minimum cluster energy", &fEmin,      kFloat,  1  },
      { "ADC pedestals",          fPed,        kFloat,  N  },
      { "ADC pedestals",          fPed,        kFloat,  N  },
      { "ADC gains",              fGain,       kFloat,  N  },
      { 0 }
    };
    DebugPrint( list );
  }

  return kOK;
}

//_____________________________________________________________________________
Int_t THaShower::ReadDatabase( const TDatime& date )
{
  // Wrapper around actual database reader. Using a wrapper makes it much
  // easier to close the input file in case of an error.

  FILE* fi = OpenFile( date );
  if( !fi ) return kFileError;

  Int_t ret = DoReadDatabase( fi, date );

  fclose(fi);
  return ret;
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
    { "trx",    "track x-position in det plane",      "fTRX" },
    { "try",    "track y-position in det plane",      "fTRY" },
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

  delete [] fNChan; fNChan = 0;
  UShort_t mapsize = fDetMap->GetSize();
  if( fChanMap ) {
    for( UShort_t i = 0; i<mapsize; i++ )
      delete [] fChanMap[i];
  }
  delete [] fChanMap; fChanMap = 0;
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
inline
void THaShower::ClearEvent()
{
  // Reset all local data to prepare for next event.

  fNhits = fNclust = fMult = 0;
  assert(fIsInit);
  for( Int_t i=0; i<fNelem; ++i ) {
    fA[i] = fA_p[i] = fA_c[i] = kBig;
  }
  fAsum_p = fAsum_c = fE = fX = fY = fTRX = fTRY = kBig;
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

  ClearEvent();

  // Loop over all modules defined for shower detector
  for( UShort_t i = 0; i < fDetMap->GetSize(); i++ ) {
    THaDetMap::Module* d = fDetMap->GetModule( i );

    // Loop over all channels that have a hit.
    for( Int_t j = 0; j < evdata.GetNumChan( d->crate, d->slot ); j++) {

      Int_t chan = evdata.GetNextChan( d->crate, d->slot, j );
      if( chan > d->hi || chan < d->lo ) continue;    // Not one of my channels.

      // Get the data. shower blocks are assumed to have only single hit (hit=0)
      Int_t data = evdata.GetData( d->crate, d->slot, chan, 0 );

      // Copy the data to the local variables.
      Int_t k = *(*(fChanMap+i)+((d->reverse) ? d->hi - chan : chan-d->lo)) - 1;
#ifdef WITH_DEBUG
      if( k<0 || k>=fNelem ) 
	Warning( Here("Decode()"), "Bad array index: %d. Your channel map is "
		 "invalid. Data skipped.", k );
      else
#endif
      {
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
  }

  if ( fDebug > 3 ) {
    printf("\nShower Detector %s:\n",GetPrefix());
    int ncol=3;
    for (int i=0; i<ncol; i++) {
      printf("  Block  ADC  ADC_p  ");
    }
    printf("\n");
    
    for (int i=0; i<(fNelem+ncol-1)/ncol; i++ ) {
      for (int c=0; c<ncol; c++) {
	int ind = c*fNelem/ncol+i;
	if (ind < fNelem) {
	  printf("  %3d  %5.0f  %5.0f  ",ind+1,fA[ind],fA_p[ind]);
	} else {
	  //	  printf("\n");
	  break;
	}
      }
      printf("\n");
    }
  }

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
  // fTRX;          -  X-coordinate of track cross point with shower plane
  // fTRY;          -  Y-coordinate of track cross point with shower plane
  //
  // Only one ("main") cluster, i.e. the cluster with the largest energy 
  // deposition is considered. Units are MeV for energies and cm for 
  // coordinates.

  fNclust = 0;
  short mult = 0, nr, nc, ir, ic, dr, dc;
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



  // Calculation of coordinates of the track cross point with 
  // shower plane in the detector coordinate system. For this, parameters
  // of track reconstructed in THaVDC::CoarseTrack() are used.

  int ne_track = tracks.GetLast()+1;   // Number of reconstructed in Earm tracks

  if ( ne_track >= 1 && tracks.At(0) != 0 ) {
    THaTrack* theTrack = static_cast<THaTrack*>( tracks[0] );
    double fpe_x       = theTrack->GetX();
    double fpe_y       = theTrack->GetY();
    double fpe_th      = theTrack->GetTheta();
    double fpe_ph      = theTrack->GetPhi();

    fTRX = ( fpe_x + fpe_th * fOrigin.Z() ) / 
      ( cos_angle * (1.0 + fpe_th * tan_angle) );
    fTRY = fpe_y + fpe_ph * fOrigin.Z() - fpe_ph * sin_angle * fTRX;
   }

  return 0;
}
//_____________________________________________________________________________
Int_t THaShower::FineProcess( TClonesArray& /* tracks */ )
{
  // Fine Shower processing.
  // Not implemented. Does nothing, returns 0.

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
ClassImp(THaShower)

