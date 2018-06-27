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
#include "THaTrackProj.h"
#include "VarDef.h"
#include "VarType.h"
#include "THaTrack.h"
#include "TClonesArray.h"
#include "TMath.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#define OSSTREAM ostringstream

using namespace std;

//_____________________________________________________________________________
THaScintillator::THaScintillator( const char* name, const char* description,
				  THaApparatus* apparatus )
  : THaNonTrackingDetector(name,description,apparatus),
    fLOff(0), fROff(0), fLPed(0), fRPed(0), fLGain(0), fRGain(0),
    fNTWalkPar(0), fTWalkPar(0), fTrigOff(0),
    fLTNhit(0), fLT(0), fLT_c(0), fRTNhit(0), fRT(0), fRT_c(0),
    fLANhit(0), fLA(0), fLA_p(0), fLA_c(0), fRANhit(0), fRA(0), fRA_p(0), fRA_c(0),
    fNhit(0), fHitPad(0), fTime(0), fdTime(0), fAmpl(0), fYt(0), fYa(0)
{
  // Constructor
}

//_____________________________________________________________________________
THaScintillator::THaScintillator()
  : THaNonTrackingDetector(), fLOff(0), fROff(0), fLPed(0), fRPed(0),
    fLGain(0), fRGain(0), fTWalkPar(0), fAdcMIP(0), fTrigOff(0),
    fLT(0), fLT_c(0), fRT(0), fRT_c(0), fLA(0), fLA_p(0), fLA_c(0),
    fRA(0), fRA_p(0), fRA_c(0), fHitPad(0), fTime(0), fdTime(0), fAmpl(0),
    fYt(0), fYa(0)
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
Int_t THaScintillator::ReadDatabase( const TDatime& date )
{
  // Read this detector's parameters from the database

  const char* const here = "ReadDatabase";

  FILE* file = OpenFile( date );
  if( !file ) return kFileError;

  // Read fOrigin and fSize (required!)
  Int_t err = ReadGeometry( file, date, true );
  if( err ) {
    fclose(file);
    return err;
  }

  vector<Int_t> detmap;
  Int_t nelem;
  Int_t tdc_mode = -255; // indicates unset
  fTdc2T = 5e-10;  // TDC resolution (s/channel), for reference, required anyway

  // Read configuration parameters
  DBRequest config_request[] = {
    { "detmap",       &detmap,   kIntV },
    { "npaddles",     &nelem,    kInt  },
    { "tdc.res",      &fTdc2T,   kDouble },
    { "tdc.cmnstart", &tdc_mode, kInt, 0, 1 },
    { 0 }
  };
  err = LoadDB( file, date, config_request, fPrefix );

  // Sanity checks
  if( !err && nelem <= 0 ) {
    Error( Here(here), "Invalid number of paddles: %d", nelem );
    err = kInitError;
  }

  // Reinitialization only possible for same basic configuration
  if( !err ) {
    if( fIsInit && nelem != fNelem ) {
      Error( Here(here), "Cannot re-initalize with different number of paddles. "
	     "(was: %d, now: %d). Detector not re-initialized.", fNelem, nelem );
      err = kInitError;
    } else
      fNelem = nelem;
  }

  UInt_t flags = THaDetMap::kFillLogicalChannel | THaDetMap::kFillModel;
  if( !err && FillDetMap(detmap, flags, here) <= 0 ) {
    err = kInitError;  // Error already printed by FillDetMap
  }

  if( !err && (nelem = fDetMap->GetTotNumChan()) != 4*fNelem ) {
    Error( Here(here), "Number of detector map channels (%d) "
	   "inconsistent with 4*number of paddles (%d)", nelem, 4*fNelem );
    err = kInitError;
  }

  // Set TDCs to common start mode, if set
  if( tdc_mode != -255 ) {
    // TDC mode was specified. Configure all TDC modules accordingly
    Int_t nmodules = fDetMap->GetSize();
    for( Int_t i = 0; i < nmodules; i++ ) {
      THaDetMap::Module* d = fDetMap->GetModule(i);
      if( d->model ? d->IsTDC() : i>=nmodules/2 ) {
	if( !d->model ) d->MakeTDC();
	d->SetTDCMode(tdc_mode);
      }
    }
    // If the TDC mode was set explicitly, override negative TDC
    // resolutions that historically have indicated the TDC mode
    if( tdc_mode != 0 && fTdc2T < 0.0 )
      // Warn user if tdc_mode and fTdc2T seem to indicate different things.
      // tdc_mode takes preference
      Warning( Here(here), "Negative TDC resolution = %lf converted to "
	       "positive since TDC mode explicitly set to common start.",
	       fTdc2T );
    fTdc2T = TMath::Abs(fTdc2T);
  }

  if( err ) {
    fclose(file);
    return err;
  }

  // Dimension arrays
  //FIXME: use a structure!
  UInt_t nval = fNelem, nval_twalk = 2*nval;
  if( !fIsInit ) {
    fNTWalkPar = nval_twalk;
    // Calibration data
    fLOff  = new Double_t[ nval ];
    fROff  = new Double_t[ nval ];
    fLPed  = new Double_t[ nval ];
    fRPed  = new Double_t[ nval ];
    fLGain = new Double_t[ nval ];
    fRGain = new Double_t[ nval ];

    fTrigOff = new Double_t[ nval ];

    // Per-event data
    fLT   = new Double_t[ nval ];
    fLT_c = new Double_t[ nval ];
    fRT   = new Double_t[ nval ];
    fRT_c = new Double_t[ nval ];
    fLA   = new Double_t[ nval ];
    fLA_p = new Double_t[ nval ];
    fLA_c = new Double_t[ nval ];
    fRA   = new Double_t[ nval ];
    fRA_p = new Double_t[ nval ];
    fRA_c = new Double_t[ nval ];

    fTWalkPar = new Double_t[ nval_twalk ];

    fHitPad = new Int_t[ nval ];
    fTime   = new Double_t[ nval ]; // analysis indexed by paddle (yes, inefficient)
    fdTime  = new Double_t[ nval ];
    fAmpl   = new Double_t[ nval ];

    fYt     = new Double_t[ nval ];
    fYa     = new Double_t[ nval ];

    fIsInit = true;
  }

 // Read calibration parameters

  // Set DEFAULT values here
  fResolution = kBig;   // actual timing resolution
  // Speed of light in the scintillator material
  fCn = 1.7e+8;         // meters/second (for reference, required anyway)
  // Attenuation length
  fAttenuation = 0.7;   // inverse meters
  // Time-walk correction parameters
  fAdcMIP = 1.e10;      // large number for offset, so reference is effectively disabled
  // timewalk coefficients for tw = coeff*(1./sqrt(ADC-Ped)-1./sqrt(ADCMip))
  memset( fTWalkPar, 0, nval_twalk*sizeof(fTWalkPar[0]) );
  // trigger-timing offsets (s)
  memset( fTrigOff, 0, nval*sizeof(fTrigOff[0]) );

  // Default TDC offsets (0), ADC pedestals (0) and ADC gains (1)
  memset( fLOff, 0, nval*sizeof(fLOff[0]) );
  memset( fROff, 0, nval*sizeof(fROff[0]) );
  memset( fLPed, 0, nval*sizeof(fLPed[0]) );
  memset( fRPed, 0, nval*sizeof(fRPed[0]) );
  for( UInt_t i=0; i<nval; ++i ) {
    fLGain[i] = 1.0;
    fRGain[i] = 1.0;
  }

  DBRequest calib_request[] = {
    { "L.off",            fLOff,         kDouble, nval, 1 },
    { "R.off",            fROff,         kDouble, nval, 1 },
    { "L.ped",            fLPed,         kDouble, nval, 1 },
    { "R.ped",            fRPed,         kDouble, nval, 1 },
    { "L.gain",           fLGain,        kDouble, nval, 1 },
    { "R.gain",           fRGain,        kDouble, nval, 1 },
    { "Cn",               &fCn,          kDouble },
    { "MIP",              &fAdcMIP,      kDouble, 0, 1 },
    { "timewalk_params",  fTWalkPar,     kDouble, nval_twalk, 1 },
    { "retiming_offsets", fTrigOff,      kDouble, nval, 1 },
    { "avgres",           &fResolution,  kDouble, 0, 1 },
    { "atten",            &fAttenuation, kDouble, 0, 1 },
    { 0 }
  };
  err = LoadDB( file, date, calib_request, fPrefix );
  fclose(file);
  if( err )
    return err;

  if( fResolution == kBig )
    fResolution = 3.*fTdc2T; // guess at timing resolution

#ifdef WITH_DEBUG
  // Debug printout
  if ( fDebug > 2 ) {
    const UInt_t N = static_cast<UInt_t>(fNelem);
    Double_t pos[3]; fOrigin.GetXYZ(pos);
    DBRequest list[] = {
      { "Number of paddles",    &fNelem,     kInt         },
      { "Detector position",    pos,         kDouble, 3   },
      { "Detector size",        fSize,       kDouble, 3   },
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
#endif

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
    { "lt_c",   "Calibrated times left side (s)",    "fLT_c" },
    { "rt",     "TDC values right side",             "fRT" },
    { "rt_c",   "Calibrated times right side (s)",   "fRT_c" },
    { "la",     "ADC values left side",              "fLA" },
    { "la_p",   "Ped-sub ADC values left side",      "fLA_p" },
    { "la_c",   "Corrected ADC values left side",    "fLA_c" },
    { "ra",     "ADC values right side",             "fRA" },
    { "ra_p",   "Ped-sub ADC values right side",     "fRA_p" },
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
void THaScintillator::Clear( Option_t* opt )
{
  // Reset per-event data.

  THaNonTrackingDetector::Clear(opt);
  fNhit = fLTNhit = fRTNhit = fLANhit = fRANhit = 0;
  assert(fIsInit);
  for( Int_t i=0; i<fNelem; ++i ) {
    fLT[i] = fLT_c[i] = fRT[i] = fRT_c[i] = kBig;
    fLA[i] = fLA_p[i] = fLA_c[i] = fRA[i] = fRA_p[i] = fRA_c[i] = kBig;
    fTime[i] = fdTime[i] = fAmpl[i] = fYt[i] = fYa[i] = kBig;
  }
  memset( fHitPad, 0, fNelem*sizeof(fHitPad[0]) );
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

  // Loop over all modules defined for this detector

  const char* const here = "Decode";

  bool has_warning = false;
  for( Int_t i = 0; i < fDetMap->GetSize(); i++ ) {
    THaDetMap::Module* d = fDetMap->GetModule( i );
    bool adc = ( d->model ? d->IsADC() : (i < fDetMap->GetSize()/2) );
    bool not_common_stop_tdc = (adc || d->IsCommonStart());

    // Loop over all channels that have a hit.
    for( Int_t j = 0; j < evdata.GetNumChan( d->crate, d->slot ); j++) {

      Int_t chan = evdata.GetNextChan( d->crate, d->slot, j );
      if( chan < d->lo || chan > d->hi ) continue;     // Not one of my channels

      Int_t nhit = evdata.GetNumHits(d->crate, d->slot, chan);
      if( nhit > 1 || nhit == 0 ) {
	OSSTREAM msg;
	msg << nhit << " hits on " << (adc ? "ADC" : "TDC")
	    << " channel " << d->crate << "/" << d->slot << "/" << chan;
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

      // Get the data. If multiple hits on a TDC channel, take
      // either first or last hit, depending on TDC mode
      assert( nhit>0 );
      Int_t ihit = ( not_common_stop_tdc ) ? 0 : nhit-1;
      Int_t data = evdata.GetData( d->crate, d->slot, chan, ihit );

      // Get the detector channel number, starting at 0
      Int_t k = d->first + ((d->reverse) ? d->hi - chan : chan - d->lo) - 1;

      if( k<0 || k>NDEST*fNelem ) {
	// Indicates bad database
	Error( Here(here), "Illegal detector channel: %d. "
	       "Fix detector map in database", k );
	continue;
      }
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
	if( fTdc2T > 0.0 && !not_common_stop_tdc ) {
	  // For common stop TDCs, time is negatively correlated to raw data
	  // time = (offset-data)*res, so reverse the sign.
	  // However, historically, people have been using negative TDC
	  // resolutions to indicate common stop mode, so in that case
	  // the sign flip has already been applied.
	  dest->tdc_c[k] *= -1.0;
	}
	(*dest->nthit)++;
      }
    }
  }

  if( has_warning )
    ++fNEventsWithWarnings;

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
Int_t THaScintillator::ApplyCorrections()
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
    if (fLA[i] > 0. && fLA[i] < 0.5*kBig) {
      fLA_p[i] = fLA[i] - fLPed[i];
      fLA_c[i] = fLA_p[i]*fLGain[i];
      nla++;
    }
    if (fRA[i] > 0. && fRA[i] < 0.5*kBig) {
      fRA_p[i] = fRA[i] - fRPed[i];
      fRA_c[i] = fRA_p[i]*fRGain[i];
      nra++;
    }
    if (fLT[i] < 0.5*kBig) {
      fLT_c[i] = (fLT[i] - fLOff[i])*fTdc2T - TimeWalkCorrection(i,kLeft);
      nlt++;
    }
    if (fRT[i] < 0.5*kBig) {
      fRT_c[i] = (fRT[i] - fROff[i])*fTdc2T - TimeWalkCorrection(i,kRight);
      nrt++;
    }
  }
  // returns FALSE (0) if all matches up
  return !(fLTNhit==nlt && fLANhit==nla && fRTNhit==nrt && fRANhit==nra );
}

//_____________________________________________________________________________
Double_t THaScintillator::TimeWalkCorrection( const Int_t& paddle,
					      const ESide side )
{
  // Calculate the time-walk correction. The timewalk might be
  // dependent upon the specific PMT, so information about exactly
  // which PMT fired is required.

  if (fNTWalkPar<=0 || !fTWalkPar || fNelem == 0)
    return 0.; // uninitialized return safe 0

  Double_t adc, ref = fAdcMIP;
  if (side == kLeft)
    adc = fLA_p[paddle];
  else
    adc = fRA_p[paddle];

  // get the ADC value above the pedestal
  if ( adc <=0. || ref <= 0.)
    return 0.;

  int npar = fNTWalkPar/(2*fNelem);

  Double_t par = fTWalkPar[npar*(fNelem*side+paddle)];

  // Traditional correction according to
  // J.S.Brown et al., NIM A221, 503 (1984); T.Dreyer et al., NIM A309, 184 (1991)
  // Assume that for a MIP (peak ~2000 ADC channels) the correction is 0
  Double_t corr = par * ( 1./TMath::Sqrt(adc) - 1./TMath::Sqrt(ref) );

  return corr;
}

//_____________________________________________________________________________
Int_t THaScintillator::CoarseProcess( TClonesArray& tracks )
{
  // Scintillator coarse processing:
  //
  // - Apply timewalk corrections
  // - Calculate rough transverse (y) position and energy deposition for hits
  //   for which PMTs on both ends of the paddle fired
  // - Calculate rough track crossing points

  static const Double_t sqrt2 = TMath::Sqrt(2.);

  ApplyCorrections();

  // count the number of paddles with complete TDC hits
  // Fill in information available from timing
  fNhit = 0;
  for (int i=0; i<fNelem; i++) {
    if( fLT[i]<0.5*kBig && fRT[i]<0.5*kBig ) {
      fHitPad[fNhit++] = i;
      fTime[i] = .5*(fLT_c[i]+fRT_c[i])-fSize[1]/fCn;
      fdTime[i] = fResolution/sqrt2;
      fYt[i] = .5*fCn*(fRT_c[i]-fLT_c[i]);
    }

    // rough calculation of position from ADC reading
    if( fLA[i]<0.5*kBig && fRA[i]<0.5*kBig && fLA_c[i]>0 && fRA_c[i]>0 ) {
      fYa[i] = TMath::Log(fLA_c[i]/fRA_c[i])/(2.*fAttenuation);
      // rough dE/dX-like quantity, not correcting for track angle
      fAmpl[i] = TMath::Sqrt(fLA_c[i]*fRA_c[i]*TMath::Exp(fAttenuation*2*fSize[1]))
	/ fSize[2];
    }
  }

  // Project tracks onto scintillator plane
  CalcTrackProj( tracks );

  return 0;
}

//_____________________________________________________________________________
Int_t THaScintillator::FineProcess( TClonesArray& tracks )
{
  // Scintillator fine processing:
  //
  // - Reconstruct coordinates of track cross point with scintillator plane
  // - For each crossing track, determine position residual and paddle number
  //
  // The position residuals are calculated along the x-coordinate (dispersive
  // direction) only. They are useful to determine whether a track has a
  // matching hit ( if abs(dx) <= 0.5*paddle x-width ) and, if so, how close
  // to the edge of the paddle the track crossed. This assumes scintillator
  // paddles oriented along the transverse (non-dispersive, y) direction.

  // Redo projection of tracks since FineTrack may have changed tracks
  Int_t n_cross = CalcTrackProj( tracks );

  // Find the closest hits to the track crossing points
  if( n_cross > 0 ) {
    Double_t dpadx = 2.0*fSize[0]/fNelem;   // Width of a paddle
    Double_t padx0 = -fSize[0]+0.5*dpadx;   // center of paddle '0'
    for( Int_t i=0; i<fTrackProj->GetLast()+1; i++ ) {
      THaTrackProj* proj = static_cast<THaTrackProj*>( fTrackProj->At(i) );
      assert( proj );
      if( !proj->IsOK() )
	continue;
      Int_t pad = -1;                      // paddle number of closest hit
      Double_t xc = proj->GetX();          // track intercept x-coordinate
      Double_t dx = kBig;                  // xc - distance paddle center
      for( Int_t j = 0; j < fNhit; j++ ) {
	Double_t dx2 = xc - (padx0 + fHitPad[j]*dpadx);
	if (TMath::Abs(dx2) < TMath::Abs(dx) ) {
	  pad = fHitPad[j];
	  dx = dx2;
	}
      }
      assert( pad >= 0 || fNhit == 0 ); // Must find a pad unless no hits
      if( pad >= 0 ) {
	proj->SetdX(dx);
	proj->SetChannel(pad);
      }
    }
  }

  return 0;
}

//_____________________________________________________________________________
ClassImp(THaScintillator)
