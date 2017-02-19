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

#include "THaTrackProj.h"

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <cassert>
#include <iomanip>

using namespace std;

//_____________________________________________________________________________
THaScintillator::THaScintillator( const char* name, const char* description,
				  THaApparatus* apparatus )
  : THaNonTrackingDetector(name,description,apparatus),
    fLOff(0), fROff(0), fLPed(0), fRPed(0), fLGain(0), fRGain(0),
    fNTWalkPar(0), fTWalkPar(0), fTrigOff(0),
    fLTNhit(0), fLT(0), fLT_c(0), fRTNhit(0), fRT(0), fRT_c(0),
    fLANhit(0), fLA(0), fLA_p(0), fLA_c(0),
    fRANhit(0), fRA(0), fRA_p(0), fRA_c(0),
    fNhit(0), fHitPad(0), fTime(0), fdTime(0), fAmpl(0), fYt(0), fYa(0),
    tan_angle(0), sin_angle(0), cos_angle(1.)
{
  // Constructor
  fTrackProj = new TClonesArray( "THaTrackProj", 5 );
}

//_____________________________________________________________________________
THaScintillator::THaScintillator( )
  : THaNonTrackingDetector(),
    fLOff(0), fROff(0), fLPed(0), fRPed(0), fLGain(0), fRGain(0),
    fNTWalkPar(0), fTWalkPar(0), fTrigOff(0),
    fLTNhit(0), fLT(0), fLT_c(0), fRTNhit(0), fRT(0), fRT_c(0),
    fLANhit(0), fLA(0), fLA_p(0), fLA_c(0),
    fRANhit(0), fRA(0), fRA_p(0), fRA_c(0),
    fNhit(0), fHitPad(0), fTime(0), fdTime(0), fAmpl(0), fYt(0), fYa(0),
    fTrackProj(0)
{
  // Default constructor (for ROOT I/O)
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
Int_t THaScintillator::DoReadDatabase( FILE* fi, const TDatime& date )
{
  // Read this detector's parameters from the database file 'fi'.
  // This function is called by THaDetectorBase::Init() once at the
  // beginning of the analysis.
  // 'date' contains the date/time of the run being analyzed.

  const char* const here = "ReadDatabase";
  const int LEN = 256;
  char buf[LEN];
  Int_t nelem;

  Int_t flags = kErrOnTooManyValues|kWarnOnDataGuess;

  //  fDetMapHasModel = fHaveExtras = false;

  if( ReadBlock(fi,&nelem,1,here,flags|kNoNegativeValues) ) // Number of paddles
    return kInitError;

  // Reinitialization only possible for same basic configuration
  if( fIsInit && nelem != fNelem ) {
    Error( Here(here), "Cannot re-initalize with different number of paddles. "
	   "(was: %d, now: %d). Detector not re-initialized.", fNelem, nelem );
    return kInitError;
  }
  fNelem = nelem;

  if( fIsInit )
    ClearEvent();

  // Read detector map. Unless a model-number is given
  // for the detector type, this assumes that the first half of the entries
  // are for ADCs and the second half, for TDCs.
  while( ReadComment(fi, buf, LEN) ) {}
  fDetMap->Clear();
  while (1) {
    Int_t pos, first_chan, model, crate, slot, first, last;
    fgets ( buf, LEN, fi );
    Int_t n = sscanf( buf, "%6d %6d %6d %6d %6d %n",
		      &crate, &slot, &first, &last, &first_chan, &pos );
    if( crate < 0 ) break;
    if( n < 5 ) return ErrPrint(fi,here);
    model=atoi(buf+pos); // if there is no model number given, set to zero
    // if( model != 0 )
    //   fDetMapHasModel = true;
    if( fDetMap->AddModule( crate, slot, first, last, first_chan, model ) < 0 ) {
      Error( Here(here), "Too many DetMap modules (maximum allowed - %d).",
	     THaDetMap::kDetMapSize);
      return kInitError;
    }
  }
  if( fDetMap->GetTotNumChan() != 4*nelem ) {
    Error( Here(here), "Database inconsistency.\n Defined %d channels in detector map, "
	   "but have %d total channels (%d paddles with 2 ADCs and 2 TDCs each)",
	   fDetMap->GetTotNumChan(), 4*nelem, nelem );
    return kInitError;
  }

  // Read geometry
  Double_t dvals[3], angle;
  if( ReadBlock(fi,dvals,3,here,flags) )          // Detector X,Y,Z coordinates
    return kInitError;
  fOrigin.SetXYZ( dvals[0], dvals[1], dvals[2] );

  if( ReadBlock(fi,fSize,3,here,flags|kNoNegativeValues) ) // Size of detector in X,Y,Z
    return kInitError;

  if( ReadBlock(fi,&angle,1,here,flags) )         // Rotation angle of detector
    return kInitError;

  const Double_t degrad = TMath::Pi()/180.0;
  tan_angle = TMath::Tan(angle*degrad);
  sin_angle = TMath::Sin(angle*degrad);
  cos_angle = TMath::Cos(angle*degrad);

  DefineAxes(angle*degrad);

  // Dimension arrays when initializing for the first time
  if( !fIsInit ) {
    fNTWalkPar = 2*fNelem; // 1 parameter per paddle

    // Calibration data
    fLOff     = new Double_t[ fNelem ];
    fROff     = new Double_t[ fNelem ];
    fLPed     = new Double_t[ fNelem ];
    fRPed     = new Double_t[ fNelem ];
    fLGain    = new Double_t[ fNelem ];
    fRGain    = new Double_t[ fNelem ];
    fTrigOff  = new Double_t[ fNelem ];
    fTWalkPar = new Double_t[ fNTWalkPar ];

    // Per-event data
    fLT       = new Double_t[ fNelem ];
    fLT_c     = new Double_t[ fNelem ];
    fRT       = new Double_t[ fNelem ];
    fRT_c     = new Double_t[ fNelem ];
    fLA       = new Double_t[ fNelem ];
    fLA_p     = new Double_t[ fNelem ];
    fLA_c     = new Double_t[ fNelem ];
    fRA       = new Double_t[ fNelem ];
    fRA_p     = new Double_t[ fNelem ];
    fRA_c     = new Double_t[ fNelem ];

    fHitPad   = new Int_t[ fNelem ];
    fTime     = new Double_t[ fNelem ]; // analysis indexed by paddle (yes, inefficient)
    fdTime    = new Double_t[ fNelem ];
    fAmpl     = new Double_t[ fNelem ];
    fYt       = new Double_t[ fNelem ];
    fYa       = new Double_t[ fNelem ];

    fIsInit = true;
    ClearEvent();
  }

  // Set DEFAULT values here
  // TDC resolution (s/channel)
  fTdc2T = 0.1e-9;      // seconds/channel
  fResolution = fTdc2T; // actual timing resolution
  // Speed of light in the scintillator material
  fCn = 1.7e+8;         // meters/second
  // Attenuation length
  fAttenuation = 0.7;   // inverse meters
  // Time-walk correction parameters
  fAdcMIP = 1.e10;      // large number for offset, so reference is effectively disabled
  // timewalk coefficients for tw = coeff*(1./sqrt(ADC-Ped)-1./sqrt(ADCMip))
  memset( fTWalkPar,0, fNTWalkPar*sizeof(fTWalkPar[0]) );
  // trigger-timing offsets (s)
  memset( fTrigOff, 0, fNelem*sizeof(fTrigOff[0]) );

  // Read in the timing/adc calibration constants
  // For fine-tuning of these data, we seek to a matching time stamp, or
  // if no time stamp found, to a "configuration" section. Examples:
  //
  // [ 2002-10-10 15:30:00 ]
  // #comment line goes here
  // <left TDC offsets>
  // <right TDC offsets>
  // <left ADC peds>
  // <rigth ADC peds>
  // <left ADC coeff>
  // <right ADC coeff>
  //
  // if below aren't present, 'default' values are used
  // <TDC resolution: seconds/channel>
  // <speed-of-light in medium m/s>
  // <attenuation length m^-1>
  // <ADC of MIP>
  // <number of timewalk parameters>
  // <timewalk paramters>
  //
  //
  // or
  //
  // [ config=highmom ]
  // comment line
  // ...etc.
  //
  TDatime datime(date);
  if( SeekDBdate( fi, datime ) == 0 && fConfig.Length() > 0 &&
      SeekDBconfig( fi, fConfig.Data() )) {}

  // Read calibration data

  // Most scintillator databases have two data lines in each section, one for left,
  // one for right paddles, without a separator comment. Hence we read the first
  // line without the check for "too many values".

  // Left pads TDC offsets
  Int_t err = ReadBlock(fi,fLOff,fNelem,here,flags|kStopAtNval|kQuietOnTooMany);
  if( err ) {
    if( fNelem > 1 || err != kTooManyValues )
      return kInitError;
    // Attempt to recover S0 databases with nelem == 1 that have L/R values
    // on a single line
    Double_t dval[2];
    if( ReadBlock(fi,dval,2,here,flags) )
      return kInitError;
    fLOff[0] = dval[0];
    fROff[0] = dval[1];
  } else {
    // Right pads TDC offsets
    if( ReadBlock(fi,fROff,fNelem,here,flags) )
      return kInitError;
  }

  // Left pads ADC pedestals
  err = ReadBlock(fi,fLPed,fNelem,here,flags|kNoNegativeValues|kStopAtNval|kQuietOnTooMany);
  if( err ) {
    if( fNelem > 1 || err != kTooManyValues )
      return kInitError;
    Double_t dval[2];
    if( ReadBlock(fi,dval,2,here,flags|kNoNegativeValues) )
      return kInitError;
    fLPed[0] = dval[0];
    fRPed[0] = dval[1];
  } else {
    // Right pads ADC pedestals
    if( ReadBlock(fi,fRPed,fNelem,here,flags|kNoNegativeValues) )
      return kInitError;
  }

  // Left pads ADC gains
  err = ReadBlock(fi,fLGain,fNelem,here,flags|kNoNegativeValues|kStopAtNval|kQuietOnTooMany);
  if( err ) {
    if( fNelem > 1 || err != kTooManyValues )
      return kInitError;
    Double_t dval[2];
    if( ReadBlock(fi,dval,2,here,flags|kNoNegativeValues) )
      return kInitError;
    fLGain[0] = dval[0];
    fRGain[0] = dval[1];
  } else {
    // Right pads ADC gains
    if( ReadBlock(fi,fRGain,fNelem,here,flags|kNoNegativeValues) )
      return kInitError;
  }

  // Here on down, look ahead line-by-line
  // stop reading if a '[' is found on a line (corresponding to the next date-tag)

  flags = kErrOnTooManyValues|kQuietOnTooFew|kStopAtNval|kStopAtSection;
  // TDC resolution (s/channel)
  if( (err = ReadBlock(fi,&fTdc2T,1,here,flags|kRequireGreaterZero)) )
    goto exit;
  //  fHaveExtras = true; // FIXME: should always be true?
  // Speed of light in the scintillator material (m/s)
  if( (err = ReadBlock(fi,&fCn,1,here,flags|kRequireGreaterZero)) )
    goto exit;
  // Attenuation length (inverse meters)
  if( (err = ReadBlock(fi,&fAttenuation,1,here,flags|kRequireGreaterZero)) )
    goto exit;
  // Typical ADC value for minimum ionizing particle (MIP)
  if( (err = ReadBlock(fi,&fAdcMIP,1,here,flags|kRequireGreaterZero)) )
    goto exit;
  // Timewalk coefficients (1 for each PMT) -- seconds*1/sqrt(ADC)
  if( (err = ReadBlock(fi,fTWalkPar,fNTWalkPar,here,flags)) )
    goto exit;
  // Trigger-timing offsets -- seconds offset per paddle
  err = ReadBlock(fi,fTrigOff,fNelem,here,flags);

 exit:
  if( err && err != kNoValues )
    return kInitError;

  // Debug printout
  if ( fDebug > 1 ) {
    const UInt_t N = static_cast<UInt_t>(fNelem);
    Double_t pos[3]; fOrigin.GetXYZ(pos);
    DBRequest list[] = {
      { "Number of paddles",    &fNelem,     kInt         },
      { "Detector position",    pos,         kDouble, 3   },
      { "Detector size",        fSize,       kFloat,  3   },
      { "Detector angle",       &angle,                   },
      { "TDC offsets Left",     fLOff,       kDouble, N   },
      { "TDC offsets Right",    fROff,       kDouble, N   },
      { "ADC pedestals Left",   fLPed,       kDouble, N   },
      { "ADC pedestals Right",  fRPed,       kDouble, N   },
      { "ADC gains Left",       fLGain,      kDouble, N   },
      { "ADC gains Right",      fRGain,      kDouble, N   },
      { "TDC resolution",       &fTdc2T                   },
      { "Light propag. speed",  &fCn                      },
      { "ADC MIP",              &fAdcMIP                  },
      { "Num timewalk params",  &fNTWalkPar, kInt         },
      { "Timewalk params",      fTWalkPar,   kDouble, 2*N },
      { "Trigger time offsets", fTrigOff,    kDouble, N   },
      { "Time resolution",      &fResolution              },
      { "Attenuation",          &fAttenuation             },
      { 0 }
    };
    DebugPrint( list );
  }

  return kOK;
}

//_____________________________________________________________________________
Int_t THaScintillator::ReadDatabase( const TDatime& date )
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
    { "lt_c",   "Corrected times left side",         "fLT_c" },
    { "rt",     "TDC values right side",             "fRT" },
    { "rt_c",   "Corrected times right side",        "fRT_c" },
    { "la",     "ADC values left side",              "fLA" },
    { "la_p",   "Corrected ADC values left side",    "fLA_p" },
    { "la_c",   "Corrected ADC values left side",    "fLA_c" },
    { "ra",     "ADC values right side",             "fRA" },
    { "ra_p",   "Corrected ADC values right side",   "fRA_p" },
    { "ra_c",   "Corrected ADC values right side",   "fRA_c" },
    { "nthit",  "Number of paddles with l&r TDCs",   "fNhit" },
    { "t_pads", "Paddles with l&r coincidence TDCs", "fHitPad" },
    { "y_t",    "y-position from timing (m)",        "fYt" },
    { "y_adc",  "y-position from amplitudes (m)",    "fYa" },
    { "time",   "Time of hit at plane (s)",          "fTime" },
    { "dtime",  "Est. uncertainty of time (s)",      "fdTime" },
    { "dedx",   "dEdX-like deposited in paddle",     "fAmpl" },
    { "troff",  "Trigger offset for paddles",        "fTrigOff"},
    { "trn",    "Number of tracks for hits",         "GetNTracks()" },
    { "trx",    "x-position of track in det plane",  "fTrackProj.THaTrackProj.fX" },
    { "try",    "y-position of track in det plane",  "fTrackProj.THaTrackProj.fY" },
    { "trpath", "TRCS pathlen of track to det plane","fTrackProj.THaTrackProj.fPathl" },
    { "trdx",   "track deviation in x-position (m)", "fTrackProj.THaTrackProj.fdX" },
    { "trpad",  "paddle-hit associated with track",  "fTrackProj.THaTrackProj.fChannel" },
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
  if (fTrackProj) {
    fTrackProj->Clear();
    delete fTrackProj; fTrackProj = 0;
  }
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
  delete [] fTWalkPar; fTWalkPar = NULL;
  delete [] fTrigOff; fTrigOff = NULL;

  delete [] fHitPad;  fHitPad  = NULL;
  delete [] fTime;    fTime    = NULL;
  delete [] fdTime;   fdTime   = NULL;
  delete [] fAmpl;    fAmpl    = NULL;
  delete [] fYt;      fYt      = NULL;
  delete [] fYa;      fYa      = NULL;
}

//_____________________________________________________________________________
inline 
void THaScintillator::ClearEvent()
{
  // Reset per-event data.

  fNhit = fLTNhit = fRTNhit = fLANhit = fRANhit = 0;
  assert(fIsInit);
  for( Int_t i=0; i<fNelem; ++i ) {
    fLT[i] = fLT_c[i] = fRT[i] = fRT_c[i] = kBig;
    fLA[i] = fLA_p[i] = fLA_c[i] = fRA[i] = fRA_p[i] = fRA_c[i] = kBig;
    fTime[i] = fdTime[i] = fAmpl[i] = fYt[i] = fYa[i] = kBig;
  }
  memset( fHitPad, 0, fNelem*sizeof(fHitPad[0]) );
  
  fTrackProj->Clear();
}

//_____________________________________________________________________________
#ifdef WITH_DEBUG
static void WriteValue( Double_t val )
{
  // Helper function for printing debug information
  if( val < THaAnalysisObject::kBig )
    cout << fixed << setprecision(0) << setw(5) << val;
  else
    cout << " --- ";
}
#endif

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

  static const char* const here = "Decode";

  ClearEvent();

  // Loop over all modules defined for Scintillator 1 detector

  for( Int_t i = 0; i < fDetMap->GetSize(); i++ ) {
    THaDetMap::Module* d = fDetMap->GetModule( i );
    bool adc = ( d->model ? fDetMap->IsADC(d) : (i < fDetMap->GetSize()/2) );

    // Loop over all channels that have a hit.
    for( Int_t j = 0; j < evdata.GetNumChan( d->crate, d->slot ); j++) {

      Int_t chan = evdata.GetNextChan( d->crate, d->slot, j );
      if( chan < d->lo || chan > d->hi ) continue;     // Not one of my channels

#ifdef WITH_DEBUG      
      Int_t nhit = evdata.GetNumHits(d->crate, d->slot, chan);
      // if( nhit > 1 )
      if( nhit > 3 )
	Warning( Here(here), "Event %d: %d hits on %s channel %d/%d/%d",
		 evdata.GetEvNum(),
		 nhit, adc ? "ADC" : "TDC", d->crate, d->slot, chan );
#endif
      // Get the data. Scintillators are assumed to have only single hit (hit=0)
      Int_t data = evdata.GetData( d->crate, d->slot, chan, 0 );

      // Get the detector channel number, starting at 0
      Int_t k = d->first + ((d->reverse) ? d->hi - chan : chan - d->lo) - 1;

      if( k<0 || k>NDEST*fNelem ) {
	// Indicates bad database
	Warning( Here(here), "Event %d: Illegal detector channel: %d", evdata.GetEvNum(), k );
	continue;
      }
//        cout << "adc,j,k = " <<adc<<","<<j<< ","<<k<< endl;
      // Copy the data to the local variables.
      DataDest* dest = fDataDest + k/fNelem;
      k = k % fNelem;
      if( adc ) {
	dest->adc[k]   = static_cast<Double_t>( data );
	dest->adc_p[k] = data - dest->ped[k];
	dest->adc_c[k] = dest->adc_p[k] * dest->gain[k];
	(*dest->nahit)++;
      } else {
	dest->tdc[k]   = static_cast<Double_t>( data );
	dest->tdc_c[k] = (data - dest->offset[k])*fTdc2T;
	(*dest->nthit)++;
      }
    }
  }

#ifdef WITH_DEBUG
  if ( fDebug > 3 ) {
    cout << endl << endl;
    cout << "Event " << evdata.GetEvNum() << "   Trigger " << evdata.GetEvType()
	 << " Scintillator " << GetPrefix() << endl;
    cout << "   paddle  Left(TDC    ADC   ADC_p)  Right(TDC    ADC   ADC_p)" << endl;
    cout << right;
    for ( int i=0; i<fNelem; i++ ) {
      cout << "     "     << setw(2) << i+1;
      cout << "        "; WriteValue(fLT[i]);
      cout << "  ";       WriteValue(fLA[i]);
      cout << "  ";       WriteValue(fLA_p[i]);
      cout << "        "; WriteValue(fRT[i]);
      cout << "  ";       WriteValue(fRA[i]);
      cout << "  ";       WriteValue(fRA_p[i]);
      cout << endl;
    }
    cout << left;
  }
#endif

  return fLTNhit+fRTNhit;
}

//_____________________________________________________________________________
Int_t THaScintillator::ApplyCorrections( void )
{
  // Apply the ADC/TDC corrections to get the 'REAL' relevant 
  // TDC and ADC values. No tracking needs to have been done yet.
  //
  // Permits the dividing up of the decoding step (events could come from
  // a different source) to the applying of corrections. For ease when
  // trying to optimize calibrations
  //
  Int_t nlt=0, nrt=0, nla=0, nra=0;
  for (Int_t i=0; i<fNelem; i++) {
    if (fLA[i] != 0.) {
      fLA_p[i] = fLA[i] - fLPed[i];
      fLA_c[i] = fLA_p[i]*fLGain[i];
      nla++;
    }
    if (fRA[i] != 0.) {
      fRA_p[i] = fRA[i] - fRPed[i];
      fRA_c[i] = fRA_p[i]*fRGain[i];
      nra++;
    }
    if (fLT[i] != 0.) {
      fLT_c[i] = (fLT[i] - fLOff[i])*fTdc2T - TimeWalkCorrection(i,kLeft);
      nlt++;
    }
    if (fRT[i] != 0.) {
      fRT_c[i] = (fRT[i] - fROff[i])*fTdc2T - TimeWalkCorrection(i,kRight);
      nrt++;
    }
  }
  // returns FALSE (0) if all matches up
  return !(fLTNhit==nlt && fLANhit==nla && fRTNhit==nrt && fRANhit==nra );
}

//_____________________________________________________________________________
Double_t THaScintillator::TimeWalkCorrection(const Int_t& paddle,
					     const ESide side)
{
  // Calculate the time-walk correction. The timewalk might be
  // dependent upon the specific PMT, so information about exactly
  // which PMT fired is required.
  Double_t adc=0;
  if (side == kLeft)
    adc = fLA_p[paddle];
  else
    adc = fRA_p[paddle];

  if (fNTWalkPar<=0 || !fTWalkPar) return 0.; // uninitialized return safe 0

  // get the ADC value above the pedestal
  if ( adc <=0. ) return 0.;
  
  // we have an arbitrary timing offset, and will declare here that
  // for a MIP ( peak ~2000 ADC channels ) the timewalk correction is 0
  
  Double_t ref = fAdcMIP;
  Double_t tw(0), tw_ref(0.);
  int npar = fNTWalkPar/(2*fNelem);
  
  Double_t *par = &(fTWalkPar[npar*(side*fNelem+paddle)]);

  tw = par[0]*pow(adc,-.5);
  tw_ref = par[0]*pow(ref,-.5);

  return tw-tw_ref;
}

//_____________________________________________________________________________
Int_t THaScintillator::CoarseProcess( TClonesArray& /* tracks */ )
{
  // Calculation of coordinates of particle track cross point with scint
  // plane in the detector coordinate system. For this, parameters of track 
  // reconstructed in THaVDC::CoarseTrack() are used.
  //
  // Apply corrections and reconstruct the complete hits.
  //
  static const Double_t sqrt2 = TMath::Sqrt(2.);
  
  ApplyCorrections();

  // count the number of paddles with complete TDC hits
  // Fill in information available from timing
  fNhit = 0;
  for (int i=0; i<fNelem; i++) {
    if (fLT[i]>0 && fRT[i]>0) {
      fHitPad[fNhit++] = i;
      fTime[i] = .5*(fLT_c[i]+fRT_c[i])-fSize[1]/fCn;
      fdTime[i] = fResolution/sqrt2;
      fYt[i] = .5*fCn*(fRT_c[i]-fLT_c[i]);
    }

    // rough calculation of position from ADC reading
    if (fLA_c[i]>0&&fRA_c[i]>0) {
      fYa[i] = TMath::Log(fLA_c[i]/fRA_c[i])/(2.*fAttenuation);
      // rough dE/dX-like quantity, not correcting for track angle
      fAmpl[i] = TMath::Sqrt(fLA_c[i]*fRA_c[i]*TMath::Exp(fAttenuation*2*fSize[1]))
	/ fSize[2];
    }
  }
  
  return 0;
}

//_____________________________________________________________________________
Int_t THaScintillator::FineProcess( TClonesArray& tracks )
{
  // Reconstruct coordinates of particle track cross point with scintillator
  // plane, and copy the data into the following local data structure:
  //
  // Units of measurements are meters.

  // Calculation of coordinates of particle track cross point with scint
  // plane in the detector coordinate system. For this, parameters of track 
  // reconstructed in THaVDC::FineTrack() are used.

  int n_track = tracks.GetLast()+1;   // Number of reconstructed tracks
  
  Double_t dpadx = (2.*fSize[0])/(fNelem); // width of a paddle
  // center of paddle '0'
  Double_t padx0 = -dpadx*(fNelem-1)*.5;
  
  for ( int i=0; i<n_track; i++ ) {
    THaTrack* theTrack = static_cast<THaTrack*>( tracks[i] );

    Double_t pathl=kBig, xc=kBig, yc=kBig, dx=kBig;
    Int_t pad=-1;
    
    if ( ! CalcTrackIntercept(theTrack, pathl, xc, yc) ) { // failed to hit
      new ( (*fTrackProj)[i] )
	THaTrackProj(xc,yc,pathl,dx,pad,this);
      continue;
    }
    
    // xc, yc are the positions of the track intercept
    //  _RELATIVE TO THE DETECTOR PLANE's_ origin.
    //
    // look through set of complete hits for closest match
    // loop through due to possible poor matches
    dx = kBig;
    for ( Int_t j=0; j<fNhit; j++ ) {
      Double_t dx2 = ( padx0 + fHitPad[j]*dpadx) - xc;
      if (TMath::Abs(dx2) < TMath::Abs(dx) ) {
	pad = fHitPad[j];
	dx = dx2;
      }
      else if (pad>=0) break; // stop after finding closest in X
    }

    // record information, found or not
    new ( (*fTrackProj)[i] )
      THaTrackProj(xc,yc,pathl,dx,pad,this);
  }
  
  return 0;
}

ClassImp(THaScintillator)
////////////////////////////////////////////////////////////////////////////////
