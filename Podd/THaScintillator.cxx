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

#include <cstring>
#include <iostream>
#include <cassert>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <type_traits>

#define ALL(c) (c).begin(), (c).end()

using namespace std;

//_____________________________________________________________________________
THaScintillator::THaScintillator( const char* name, const char* description,
				  THaApparatus* apparatus )
  : THaNonTrackingDetector(name,description,apparatus),
    fTdc2T(0.), fCn(0.), fAdcMIP(0.), fAttenuation(0.), fResolution(0.)
{
  // Constructor
}

//_____________________________________________________________________________
THaScintillator::THaScintillator() : THaNonTrackingDetector(),
   fTdc2T(0.), fCn(0.), fAdcMIP(0.), fAttenuation(0), fResolution(0)
{
  // Default constructor (for ROOT RTTI)
}

//_____________________________________________________________________________
Int_t THaScintillator::ReadDatabase( const TDatime& date )
{
  // Read this detector's parameters from the database

  const char* const here = "ReadDatabase";

  static_assert( std::is_same<Data_t, Float_t>::value ||
                 std::is_same<Data_t, Double_t>::value,
                 "Data_t must be Float_t or Double_t" );
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

  vector<Int_t> detmap;
  Int_t nelem;
  Int_t tdc_mode = -255; // indicates unset
  fTdc2T = 5e-10;  // TDC resolution (s/channel), for reference, required anyway

  // Read configuration parameters
  DBRequest config_request[] = {
    { "detmap",       &detmap,   kIntV },
    { "npaddles",     &nelem,    kInt  },
    { "tdc.res",      &fTdc2T,   kDataType },
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
  UInt_t nval = fNelem;
  if( !fIsInit ) {
    // Calibration data
    fCalib[kRight].resize(nval);
    fCalib[kLeft].resize(nval);
    fTWalkPar[kRight].reserve(nval);
    fTWalkPar[kLeft].reserve(nval);

    // Per-event data
    fRightPMTs.resize(nval);
    fLeftPMTs.resize(nval);
    fPadData.resize(nval);
    fHits.reserve(nval);

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
  fTWalkPar[kRight].clear();
  fTWalkPar[kLeft].clear();

  // Default TDC offsets (0), ADC pedestals (0) and ADC gains (1)
  for_each( ALL(fCalib[kRight]), [](PMTCalib_t& c){ c.clear(); });
  for_each( ALL(fCalib[kLeft]),  [](PMTCalib_t& c){ c.clear(); });

  vector<Data_t> loff, roff, lped, rped, lgain, rgain, twalk;
  DBRequest calib_request[] = {
    { "L.off",            &loff,         kDataTypeV, nval,   true },
    { "R.off",            &roff,         kDataTypeV, nval,   true },
    { "L.ped",            &lped,         kDataTypeV, nval,   true },
    { "R.ped",            &rped,         kDataTypeV, nval,   true },
    { "L.gain",           &lgain,        kDataTypeV, nval,   true },
    { "R.gain",           &rgain,        kDataTypeV, nval,   true },
    { "Cn",               &fCn,          kDataType                },
    { "MIP",              &fAdcMIP,      kDataType,  0,      true },
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

  for( UInt_t i = 0; i < nval; ++i ) {
    fCalib[kRight][i].off  = roff[i];
    fCalib[kRight][i].ped  = rped[i];
    fCalib[kRight][i].gain = rgain[i];
    fCalib[kLeft][i].off   = loff[i];
    fCalib[kLeft][i].ped   = lped[i];
    fCalib[kLeft][i].gain  = lgain[i];
  }
  if( !twalk.empty() ) {
    for( UInt_t i = 0; i < nval; ++i ) {
      fTWalkPar[kRight][i] = twalk[i];
      fTWalkPar[kLeft][i]  = twalk[nval+i];
    }
  }
  if( fResolution == kBig )
    fResolution = 3.*fTdc2T; // guess at timing resolution

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
      { "TDC resolution",       &fTdc2T,       kDataType       },
      { "Light propag. speed",  &fCn,          kDataType       },
      { "ADC MIP",              &fAdcMIP,      kDataType       },
      { "Timewalk params",      &twalk,        kDataTypeV, 2*N },
      { "Time resolution",      &fResolution,  kDataType       },
      { "Attenuation",          &fAttenuation, kDataType       },
      { nullptr }
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
    { "nlthit", "Number of Left paddles TDC times",  "fNHits[1].tdc" },
    { "nrthit", "Number of Right paddles TDC times", "fNHits[0].tdc" },
    { "nlahit", "Number of Left paddles ADCs amps",  "fNHits[0].tdc" },
    { "nrahit", "Number of Right paddles ADCs amps", "fNHits[0].adc" },
    { "lt",     "TDC values left side",              "fLeftPMTs.THaScintillator::PMTData_t.tdc" },
    { "lt_c",   "Calibrated times left side (s)",    "fLeftPMTs.THaScintillator::PMTData_t.tdc_c" },
    { "rt",     "TDC values right side",             "fRightPMTs.THaScintillator::PMTData_t.tdc" },
    { "rt_c",   "Calibrated times right side (s)",   "fRightPMTs.THaScintillator::PMTData_t.tdc_c" },
    { "la",     "ADC values left side",              "fLeftPMTs.THaScintillator::PMTData_t.adc" },
    { "la_p",   "Ped-sub ADC values left side",      "fLeftPMTs.THaScintillator::PMTData_t.adc_p" },
    { "la_c",   "Corrected ADC values left side",    "fLeftPMTs.THaScintillator::PMTData_t.adc_c" },
    { "ra",     "ADC values right side",             "fRightPMTs.THaScintillator::PMTData_t.adc" },
    { "ra_p",   "Ped-sub ADC values right side",     "fRightPMTs.THaScintillator::PMTData_t.adc_p" },
    { "ra_c",   "Corrected ADC values right side",   "fRightPMTs.THaScintillator::PMTData_t.adc_c" },
    { "nthit",  "Number of paddles with l&r TDCs",   "GetNHits()" },
    { "t_pads", "Paddles with l&r coincidence TDCs", "fHits.THaScintillator::HitData_t.pad" },
    { "y_t",    "y-position from timing (m)",        "fPadData.THaScintillator::HitData_t.yt" },
    { "y_adc",  "y-position from amplitudes (m)",    "fPadData.THaScintillator::HitData_t.ya" },
    { "time",   "Time of hit at plane (s)",          "fPadData.THaScintillator::HitData_t.time" },
    { "dtime",  "Est. uncertainty of time (s)",      "fPadData.THaScintillator::HitData_t.dtime" },
    { "dedx",   "dEdX-like deposited in paddle",     "fPadData.THaScintillator::HitData_t.ampl" },
    { "hit.y_t","y-position from timing (m)",        "fHits.THaScintillator::HitData_t.yt" },
    { "hit.y_adc", "y-position from amplitudes (m)", "fHits.THaScintillator::HitData_t.ya" },
    { "hit.time",  "Time of hit at plane (s)",       "fHits.THaScintillator::HitData_t.time" },
    { "hit.dtime", "Est. uncertainty of time (s)",   "fHits.THaScintillator::HitData_t.dtime" },
    { "hit.dedx"  ,"dEdX-like deposited in paddle",  "fHits.THaScintillator::HitData_t.ampl" },
    { "trn",    "Number of tracks for hits",         "GetNTracks()" },
    { "trx",    "x-position of track in det plane",  "fTrackProj.THaTrackProj.fX" },
    { "try",    "y-position of track in det plane",  "fTrackProj.THaTrackProj.fY" },
    { "trpath", "TRCS pathlen of track to det plane","fTrackProj.THaTrackProj.fPathl" },
    { "trdx",   "track deviation in x-position (m)", "fTrackProj.THaTrackProj.fdX" },
    { "trpad",  "paddle-hit associated with track",  "fTrackProj.THaTrackProj.fChannel" },
    { nullptr }
  };
  return DefineVarsFromList( vars, mode );
}

//_____________________________________________________________________________
THaScintillator::~THaScintillator()
{
  // Destructor. Remove variables from global list.

  if( fIsSetup )
    RemoveVariables();
}

//_____________________________________________________________________________
void THaScintillator::Clear( Option_t* opt )
{
  // Reset per-event data.

  THaNonTrackingDetector::Clear(opt);
  for( auto& n : fNHits )
    n.clear();
  fHitIdx.clear();
  for_each( ALL(fRightPMTs), [](PMTData_t& d){ d.clear(); });
  for_each( ALL(fLeftPMTs),  [](PMTData_t& d){ d.clear(); });
  for_each( ALL(fPadData),   [](HitData_t& d){ d.clear(); });
  fHits.clear();
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

      // The scintillator detector is assumed to have fNelem paddles, each
      // with two PMTs, one on the right side, one on the left.
      // As for the readout channels, the convention is to configure twice as
      // many logical channels as there are paddles. The first half of the
      // logical channels corresponds to the right-side PMTs, the second,
      // to the left-side ones.
      if( k<0 || k > NSIDES * fNelem ) {
	// Indicates bad database
	Error( Here(here), "Illegal detector channel: %d. "
	       "Fix detector map in database", k );
	continue;
      }
      Int_t pad = k % fNelem; // Actual paddle number

      // Copy the raw and calibrated ADC/TDC data to the member variables
      ESide side = k<fNelem ? kRight : kLeft;
      auto& PMT = (side==kRight) ? fRightPMTs[pad] : fLeftPMTs[pad];
      auto& nhits = fNHits[side];
      const auto& calib = fCalib[side][pad];
      if( adc ) {
        PMT.adc   = data;
        PMT.adc_p = PMT.adc - calib.ped;
        PMT.adc_c = PMT.adc_p * calib.gain;
        PMT.adc_set = true;
        nhits.adc++;
      } else {
        PMT.tdc = data;
        PMT.tdc_c = (data - calib.off) * fTdc2T;
        if( fTdc2T > 0.0 && !not_common_stop_tdc ) {
          // For common stop TDCs, time is negatively correlated to raw data
          // time = (offset-data)*res, so reverse the sign.
          // However, historically, people have been using negative TDC
          // resolutions to indicate common stop mode, so in that case
          // the sign flip has already been applied.
          PMT.tdc_c *= -1.0;
        }
        PMT.tdc_set = true;
        nhits.tdc++;
      }
      fHitIdx.emplace(side, pad);
    }
  }

  if( has_warning )
    ++fNEventsWithWarnings;

  ApplyCorrections();
  FindPaddleHits();

#ifdef WITH_DEBUG
  if( fDebug > 3 ) {
    cout << endl << endl;
    cout << "Event " << evdata.GetEvNum() << "   Trigger " << evdata.GetEvType()
	 << " Scintillator " << GetPrefix() << endl;
    cout << "   paddle  Left(TDC    ADC   ADC_p)  Right(TDC    ADC   ADC_p)" << endl;
    cout << right;
    for( int i = 0; i < fNelem; i++ ) {
      cout << "     "     << setw(2) << i+1;
      cout << "        "; WriteValue(fLeftPMTs[i].tdc);
      cout << "  ";       WriteValue(fLeftPMTs[i].adc);
      cout << "  ";       WriteValue(fLeftPMTs[i].adc_p);
      cout << "        "; WriteValue(fRightPMTs[i].tdc);
      cout << "  ";       WriteValue(fRightPMTs[i].adc);
      cout << "  ";       WriteValue(fRightPMTs[i].adc_p);
      cout << endl;
    }
    cout << left;
  }
#endif

  return fNHits[kRight].tdc + fNHits[kLeft].tdc;
}

//_____________________________________________________________________________
Int_t THaScintillator::ApplyCorrections()
{
  // Apply ADC/TDC corrections which are possible without tracking.
  //
  // Currently only TDC timewalk corrections are applied, and those only if
  // the database parameters "MIP" and "timewalk_params" are set.

  for( const auto idx : fHitIdx ) {
    ESide side = idx.first;
    Int_t pad = idx.second;
    auto& PMT = (side == kRight) ? fRightPMTs[pad] : fLeftPMTs[pad];
    if( PMT.adc_set && PMT.tdc_set )
      PMT.tdc_c -= TimeWalkCorrection(idx, PMT.adc_p);
  }

  return 0;
}

//_____________________________________________________________________________
THaScintillator::Data_t
THaScintillator::TimeWalkCorrection( Idx_t idx, Data_t adc )
{
  // Calculate TDC timewalk correction.
  // The timewalk parameters depend on the specific PMT given by 'idx'.
  // 'adc' is the PMT's ADC value above pedestal (adc_p).

  ESide side = idx.first;
  Int_t pad  = idx.second;

  if( fTWalkPar[side].empty() )
    return 0.; // no parameters defined

  // Assume that for a MIP (peak ~2000 ADC channels) the correction is 0
  Data_t ref = fAdcMIP;
  if ( adc <=0. || ref <= 0.)
    return 0.;

  Data_t par = fTWalkPar[side][pad];

  // Traditional correction according to
  // J.S.Brown et al., NIM A221, 503 (1984); T.Dreyer et al., NIM A309, 184 (1991)
  Data_t corr = par * ( 1./TMath::Sqrt(adc) - 1./TMath::Sqrt(ref) );

  return corr;
}

//_____________________________________________________________________________
Int_t THaScintillator::FindPaddleHits()
{
  // Find paddles with TDC hits on both sides (likely true hits)

  static const Double_t sqrt2 = TMath::Sqrt(2.);

  fHits.clear();
  for( const auto idx : fHitIdx ) {
    const ESide side = idx.first;
    if( side == kLeft )
      // std::pair is sorted by first, then second, so from this point on only
      // kLeft elements will follow, the matching ones of which we've already
      // paired up. So no more work to do :)
      break;
    assert(side == kRight);
    const Int_t pad = idx.second;
    if( fHitIdx.find(make_pair(kLeft, pad)) != fHitIdx.end()) {
      // There are data from both PMTs of this paddle
      const auto& RPMT = fRightPMTs[pad], LPMT = fLeftPMTs[pad];

      // Calculate mean time and rough transverse (y) position
      if( RPMT.tdc_set && LPMT.tdc_set ) {
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
  return 0;
}

//_____________________________________________________________________________
Int_t THaScintillator::CoarseProcess( TClonesArray& tracks )
{
  // Scintillator coarse processing:
  //
  // - Calculate rough transverse position and energy deposition from ADC data
  // - Calculate rough track crossing points

  for( const auto idx : fHitIdx ) {
    const ESide side = idx.first;
    if( side == kLeft )
      break;
    assert(side == kRight);
    const Int_t pad = idx.second;
    if( fHitIdx.find(make_pair(kLeft, pad)) != fHitIdx.end()) {
      const auto& RPMT = fRightPMTs[pad], LPMT = fLeftPMTs[pad];

      // rough calculation of position from ADC reading
      if( RPMT.adc_set && RPMT.adc_c > 0 && LPMT.adc_set && LPMT.adc_c > 0 ) {
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
  Int_t n_cross = CalcTrackProj( tracks );

  // Find the closest hits to the track crossing points
  if( n_cross > 0 ) {
    Double_t dpadx = 2.0*fSize[0]/fNelem;   // Width of a paddle
    Double_t padx0 = -fSize[0]+0.5*dpadx;   // center of paddle '0'
    for( Int_t i=0; i<fTrackProj->GetLast()+1; i++ ) {
      auto proj = static_cast<THaTrackProj*>( fTrackProj->At(i) );
      assert( proj );
      if( !proj->IsOK() )
	continue;
      Int_t pad = -1;                      // paddle number of closest hit
      Double_t xc = proj->GetX();          // track intercept x-coordinate
      Double_t dx = kBig;                  // xc - distance paddle center
      for( const auto& h : fHits ) {
        Double_t dx2 = xc - (padx0 + h.pad*dpadx);
	if (TMath::Abs(dx2) < TMath::Abs(dx) ) {
	  pad = h.pad;
	  dx = dx2;
	}
      }
      assert( pad >= 0 || fHits.empty() ); // Must find a pad unless no hits
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
