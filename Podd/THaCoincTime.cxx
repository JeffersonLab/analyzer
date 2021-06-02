//*-- Author :    Rob Feuerbach 9-Sep-03 */

//////////////////////////////////////////////////////////////////////////
//
// THaCoincTime
//
// Calculate coincidence time between tracks in two spectrometers.
//  
//  Loop through all combinations of tracks in the two spectrometers.
//  Here we assume that the time difference+fixed delay between the
//  common TDC starts is measured.
//
//////////////////////////////////////////////////////////////////////////

#include "TMath.h"
#include "THaCoincTime.h"
#include "THaTrack.h"
#include "THaDetMap.h"
#include "THaSpectrometer.h"
#include "THaEvData.h"
#include "VarDef.h"
#include <cassert>

using namespace std;

// Reserve initial space for 10 tracks per spectrometer (can grow dynamically)
#define NTR 10

//_____________________________________________________________________________
THaCoincTime::THaCoincTime( const char* name,
			    const char* description,
			    const char* spec1, const char* spec2,
			    Double_t m1, Double_t m2,
			    const char* ch_name1, const char* ch_name2 )
  : THaPhysicsModule(name,description), fSpectN1(spec1), fSpectN2(spec2),
    fpmass1(m1), fpmass2(m2), fSpect1(nullptr), fSpect2(nullptr),
    fDetMap{new THaDetMap()},
    fTdcRes{0,0}, fTdcOff{0,0}, fTdcData{0, 0}

{
  // Normal constructor.

  fVxTime1.reserve(NTR);
  fVxTime2.reserve(NTR);
  fTimeCombos.reserve(NTR*NTR);

  if (ch_name1 && strlen(ch_name1)>0) {
    fTdcLabels[0] = ch_name1;
  } else {
    fTdcLabels[0] = Form("%sby%s",spec2,spec1);
  } 
  if (ch_name2 && strlen(ch_name2)>0) {
    fTdcLabels[1] = ch_name2;
  } else {
    fTdcLabels[1] = Form("%sby%s",spec1,spec2);
  }
}

//_____________________________________________________________________________
THaCoincTime::~THaCoincTime()
{
  // Destructor
  RemoveVariables();
}

//_____________________________________________________________________________
void THaCoincTime::Clear( Option_t* opt )
{
  // Clear all internal variables
  // some overkill, but is event-independent (back to a known state)
  THaPhysicsModule::Clear(opt);
  fVxTime1.clear();
  fVxTime2.clear();
  fTimeCombos.clear();
}

//_____________________________________________________________________________
Int_t THaCoincTime::DefineVariables( EMode mode )
{
  // Define/delete global variables.
  
  RVarDef vars[] = {
    { "d_trig","Measured TDC start+delay of spec 2 (1), by spec 1 (2)", "fdTdc" },
    { "ntr1",  "Number of tracks in first spec.",  "GetNTr1()" },
    { "ntr2",  "Number of tracks in first spec.",  "GetNTr2()" },
    { "vx_t1", "Time of track from spec1 at target vertex", "fVxTime1" },
    { "vx_t2", "Time of track from spec2 at target vertex", "fVxTime2" },
    { "ncomb", "Number of track combinations considered",   "GetNTimes()" },
    { "ct_2by1", "Coinc. times of tracks, d_trig from spec 1", "fTimeCombos.fDiffT2by1" },
    { "ct_1by2", "Coinc. times of tracks, d_trig from spec 2", "fTimeCombos.fDiffT1by2" },
    { "trind1",  "Track indices for spec1 match entries in ct_*", "fTimeCombos.fTrInd1" },
    { "trind2",  "Track indices for spec2 match entries in ct_*", "fTimeCombos.fTrInd2" },
    { nullptr }
  };
  return DefineVarsFromList( vars, mode );
}

//_____________________________________________________________________________
THaAnalysisObject::EStatus THaCoincTime::Init( const TDatime& run_time )
{
  // Initialize the module.
  

  // Locate the spectrometer apparatus named in fSpectroName and save
  // pointer to it.

  // Standard initialization. Calls this object's DefineVariables().
  if( THaPhysicsModule::Init( run_time ) != kOK )
    return fStatus;

  fSpect1 = dynamic_cast<THaSpectrometer*>
    ( FindModule( fSpectN1, "THaSpectrometer"));
  fSpect2 = dynamic_cast<THaSpectrometer*>
    ( FindModule( fSpectN2, "THaSpectrometer"));
  
  if( !fSpect1 || !fSpect2 )
    fStatus = kInitError;

  return fStatus;
}

//_____________________________________________________________________________
Int_t THaCoincTime::ReadDatabase( const TDatime& date )
{
  // Read configuration parameters from the database.
  // 'date' contains the date/time of the run being analyzed.

  const char* const here = "ReadDatabase";

  FILE* file = OpenFile( date );
  if( !file ) {
    // look for more general coincidence-timing database
    file = Podd::OpenDBFile("CT", date);
  }
  if ( !file )
    return kFileError;

  fDetMap->Clear();

  // the list of detector names to use is in fTdcLabels, set
  // at creation time as "{spec2Name}by{spec1Name}"
  // and the inverse

  Int_t err = 0;
  for( int i=0; i<2 && !err; i++) {
    vector<Int_t> detmap;
    fTdcOff[i] = 0.0;
    DBRequest request[] = {
      { "detmap",      &detmap, kIntV },
      { "tdc_res",     &fTdcRes[i] },
      { "tdc_offset",  &fTdcOff[i], kDouble, 1 },
      { nullptr }
    };
    TString pref(fPrefix); pref.Append(fTdcLabels[i]); pref.Append(".");
    err = LoadDB( file, date, request, pref );

    if( !err ) {
      if( detmap.size() != 6 ) {
	Error( Here(here), "Invalid number of detector map values = %d for "
	       "database key %sdetmap. Must be exactly 6. Fix database.",
	       static_cast<Int_t>(detmap.size()), pref.Data() );
	err = kInitError;
      } else {
	if( detmap[2] != detmap[3] ) {
	  Warning( Here(here), "Detector map %sdetmap must have exactly 1 "
		   "channel. Setting last = first. Fix database.",
		   pref.Data() );
	  detmap[3] = detmap[2];
	}
	Int_t ret =
	  fDetMap->Fill( detmap, THaDetMap::kDoNotClear|THaDetMap::kFillModel|
			 THaDetMap::kFillLogicalChannel );
	if( ret <= 0 ) {
	  Error( Here(here), "Error %d filling detector map element %d",
		 ret, i );
	  err = kInitError;
	}
      }
    }
  }
  fclose(file);
  if( err )
    return err;

  if( fDetMap->GetSize() != 2 ) {
    Error( Here(here), "Unexpected number of detector map modules = %d. "
	   "Must be exactly 2. Fix database.", fDetMap->GetSize() );
    return kInitError;
  }

  fIsInit = true;
  return kOK;
}

//_____________________________________________________________________________
Int_t THaCoincTime::Process( const THaEvData& evdata )
{
  // Read in coincidence TDCs

  const char* const here = "Process";

  if( !IsOK() ) return -1;

  // read in the two relevant channels
  int detmap_pos=0;
  for( double tdcres : fTdcRes ) {
    if( tdcres != 0. ) {
      THaDetMap::Module* d = fDetMap->GetModule(detmap_pos);
      // grab only the first hit in a TDC
      if( evdata.GetNumHits(d->crate, d->slot, d->lo) > 0 ) {
        fTdcData[d->first] =
          evdata.GetData(d->crate, d->slot, d->lo, 0) * fTdcRes[d->first]
          - fTdcOff[d->first];
      }
      detmap_pos++;
    }
  }

  // Calculate the time at the vertex (relative to the trigger time)
  // for each track in each spectrometer
  // Use the Beta of the assumed particle type.
  class Spec_short {
  public:
    THaSpectrometer*  Sp;
    vector<Double_t>& Vxtime;
    Double_t          Mass;
  };

  const vector<Spec_short> SpList = {
    { fSpect1, fVxTime1, fpmass1 },
    { fSpect2, fVxTime2, fpmass2 },
  };

  for( const auto& sp : SpList ) {
    assert(sp.Vxtime.empty());  // else Clear() not called in order
    Int_t ntr = sp.Sp->GetNTracks();
    TClonesArray* tracks = sp.Sp->GetTracks();
    for( Int_t i = 0; i < ntr; i++ ) {
      auto* tr = dynamic_cast<THaTrack*>(tracks->At(i));
      // this should be safe to assume -- only THaTracks go into the tracks array
      if( !tr ) {
        Warning(Here(here), "non-THaTrack in %s's tracks array at %d.",
                sp.Sp->GetName(), i);
        continue;
      }
      Double_t p = tr->GetP();
      if( tr->GetBeta() != 0. && p > 0. ) {
        Double_t beta = p / TMath::Sqrt(p * p + sp.Mass * sp.Mass);
	Double_t c = TMath::C();
        sp.Vxtime.push_back(tr->GetTime() - tr->GetPathLen() / (beta * c));
      } else {
	// Using (i+1)*kBig here prevents differences from being zero
        sp.Vxtime.push_back((i + 1) * kBig);
      }
    }
  }

  // now, we have the vertex times -- go through the combinations

  // Take the tracks and the coincidence TDC and construct
  // the coincidence time
  assert(fTimeCombos.empty());  // else Clear() not called in order
  for( size_t i = 0; i < fVxTime1.size(); i++ ) {
    for( size_t j = 0; j < fVxTime2.size(); j++ ) {
      fTimeCombos.emplace_back( i, j,
                                fVxTime2[j] - fVxTime1[i] + fTdcData[0],
                                fVxTime1[i] - fVxTime2[j] + fTdcData[1]
      );
    }
  }

  fDataValid = true;
  return 0;
}

ClassImp(THaCoincTime)

///////////////////////////////////////////////////////////////////////////////
