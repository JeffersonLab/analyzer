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
#include "Helper.h"
#include "THaTrack.h"
#include "TClonesArray.h"
#include "TMath.h"

#include <iostream>
#include <cassert>
#include <iomanip>
#include <cstdlib>
#include <algorithm>
#include <memory>

using namespace std;
using namespace Podd;

#if __cplusplus >= 201402L
# define MKPMTDATA(name,title,nelem) make_unique<PMTData>((name),(title),(nelem))
#else
# define MKPMTDATA(name,title,nelem) unique_ptr<PMTData>(new PMTData((name),(title),(nelem)))
#endif

//_____________________________________________________________________________
THaScintillator::THaScintillator( const char* name, const char* description,
				  THaApparatus* apparatus )
  : THaNonTrackingDetector(name,description,apparatus), fCn(0),
    fAttenuation(0), fResolution(0), fRightPMTs(nullptr), fLeftPMTs(nullptr)
{
  // Constructor

  fNviews = 2;
}

//_____________________________________________________________________________
THaScintillator::THaScintillator()
  : THaNonTrackingDetector(), fCn(0), fAttenuation(0), fResolution(0),
    fRightPMTs(nullptr), fLeftPMTs(nullptr)
{
  // Default constructor (for ROOT RTTI)

  fNviews = 2;
}

//_____________________________________________________________________________
Int_t THaScintillator::ReadDatabase( const TDatime& date )
{
  // Read this detector's parameters from the database

  const char* const here = "ReadDatabase";

  VarType kDataType  = std::is_same<Data_t, Float_t>::value ? kFloat  : kDouble;
  VarType kDataTypeV = std::is_same<Data_t, Float_t>::value ? kFloatV : kDoubleV;

  FILE* file = OpenFile( date );
  if( !file ) return kFileError;

  // Read fOrigin and fSize (required!)
  Int_t err = ReadGeometry( file, date, true );
  if( err ) {
    fclose(file);
    return err;
  }

  enum { kModeUnset = -255, kCommonStop = 0, kCommonStart = 1 };

  vector<Int_t> detmap;
  Int_t nelem = 0;
  Int_t tdc_mode = kModeUnset;
  Data_t tdc2t = 5e-10;  // TDC resolution (s/channel), for reference, required anyway

  // Read configuration parameters
  DBRequest config_request[] = {
    { "detmap",       &detmap,   kIntV },
    { "npaddles",     &nelem,    kInt  },
    { "tdc.res",      &tdc2t,    kDataType },
    { "tdc.cmnstart", &tdc_mode, kInt, 0, true },
    { nullptr }
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
      Error( Here(here), "Cannot re-initialize with different number of paddles. "
	     "(was: %d, now: %d). Detector not re-initialized.", fNelem, nelem );
      err = kInitError;
    } else
      fNelem = nelem;
  }

  UInt_t flags = THaDetMap::kFillLogicalChannel | THaDetMap::kFillModel;
  if( !err && FillDetMap(detmap, flags, here) <= 0 ) {
    err = kInitError;  // Error already printed by FillDetMap
  }

  UInt_t nval = fNelem;
  if( !err ) {
    UInt_t tot_nchan = fDetMap->GetTotNumChan();
    if( tot_nchan != 4 * nval ) {
      Error(Here(here), "Number of detector map channels (%u) "
                        "inconsistent with 4*number of paddles (%d)",
            tot_nchan, 4*fNelem);
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
  // This implementation makes the following assumptions about the detector map:
  // - The first half of the map entries corresponds to ADCs,
  //   the second half, to TDCs.
  // - The first fNelem detector channels correspond to the PMTs on the
  //   right hand side, the next fNelem channels, to the left hand side.
  //   (Thus channel numbering for each module must be consecutive.)
  UInt_t nmodules = fDetMap->GetSize();
  for( UInt_t i = 0; i < nmodules; i++ ) {
    THaDetMap::Module* d = fDetMap->GetModule(i);
    if( !d->model ) {
      if( i < nmodules/2 ) d->MakeADC(); else d->MakeTDC();
    }
    if( d->IsTDC() )
      d->SetTDCMode(tdc_mode);
  }

  if( err ) {
    fclose(file);
    return err;
  }

  // Set up storage for basic detector data
  // Per-event data
  fDetectorData.clear();
  for( int i = kRight; i <= kLeft; ++i ) {
    auto detdata = MKPMTDATA(GetPrefixName(), fTitle, nval);
    // Keep pointers to the elements around for convenient access
    PMTData*& pmtData = (i == kRight) ? fRightPMTs : fLeftPMTs;
    pmtData = detdata.get();
    assert(pmtData->GetSize() - nval == 0);
    fDetectorData.emplace_back(std::move(detdata));
  }
  fPadData.resize(nval);
  fHits.reserve(nval);

  // Read calibration parameters

  // Reset calibrations. In particular, this will set the default TDC offsets (0),
  // ADC pedestals (0) and ADC gains (1).
  Reset();

  // Set other DEFAULT values here
  fResolution = kBig;   // actual timing resolution
  // Speed of light in the scintillator material
  fCn = 1.7e+8;         // meters/second (for reference, required anyway)
  // Attenuation length
  fAttenuation = 0.7;   // inverse meters
  Data_t adcmip = 1.e10;  // large number for offset, so reference is effectively disabled

  vector<Data_t> loff, roff, lped, rped, lgain, rgain, twalk;
  DBRequest calib_request[] = {
    { "L.off",            &loff,         kDataTypeV, nval,   true },
    { "R.off",            &roff,         kDataTypeV, nval,   true },
    { "L.ped",            &lped,         kDataTypeV, nval,   true },
    { "R.ped",            &rped,         kDataTypeV, nval,   true },
    { "L.gain",           &lgain,        kDataTypeV, nval,   true },
    { "R.gain",           &rgain,        kDataTypeV, nval,   true },
    { "Cn",               &fCn,          kDataType                },
    { "MIP",              &adcmip,       kDataType,  0,      true },
    // timewalk coefficients for tw = coeff*(1./sqrt(ADC-Ped)-1./sqrt(ADCMip))
    // TODO: Perhaps the timewalk parameters should be split into L/R blocks?
    // Currently, these need to be 2*nelem numbers, first nelem for the RPMTs,
    // then another nelem for the LPMTs. Easy to mix up.
    { "timewalk_params",  &twalk,        kDataTypeV, 2*nval, true },
    { "avgres",           &fResolution,  kDataType,  0,      true },
    { "atten",            &fAttenuation, kDataType,  0,      true },
    { nullptr }
  };
  err = LoadDB( file, date, calib_request, fPrefix );
  fclose(file);
  if( err )
    return err;

  // Copy calibration constants to the PMTData in fDetectorData
  for( UInt_t i = 0; i < nval; ++i ) {
    auto& calibR = fRightPMTs->GetCalib(i);
    auto& calibL = fLeftPMTs->GetCalib(i);
    calibR.tdc2t = tdc2t;
    calibR.off   = roff[i];
    calibR.ped   = rped[i];
    calibR.gain  = rgain[i];
    calibR.mip   = adcmip;
    calibL.tdc2t = tdc2t;
    calibL.off   = loff[i];
    calibL.ped   = lped[i];
    calibL.gain  = lgain[i];
    calibR.mip   = adcmip;
    if( !twalk.empty() ) {
      calibR.twalk = twalk[i];
      calibL.twalk = twalk[nval+i];
    } else {
      calibR.twalk = calibL.twalk = 0;
    }
  }
  if( fResolution == kBig )
    fResolution = 3.*tdc2t; // guess at timing resolution

#ifdef WITH_DEBUG
  // Debug printout
  if ( fDebug > 2 ) {
    const auto N = static_cast<UInt_t>(fNelem);
    Double_t pos[3]; fOrigin.GetXYZ(pos);
    DBRequest list[] = {
      { "Number of paddles",    &fNelem,       kInt            },
      { "Detector position",    pos,           kDouble,    3   },
      { "Detector size",        fSize,         kDouble,    3   },
      { "TDC offsets Left",     &loff,         kDataTypeV, N   },
      { "TDC offsets Right",    &roff,         kDataTypeV, N   },
      { "ADC pedestals Left",   &lped,         kDataTypeV, N   },
      { "ADC pedestals Right",  &rped,         kDataTypeV, N   },
      { "ADC gains Left",       &lgain,        kDataTypeV, N   },
      { "ADC gains Right",      &rgain,        kDataTypeV, N   },
      { "TDC resolution",       &tdc2t,        kDataType       },
      { "TDC mode",             &tdc_mode,     kInt            },
      { "Light propag. speed",  &fCn,          kDataType       },
      { "ADC MIP",              &adcmip,       kDataType       },
      { "Timewalk params",      &twalk,        kDataTypeV, 2*N },
      { "Time resolution",      &fResolution,  kDataType       },
      { "Attenuation",          &fAttenuation, kDataType       },
      { nullptr }
    };
    DebugPrint( list );
  }
#endif

  fIsInit = true;
  return kOK;
}

//_____________________________________________________________________________
Int_t THaScintillator::DefineVariables( EMode mode )
{
  // Define global analysis variables

  // Add variables for left- and right-side PMT raw data
  class VarDefInfo {
  public:
    PMTData* pmtData;
    const char* key_prefix;
    const char* comment_subst;
    Int_t DefineVariables( EMode mode ) const  // Convenience function
    { return pmtData->DefineVariables(mode, key_prefix, comment_subst); }
  };
  const vector<VarDefInfo> sides = {
    { fRightPMTs, "r", "right-side" },
    { fLeftPMTs,  "l", "left-side"  }
  };
  for( const auto& side : sides )
    if( Int_t ret = side.DefineVariables(mode) )
      return ret;

  // Define variables on the remaining event data
  RVarDef vars[] = {
    { "nthit",  "Number of paddles with L&R TDCs",   "GetNHits()" },
    { "t_pads", "Paddles with L&R coincidence TDCs", "fHits.pad" },
    { "y_t",    "y-position from timing (m)",        "fPadData.yt" },
    { "y_adc",  "y-position from amplitudes (m)",    "fPadData.ya" },
    { "time",   "Time of hit at plane (s)",          "fPadData.time" },
    { "dtime",  "Est. uncertainty of time (s)",      "fPadData.dtime" },
    { "dedx",   "dEdX-like deposited in paddle",     "fPadData.ampl" },
    { "hit.y_t","y-position from timing (m)",        "fHits.yt" },
    { "hit.y_adc", "y-position from amplitudes (m)", "fHits.ya" },
    { "hit.time",  "Time of hit at plane (s)",       "fHits.time" },
    { "hit.dtime", "Est. uncertainty of time (s)",   "fHits.dtime" },
    { "hit.dedx"  ,"dEdX-like deposited in paddle",  "fHits.ampl" },
    { "trdx",   "track deviation in x-position (m)", "fTrackProj.THaTrackProj.fdX" },
    { "trpad",  "paddle-hit associated with track",  "fTrackProj.THaTrackProj.fChannel" },
    { nullptr }
  };
  Int_t ret = DefineVarsFromList( vars, mode );
  if( ret )
    return ret;

  // Define general detector variables (track crossing coordinates etc.)
  // Objects in fDetectorData whose variables are not yet set up will be set up
  // as well. Our PMTData have already been initialized above & will be skipped.
  return THaNonTrackingDetector::DefineVariables(mode);
}

//_____________________________________________________________________________
THaScintillator::~THaScintillator()
{
  // Destructor. Remove variables from global list.

  RemoveVariables();
}

//_____________________________________________________________________________
void THaScintillator::Clear( Option_t* opt )
{
  // Reset per-event data.

  THaNonTrackingDetector::Clear(opt);
  fHitIdx.clear();
  for_each(ALL(fPadData), []( HitData_t& d ) { d.clear(); });
  fHits.clear();
}

//_____________________________________________________________________________
Int_t THaScintillator::StoreHit( const DigitizerHitInfo_t& hitinfo, UInt_t data )
{
  // Put decoded frontend data into fDetectorData.
  // Called from ThaDetectorBase::Decode().
  //
  // hitinfo: channel info (crate/slot/channel/hit/type)
  // data:    data registered in this channel

  static_assert(kRight == 0 || kLeft == 0, "kRight or kLeft must be 0");
  assert(fNviews == abs(kLeft - kRight) + 1);

  Int_t pad = hitinfo.lchan % fNelem;
  auto side = static_cast<ESide>(GetView(hitinfo));

  // Make a note that this side/pad registered some kind of data
  fHitIdx.emplace(side, pad);

  // Store data for either left or right PMTs, as determined by 'side'
  Podd::PMTData* pmtData = (side == kRight) ? fRightPMTs : fLeftPMTs;
  if( !pmtData->HitDone() )
    pmtData->StoreHit(hitinfo, data);

  return 0;
}

//_____________________________________________________________________________
void THaScintillator::PrintDecodedData( const THaEvData& evdata ) const
{
  // Print decoded data (for debugging). Called form Decode()

  //  cout << endl << endl;
  cout << "Event " << evdata.GetEvNum() << "   Trigger " << evdata.GetEvType()
       << " Scintillator " << GetPrefix() << endl;
  cout << "   paddle  Left(TDC    ADC   ADC_p)  Right(TDC    ADC   ADC_p)" << endl;
  cout << right;
  for( int i = 0; i < fNelem; i++ ) {
    const auto& LPMT = fLeftPMTs->GetPMT(i);
    const auto& RPMT = fRightPMTs->GetPMT(i);
    cout << "     "      << setw(2) << i + 1;
    cout << "        ";  WriteValue(LPMT.tdc);
    cout << "  ";        WriteValue(LPMT.adc);
    cout << "  ";        WriteValue(LPMT.adc_p);
    cout << "        ";  WriteValue(RPMT.tdc);
    cout << "  ";        WriteValue(RPMT.adc);
    cout << "  ";        WriteValue(RPMT.adc_p);
    cout << endl;
  }
  cout << left;
}

//_____________________________________________________________________________
Int_t THaScintillator::Decode( const THaEvData& evdata )
{
  // Decode scintillator data, correct TDC times and ADC amplitudes, and copy
  // the data to the local data members.
  // Additionally, apply timewalk corrections and find "paddle hits" (= hits
  // with TDC signals on both sides).

  THaNonTrackingDetector::Decode(evdata);

  ApplyCorrections();
  FindPaddleHits();

  return static_cast<Int_t>(fRightPMTs->GetHitCount().tdc +
                            fLeftPMTs->GetHitCount().tdc);
}

//_____________________________________________________________________________
Int_t THaScintillator::ApplyCorrections()
{
  // Apply ADC/TDC corrections which are possible without tracking.
  //
  // Currently only TDC timewalk corrections are applied, and those only if
  // the database parameters "MIP" and "timewalk_params" are set.

  for( const auto& idx : fHitIdx ) {
    ESide side = idx.first;
    Int_t pad = idx.second;
    auto& PMT = (side == kRight) ? fRightPMTs->GetPMT(pad)
                                 : fLeftPMTs->GetPMT(pad);
    if( PMT.nadc > 0 && PMT.ntdc > 0 )
      PMT.tdc_c -= TimeWalkCorrection(idx, PMT.adc_p);
  }

  return 0;
}

//_____________________________________________________________________________
Data_t THaScintillator::TimeWalkCorrection( Idx_t idx, Data_t adc )
{
  // Calculate TDC timewalk correction.
  // The timewalk parameters depend on the specific PMT given by 'idx'.
  // 'adc' is the PMT's ADC value above pedestal (adc_p).

  ESide side = idx.first;
  Int_t pad  = idx.second;

  const PMTCalib_t& calib = (side == kRight) ? fRightPMTs->GetCalib(pad)
                                             : fLeftPMTs->GetCalib(pad);
  Data_t par = calib.twalk;
  Data_t ref = calib.mip;
  if ( adc <=0 || ref <= 0 || par == 0)
    return 0;

  // Traditional correction according to
  // J.S.Brown et al., NIM A221, 503 (1984); T.Dreyer et al., NIM A309, 184 (1991)
  // Assumes that for a MIP (peak ~2000 ADC channels) the correction is 0
  Data_t corr = par * ( 1./TMath::Sqrt(adc) - 1./TMath::Sqrt(ref) );

  return corr;
}

//_____________________________________________________________________________
Int_t THaScintillator::FindPaddleHits()
{
  // Find paddles with TDC hits on both sides (likely true hits)

  static const Double_t sqrt2 = TMath::Sqrt(2.);

  fHits.clear();
  for( const auto& idx : fHitIdx ) {
    const ESide side = idx.first;
    if( side == kLeft )
      // std::pair is sorted by first, then second, so from this point on only
      // kLeft elements will follow, the matching ones of which we've already
      // paired up. So, no more work to do :)
      break;
    assert(side == kRight);
    const Int_t pad = idx.second;
    if( fHitIdx.find(make_pair(kLeft, pad)) != fHitIdx.end()) {
      // There are data from both PMTs of this paddle
      const auto &RPMT = fRightPMTs->GetPMT(pad), &LPMT = fLeftPMTs->GetPMT(pad);

      // Calculate mean time and rough transverse (y) position
      if( RPMT.ntdc > 0 && LPMT.ntdc > 0 ) {
        Data_t time = 0.5 * (RPMT.tdc_c + LPMT.tdc_c) - fSize[1] / fCn;
        Data_t dtime = fResolution / sqrt2;
        Data_t yt = 0.5 * fCn * (RPMT.tdc_c - LPMT.tdc_c);

        // Record a hit on this paddle
        fHits.emplace_back(pad, time, dtime, yt, kBig, kBig);
        // Also save the hit data in the per-paddle array
        fPadData[pad] = fHits.back();
      }
    }
  }

  // Sort hits by mean time, earliest first
  std::sort( ALL(fHits) );

  return 0;
}

//_____________________________________________________________________________
Int_t THaScintillator::CoarseProcess( TClonesArray& tracks )
{
  // Scintillator coarse processing:
  //
  // - Calculate rough transverse position and energy deposition from ADC data
  // - Calculate rough track crossing points

  for( const auto& idx : fHitIdx ) {
    const ESide side = idx.first;
    if( side == kLeft )
      break;
    assert(side == kRight);
    const Int_t pad = idx.second;
    if( fHitIdx.find(make_pair(kLeft, pad)) != fHitIdx.end()) {
      const auto &RPMT = fRightPMTs->GetPMT(pad), &LPMT = fLeftPMTs->GetPMT(pad);

      // rough calculation of position from ADC reading
      if( RPMT.nadc > 0 && RPMT.adc_c > 0 && LPMT.nadc > 0 && LPMT.adc_c > 0 ) {
        auto& thePad = fPadData[pad];
        thePad.ya = TMath::Log(LPMT.adc_c / RPMT.adc_c) / (2. * fAttenuation);

        // rough dE/dX-like quantity, not correcting for track angle
        thePad.ampl = TMath::Sqrt(LPMT.adc_c * RPMT.adc_c *
          TMath::Exp(fAttenuation * 2. * fSize[1])) / fSize[2];

        // Save these ADC-derived values to the entry in the hit array as well
        // (may not exist if TDCs didn't fire on both sides)
        auto theHit = find_if(ALL(fHits),
          [pad]( const HitData_t& h ) { return h.pad == pad; });
        if( theHit != fHits.end() ) {
          theHit->yt = thePad.yt;
          theHit->ampl = thePad.ampl;
        }
      }
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
  Int_t n_cross = CalcTrackProj(tracks);

  // Find the closest hits to the track crossing points
  if( n_cross > 0 ) {
    Double_t dpadx = 2.0 * fSize[0] / fNelem;   // Width of a paddle
    Double_t padx0 = -fSize[0] + 0.5 * dpadx;   // center of paddle '0'
    for( Int_t i = 0; i < fTrackProj->GetLast() + 1; i++ ) {
      auto* proj = static_cast<THaTrackProj*>( fTrackProj->At(i));
      assert(proj);
      if( !proj->IsOK())
        continue;
      Int_t pad = -1;                      // paddle number of closest hit
      Double_t xc = proj->GetX();          // track intercept x-coordinate
      Double_t dx = kBig;                  // xc - distance paddle center
      for( const auto& h : fHits ) {
        Double_t dx2 = xc - (padx0 + h.pad * dpadx);
        if( TMath::Abs(dx2) < TMath::Abs(dx)) {
          pad = h.pad;
          dx = dx2;
        }
      }
      assert(pad >= 0 || fHits.empty());   // Must find a pad unless no hits
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
