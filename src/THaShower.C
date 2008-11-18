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

ClassImp(THaShower)

using namespace std;

//_____________________________________________________________________________
THaShower::THaShower( const char* name, const char* description,
		      THaApparatus* apparatus ) :
  THaPidDetector(name,description,apparatus), fNChan(NULL), fChanMap(NULL)
{
  // Constructor.

}

//_____________________________________________________________________________
Int_t THaShower::ReadDatabase( const TDatime& date )
{
  // Read this detector's parameters from the database file 'fi'.
  // This function is called by THaDetectorBase::Init() once at the
  // beginning of the analysis.
  // 'date' contains the date/time of the run being analyzed.

  static const char* const here = "ReadDatabase()";
  const int LEN = 100;
  char buf[LEN];
  Int_t nelem, ncols, nrows, nclbl;

  // Read data from database

  FILE* fi = OpenFile( date );
  if( !fi ) return kFileError;

  // Blocks, rows, max blocks per cluster
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );          
  fscanf ( fi, "%d%d", &ncols, &nrows );  

  nelem = ncols * nrows;
  nclbl = TMath::Min( 3, nrows ) * TMath::Min( 3, ncols );
  // Reinitialization only possible for same basic configuration 
  if( fIsInit && (nelem != fNelem || nclbl != fNclublk) ) {
    Error( Here(here), "Cannot re-initalize with different number of blocks or "
	   "blocks per cluster. Detector not re-initialized." );
    fclose(fi);
    return kInitError;
  }

  if( nrows <= 0 || ncols <= 0 || nclbl <= 0 ) {
    Error( Here(here), "Illegal number of rows or columns: "
	   "%d %d", nrows, ncols );
    fclose(fi);
    return kInitError;
  }
  fNelem = nelem;
  fNrows = nrows;
  fNclublk = nclbl;

  // Clear out the old detector map before reading a new one
  UShort_t mapsize = fDetMap->GetSize();
  delete [] fNChan;
  if( fChanMap ) {
    for( UShort_t i = 0; i<mapsize; i++ )
      delete [] fChanMap[i];
  }
  delete [] fChanMap;
  fDetMap->Clear();

  // Read detector map

  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  while (1) {
    Int_t crate, slot, first, last;
    fscanf ( fi,"%d%d%d%d", &crate, &slot, &first, &last );
    fgets ( buf, LEN, fi );
    if( crate < 0 ) break;
    if( fDetMap->AddModule( crate, slot, first, last ) < 0 ) {
      Error( Here(here), "Too many DetMap modules (maximum allowed - %d).", 
	    THaDetMap::kDetMapSize);
      fclose(fi);
      return kInitError;
    }
  }

  // Set up the new channel map
  mapsize = fDetMap->GetSize();
  if( mapsize == 0 ) {
    Error( Here(here), "No modules defined in detector map.");
    fclose(fi);
    return kInitError;
  }

  fNChan = new UShort_t[ mapsize ];
  fChanMap = new UShort_t*[ mapsize ];
  for( UShort_t i=0; i < mapsize; i++ ) {
    THaDetMap::Module* module = fDetMap->GetModule(i);
    fNChan[i] = module->hi - module->lo + 1;
    if( fNChan[i] > 0 )
      fChanMap[i] = new UShort_t[ fNChan[i] ];
    else {
      Error( Here(here), "No channels defined for module %d.", i);
      delete [] fNChan; fNChan = NULL;
      for( UShort_t j=0; j<i; j++ )
	delete [] fChanMap[j];
      delete [] fChanMap; fChanMap = NULL;
      fclose(fi);
      return kInitError;
    }
  }
  // Read channel map
  //
  // Loosen the formatting restrictions: remove from each line the portion
  // after a '#', and do the pattern matching to the remaining string
  fgets ( buf, LEN, fi );

  // get the line and end it at a '#' symbol
  *buf = '\0';
  char *ptr=buf;
  int nchar=0;
  for ( UShort_t i = 0; i < mapsize; i++ ) {
    for ( UShort_t j = 0; j < fNChan[i]; j++ ) {
      while ( !strpbrk(ptr,"0123456789") ) {
	fgets ( buf, LEN, fi );
	if( (ptr = strchr(buf,'#')) ) *ptr = '\0';
	ptr = buf;
	nchar=0;
      }
      sscanf (ptr, "%hu%n", *(fChanMap+i)+j, &nchar );
      ptr += nchar;
    }
  }
  
  fgets ( buf, LEN, fi );

  Float_t x,y,z;
  fscanf ( fi, "%f%f%f", &x, &y, &z );               // Detector's X,Y,Z coord
  fOrigin.SetXYZ( x, y, z );
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  fscanf ( fi, "%f%f%f", fSize, fSize+1, fSize+2 );  // Sizes of det in X,Y,Z
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );

  Float_t angle;
  fscanf ( fi, "%f", &angle );                       // Rotation angle of det
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  const Double_t degrad = TMath::Pi()/180.0;
  tan_angle = TMath::Tan(angle*degrad);
  sin_angle = TMath::Sin(angle*degrad);
  cos_angle = TMath::Cos(angle*degrad);

  DefineAxes(angle*degrad);

  // Dimension arrays
  if( !fIsInit ) {
    fBlockX = new Float_t[ fNelem ];
    fBlockY = new Float_t[ fNelem ];
    fPed    = new Float_t[ fNelem ];
    fGain   = new Float_t[ fNelem ];

    // Per-event data
    fA    = new Float_t[ fNelem ];
    fA_p  = new Float_t[ fNelem ];
    fA_c  = new Float_t[ fNelem ];
    fNblk = new Int_t[ fNclublk ];
    fEblk = new Float_t[ fNclublk ];

    fIsInit = true;
  }

  fscanf ( fi, "%f%f", &x, &y );                     // Block 1 center position
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  Float_t dx, dy;
  fscanf ( fi, "%f%f", &dx, &dy );                   // Block spacings in x and y
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  fscanf ( fi, "%f", &fEmin );                       // Emin thresh for center
  fgets ( buf, LEN, fi );

  // Read calibrations.
  // Before doing this, search for any date tags that follow, and start reading from
  // the best matching tag if any are found. If none found, but we have a configuration
  // string, search for it.
  if( SeekDBdate( fi, date ) == 0 && fConfig.Length() > 0 && 
      SeekDBconfig( fi, fConfig.Data() )) {}

  fgets ( buf, LEN, fi );  
  // Crude protection against a missed date/config tag
  if( buf[0] == '[' ) fgets ( buf, LEN, fi );

  // Read ADC pedestals and gains (in order of logical channel number)
  for (int j=0; j<fNelem; j++)
    fscanf (fi,"%f",fPed+j);
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  for (int j=0; j<fNelem; j++) 
    fscanf (fi, "%f",fGain+j);


  // Compute block positions
  for( int c=0; c<ncols; c++ ) {
    for( int r=0; r<nrows; r++ ) {
      int k = nrows*c + r;
      fBlockX[k] = x + r*dx;                         // Units are meters
      fBlockY[k] = y + c*dy;
    }
  }
  fclose(fi);
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

  const int lsh = fNelem*sizeof(Float_t);
  const int lsc = fNclublk*sizeof(Float_t);
  const int lsi = fNclublk*sizeof(Int_t);

  fNhits = 0;
  memset( fA, 0, lsh );
  memset( fA_p, 0, lsh );
  memset( fA_c, 0, lsh );
  fAsum_p = 0.0;
  fAsum_c = 0.0;
  fNclust = 0;
  fE = 0.0;
  fX = 0.0;
  fY = 0.0;
  fMult = 0;
  memset( fNblk, 0, lsi );
  memset( fEblk, 0, lsc );
  fTRX = 0.0;
  fTRY = 0.0;
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
      Int_t k = *(*(fChanMap+i)+(chan-d->lo)) - 1;
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

