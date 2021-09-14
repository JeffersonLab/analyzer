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

#include <cstdlib>
#include <cassert>
#include <iostream>
#include <iomanip>

using namespace std;
using namespace Podd;

// Macro for better readability
#if __cplusplus >= 201402L
# define MKPMTDATA(name,title,nelem) make_unique<PMTData>((name),(title),(nelem))
#else
# define MKPMTDATA(name,title,nelem) unique_ptr<PMTData>(new PMTData((name),(title),(nelem)))
#endif

//_____________________________________________________________________________
THaCherenkov::THaCherenkov( const char* name, const char* description,
                            THaApparatus* apparatus )
  : THaPidDetector(name,description,apparatus), fPMTData(nullptr),
    fASUM_p(kBig), fASUM_c(kBig)
{
  // Constructor
}

//_____________________________________________________________________________
THaCherenkov::THaCherenkov()
  : THaPidDetector(), fPMTData(nullptr), fASUM_p(kBig), fASUM_c(kBig)
{
  // Default constructor (for ROOT I/O)
}

//_____________________________________________________________________________
Int_t THaCherenkov::ReadDatabase( const TDatime& date )
{
  // Read parameters from the database.
  // 'date' contains the date/time of the run being analyzed.

  const char* const here = "ReadDatabase";

  VarType kDataType  = std::is_same<Data_t, Float_t>::value ? kFloat  : kDouble;
  VarType kDataTypeV = std::is_same<Data_t, Float_t>::value ? kFloatV : kDoubleV;

  // Read database
  Int_t err = THaPidDetector::ReadDatabase(date);
  if( err )
    return err;

  FILE* file = OpenFile( date );
  if( !file ) return kFileError;

  // Read fOrigin and fSize (required!)
  err = ReadGeometry( file, date, true );
  if( err ) {
    fclose(file);
    return err;
  }

  enum { kModeUnset = -255, kCommonStop = 0, kCommonStart = 1 };

  vector<Int_t> detmap;
  Int_t nelem = 0;
  Int_t tdc_mode = kModeUnset;
  Data_t tdc2t = 5e-10;  // TDC resolution (s/channel)

  // Read configuration parameters
  DBRequest config_request[] = {
    { "detmap",       &detmap,   kIntV },
    { "npmt",         &nelem,    kInt  },
    { "tdc.res",      &tdc2t,    kDataType, 0, true }, // optional to support old DBs
    { "tdc.cmnstart", &tdc_mode, kInt,      0, true },
    { nullptr }
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
      Error( Here(here), "Cannot re-initialize with different number of PMTs. "
	     "(was: %d, now: %d). Detector not re-initialized.", fNelem, nelem );
      err = kInitError;
    } else
      fNelem = nelem;
  }

  UInt_t flags = THaDetMap::kFillLogicalChannel | THaDetMap::kFillModel;
  if( !err && FillDetMap(detmap, flags, here) <= 0 ) {
    err = kInitError;  // Error already printed by FillDetMap
  }

  const UInt_t nval = fNelem;
  if( !err ) {
    UInt_t tot_nchan = fDetMap->GetTotNumChan();
    if( tot_nchan != 2*nval ) {
      Error(Here(here), "Number of detector map channels (%u) "
                        "inconsistent with 2*number of PMTs (%d)",
            tot_nchan, 2*fNelem);
      err = kInitError;
    }
  }

  // Deal with the TDC mode (common stop (default) vs. common start).
  if( tdc_mode == kModeUnset ) {
    // TDC mode not specified. Mimic the behavior of previous analyzer versions.
    // TDCs are always common stop unless c.start mode is explicitly requested.
    tdc_mode = kCommonStop;
  } else {
    // TDC mode IS explicitly specified
    if( tdc_mode != kCommonStop && tdc2t < 0.0 ) {
      // A negative TDC resolution, tdc2t, in a legacy databases indicates
      // common stop mode. Warn user if tdc_mode and tdc2t are inconsistent.
      // tdc_mode takes preference.
      Warning(Here(here), "Negative TDC resolution = %lf converted to "
                          "positive since TDC mode explicitly set to common start.",tdc2t);
    }
  }
  assert( tdc_mode != kModeUnset );
  // Ensure tdc2t is positive. The tdc_mode flag handles the sign now.
  tdc2t = TMath::Abs(tdc2t);

  // Set module capability flags for legacy databases without model info
  UInt_t nmodules = fDetMap->GetSize();
  for( UInt_t i = 0; i < nmodules; i++ ) {
    THaDetMap::Module* d = fDetMap->GetModule(i);
    if( !d->model ) {
      if( i < nmodules/2 ) d->MakeADC(); else d->MakeTDC();
    }
    if( d->IsTDC() ) {
      d->SetTDCMode(tdc_mode);
    }
  }

  if( err ) {
    fclose(file);
    return err;
  }

  fDetectorData.clear();
  auto detdata = MKPMTDATA(GetPrefixName(), fTitle, nval);
  fPMTData = detdata.get();
  fDetectorData.emplace_back(std::move(detdata));
  assert(fPMTData->GetSize() == nval);

  // Read calibration parameters
  vector<Data_t> off, ped, gain;
  DBRequest calib_request[] = {
    { "tdc.offsets",    &off,  kDataTypeV, nval, true },
    { "adc.pedestals",  &ped,  kDataTypeV, nval, true },
    { "adc.gains",      &gain, kDataTypeV, nval, true },
    { nullptr }
  };
  err = LoadDB( file, date, calib_request, fPrefix );
  fclose(file);
  if( err )
    return err;

  for( UInt_t i = 0; i < nval; ++i ) {
    auto& calib = fPMTData->GetCalib(i);
    calib.tdc2t = tdc2t;
    calib.off   = off[i];
    calib.ped   = ped[i];
    calib.gain  = gain[i];
  }

#ifdef WITH_DEBUG
  // Debug printout
  if ( fDebug > 2 ) {
    const auto N = static_cast<UInt_t>(fNelem);
    Double_t pos[3]; fOrigin.GetXYZ(pos);
    DBRequest list[] = {
      { "Number of mirrors", &fNelem, kInt       },
      { "Detector position", pos,     kDouble, 3 },
      { "Detector size",     fSize,   kDouble, 3 },
      { "TDC offsets",       &off,    kDataTypeV,  N },
      { "ADC pedestals",     &ped,    kDataTypeV,  N },
      { "ADC gains",         &gain,   kDataTypeV,  N },
      { nullptr }
    };
    DebugPrint( list );
  }
#endif

  fIsInit = true;
  return kOK;
}

//_____________________________________________________________________________
Int_t THaCherenkov::DefineVariables( EMode mode )
{
  // Initialize global variables

  // Set up standard variables, including objects in fDetectorData
  Int_t ret = THaPidDetector::DefineVariables(mode);
  if( ret )
    return ret;

  RVarDef vars[] = {
    {"asum_p", "Sum of ADC minus pedestal values",   "fASUM_p"},
    {"asum_c", "Sum of corrected ADC amplitudes",    "fASUM_c"},
    {nullptr}
  };
  return DefineVarsFromList(vars, mode);
}

//_____________________________________________________________________________
THaCherenkov::~THaCherenkov()
{
  // Destructor. Remove variables from global list.

  RemoveVariables();
}

//_____________________________________________________________________________
void THaCherenkov::Clear( Option_t* opt )
{
  // Clear event data

  THaPidDetector::Clear(opt);
  fASUM_p = fASUM_c = 0;
}

//_____________________________________________________________________________
Int_t THaCherenkov::StoreHit( const DigitizerHitInfo_t& hitinfo, UInt_t data )
{
  // Put decoded frontend data into fDetectorData. Used by Decode().
  // hitinfo: channel info (crate/slot/channel/hit/type)
  // data:    data registered in this channel

  THaPidDetector::StoreHit(hitinfo, data);

  // Add channels with signals to the amplitude sums
  const auto& PMT = fPMTData->GetPMT(hitinfo);
  if( PMT.adc_p > 0 )
    fASUM_p += PMT.adc_p;             // Sum of ADC minus ped
  if( PMT.adc_c > 0 )
    fASUM_c += PMT.adc_c;             // Sum of ADC corrected

  return 0;
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
void THaCherenkov::PrintDecodedData( const THaEvData& evdata ) const
{
  // Print decoded data (for debugging). Called form Decode()

//  cout << endl << endl;
  cout << "Event " << evdata.GetEvNum() << "   Trigger " << evdata.GetEvType()
       << " Cherenkov " << GetPrefix() << endl;
  int ncol=3;
  for (int i=0; i<ncol; i++) {
    cout << "  Mirror TDC   ADC  ADC_p  ";
  }
  cout << endl;

  for (int i=0; i<(fNelem+ncol-1)/ncol; i++ ) {
    for (int c=0; c<ncol; c++) {
      int ind = c*fNelem/ncol+i;
      if (ind < fNelem) {
        const auto& PMT = fPMTData->GetPMT(ind);
        cout << "  " << setw(3) << ind+1;
        cout << "  "; WriteValue(PMT.tdc);
        cout << "  "; WriteValue(PMT.adc);
        cout << "  "; WriteValue(PMT.adc_p);
        cout << "  ";
      } else {
        //	  cout << endl;
        break;
      }
    }
    cout << endl;
  }
}

//_____________________________________________________________________________
ClassImp(THaCherenkov)
