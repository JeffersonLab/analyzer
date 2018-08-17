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
#include <cstdlib>
#include <vector>
#include <cassert>
#include <iostream>
#include <iomanip>
#include <sstream>

using namespace std;

//_____________________________________________________________________________
THaCherenkov::THaCherenkov( const char* name, const char* description,
                            THaApparatus* apparatus )
  : THaPidDetector(name,description,apparatus), fTdc2T(0), fOff(0), fPed(0),
    fGain(0), fNThit(0), fT(0), fT_c(0), fNAhit(0), fA(0), fA_p(0), fA_c(0),
    fASUM_p(kBig), fASUM_c(kBig)
{
  // Constructor
}

//_____________________________________________________________________________
THaCherenkov::THaCherenkov()
  : THaPidDetector(), fTdc2T(0), fOff(0), fPed(0),
    fGain(0), fNThit(0), fT(0), fT_c(0), fNAhit(0), fA(0), fA_p(0), fA_c(0),
    fASUM_p(kBig), fASUM_c(kBig)
{
  // Default constructor (for ROOT I/O)
}

//_____________________________________________________________________________
Int_t THaCherenkov::ReadDatabase( const TDatime& date )
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

  vector<Int_t> detmap;
  Int_t nelem;
  Int_t tdc_mode = -255; // indicates unset
  fTdc2T = 0.5e-9; // TDC resolution (s/channel)

  // Read configuration parameters
  DBRequest config_request[] = {
    { "detmap",       &detmap,   kIntV },
    { "npmt",         &nelem,    kInt  },
    { "tdc.res",      &fTdc2T,   kDouble, 0, 1 }, // optional to support old DBs
    { "tdc.cmnstart", &tdc_mode, kInt, 0, 1 },
    { 0 }
  };
  err = LoadDB( file, date, config_request, fPrefix );

  // Sanity checks
  if( !err && nelem <= 0 ) {
    Error( Here(here), "Invalid number of PMTs: %d. Must be > 0. "
	   "Fix database.", nelem );
    err = kInitError;
  }

  // Reinitialization only possible for same basic configuration
  if( !err ) {
    if( fIsInit && nelem != fNelem ) {
      Error( Here(here), "Cannot re-initalize with different number of PMTs. "
	     "(was: %d, now: %d). Detector not re-initialized.", fNelem, nelem );
      err = kInitError;
    } else
      fNelem = nelem;
  }

  UInt_t flags = THaDetMap::kFillLogicalChannel | THaDetMap::kFillModel;
  if( !err && FillDetMap(detmap, flags, here) <= 0 ) {
    err = kInitError;  // Error already printed by FillDetMap
  }

  if( !err && (nelem = fDetMap->GetTotNumChan()) != 2*fNelem ) {
    Error( Here(here), "Number of detector map channels (%d) "
	   "inconsistent with 2*number of PMTs (%d)", nelem, 2*fNelem );
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
    // If the TDC mode was set explicitly, assume that negative TDC
    // resolutions are database errors
    Warning( Here(here), "Negative TDC resolution = %lf converted to "
	     "positive since TDC mode explicitly set.", fTdc2T );
    fTdc2T = TMath::Abs(fTdc2T);
  }

  if( err ) {
    fclose(file);
    return err;
  }

  // Dimension arrays
  //FIXME: use a structure!
  UInt_t nval = fNelem;
  if( !fIsInit ) {
    // Calibration data
    fOff  = new Float_t[ nval ];
    fPed  = new Float_t[ nval ];
    fGain = new Float_t[ nval ];

    // Per-event data
    fT    = new Float_t[ nval ];
    fT_c  = new Float_t[ nval ];
    fA    = new Float_t[ nval ];
    fA_p  = new Float_t[ nval ];
    fA_c  = new Float_t[ nval ];

    fIsInit = true;
  }

  // Read calibration parameters

  // Set DEFAULT values here
  // Default TDC offsets (0), ADC pedestals (0) and ADC gains (1)
  memset( fOff, 0, nval*sizeof(fOff[0]) );
  memset( fPed, 0, nval*sizeof(fPed[0]) );
  for( UInt_t i=0; i<nval; ++i ) {
    fGain[i] = 1.0;
  }

  DBRequest calib_request[] = {
    { "tdc.offsets",      fOff,         kFloat, nval, 1 },
    { "adc.pedestals",    fPed,         kFloat, nval, 1 },
    { "adc.gains",        fGain,        kFloat, nval, 1 },
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
      { "Number of mirrors", &fNelem,     kInt       },
      { "Detector position", pos,         kDouble, 3 },
      { "Detector size",     fSize,       kDouble, 3 },
      { "TDC offsets",       fOff,        kFloat,  N },
      { "ADC pedestals",     fPed,        kFloat,  N },
      { "ADC gains",         fGain,       kFloat,  N },
      { 0 }
    };
    DebugPrint( list );
  }
#endif

  return kOK;
}

//_____________________________________________________________________________
Int_t THaCherenkov::DefineVariables( EMode mode )
{
  // Initialize global variables

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  RVarDef vars[] = {
    { "nthit",  "Number of PMTs with valid TDC",     "fNThit" },
    { "nahit",  "Number of PMTs with ADC signal",    "fNAhit" },
    { "t",      "Raw TDC values",                    "fT" },
    { "t_c",    "Calibrated TDC times (s)",          "fT_c" },
    { "a",      "Raw ADC values",                    "fA" },
    { "a_p",    "Pedestal-subtracted ADC values ",   "fA_p" },
    { "a_c",    "Gain-corrected ADC values",         "fA_c" },
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
void THaCherenkov::Clear( Option_t* opt )
{
  // Clear event data

  THaPidDetector::Clear(opt);
  fNThit = fNAhit = 0;
  assert(fIsInit);
  for( Int_t i=0; i<fNelem; ++i ) {
    fT[i] = fT_c[i] = fA[i] = fA_p[i] = fA_c[i] = kBig;
  }
  fASUM_p = fASUM_c = 0.0;
}

//_____________________________________________________________________________
Int_t THaCherenkov::Decode( const THaEvData& evdata )
{
  // Decode Cherenkov data, correct TDC times and ADC amplitudes, and copy
  // the data into the local data members.
  // This implementation assumes that the first half of the detector map
  // entries corresponds to ADCs, and the second half, to TDCs.

  const char* const here = "Decode";

  // Loop over all modules defined for Cherenkov detector
  bool has_warning = false;
  for( Int_t i = 0; i < fDetMap->GetSize(); i++ ) {
    THaDetMap::Module* d = fDetMap->GetModule( i );
    bool adc = (d->model ? d->IsADC() : i < fDetMap->GetSize()/2 );
    bool not_common_stop_tdc = (adc || d->IsCommonStart());

    // Loop over all channels that have a hit.
    for( Int_t j = 0; j < evdata.GetNumChan( d->crate, d->slot ); j++) {

      Int_t chan = evdata.GetNextChan( d->crate, d->slot, j );
      if( chan < d->lo || chan > d->hi ) continue;     // Not one of my channels

      Int_t nhit = evdata.GetNumHits(d->crate, d->slot, chan);
      if( nhit > 1 || nhit == 0 ) {
	ostringstream msg;
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

      if( k<0 || k>= fNelem ) {
	Error( Here(here), "Illegal detector channel: %d", k );
        continue;
      }

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
	fT_c[k] = (data - fOff[k]) * fTdc2T;
	if( fTdc2T > 0.0 && !not_common_stop_tdc ) {
	  // For common stop TDCs, time is negatively correlated to raw data
	  // time = (offset-data)*res, so reverse the sign.
	  // Assume that a negative TDC resolution indicates common stop mode
	  // as well, so the sign flip has already been applied.
	  fT_c[k] *= -1.0;
	}
	fNThit++;
      }
    }
  }

  if( has_warning )
    ++fNEventsWithWarnings;

#ifdef WITH_DEBUG
  if ( fDebug > 3 ) {
    cout << endl << "Cherenkov " << GetPrefix() << ":" << endl;
    int ncol=3;
    for (int i=0; i<ncol; i++) {
      cout << "  Mirror TDC   ADC  ADC_p  ";
    }
    cout << endl;

    for (int i=0; i<(fNelem+ncol-1)/ncol; i++ ) {
      for (int c=0; c<ncol; c++) {
	int ind = c*fNelem/ncol+i;
	if (ind < fNelem) {
	  cout << "  " << setw(3) << ind+1;
	  cout << "  "; WriteValue(fT[ind]);
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

  return fNThit;
}

//_____________________________________________________________________________
Int_t THaCherenkov::CoarseProcess( TClonesArray& tracks )
{
  // Reconstruct coordinates of where a particle track crosses
  // the Cherenkov plane, and copy the point into the fTrackProj array.

  CalcTrackProj( tracks );

  return 0;
}

//_____________________________________________________________________________
Int_t THaCherenkov::FineProcess( TClonesArray& tracks )
{
  // Fine Cherenkov processing

  // Redo the track matching since tracks might have been thrown out
  // during the FineTracking stage.

  CalcTrackProj( tracks );

  return 0;
}

//_____________________________________________________________________________
ClassImp(THaCherenkov)
