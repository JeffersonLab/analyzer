//*-- Author :    Rob Feuerbach 9-Sep-03

//////////////////////////////////////////////////////////////////////////
//
// THaCoincidenceTime
//
// Calculate coincidence time between tracks in two spectrometers.
//
//  The spectrometers must be listed in the constructor in the same order
//  as their coincidence-timing readouts in the database. (yes, messy)
//  In other words, the spectrometer that provides the COMMON START for
//  the first coinc. TDC (and the TDC is stopped by the other spectr.
//  trigger) must be the first spectrometer in the Constructor.
//
//////////////////////////////////////////////////////////////////////////

#include <iostream>

#include "TLorentzVector.h"
#include "TVector3.h"
#include "TMath.h"

#include "THaCoincidenceTime.h"

#include "THaTrack.h"
#include "THaScintillator.h"
#include "THaDetMap.h"
#include "THaSpectrometer.h"
#include "THaEvData.h"
#include "THaTrackProj.h"

#include "VarDef.h"
#include "THaDB.h"

using namespace std;

//_____________________________________________________________________________
THaCoincidenceTime::THaCoincidenceTime( const char* name,
					const char* description,
					const char* spec1, const char* spec2,
					Double_t m1, Double_t m2 )
  : THaPhysicsModule(name,description),
    fSpectroName1(spec1), fSpectroName2(spec2),fpmass1(m1),fpmass2(m2),
    fTdcRes(NULL), fTdcLabels(NULL), fGoldTr(NULL)
{
  // Normal constructor.
  fDetMap = new THaDetMap();
}

//_____________________________________________________________________________
THaCoincidenceTime::~THaCoincidenceTime()
{
  // Destructor
  delete fDetMap;
  DefineVariables( kDelete );
}

//_____________________________________________________________________________
void THaCoincidenceTime::Clear( Option_t* opt )
{
  // Clear all internal variables.
  memset(fVxTime,0,fNSpectro*sizeof(fVxTime[0]));
  memset(fdT1_2,0,fNtimes*sizeof(fdT1_2[0]));
  memset(fDiffT,0,fNtimes*sizeof(fDiffT[0]));
  memset(fGoldTr,0,fNSpectro*sizeof(fGoldTr[0]));
}

//_____________________________________________________________________________
Int_t THaCoincidenceTime::DefineVariables( EMode mode )
{
  // Define/delete global variables.

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  RVarDef vars[] = {
    { "VxTime",    "Time of tracks at the vertex",              "fVxTime"},
    { "dtdc",      "Coincidence times from TDCs (s)",           "fdT1_2" },
    { "Ctime" ,    "Corrected coin. time for spec. 1 vs 2 (s)", "fDiffT" },
    { 0 }
  };
  return DefineVarsFromList( vars, mode );
}

//_____________________________________________________________________________
THaAnalysisObject::EStatus THaCoincidenceTime::Init( const TDatime& run_time )
{
  // Initialize the module.
  

  // Locate the spectrometer apparatus named in fSpectroName and save
  // pointer to it.
  fSpectro.push_back(SpecInfo(dynamic_cast<THaSpectrometer*>
			      (gHaApps->FindObject(fSpectroName1)),
			      fpmass1) );
  if (!fSpectro.back().spec) fSpectro.pop_back();
  
  fSpectro.push_back(SpecInfo(dynamic_cast<THaSpectrometer*>
			      (gHaApps->FindObject(fSpectroName2)),
			      fpmass2) );
  if (!fSpectro.back().spec) fSpectro.pop_back();

  fNSpectro = fSpectro.size();
#if ROOT_VERSION_CODE >= ROOT_VERSION(3,3,0)
  fNtimes = static_cast<Int_t>(TMath::Factorial(fNSpectro)+.5);
#else
  Double_t x=1;
  Int_t n = fNSpectro;
  if( n>0 ) {
    Int_t b=0;
    do { b++; x *= b; } while(b!=n);
  }
  fNtimes = static_cast<Int_t>(x);
#endif

  if (!fIsInit) {
    fGoldTr = new THaTrack*[fSpectro.size()];
    fVxTime = new Double_t[fSpectro.size()];
    fdT1_2  = new Double_t[fNtimes];
    fDiffT  = new Double_t[fNtimes];
  }
  
  // Standard initialization. Calls this object's DefineVariables().
  if( THaPhysicsModule::Init( run_time ) != kOK )
    return fStatus;

  return fStatus;
}

//_____________________________________________________________________________
Int_t THaCoincidenceTime::ReadDatabase( const TDatime& date )
{
  // Read this the TDC channels (and resolutions) from the database.
  // 'date' contains the date/time of the run being analyzed.

  static const char* const here = "ReadDatabase()";
  const int LEN = 200;
  char buf[LEN];

  FILE* fi = OpenFile( date );
  if( !fi ) return kFileError;

  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  fDetMap->Clear();
  
  if ( !fTdcRes ) fTdcRes = new Double_t[fNtimes];
  if ( !fTdcLabels) fTdcLabels = new TString[fNtimes];
  
  int cnt = 0;

  while ( 1 ) {
    Int_t model;
    Float_t tres;   //  TDC resolution
    char label[21]; // string label for TDC = Stop_by_Start
                    // Label is a space-holder to be used in the future
    
    Int_t crate, slot, first, last;

    while ( ReadComment( fi, buf, LEN ) );

    fgets ( buf, LEN, fi );
    
    int nread = sscanf( buf, "%d %d %d %d %d %f %20s", &crate, &slot, &first,
			&last, &model, &tres, label );
    if ( crate < 0 ) break;
    if ( nread != 7 ) {
      Error( Here(here), "Invalid detector map! Need all 7 columns." );
      fclose(fi);
      return kInitError;
    }
    if( fDetMap->AddModule( crate, slot, first, last, cnt, model ) < 0 ) {
      Error( Here(here), "Too many DetMap modules (maximum allowed - %d).", 
             THaDetMap::kDetMapSize);
      fclose(fi);
      return kInitError;
    }

    if ( cnt+(last-first) < fNtimes )
      for (int j=cnt; j<=cnt+(last-first); j++)  fTdcRes[j] = tres;
    else 
      Warning( Here(here), "Too many entries. Expected %d but found %d",
	       fNtimes,cnt+1);
    cnt+= (last-first+1);
  }

  fclose(fi);

  return kOK;
}

//_____________________________________________________________________________
Int_t THaCoincidenceTime::Process( const THaEvData& evdata )
{
  // Read in coincidence TDC's
  if( !IsOK() ) return -1;

  if (!fDetMap) return -1;

  for ( Int_t i=0; i < fDetMap->GetSize(); i++ ) {
    THaDetMap::Module* d = fDetMap->GetModule(i);
    // grab only the first hit in a TDC
    for (Int_t j=d->lo; j<=d->hi; j++) {
      Int_t ch = j - d->lo + d->first;
      if ( evdata.GetNumHits(d->crate,d->slot,j) > 0 ) {
	fdT1_2[ch] = evdata.GetData(d->crate,d->slot,j,0);
	fdT1_2[ch] *= fTdcRes[ch];
      }
    }
  }

  // Calculate the time at the vertex (relative to the trigger time)
  // Use the Beta of the assumed particle type.
  for ( UInt_t i=0; i<fSpectro.size(); i++ ) {
    THaTrack *tr = fGoldTr[i] = fSpectro[i].spec->GetGoldenTrack();
    if ( tr && tr->GetBeta()!=0. ) {
      Double_t m = fSpectro[i].pmass;
      Double_t p = tr->GetP();
      Double_t beta = p/TMath::Sqrt(p*p+m*m);
#if ROOT_VERSION_CODE >= ROOT_VERSION(3,4,0)
      Double_t c = TMath::C();
#else
      Double_t c = 2.99792458e8;
#endif
      fVxTime[i] = tr->GetTime() - tr->GetPathLen()/(beta*c);
    } else {
      fVxTime[i] = i*kBig;
      continue;
    }
  }

  // Take the golden tracks and the coincidence TDC and construct
  // the coincidence time
  Int_t cnt=0;
  for ( UInt_t i=0; i<fSpectro.size(); i++ ) {
    for ( UInt_t j=0; j<fSpectro.size(); j++ ) {
      if (i==j) continue;
      fDiffT[cnt] = fVxTime[j] - fVxTime[i] + fdT1_2[i];
      cnt++;
    }
  }
  
  return 0;
}
  
ClassImp(THaCoincidenceTime)

///////////////////////////////////////////////////////////////////////////////
