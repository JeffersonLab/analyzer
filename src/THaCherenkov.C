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
#include "TClonesArray.h"
#include "TDatime.h"
#include "TMath.h"
#include <cstring>

ClassImp(THaCherenkov)

//_____________________________________________________________________________
THaCherenkov::THaCherenkov( const char* name, const char* description,
			THaApparatus* apparatus )
  : THaPidDetector(name,description,apparatus), fFirstChan(NULL)
{
  // Constructor
}

//_____________________________________________________________________________
Int_t THaCherenkov::ReadDatabase( FILE* fi, const TDatime& date )
{
  // Read this detector's parameters from the database file 'fi'.
  // This function is called by THaDetectorBase::Init() once at the
  // beginning of the analysis.
  // 'date' contains the date/time of the run being analyzed.

  static const char* const here = "ReadDatabase()";

  // Read database

  const int LEN = 100;
  char buf[LEN];
  Int_t nelem;

  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  fscanf ( fi, "%d", &nelem );                      // Number of mirrors

  // Reinitialization only possible for same basic configuration 
  if( fIsInit && nelem != fNelem ) {
    Error( Here(here), "Cannot re-initalize with different number of mirrors. "
	   "(was: %d, now: %d). Detector not re-initialized.", fNelem, nelem );
    return kInitError;
  }
  fNelem = nelem;

  // Read detector map.  Assumes that the first half of the entries 
  // is for ADCs, and the second half, for TDCs
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  int i = 0;
  delete [] fFirstChan;
  fFirstChan = new UShort_t[ THaDetMap::kDetMapSize ];
  while (1) {
    Int_t crate, slot, first, last, first_chan;
    fscanf ( fi,"%d%d%d%d%d", 
	     &crate, &slot, &first, &last, &first_chan );
    fgets ( buf, LEN, fi );
    if( crate < 0 ) break;
    if( fDetMap->AddModule( crate, slot, first, last ) < 0 ) {
      Error( Here(here), "Too many DetMap modules (maximum allowed - %d).", 
	     THaDetMap::kDetMapSize);
      delete [] fFirstChan; fFirstChan = NULL;
      return kInitError;
    }
    fFirstChan[i++] = first_chan;
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

  return kOK;
}

//_____________________________________________________________________________
Int_t THaCherenkov::SetupDetector( const TDatime& date )
{
  // Initialize global variables

  if( fIsSetup ) return kOK;
  fIsSetup = true;

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
    { "trx",    "x-position of track in det plane",  "fTRX" },
    { "try",    "y-position of track in det plane",  "fTRY" },
    { 0 }
  };
  DefineVariables( vars );

  return fStatus = kOK;
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
  delete [] fFirstChan; fFirstChan = NULL;
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
  fTRX = 0.0;                             // Xcrd of track point on Aero plane
  fTRY = 0.0;                             // Ycrd of track point on Aero plane
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
    bool adc = (i < fDetMap->GetSize()/2);

    // Loop over all channels that have a hit.
    for( Int_t j = 0; j < evdata.GetNumChan( d->crate, d->slot ); j++) {

      Int_t chan = evdata.GetNextChan( d->crate, d->slot, j );
      if( chan < d->lo || chan > d->hi ) continue;     // Not one of my channels

      // Get the data. Aero mirrors are assumed to have only single hit (hit=0)
      Int_t data = evdata.GetData( d->crate, d->slot, chan, 0 );

      // Get the detector channel number, starting at 0
      Int_t k = fFirstChan[i] + chan - d->lo - 1;

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
	fASUM_p += fA_p[k];
	fASUM_c += fA_c[k];
	fNAhit++;
      } else {
	fT[k]   = data;
	fT_c[k] = data - fOff[k];
	fNThit++;
      }
    }
  }
  return fNThit;
}

//_____________________________________________________________________________
Int_t THaCherenkov::CoarseProcess( TClonesArray& tracks )
{
  // Reconstruct coordinates of particle track cross point with Cherenkov plane,
  // and copy the data into the following local data structure:
  //
  // fTRX  -  X-coordinate of particle track cross point with Cherenkov plane
  // fTRY  -  Y-coordinate of particle track cross point with Cherenkov plane
  //
  // Units of measurements are centimeters.

  // Calculation of coordinates of particle track cross point with Cherenkov
  // plane in the detector coordinate system. For this, parameters of track 
  // reconstructed in THaVDC::CrudeTrack() are used.

  int ne_track = tracks.GetLast()+1;   // Number of reconstructed in Earm tracks

  if ( ne_track == 1 ) {
    THaTrack* theTrack = static_cast<THaTrack*>( tracks[0] );
    double fpe_x       = theTrack->GetX();
    double fpe_y       = theTrack->GetY();
    double fpe_th      = theTrack->GetTheta();
    double fpe_ph      = theTrack->GetPhi();

    fTRX = ( fpe_x + fpe_th * fOrigin.Z() ) / 
      ( cos_angle * (1 + fpe_th * tan_angle) );
    fTRY = fpe_y + fpe_ph * fOrigin.Z() 
      - fpe_ph * sin_angle * fTRX;
  }

  return 0;
}

//_____________________________________________________________________________
Int_t THaCherenkov::FineProcess( TClonesArray& tracks )
{
  // Fine Cherenkov processing.
  // Not implemented. Does nothing, returns 0.

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
