///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// THaScintillator                                                           //
//                                                                           //
// Class for a generic scintillator (hodoscope) consisting of multiple       //
// paddles with phototubes on both ends.                                     //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaScintillator.h"
#include "THaEvData.h"
#include "THaDetMap.h"
#include "VarDef.h"
#include "VarType.h"
#include "THaTrack.h"
#include "TClonesArray.h"
#include "TMath.h"

// #include <iostream>
#include <cstring>

ClassImp(THaScintillator)

//_____________________________________________________________________________
THaScintillator::THaScintillator( const char* name, const char* description,
				  THaApparatus* apparatus ) :
  THaNonTrackingDetector(name,description,apparatus), fFirstChan(NULL)
{
  // Constructor
}

//_____________________________________________________________________________
THaAnalysisObject::EStatus THaScintillator::Init( const TDatime& date )
{
  // Extra initialization for scintillators: set up DataDest map

  if( THaNonTrackingDetector::Init( date ) )
    return fStatus;

  const DataDest tmp[NDEST] = {
    { &fRTNhit, &fRANhit, fRT, fRT_c, fRA, fRA_p, fRA_c, fROff, fRPed, fRGain },
    { &fLTNhit, &fLANhit, fLT, fLT_c, fLA, fLA_p, fLA_c, fLOff, fLPed, fLGain }
  };
  memcpy( fDataDest, tmp, NDEST*sizeof(DataDest) );

  return fStatus = kOK;
}

//_____________________________________________________________________________
Int_t THaScintillator::ReadDatabase( FILE* fi, const TDatime& date )
{
  // Read this detector's parameters from the database file 'fi'.
  // This function is called by THaDetectorBase::Init() once at the
  // beginning of the analysis.
  // 'date' contains the date/time of the run being analyzed.

  static const char* const here = "ReadDatabase()";
  const int LEN = 100;
  char buf[LEN];
  Int_t nelem;

  // Read data from database 

  rewind( fi );
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  fscanf ( fi, "%d", &nelem );                        // Number of  paddles

  // Reinitialization only possible for same basic configuration 
  if( fIsInit && nelem != fNelem ) {
    Error( Here(here), "Cannot re-initalize with different number of paddles. "
	   "(was: %d, now: %d). Detector not re-initialized.", fNelem, nelem );
    return kInitError;
  }
  fNelem = nelem;

  // Read detector map. Assumes that the first half of the entries 
  // is for ADCs and the second half, for TDCs
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  int i = 0;
  delete [] fFirstChan;
  fFirstChan = new UShort_t[ THaDetMap::kDetMapSize ];
  fDetMap->Clear();
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

  Float_t x,y,z;
  fscanf ( fi, "%f%f%f", &x, &y, &z );             // Detector's X,Y,Z coord
  fOrigin.SetXYZ( x, y, z );
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  fscanf ( fi, "%f%f%f", fSize, fSize+1, fSize+2 ); // Sizes of det on X,Y,Z
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );

  Float_t angle;
  fscanf ( fi, "%f", &angle );                     // Rotation angle of detector
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  const Float_t degrad = TMath::Pi()/180.0;
  tan_angle = TMath::Tan(angle*degrad);
  sin_angle = TMath::Sin(angle*degrad);
  cos_angle = TMath::Cos(angle*degrad);

  DefineAxes(0.0);

  // Dimension arrays
  if( !fIsInit ) {
    // Calibration data
    fLOff  = new Float_t[ fNelem ];
    fROff  = new Float_t[ fNelem ];
    fLPed  = new Float_t[ fNelem ];
    fRPed  = new Float_t[ fNelem ];
    fLGain = new Float_t[ fNelem ];
    fRGain = new Float_t[ fNelem ];

    // Per-event data
    fLT   = new Float_t[ fNelem ];
    fLT_c = new Float_t[ fNelem ];
    fRT   = new Float_t[ fNelem ];
    fRT_c = new Float_t[ fNelem ];
    fLA   = new Float_t[ fNelem ];
    fLA_p = new Float_t[ fNelem ];
    fLA_c = new Float_t[ fNelem ];
    fRA   = new Float_t[ fNelem ];
    fRA_p = new Float_t[ fNelem ];
    fRA_c = new Float_t[ fNelem ];

    fIsInit = true;
  }

  // Read calibration data
  for (i=0;i<fNelem;i++) 
    fscanf(fi,"%f",fLOff+i);                    // Left Pads TDC offsets
  fgets ( buf, LEN, fi );
  for (i=0;i<fNelem;i++) 
    fscanf(fi,"%f",fROff+i);                    // Right Pads TDC offsets
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  for (i=0;i<fNelem;i++) 
    fscanf(fi,"%f",fLPed+i);                    // Left Pads ADC Pedest-s
  fgets ( buf, LEN, fi );
  for (i=0;i<fNelem;i++) 
    fscanf(fi,"%f",fRPed+i);                    // Right Pads ADC Pedes-s
  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  for (i=0;i<fNelem;i++) 
    fscanf (fi,"%f",fLGain+i);                  // Left Pads ADC Coeff-s
  fgets ( buf, LEN, fi );
  for (i=0;i<fNelem;i++) 
    fscanf (fi,"%f",fRGain+i);                  // Right Pads ADC Coeff-s
  fgets ( buf, LEN, fi );

  return kOK;
}

//_____________________________________________________________________________
Int_t THaScintillator::DefineVariables( EMode mode )
{
  // Initialize global variables and lookup table for decoder

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  // Register variables in global list

  RVarDef vars[] = {
    { "nlthit", "Number of Left paddles TDC times",  "fLTNhit" },
    { "nrthit", "Number of Right paddles TDC times", "fRTNhit" },
    { "nlahit", "Number of Left paddles ADCs amps",  "fLANhit" },
    { "nrahit", "Number of Right paddles ADCs amps", "fRANhit" },
    { "lt",     "TDC values left side",              "fLT" },
    { "lt_c",   "Corrected TDC values left side",    "fLT_c" },
    { "rt",     "TDC values right side",             "fRT" },
    { "rt_c",   "Corrected TDC values right side",   "fRT_c" },
    { "la",     "ADC values left side",              "fLA" },
    { "la_p",   "Corrected ADC values left side",    "fLA_p" },
    { "la_c",   "Corrected ADC values left side",    "fLA_c" },
    { "ra",     "ADC values right side",             "fRA" },
    { "ra_p",   "Corrected ADC values right side",   "fRA_p" },
    { "ra_c",   "Corrected ADC values right side",   "fRA_c" },
    { "trx",    "x-position of track in det plane",  "fTRX" },
    { "try",    "y-position of track in det plane",  "fTRY" },
    { 0 }
  };
  return DefineVarsFromList( vars, mode );
}

//_____________________________________________________________________________
THaScintillator::~THaScintillator()
{
  // Destructor. Remove variables from global list.

  if( fIsSetup )
    RemoveVariables();
  if( fIsInit )
    DeleteArrays();
}

//_____________________________________________________________________________
void THaScintillator::DeleteArrays()
{
  // Delete member arrays. Used by destructor.

  delete [] fRA_c;    fRA_c    = NULL;
  delete [] fRA_p;    fRA_p    = NULL;
  delete [] fRA;      fRA      = NULL;
  delete [] fLA_c;    fLA_c    = NULL;
  delete [] fLA_p;    fLA_p    = NULL;
  delete [] fLA;      fLA      = NULL;
  delete [] fRT_c;    fRT_c    = NULL;
  delete [] fRT;      fRT      = NULL;
  delete [] fLT_c;    fLT_c    = NULL;
  delete [] fLT;      fLT      = NULL;

  delete [] fRGain;   fRGain   = NULL;
  delete [] fLGain;   fLGain   = NULL;
  delete [] fRPed;    fRPed    = NULL;
  delete [] fLPed;    fLPed    = NULL;
  delete [] fROff;    fROff    = NULL;
  delete [] fLOff;    fLOff    = NULL;
  delete [] fFirstChan; fFirstChan = NULL;
}

//_____________________________________________________________________________
inline 
void THaScintillator::ClearEvent()
{
  // Reset per-event data.

  const int lf = fNelem*sizeof(Float_t);
  fLTNhit = 0;                            // Number of Left paddles TDC times
  memset( fLT, 0, lf );                   // Left paddles TDCs
  memset( fLT_c, 0, lf );                 // Left paddles corrected TDCs
  fRTNhit = 0;                            // Number of Right paddles TDC times
  memset( fRT, 0, lf );                   // Right paddles TDCs
  memset( fRT_c, 0, lf );                 // Right paddles corrected TDCs
  fLANhit = 0;                            // Number of Left paddles ADC amps
  memset( fLA, 0, lf );                   // Left paddles ADCs
  memset( fLA_p, 0, lf );                 // Left paddles ADC minus pedestal
  memset( fLA_c, 0, lf );                 // Left paddles corrected ADCs
  fRANhit = 0;                            // Number of Right paddles ADC smps
  memset( fRA, 0, lf );                   // Right paddles ADCs
  memset( fRA_p, 0, lf );                 // Right paddles ADC minus pedestal
  memset( fRA_c, 0, lf );                 // Right paddles corrected ADCs
  fTRX = 0.0;                             // Xcoord of track point on scint plane
  fTRY = 0.0;                             // Ycoord of track point on scint plane
}

//_____________________________________________________________________________
Int_t THaScintillator::Decode( const THaEvData& evdata )
{
  // Decode scintillator data, correct TDC times and ADC amplitudes, and copy
  // the data to the local data members.
  // This implementation makes the following assumptions about the detector map:
  // - The first half of the map entries corresponds to ADCs, 
  //   the second half, to TDCs.
  // - The first fNelem detector channels correspond to the PMTs on the
  //   right hand side, the next fNelem channels, to the left hand side.
  //   (Thus channel numbering for each module must be consecutive.)

  ClearEvent();

  // Loop over all modules defined for Scintillator 1 detector

  for( Int_t i = 0; i < fDetMap->GetSize(); i++ ) {
    THaDetMap::Module* d = fDetMap->GetModule( i );
    bool adc = (i < fDetMap->GetSize()/2);

    // Loop over all channels that have a hit.
    for( Int_t j = 0; j < evdata.GetNumChan( d->crate, d->slot ); j++) {

      Int_t chan = evdata.GetNextChan( d->crate, d->slot, j );
      if( chan < d->lo || chan > d->hi ) continue;     // Not one of my channels

      // Get the data. Scintillators are assumed to have only single hit (hit=0)
      Int_t data = evdata.GetData( d->crate, d->slot, chan, 0 );

      // Get the detector channel number, starting at 0
      Int_t k = fFirstChan[i] + chan - d->lo - 1;   

#ifdef WITH_DEBUG      
      if( k<0 || k>NDEST*fNelem ) {
	// Indicates bad database
	Warning( Here("Decode()"), "Illegal detector channel: %d", k );
	continue;
      }
//        cout << "adc,j,k = " <<adc<<","<<j<< ","<<k<< endl;
#endif
      // Copy the data to the local variables.
      DataDest* dest = fDataDest + k/fNelem;
      k = k % fNelem;
      if( adc ) {
	dest->adc[k]   = static_cast<Float_t>( data );
	dest->adc_p[k] = data - dest->ped[k];
	dest->adc_c[k] = dest->adc_p[k] * dest->gain[k];
	(*dest->nahit)++;
      } else {
	dest->tdc[k]   = static_cast<Float_t>( data );
	dest->tdc_c[k] = data - dest->offset[k];
	(*dest->nthit)++;
      }
    }
  }
  return fLTNhit+fRTNhit;
}

//_____________________________________________________________________________
Int_t THaScintillator::CoarseProcess( TClonesArray& tracks )
{
  // Reconstruct coordinates of particle track cross point with scintillator
  // plane, and copy the data into the following local data structure:
  //
  // fTRX  -  X-coordinate of particle track cross point with scint plane;
  // fTRY  -  Y-coordinate of particle track cross point with scint plane.
  //
  // Units of measurements are centimeters.

  // Calculation of coordinates of particle track cross point with scint
  // plane in the detector coordinate system. For this, parameters of track 
  // reconstructed in THaVDC::CoarseTrack() are used.

  int ne_track = tracks.GetLast()+1;   // Number of reconstructed in Earm tracks

  if ( ne_track > 0 ) {
    THaTrack* theTrack = static_cast<THaTrack*>( tracks[0] );
    double fpe_x       = theTrack->GetX();
    double fpe_y       = theTrack->GetY();
    double fpe_th      = theTrack->GetTheta();
    double fpe_ph      = theTrack->GetPhi();

    fTRX = ( fpe_x + fpe_th * fOrigin.Z() ) / 
      ( cos_angle * (1.0 + fpe_th * tan_angle ) );
    fTRY = fpe_y + fpe_ph * fOrigin.Z() - fpe_ph * sin_angle * fTRX;
   }

  return 0;
}

//_____________________________________________________________________________
Int_t THaScintillator::FineProcess( TClonesArray& tracks )
{
  // Scintillator fine processing.
  // Not implemented. Does nothing, returns 0.

  return 0;
}

////////////////////////////////////////////////////////////////////////////////
