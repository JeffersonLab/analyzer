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
#include <vector>

using namespace std;
using namespace Podd;

//_____________________________________________________________________________
THaScintillator::THaScintillator( const char* name, const char* description,
				  THaApparatus* apparatus )
  : THaNonTrackingDetector(name,description,apparatus)
  , fCn(0)
  , fAttenuation(0)
  , fResolution(0)
  , fRightPMTs(nullptr)
  , fLeftPMTs(nullptr)
{
  // Constructor

  fNviews = 2;
}

//_____________________________________________________________________________
THaScintillator::THaScintillator()
  : fCn(0)
  , fAttenuation(0)
  , fResolution(0)
  , fRightPMTs(nullptr)
  , fLeftPMTs(nullptr)
{
  // Default constructor (for ROOT RTTI)

  fNviews = 2;
}

//_____________________________________________________________________________
Int_t THaScintillator::ReadDatabase( const TDatime& date )
{
  // Read this detector's parameters from the database

  const char* const here = "ReadDatabase";

  constexpr VarType kDataType  = std::is_same_v<Data_t, Float_t> ? kFloat  : kDouble;
  constexpr VarType kDataTypeV = std::is_same_v<Data_t, Float_t> ? kFloatV : kDoubleV;

  FILE* file = OpenFile(date);
  if( !file )
    return kFileError;

  // Read fOrigin and fSize (required!)
  Int_t err = ReadGeometry( file, date, true );
  if( err ) {
    (void)fclose(file);
    return err;
  }

  enum : Char_t { kModeUnset = -1, kCommonStop = 0, kCommonStart = 1 };

  vector<Int_t> detmap;
  Int_t nelem = 0;
  Int_t tdc_mode = kModeUnset;
  Data_t tdc2t = 5e-10;  // TDC resolution (s/channel), for reference, required anyway
  detmap.reserve(24);

  // Read configuration parameters
  const vector<DBRequest> config_request = {
    { .name = "detmap",       .var = &detmap,   .type = kIntV },
    { .name = "npaddles",     .var = &nelem,    .type = kInt  },
    { .name = "tdc.res",      .var = &tdc2t,    .type = kDataType },
    { .name = "tdc.cmnstart", .var = &tdc_mode, .type = kInt, .optional = true },
  };
  err = LoadDB( file, date, config_request );

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

  if( !err ) {
    if( UInt_t flags = THaDetMap::kFillLogicalChannel | THaDetMap::kFillModel;
        FillDetMap(detmap, flags, here) <= 0 ) {
      err = kInitError;  // Error already printed by FillDetMap
    }
  }

  const UInt_t nval = fNelem;
  if( !err ) {
    if( UInt_t tot_nchan = fDetMap->GetTotNumChan(); tot_nchan != 4 * nval ) {
      Error(Here(here), "Number of detector map channels (%u) "
                        "inconsistent with 4*number of paddles (%u)",
            tot_nchan, 4 * nval);
      err = kInitError;
    }
  }

  if( err ) {
    (void)fclose(file);
    return err;
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

  // Set up storage for basic detector data
  // Per-event data
  fChannelData.clear();
  for( int i = kRight; i <= kLeft; ++i ) {
    auto detdata = make_unique<TDCData>(this);
    // Keep pointers to the elements around for convenient access
    TDCData*& pmtData = (i == kRight) ? fRightPMTs : fLeftPMTs;
    pmtData = detdata.get();
    assert(pmtData->GetSize() == nval);
    fChannelData.emplace_back(std::move(detdata));
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
  const vector<DBRequest> calib_request = {
    { .name = "L.off",            .var = &loff,         .type = kDataTypeV, .nelem = nval,   .optional = true },
    { .name = "R.off",            .var = &roff,         .type = kDataTypeV, .nelem = nval,   .optional = true },
    { .name = "L.ped",            .var = &lped,         .type = kDataTypeV, .nelem = nval,   .optional = true },
    { .name = "R.ped",            .var = &rped,         .type = kDataTypeV, .nelem = nval,   .optional = true },
    { .name = "L.gain",           .var = &lgain,        .type = kDataTypeV, .nelem = nval,   .optional = true },
    { .name = "R.gain",           .var = &rgain,        .type = kDataTypeV, .nelem = nval,   .optional = true },
    { .name = "Cn",               .var = &fCn,          .type = kDataType },
    { .name = "MIP",              .var = &adcmip,       .type = kDataType,  .nelem = 0,      .optional = true },
    // timewalk coefficients for tw = coeff*(1./sqrt(ADC-Ped)-1./sqrt(ADCMip))
    // TODO: Perhaps the timewalk parameters should be split into L/R blocks?
    // Currently, these need to be 2*nelem numbers, first nelem for the RPMTs,
    // then another nelem for the LPMTs. Easy to mix up.
    { .name = "timewalk_params",  .var = &twalk,        .type = kDataTypeV, .nelem = 2*nval, .optional = true },
    { .name = "avgres",           .var = &fResolution,  .type = kDataType,  .nelem = 0,      .optional = true },
    { .name = "atten",            .var = &fAttenuation, .type = kDataType,  .nelem = 0,      .optional = true },
  };
  err = LoadDB( file, date, calib_request );
  (void)fclose(file);
  if( err )
    return err;

  // Copy calibration constants to the PMTData in fChannelData
  for( UInt_t i = 0; i < nval; ++i ) {
    auto& calibR = fRightPMTs->GetCalib(i);
    auto& calibL = fLeftPMTs->GetCalib(i);
    // calibR.tdc2t   = tdc2t;
    // if( !roff.empty() )
    //   calibR.off   = roff[i];
    // if( !rped.empty() )
    //   calibR.ped   = rped[i];
    // if( !rgain.empty() )
    //   calibR.gain  = rgain[i];
    // calibR.mip     = adcmip;
    // calibL.tdc2t   = tdc2t;
    // if( !loff.empty() )
    //   calibL.off   = loff[i];
    // if( !lped.empty() )
    //   calibL.ped   = lped[i];
    // if( !lgain.empty() )
    //   calibL.gain  = lgain[i];
    // calibR.mip     = adcmip;
    // if( !twalk.empty() ) {
    //   calibR.twalk = twalk[i];
    //   calibL.twalk = twalk[nval+i];
    // } else {
    //   calibR.twalk = calibL.twalk = 0;
    // }
  }
  if( fResolution == kBig )
    fResolution = 3.*tdc2t; // guess at timing resolution

#ifdef WITH_DEBUG
  // Debug printout
  if ( fDebug > 2 ) {
    const auto N = static_cast<UInt_t>(fNelem);
    Double_t pos[3]; fOrigin.GetXYZ(pos);
    const vector<DBRequest> list = {
      { .name = "Number of paddles",    .var = &fNelem,       .type = kInt                     },
      { .name = "Detector position",    .var = pos,                               .nelem = 3   },
      { .name = "Detector size",        .var = fSize,                             .nelem = 3   },
      { .name = "TDC offsets Left",     .var = &loff,         .type = kDataTypeV, .nelem = N   },
      { .name = "TDC offsets Right",    .var = &roff,         .type = kDataTypeV, .nelem = N   },
      { .name = "ADC pedestals Left",   .var = &lped,         .type = kDataTypeV, .nelem = N   },
      { .name = "ADC pedestals Right",  .var = &rped,         .type = kDataTypeV, .nelem = N   },
      { .name = "ADC gains Left",       .var = &lgain,        .type = kDataTypeV, .nelem = N   },
      { .name = "ADC gains Right",      .var = &rgain,        .type = kDataTypeV, .nelem = N   },
      { .name = "TDC resolution",       .var = &tdc2t,        .type = kDataType                },
      { .name = "TDC mode",             .var = &tdc_mode,     .type = kInt                     },
      { .name = "Light propag. speed",  .var = &fCn,          .type = kDataType                },
      { .name = "ADC MIP",              .var = &adcmip,       .type = kDataType                },
      { .name = "Timewalk params",      .var = &twalk,        .type = kDataTypeV, .nelem = 2*N },
      { .name = "Time resolution",      .var = &fResolution,  .type = kDataType                },
      { .name = "Attenuation",          .var = &fAttenuation, .type = kDataType                },
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
    TDCData* pmtData;
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
  const vector<RVarDef> vars = {
    { .name = "nthit",     .desc = "Number of paddles with L&R TDCs",   .def = "GetNHits()" },
    { .name = "t_pads",    .desc = "Paddles with L&R coincidence TDCs", .def = "fHits.pad" },
    { .name = "y_t",       .desc = "y-position from timing (m)",        .def = "fPadData.yt" },
    { .name = "y_adc",     .desc = "y-position from amplitudes (m)",    .def = "fPadData.ya" },
    { .name = "time",      .desc = "Time of hit at plane (s)",          .def = "fPadData.time" },
    { .name = "dtime",     .desc = "Est. uncertainty of time (s)",      .def = "fPadData.dtime" },
    { .name = "dedx",      .desc = "dEdX-like deposited in paddle",     .def = "fPadData.ampl" },
    { .name = "hit.y_t",   .desc = "y-position from timing (m)",        .def = "fHits.yt" },
    { .name = "hit.y_adc", .desc = "y-position from amplitudes (m)",    .def = "fHits.ya" },
    { .name = "hit.time",  .desc = "Time of hit at plane (s)",          .def = "fHits.time" },
    { .name = "hit.dtime", .desc = "Est. uncertainty of time (s)",      .def = "fHits.dtime" },
    { .name = "hit.dedx",  .desc = "dEdX-like deposited in paddle",     .def = "fHits.ampl" },
    { .name = "trdx",      .desc = "track deviation in x-position (m)", .def = "fTrackProj.THaTrackProj.fdX" },
    { .name = "trpad",     .desc = "paddle-hit associated with track",  .def = "fTrackProj.THaTrackProj.fChannel" },
    { .name = nullptr }
  };

  if( Int_t ret = DefineVarsFromList(vars, mode) )
    return ret;

  // Define general detector variables (track crossing coordinates etc.)
  // Objects in fChannelData whose variables are not yet set up will be set up
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
  // Put decoded frontend data into fChannelData.
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
  Podd::TDCData* pmtData = (side == kRight) ? fRightPMTs : fLeftPMTs;
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
    const auto& LPMT = fLeftPMTs->GetTDC(i);
    const auto& RPMT = fRightPMTs->GetTDC(i);
    cout << "     "      << setw(2) << i + 1;
    // cout << "        ";  WriteValue(LPMT.tdc);
    // cout << "  ";        WriteValue(LPMT.adc);
    // cout << "  ";        WriteValue(LPMT.adc_p);
    // cout << "        ";  WriteValue(RPMT.tdc);
    // cout << "  ";        WriteValue(RPMT.adc);
    // cout << "  ";        WriteValue(RPMT.adc_p);
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

  return 0;
  // return static_cast<Int_t>(fRightPMTs->GetHitCount().tdc +
  //                           fLeftPMTs->GetHitCount().tdc);
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
    auto& PMT = (side == kRight) ? fRightPMTs->GetTDC(pad)
                                 : fLeftPMTs->GetTDC(pad);
    // if( PMT.nadc > 0 && PMT.ntdc > 0 )
    //   PMT.tdc_c -= TimeWalkCorrection(idx, PMT.adc_p);
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

  const TDCCalib_t& calib = (side == kRight) ? fRightPMTs->GetCalib(pad)
                                             : fLeftPMTs->GetCalib(pad);
  Data_t par = 0;//calib.twalk;
  Data_t ref = 0;//calib.mip;
  if ( adc <=0 || ref <= 0 || par == 0)
    return 0;

  // Traditional correction according to W.Braunschweig et al., NIM 134, 261 (1976).
  // See also J.S.Brown et al., NIM 221, 503 (1984); T.Dreyer et al., NIM A309, 184 (1991).
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
      const auto &RPMT = fRightPMTs->GetTDC(pad), &LPMT = fLeftPMTs->GetTDC(pad);

      // Calculate mean time and rough transverse (y) position
      // if( RPMT.ntdc > 0 && LPMT.ntdc > 0 ) {
        Data_t time = 0.5 * (RPMT.tdc_c + LPMT.tdc_c) - fSize[1] / fCn;
        Data_t dtime = fResolution / sqrt2;
        Data_t yt = 0.5 * fCn * (RPMT.tdc_c - LPMT.tdc_c);

        // Record a hit on this paddle
        fHits.emplace_back(pad, time, dtime, yt, kBig, kBig);
        // Also save the hit data in the per-paddle array
        fPadData[pad] = fHits.back();
      }
    // }
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
      const auto &RPMT = fRightPMTs->GetTDC(pad), &LPMT = fLeftPMTs->GetTDC(pad);

      // rough calculation of position from ADC reading
      // if( RPMT.nadc > 0 && RPMT.adc_c > 0 && LPMT.nadc > 0 && LPMT.adc_c > 0 ) {
        auto& thePad = fPadData[pad];
        // thePad.ya = TMath::Log(LPMT.adc_c / RPMT.adc_c) / (2. * fAttenuation);
        //
        // // rough dE/dX-like quantity, not correcting for track angle
        // thePad.ampl = TMath::Sqrt(LPMT.adc_c * RPMT.adc_c *
        //   TMath::Exp(fAttenuation * 2. * fSize[1])) / fSize[2];

        // Save these ADC-derived values to the entry in the hit array as well
        // (may not exist if TDCs didn't fire on both sides)
        auto theHit = find_if(ALL(fHits),
          [pad]( const HitData_t& h ) { return h.pad == pad; });
        if( theHit != fHits.end() ) {
          theHit->yt = thePad.yt;
          theHit->ampl = thePad.ampl;
        }
      // }
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
