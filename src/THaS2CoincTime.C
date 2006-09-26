//*-- Author :    Rob Feuerbach 9-Sep-03 */

//////////////////////////////////////////////////////////////////////////
//
// THaS2CoincTime
//
// Calculate coincidence time between tracks in two spectrometers.
//  Uses only the time of the tracks in given detectors (def. is S2)
//
//  Loop through all combinations of tracks in the two spectrometers.
//  Here we assume that the time difference+fixed delay between the
//  common TDC starts is measured.
//
//////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <cstring>
#include <string>

#include "TLorentzVector.h"
#include "TVector3.h"
#include "TMath.h"

#include "THaS2CoincTime.h"

#include "THaTrack.h"
#include "THaScintillator.h"
#include "THaDetMap.h"
#include "THaSpectrometer.h"
#include "THaEvData.h"
#include "THaTrackProj.h"

#include "THaVarList.h"
#include "THaGlobals.h"
#include "VarDef.h"
#include "THaString.h"
#include "THaDB.h"

using namespace std;

#define CAN_RESIZE 0

//_____________________________________________________________________________
THaS2CoincTime::THaS2CoincTime( const char* name,
				const char* description,
				const char* spec1, const char* spec2,
				Double_t m1, Double_t m2,
				const char* ch_name1, const char* ch_name2,
				const char* detname)
  : THaCoincTime(name,description,spec1,spec2,m1,m2,ch_name1,ch_name2),
    fDetName(detname)
{
  // nothing special
}

//_____________________________________________________________________________
THaS2CoincTime::~THaS2CoincTime()
{
  // just be certain to call THaCoincTime's destructor
}

//_____________________________________________________________________________
THaAnalysisObject::EStatus THaS2CoincTime::Init( const TDatime& run_time )
{
  // Initialize the module
  //    first do the standard initialization
  THaAnalysisObject::EStatus stat = THaCoincTime::Init(run_time);

  if (stat != kOK) return stat;

  if (!fSpect1 || !fSpect2) return kInitError;
  string detn(fDetName,fDetName.Length());
  
  fTrPads1   = gHaVars->Find(Form("%s.%s.trpad",fSpect1->GetName(),detn.c_str()));
  fS2TrPath1 = gHaVars->Find(Form("%s.%s.trpath",fSpect1->GetName(),detn.c_str()));
  fS2Times1  = gHaVars->Find(Form("%s.%s.time",fSpect1->GetName(),detn.c_str()));
  fTrPath1   = gHaVars->Find(Form("%s.tr.pathl",fSpect1->GetName()));
  if (!fTrPads1 || !fS2TrPath1 || !fS2Times1 || !fTrPath1) {
    Error("THaS2CoicTime::Init","Cannot get variables for spectrometer %s detector %s", fSpect1->GetName(),detn.c_str());
    return kInitError;
  }

  fTrPads2   = gHaVars->Find(Form("%s.%s.trpad",fSpect2->GetName(),detn.c_str()));
  fS2TrPath2 = gHaVars->Find(Form("%s.%s.trpath",fSpect2->GetName(),detn.c_str()));
  fS2Times2  = gHaVars->Find(Form("%s.%s.time",fSpect2->GetName(),detn.c_str()));
  fTrPath2   = gHaVars->Find(Form("%s.tr.pathl",fSpect2->GetName()));
  
  if (!fTrPads2 || !fS2TrPath2 || !fS2Times2 || !fTrPath2) {
    Error("THaS2CoicTime::Init","Cannot get variables for spectrometer %s detector %s", fSpect2->GetName(),detn.c_str());
    return kInitError;
  }

  return kOK;
}

//_____________________________________________________________________________
Int_t THaS2CoincTime::Process( const THaEvData& evdata )
{
  // Read in coincidence TDC's
  if( !IsOK() ) return -1;

  if (!fDetMap) return -1;

  // read in the two relevant channels
  // Use of the logical channel number ensures matching of the TDC with
  // the correct difference (1by2 vs 2by1)
  for ( Int_t i=0; i < fDetMap->GetSize(); i++ ) {
    THaDetMap::Module* d = fDetMap->GetModule(i);
    if (fTdcRes[d->first] != 0.) {
      // grab only the first hit in a TDC
      if ( evdata.GetNumHits(d->crate,d->slot,d->lo) > 0 ) {
	fdTdc[d->first] = evdata.GetData(d->crate,d->slot,d->lo,0)*fTdcRes[d->first]
	  -fTdcOff[d->first];
      }
    }
  }

  // Calculate the time at the vertex (relative to the trigger time)
  // for each track in each spectrometer
  // Use the Beta of the assumed particle type.
  struct Spec_short {
    THaSpectrometer *Sp;
    Int_t *Ntr;
    Double_t **Vxtime;
    Int_t *Sz;
    Double_t Mass;
    THaVar* trpads;
    THaVar* s2trpath;
    THaVar* s2times;
    THaVar* trpath;
  };

  Spec_short SpList[] = {
    { fSpect1, &fNTr1, &fVxTime1, &fSz1, fpmass1, fTrPads1, fS2TrPath1, fS2Times1, fTrPath1 },
    { fSpect2, &fNTr2, &fVxTime2, &fSz2, fpmass2, fTrPads2, fS2TrPath2, fS2Times2, fTrPath2 },
    { 0 }
  }; 
  
  for (Spec_short* sp=SpList; sp->Sp != NULL; sp++) {
    *(sp->Ntr) = sp->Sp->GetNTracks();
    if (*(sp->Ntr) <=0) continue;   // no tracks, skip
    
    if ( *(sp->Ntr) > *(sp->Sz) ) {  // expand array if necessary
#if CAN_RESIZE
      delete [] *(sp->Vxtime);
      *(sp->Sz) = *(sp->Ntr)+5;
      *(sp->Vxtime) = new Double_t[sp->Sz];
#else
      Warning(Here("Process()"), "Using only first %d out of %d tracks in spectrometer.", *(sp->Sz), *(sp->Ntr));
      *(sp->Ntr)=*(sp->Sz); // only look at the permitted number of tracks
#endif      
    }

    TClonesArray* tracks = sp->Sp->GetTracks();
    THaVar* tr_pads  = sp->trpads;
    THaVar* s2trpath = sp->s2trpath;
    THaVar* s2times  = sp->s2times;
    THaVar* trpath   = sp->trpath;
    
    if (!tr_pads || !s2trpath || !s2times || !trpath) continue;
    
    for ( Int_t i=0; i<*(sp->Ntr); i++ ) {
      THaTrack* tr = dynamic_cast<THaTrack*>(tracks->At(i));
#ifdef DEBUG
      // this should be safe to assume -- only THaTrack's go into the tracks array
      if (!tr) {
	Warning(Here(here),"non-THaTrack in %s's tracks array at %d.",
		sp->Sp->GetName(),i);
	continue;
      }
#endif
      Double_t p;
      // get time of the track at S2
      if ( tr && (p=tr->GetP())>0. ) {
	int pad = static_cast<int>(tr_pads->GetValue(i));
	if (pad<0) {
	  // Using (i+1)*kBig prevents differences of large numbers to be zero
	  (*(sp->Vxtime))[i] = (i+1)*kBig;
	  continue;
	}
	Double_t s2t = s2times->GetValue(pad);
	
	Double_t beta = p/TMath::Sqrt(p*p+sp->Mass*sp->Mass);
#if ROOT_VERSION_CODE >= ROOT_VERSION(3,4,0)
	Double_t c = TMath::C();
#else
	Double_t c = 2.99792458e8;
#endif
	(*(sp->Vxtime))[i] = s2t - 
	  (trpath->GetValue(i)+s2trpath->GetValue(i))/(beta*c);
      } else {
	(*(sp->Vxtime))[i] = (i+1)*kBig;
      }
    }
  }
  
  // now, we have the vertex times -- go through the combinations
  fNtimes = fNTr1*fNTr2;
  if (fNtimes > fSzNtr) {  // expand the array if necessary
#if CAN_RESIZE
    delete [] fTrInd1;
    delete [] fTrInd2;
    delete [] fDiffT2by1;
    delete [] fDiffT1by2;

    fSzNtr = fNtimes+5;
    fTrInd1 = new Int_t[fSzNtr];
    fTrInd2 = new Int_t[fSzNtr];
    fDiffT2by1 = new Double_t[fSzNtr];
    fDiffT1by2 = new Double_t[fSzNtr];
#else
    Warning(Here("Process()"), "Using only first %d out of %d track combinations.", fSzNtr, fNtimes);
    fNtimes = fSzNtr; // only look at the permitted number of tracks
#endif    
  }
  
  // Take the tracks and the coincidence TDC and construct
  // the coincidence time
  Int_t cnt=0;
  for ( Int_t i=0; i<fNTr1; i++ ) {
    for ( Int_t j=0; j<fNTr2; j++ ) {
      if (cnt>=fNtimes) break;
      fTrInd1[cnt] = i;
      fTrInd2[cnt] = j;
      fDiffT2by1[cnt] = fVxTime2[j] - fVxTime1[i] + fdTdc[0];
      fDiffT1by2[cnt] = fVxTime1[i] - fVxTime2[j] + fdTdc[1];
      cnt++;
    }
  }
  
  return 0;
}
  
ClassImp(THaS2CoincTime)

///////////////////////////////////////////////////////////////////////////////
