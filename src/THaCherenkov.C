///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaCherenkov                                                              //
//                                                                           //
// Class for a generic Cherenkov consisting of one or more phototubes.       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaCherenkov.h"
#include "THaEvData.h"
#include "THaDetMap.h"
#include "VarDef.h"
#include "VarType.h"
#include "THaTrack.h"
#include "THaTrackProj.h"
#include "TClonesArray.h"
#include "TDatime.h"
#include "TMath.h"

#include <cstring>
#include <cstdlib>
#include <cstdio>

ClassImp(THaCherenkov)

//_____________________________________________________________________________
THaCherenkov::THaCherenkov( const char* name, const char* description,
			    THaApparatus* apparatus )
  : THaPidDetector(name,description,apparatus)
{
  // Constructor
  fTrackProj = new TClonesArray( "THaTrackProj", 5 );
}

//_____________________________________________________________________________
Int_t THaCherenkov::ReadDatabase( const TDatime& date )
{
  // Read this detector's parameters from the database file 'fi'.
  // This function is called by THaDetectorBase::Init() once at the
  // beginning of the analysis.
  // 'date' contains the date/time of the run being analyzed.

  static const char* const here = "ReadDatabase()";

  // Read database

  FILE* fi = OpenFile( date );
  if( !fi ) return kFileError;

  const int LEN = 100;
  char buf[LEN];
  Int_t nelem;

  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  fscanf ( fi, "%d", &nelem );                      // Number of mirrors

  // Reinitialization only possible for same basic configuration 
  if( fIsInit && nelem != fNelem ) {
    Error( Here(here), "Cannot re-initalize with different number of mirrors. "
	   "(was: %d, now: %d). Detector not re-initialized.", fNelem, nelem );
    fclose(fi);
    return kInitError;
  }
  fNelem = nelem;

  // Read detector map.  Assumes that the first half of the entries 
  // is for ADCs, and the second half, for TDCs
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  int i = 0;
  fDetMap->Clear();
  while (1) {
    Int_t crate, slot, first, last, first_chan,model;
    int pos;
    fgets ( buf, LEN, fi );
    sscanf( buf, "%d%d%d%d%d%n", &crate, &slot, &first, &last, &first_chan, &pos );
    model=atoi(buf+pos); // if there is no model number given, set to zero

    if( crate < 0 ) break;
    if( fDetMap->AddModule( crate, slot, first, last, first_chan, model ) < 0 ) {
      Error( Here(here), "Too many DetMap modules (maximum allowed - %d).", 
	     THaDetMap::kDetMapSize);
      fclose(fi);
      return kInitError;
    }
  }
  fgets ( buf, LEN, fi );
  
  // Read geometry

  Float_t x,y,z;
  fscanf ( fi, "%f%f%f", &x, &y, &z );             // Detector's X,Y,Z coord
  fOrigin.SetXYZ( x, y, z );
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  fscanf ( fi, "%f%f%f", fSize, fSize+1, fSize+2 );   // Sizes of det on X,Y,Z
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );

  Float_t angle;
  fscanf ( fi, "%f", &angle );                     // Rotation angle of det
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  const Double_t degrad = TMath::Pi()/180.0;
  tan_angle = TMath::Tan(angle*degrad);
  sin_angle = TMath::Sin(angle*degrad);
  cos_angle = TMath::Cos(angle*degrad);

  DefineAxes(angle*degrad);

  // Dimension arrays
  if( !fIsInit ) {
    // Calibration data
    fOff = new Float_t[ fNelem ];
    fPed = new Float_t[ fNelem ];
    fGain = new Float_t[ fNelem ];

    // Per-event data
    fT   = new Float_t[ fNelem ];
    fT_c = new Float_t[ fNelem ];
    fA   = new Float_t[ fNelem ];
    fA_p = new Float_t[ fNelem ];
    fA_c = new Float_t[ fNelem ];

    fIsInit = true;
  }

  // Read calibrations
  for (i=0;i<fNelem;i++) 
    fscanf( fi, "%f", fOff+i );                   // TDC offsets
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  for (i=0;i<fNelem;i++) 
    fscanf( fi, "%f", fPed+i );                   // ADC pedestals
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  for (i=0;i<fNelem;i++) 
    fscanf( fi, "%f", fGain+i);                   // ADC gains
  fgets ( buf, LEN, fi );

  fclose(fi);
  return kOK;
}

//_____________________________________________________________________________
Int_t THaCherenkov::DefineVariables( EMode mode )
{
  // Initialize global variables

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  RVarDef vars[] = {
    { "nthit",  "Number of Left paddles TDC times",  "fNThit" },
    { "nahit",  "Number of Right paddles TDC times", "fNAhit" },
    { "t",      "TDC values",                        "fT" },
    { "t_c",    "Corrected TDC values",              "fT_c" },
    { "a",      "ADC values",                        "fA" },
    { "a_p",    "Ped-subtracted ADC values ",        "fA_p" },
    { "a_c",    "Corrected ADC values",              "fA_c" },
    { "asum_p", "Sum of ADC minus pedestal values",  "fASUM_p" },
    { "asum_c", "Sum of corrected ADC amplitudes",   "fASUM_c" },
    { "trx",    "x-position of track in det plane",  "fTrackProj.THaTrackProj.fX" },
    { "try",    "y-position of track in det plane",  "fTrackProj.THaTrackProj.fY" },
    { "trpath", "TRCS pathlen of track to det plane","fTrackProj.THaTrackProj.fPathl" },
    { 0 }
  };
  return DefineVarsFromList( vars, mode );
}

//_____________________________________________________________________________
THaCherenkov::~THaCherenkov()
{
  // Destructor. Remove variables from global list.

  if( fIsSetup )
    RemoveVariables();
  if( fIsInit )
    DeleteArrays();
}

//_____________________________________________________________________________
void THaCherenkov::DeleteArrays()
{
  // Delete member arrays. Internal function used by destructor.

  delete [] fA_c;    fA_c    = NULL;
  delete [] fA_p;    fA_p    = NULL;
  delete [] fA;      fA      = NULL;
  delete [] fT_c;    fT_c    = NULL;
  delete [] fT;      fT      = NULL;

  delete [] fGain;   fGain   = NULL;
  delete [] fPed;    fPed    = NULL;
  delete [] fOff;    fOff    = NULL;
}

//_____________________________________________________________________________
inline 
void THaCherenkov::ClearEvent()
{
  // Reset all local data to prepare for next event.

  const int lf = fNelem*sizeof(Float_t);
  fNThit = 0;                             // Number of mirrors with TDC times
  memset( fT, 0, lf );                    // TDC times of channels
  memset( fT_c, 0, lf );                  // Corrected TDC times of channels
  fNAhit = 0;                             // Number of mirrors with ADC ampls
  memset( fA, 0, lf );                    // ADC amplitudes of channels
  memset( fA_p, 0, lf );                  // ADC minus ped values of channels
  memset( fA_c, 0, lf );                  // Corrected ADC amplitudes of chans
  fASUM_p = 0.0;                          // Sum of ADC minus pedestal values
  fASUM_c = 0.0;                          // Sum of corrected ADC amplitudes
  fTrackProj->Clear();
}

//_____________________________________________________________________________
Int_t THaCherenkov::Decode( const THaEvData& evdata )
{
  // Decode Cherenkov data, correct TDC times and ADC amplitudes, and copy
  // the data into the local data members.
  // This implementation assumes that the first half of the detector map 
  // entries corresponds to ADCs, and the second half, to TDCs.

  ClearEvent();

  // Loop over all modules defined for Cherenkov detector
  for( Int_t i = 0; i < fDetMap->GetSize(); i++ ) {
    THaDetMap::Module* d = fDetMap->GetModule( i );
    bool adc = (d->model ? fDetMap->IsADC(d) : i < fDetMap->GetSize()/2 );
    
    // Loop over all channels that have a hit.
    for( Int_t j = 0; j < evdata.GetNumChan( d->crate, d->slot ); j++) {

      Int_t chan = evdata.GetNextChan( d->crate, d->slot, j );
      if( chan < d->lo || chan > d->hi ) continue;     // Not one of my channels

      // Get the data. Aero mirrors are assumed to have only single hit (hit=0)
      Int_t data = evdata.GetData( d->crate, d->slot, chan, 0 );

      // Get the detector channel number, starting at 0
      Int_t k = d->first + chan - d->lo - 1;

#ifdef WITH_DEBUG
      if( k<0 || k>= fNelem ) {
	Warning( Here("Decode()"), "Illegal detector channel: %d", k );
        continue;
      }
#endif

      // Copy the data to the local variables.
      if ( adc ) {
	fA[k]   = data;
	fA_p[k] = data - fPed[k];
	fA_c[k] = fA_p[k] * fGain[k];
	// only add channels with signals to the sums
	if( fA_p[k] > 0.0 )
	  fASUM_p += fA_p[k];
	if( fA_c[k] > 0.0 )
	  fASUM_c += fA_c[k];
	fNAhit++;
      } else {
	fT[k]   = data;
	fT_c[k] = data - fOff[k];
	fNThit++;
      }
    }
  }

  if ( fDebug > 3 ) {
    printf("\nCherenkov %s:\n",GetPrefix());
    int ncol=3;
    for (int i=0; i<ncol; i++) {
      printf("  Mirror TDC   ADC  ADC_p  ");
    }
    printf("\n");
    
    for (int i=0; i<(fNelem+ncol-1)/ncol; i++ ) {
      for (int c=0; c<ncol; c++) {
	int ind = c*fNelem/ncol+i;
	if (ind < fNelem) {
	  printf("  %3d  %5.0f  %5.0f  %5.0f  ",ind+1,fT[ind],fA[ind],fA_p[ind]);
	} else {
	  //	  printf("\n");
	  break;
	}
      }
      printf("\n");
    }
  }
  
  return fNThit;
}

//_____________________________________________________________________________
Int_t THaCherenkov::CoarseProcess( TClonesArray& tracks )
{
  // Reconstruct coordinates of where a particle track crosses
  // the Cherenkov plane, and copy the point into the fTrackProj array.
  //
  // Calculation of coordinates of particle track cross point with Cherenkov
  // plane in the detector coordinate system. For this, parameters of track 
  // reconstructed in THaVDC::CrudeTrack() are used.

  int n_track = tracks.GetLast()+1;   // Number of reconstructed tracks

  for ( int i=0; i<n_track; i++ ) {
    THaTrack* theTrack = static_cast<THaTrack*>( tracks[i] );
    Double_t pathl=kBig, xc=kBig, yc=kBig;
    Double_t dx=0.; // unused
    Int_t pad=-1;   // unused
    
    CalcTrackIntercept(theTrack, pathl, xc, yc);
    // if it hit or not, store the information (defaults if no hit)
    new ( (*fTrackProj)[i] ) THaTrackProj(xc,yc,pathl,dx,pad,this);
  }

  return 0;
}

//_____________________________________________________________________________
Int_t THaCherenkov::FineProcess( TClonesArray& tracks )
{
  // Fine Cherenkov processing.
  // Redo the track-matching, since tracks might have been thrown out
  // during the FineTracking stage.
  fTrackProj->Clear();
  int n_track = tracks.GetLast()+1;   // Number of reconstructed tracks

  for ( int i=0; i<n_track; i++ ) {
    THaTrack* theTrack = static_cast<THaTrack*>( tracks[i] );
    Double_t pathl=kBig, xc=kBig, yc=kBig;
    Double_t dx=0.; // unused
    Int_t pad=-1;   // unused
    
    CalcTrackIntercept(theTrack, pathl, xc, yc);
    // if it hit or not, store the information (defaults if no hit)
    new ( (*fTrackProj)[i] ) THaTrackProj(xc,yc,pathl,dx,pad,this);
  }

  return 0;
}

//_____________________________________________________________________________
Int_t THaCherenkov::GetNTracks() const
{
  return fTrackProj->GetLast()+1;
}
///////////////////////////////////////////////////////////////////////////////
