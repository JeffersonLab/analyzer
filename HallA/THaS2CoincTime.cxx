//*-- Author :    Rob Feuerbach 9-Sep-03 */

//////////////////////////////////////////////////////////////////////////
//
// THaS2CoincTime
//
// Calculate coincidence time between tracks in two spectrometers.
//  Differs from THaCoincTime in that here the specific detectors to use
//  to get the time tracks in the spectrometers can be named.
//  The default is "s2".
//
//  Loop through all combinations of tracks in the two spectrometers.
//  Here we assume that the time difference+fixed delay between the
//  common TDC starts is measured.
//
//////////////////////////////////////////////////////////////////////////

#include "TLorentzVector.h"
#include "TMath.h"
#include "THaS2CoincTime.h"
#include "THaTrack.h"
#include "THaDetMap.h"
#include "THaSpectrometer.h"
#include "THaEvData.h"
#include "THaVarList.h"
#include "THaGlobals.h"
#include <vector>
#include <cassert>

using namespace std;

//_____________________________________________________________________________
THaS2CoincTime::THaS2CoincTime( const char* name,
				const char* description,
				const char* spec1, const char* spec2,
				Double_t m1, Double_t m2,
				const char* ch_name1, const char* ch_name2,
				const char* detname1, const char* detname2)
  : THaCoincTime(name,description,spec1,spec2,m1,m2,ch_name1,ch_name2),
    fTrPads1(nullptr), fS2TrPath1(nullptr), fS2Times1(nullptr), fTrPath1(nullptr),
    fTrPads2(nullptr), fS2TrPath2(nullptr), fS2Times2(nullptr), fTrPath2(nullptr),
    fDetName1(detname1)
{
  // To construct, specify the name of the module (usually "CT"),
  // a description ("Coincidence time calculation between L and R HRS"),
  // the FIRST and SECOND source of tracks ("L" and "R"),
  // the MASSES in GeV/c^2  of the particles in FIRST and SECOND spectrometers.
  //
  // The rest of the parameters are usually not used.
  //
  // The next two are thes name of the TDC channels in the database for this
  // object measuring the difference between the COMMON STARTS of the FIRST
  // and SECOND spectrometers, such that:
  //
  //  * ch_name1 measures  COMMON_START<sub>2</sub> - COMMON_START<sub>1</sub>,
  //  * ch_name2 measures  COMMON_START<sub>1</sub> - COMMON_START<sub>2</sub>.
  //
  //  These default to "ct_(spec2 name)by(spec1 name)" and
  //  "ct_(spec1 name)by(spec2 name)", respectively.
  //
  //  * detname1 is the name of the timing plane in the FIRST spectrometer to
  //        use to measure the time of the track in this spectrometer.
  //  * detname2 is likewise the name of the timing plane in the SECOND
  //        spectrometer. If not specified, detname1 is used.
  //      
  //  The default arguments are detname1="s2", and detname2 be assigned to
  //  match detname1.

  if( detname2 && *detname2 ) {
    fDetName2=detname2;
  } else {
    fDetName2=fDetName1;
  }
}

//_____________________________________________________________________________
THaS2CoincTime::~THaS2CoincTime() = default;

//_____________________________________________________________________________
THaAnalysisObject::EStatus THaS2CoincTime::Init( const TDatime& run_time )
{
  // Initialize the module. Grab the global variables containing the tracking
  // results (including calculated pathlengths) and the times and pads from the
  // THaScintillator planes.

  // first do the standard initialization
  THaAnalysisObject::EStatus stat = THaCoincTime::Init(run_time);

  if (stat != kOK) return stat;

  if (!fSpect1 || !fSpect2) return kInitError;
  
  fTrPads1  = gHaVars->Find(Form("%s.%s.trpad",fSpect1->GetName(),
				 fDetName1.Data()));
  fS2TrPath1= gHaVars->Find(Form("%s.%s.trpath",fSpect1->GetName(),
				 fDetName1.Data()));
  fS2Times1 = gHaVars->Find(Form("%s.%s.time",fSpect1->GetName(),
				 fDetName1.Data()));
  fTrPath1  = gHaVars->Find(Form("%s.tr.pathl",fSpect1->GetName()));
  if (!fTrPads1 || !fS2TrPath1 || !fS2Times1 || !fTrPath1) {
    Error(Here("Init"),"Cannot get variables for spectrometer %s detector %s",
	  fSpect1->GetName(),fDetName1.Data());
    return kInitError;
  }

  fTrPads2  = gHaVars->Find(Form("%s.%s.trpad",fSpect2->GetName(),
				 fDetName2.Data()));
  fS2TrPath2= gHaVars->Find(Form("%s.%s.trpath",fSpect2->GetName(),
				 fDetName2.Data()));
  fS2Times2 = gHaVars->Find(Form("%s.%s.time",fSpect2->GetName(),
				 fDetName2.Data()));
  fTrPath2  = gHaVars->Find(Form("%s.tr.pathl",fSpect2->GetName()));
  
  if (!fTrPads2 || !fS2TrPath2 || !fS2Times2 || !fTrPath2) {
    Error(Here("Init"),"Cannot get variables for spectrometer %s detector %s",
	  fSpect2->GetName(),fDetName2.Data());
    return kInitError;
  }

  return kOK;
}

//_____________________________________________________________________________
Int_t THaS2CoincTime::Process( const THaEvData& evdata )
{
  // Read in coincidence TDC's, calculate the time of the tracks at the
  // target vertex, and then assemble the time-difference between when the
  // tracks left the target for every possible combination.

  if( !IsOK() ) return -1;

  if (!fDetMap) return -1;

  // read in the two relevant channels
  // Use of the logical channel number ensures matching of the TDC with
  // the correct difference (1by2 vs 2by1)
  for ( UInt_t i=0; i < fDetMap->GetSize(); i++ ) {
    THaDetMap::Module* d = fDetMap->GetModule(i);
    if (fTdcRes[d->first] != 0.) {
      // grab only the first hit in a TDC
      if( evdata.GetNumHits(d->crate, d->slot, d->lo) > 0 ) {
        fTdcData[d->first] =
          evdata.GetData(d->crate, d->slot, d->lo, 0) * fTdcRes[d->first]
          - fTdcOff[d->first];
      }
    }
  }

  // Calculate the time at the vertex (relative to the trigger time)
  // for each track in each spectrometer
  // Use the Beta of the assumed particle type.
  class Spec_short {
  public:
    THaSpectrometer* Sp;
    vector<Double_t>& Vxtime;
    Double_t Mass;
    THaVar* trpads;
    THaVar* s2trpath;
    THaVar* s2times;
    THaVar* trpath;
  };

  const vector<Spec_short> SpList = {
    { fSpect1, fVxTime1, fpmass1, fTrPads1, fS2TrPath1, fS2Times1, fTrPath1 },
    { fSpect2, fVxTime2, fpmass2, fTrPads2, fS2TrPath2, fS2Times2, fTrPath2 },
  };

  for( const auto& sp : SpList ) {
    Int_t Ntr = sp.Sp->GetNTracks();
    if( Ntr <= 0) continue;   // no tracks, skip
    
    TClonesArray* tracks = sp.Sp->GetTracks();
    THaVar* tr_pads  = sp.trpads;
    THaVar* s2trpath = sp.s2trpath;
    THaVar* s2times  = sp.s2times;
    THaVar* trpath   = sp.trpath;
    
    if (!tr_pads || !s2trpath || !s2times || !trpath) continue;
    
    for ( Int_t i=0; i<Ntr; i++ ) {
      assert(sp.Vxtime.empty());  // else Clear() not called in order
      auto* tr = dynamic_cast<THaTrack*>(tracks->At(i));
      // this should be safe to assume -- only THaTrack's go into the tracks array
      if( !tr ) {
	Warning(Here("Process"),"non-THaTrack in %s's tracks array at %d.",
		sp.Sp->GetName(),i);
	continue;
      }
      // get time of the track at S2
      Double_t p = tr->GetP();
      if( p > 0. ) {
        int pad = static_cast<int>(tr_pads->GetValue(i));
        if( pad < 0 ) {
          // Using (i+1)*kBig prevents differences of large numbers to be zero
          sp.Vxtime.push_back((i + 1) * kBig);
          continue;
        }
        Double_t s2t = s2times->GetValue(pad);

        Double_t beta = p / TMath::Sqrt(p * p + sp.Mass * sp.Mass);
        Double_t c = TMath::C();
        sp.Vxtime.push_back(
          s2t - (trpath->GetValue(i) + s2trpath->GetValue(i)) / (beta * c)
        );
      } else {
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
  
ClassImp(THaS2CoincTime)

///////////////////////////////////////////////////////////////////////////////
