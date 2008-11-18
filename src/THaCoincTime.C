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

#include <iostream>

//#include "TLorentzVector.h"
//#include "TVector3.h"
#include "TMath.h"

#include "THaCoincTime.h"

#include "THaTrack.h"
//#include "THaScintillator.h"
#include "THaDetMap.h"
#include "THaSpectrometer.h"
#include "THaEvData.h"
//#include "THaTrackProj.h"

#include "VarDef.h"
//#include "THaString.h"
//#include "THaDB.h"

using namespace std;

#define CAN_RESIZE 0

//_____________________________________________________________________________
THaCoincTime::THaCoincTime( const char* name,
			    const char* description,
			    const char* spec1, const char* spec2,
			    Double_t m1, Double_t m2,
			    const char* ch_name1, const char* ch_name2 )
  : THaPhysicsModule(name,description),
    fSpectN1(spec1), fSpectN2(spec2),
    fpmass1(m1),fpmass2(m2)
{
  // Normal constructor.
  fDetMap = new THaDetMap();

  if (ch_name1 && strlen(ch_name1)>0) {
    fTdcLabels[0] = ch_name1;
  } else {
    fTdcLabels[0] = Form("ct_%sby%s",spec2,spec1);
  } 
  if (ch_name2 && strlen(ch_name2)>0) {
    fTdcLabels[1] = ch_name2;
  } else {
    fTdcLabels[1] = Form("ct_%sby%s",spec1,spec2);
  }

  // set aside the memory
  // only permit for a maximum of 10 tracks per spectrometer
  fSz1 = fSz2 = 10;
  fVxTime1 = new Double_t[fSz1];
  fVxTime2 = new Double_t[fSz2];
  fSzNtr = fSz1*fSz2;
  fTrInd1 = new Int_t[fSzNtr];
  fTrInd2 = new Int_t[fSzNtr];
  fDiffT2by1 = new Double_t[fSzNtr];
  fDiffT1by2 = new Double_t[fSzNtr];
  fNTr1 = fNTr2 = fNtimes = 0;
}

//_____________________________________________________________________________
THaCoincTime::~THaCoincTime()
{
  // Destructor
  RemoveVariables();
  delete fDetMap;
  delete [] fVxTime1;
  delete [] fVxTime2;
  delete [] fTrInd1;
  delete [] fTrInd2;
  delete [] fDiffT2by1;
  delete [] fDiffT1by2;
}

//_____________________________________________________________________________
void THaCoincTime::Clear( Option_t* opt )
{
  // Clear all internal variables
  // some overkill, but is event-independent (back to a known state)
  THaPhysicsModule::Clear(opt);
  memset(fVxTime1,0,fSz1*sizeof(*fVxTime1));
  memset(fVxTime2,0,fSz2*sizeof(*fVxTime2));
  memset(fdTdc,0,2*sizeof(*fdTdc));

  memset(fDiffT1by2,0,fSzNtr*sizeof(*fDiffT1by2));
  memset(fDiffT2by1,0,fSzNtr*sizeof(*fDiffT2by1));
  memset(fTrInd1,0,fSzNtr*sizeof(*fTrInd1));
  memset(fTrInd2,0,fSzNtr*sizeof(*fTrInd2));
  
  fNTr1 = fNTr2 = 0;
  fNtimes = 0;
}

//_____________________________________________________________________________
Int_t THaCoincTime::DefineVariables( EMode mode )
{
  // Define/delete global variables.
  
  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  RVarDef vars[] = {
    { "d_trig","Measured TDC start+delay of spec 2 (1), by spec 1 (2)", "fdTdc" },
    { "ntr1",  "Number of tracks in first spec.",  "fNTr1" },
    { "ntr2",  "Number of tracks in first spec.",  "fNTr2" },
    { "vx_t1", "Time of track from spec1 at target vertex", "fVxTime1" },
    { "vx_t2", "Time of track from spec2 at target vertex", "fVxTime2" },
    { "ncomb", "Number of track combinations considered",   "fNtimes" },
    { "ct_2by1", "Coinc. times of tracks, d_trig from spec 1", "fDiffT2by1" },
    { "ct_1by2", "Coinc. times of tracks, d_trig from spec 2", "fDiffT1by2" },
    { "trind1",  "Track indices for spec1 match entries in ct_*", "fTrInd1" },
    { "trind2",  "Track indices for spec2 match entries in ct_*", "fTrInd2" },
    { 0 }
  };
  return DefineVarsFromList( vars, mode );
}

//_____________________________________________________________________________
THaAnalysisObject::EStatus THaCoincTime::Init( const TDatime& run_time )
{
  // Initialize the module.
  

  // Locate the spectrometer apparatus named in fSpectroName and save
  // pointer to it.
  if (!fIsInit) {
    fIsInit = true;
  }

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
  // Read this the TDC channels (and resolutions) from the database.
  // 'date' contains the date/time of the run being analyzed.

  static const char* const here = "ReadDatabase()";
  const int LEN = 200;
  char buf[LEN];

  FILE* fi = OpenFile( date );
  if( !fi ) {
    // look for more general coincidence-timing database
    fi = OpenFile( "CT", date );
  }
  if ( !fi )
    return kFileError;

  //  fgets ( buf, LEN, fi ); fgets ( buf, LEN, fi );
  fDetMap->Clear();
  
  int cnt = 0;
  
  fTdcRes[0] = fTdcRes[1] = 0.;
  fTdcOff[0] = fTdcOff[1] = 0.;
  
  while ( 1 ) {
    Int_t model;
    Float_t tres;   //  TDC resolution
    Float_t toff;   //  TDC offset (in seconds)
    char label[21]; // string label for TDC = Stop_by_Start
                    // Label is a space-holder to be used in the future
    
    Int_t crate, slot, first, last;

    while ( ReadComment( fi, buf, LEN ) ) {}

    fgets ( buf, LEN, fi );
    
    int nread = sscanf( buf, "%d %d %d %d %f %20s %f", &crate, &slot, &first,
		        &model, &tres, label, &toff );
    if ( crate < 0 ) break;
    if ( nread < 6 ) {
      Error( Here(here), "Invalid detector map! Need at least 6 columns." );
      fclose(fi);
      return kInitError;
    }
    last = first; // only one channel per entry (one ct measurement)
    // look for the label in our list of spectrometers
    int ind = -1;
    for (int i=0; i<2; i++) {
      // enforce logical channel number 0 == 2by1, etc.
      // matching between fTdcLabels and the detector map
      if ( fTdcLabels[i] == label ) {
	ind = i;
	break;
      }
    }
    if (ind <0) {
      TString listoflabels;
      for (int i=0; i<2; i++) {
	listoflabels += " " + fTdcLabels[i];
      }
      listoflabels += '\0';
      Error( Here(here), "Invalid detector map! The timing measurement %s does not"
	     " correspond\n to the spectrometers. Expected one of \n"
	     "%s",label, listoflabels.Data());
      fclose(fi);
      return kInitError;
    }
    
    if( fDetMap->AddModule( crate, slot, first, last, ind, model ) < 0 ) {
      Error( Here(here), "Too many DetMap modules (maximum allowed - %d).", 
             THaDetMap::kDetMapSize);
      fclose(fi);
      return kInitError;
    }

    if ( ind+(last-first) < 2 )
      for (int j=ind; j<=ind+(last-first); j++)  {
	fTdcRes[j] = tres;
	if (nread>6) fTdcOff[j] = toff;
      }
    else 
      Warning( Here(here), "Too many entries. Expected 2 but found %d",
	       cnt+1);
    cnt+= (last-first+1);
  }

  fclose(fi);

  return kOK;
}

#if 0
//_____________________________________________________________________________
Int_t THaCoincTime::ReadDB( const TDatime& date )
{
  // Read this the TDC channels (and resolutions) from the database.
  // 'date' contains the date/time of the run being analyzed.

  static const char* const here = "ReadDB()";

  if (!gHaDB) return kInitError;

  //  THaString detnm;
  
  // clear out the detector map before reading a new one
  fDetMap->Clear();

  // the list of detector names to use is in fTdcLabels, set
  // at creation time or automatically generated as
  // "ct_{spec2Name}by{spec1Name}"
  // and the inverse

  for (int i=0; i<2; i++) {
    // Read in detmap entries
    Int_t nrd = gHaDB->GetDetMap(fTdcLabels[i],*fDetMap,date);
    if (nrd < 0) {
      Error( Here(here), "Too many DetMap modules (maximum allowed - %d).", 
	     THaDetMap::kDetMapSize);
      return kInitError;
    }

    // read in calibration constants
    DBRequest list[] = {
      { "TDC_res", &fTdcRes[i] },
      { "TDC_offset", &fTdcOff[i] },
      { 0 }
    };
    TString pref("CT." + fTdcLabels[i] + ".");
    if (! gHaDB->LoadValues(pref,list,date) ) {
      Warning( Here(here),"Cannot read calibration constants for %s. "
	       "Using defaults of 50ps/ch and 0seconds offset.",
	       fTdcLabels[i].Data());
      fTdcRes[i] = 50e-12;
      fTdcOff[i]=0.;
    }
  }

  Int_t mapsize = fDetMap->GetSize();
  if (mapsize!=2) {
    Error( Here(here), "Expected an entry for only %s and %s.", 
	   fTdcLabels[0].Data(),fTdcLabels[1].Data());
    return kInitError;
  }
    
  return kOK;
}
#endif

//_____________________________________________________________________________
Int_t THaCoincTime::Process( const THaEvData& evdata )
{
  // Read in coincidence TDC's
  if( !IsOK() ) return -1;

  if (!fDetMap) return -1;

  // read in the two relevant channels
  int detmap_pos=0;
  for ( Int_t i=0; i < 2; i++ ) {
    if (fTdcRes[i] != 0.) {
      THaDetMap::Module* d = fDetMap->GetModule(detmap_pos);
      // grab only the first hit in a TDC
      if ( evdata.GetNumHits(d->crate,d->slot,d->lo) > 0 ) {
	fdTdc[d->first] = evdata.GetData(d->crate,d->slot,d->lo,0)*fTdcRes[d->first]
	  -fTdcOff[d->first];
      }
      detmap_pos++;
    }
  }
  
  // Calculate the time at the vertex (relative to the trigger time)
  // for each track in each spectrometer
  // Use the Beta of the assumed particle type.
  struct Spec_short {
    THaSpectrometer* Sp;
    Int_t*           Ntr;
    Double_t**       Vxtime;
    Int_t*           Sz;
    Double_t         Mass;
  };

  Spec_short SpList[] = {
    { fSpect1, &fNTr1, &fVxTime1, &fSz1, fpmass1 },
    { fSpect2, &fNTr2, &fVxTime2, &fSz2, fpmass2 },
    { 0 }
  }; 
  
  for (Spec_short* sp=SpList; sp->Sp != NULL; sp++) {
    *(sp->Ntr) = sp->Sp->GetNTracks();
    if (*(sp->Ntr) > *(sp->Sz)) {  // expand array if necessary
    // disable the automatic array re-sizing for now
#if CAN_RESIZE
      delete [] *(sp->Vxtime);
      *(sp->Sz) = *(sp->Ntr)+5;
      *(sp->Vxtime) = new Double_t[*(sp->Sz)];
#else
      Warning(Here("Process()"), "Using only first %d out of %d tracks in spectrometer.", *(sp->Sz), *(sp->Ntr));
      *(sp->Ntr) = *(sp->Sz); // only look at the permitted number of tracks
#endif
    }

    TClonesArray* tracks = sp->Sp->GetTracks();
    for ( Int_t i=0; i<*(sp->Ntr) && i<*(sp->Sz); i++ ) {
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
      if ( tr && tr->GetBeta()!=0. && (p=tr->GetP())>0. ) {
	Double_t beta = p/TMath::Sqrt(p*p+sp->Mass*sp->Mass);
#if ROOT_VERSION_CODE >= ROOT_VERSION(3,4,0)
	Double_t c = TMath::C();
#else
	Double_t c = 2.99792458e8;
#endif
	(*(sp->Vxtime))[i] = tr->GetTime() - tr->GetPathLen()/(beta*c);
      } else {
	// Using (i+1)*kBig here prevents differences from being zero
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
  
  fDataValid = true;
  return 0;
}
  
ClassImp(THaCoincTime)

///////////////////////////////////////////////////////////////////////////////
