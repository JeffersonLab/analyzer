//*-- Author :    Ole Hansen   04-Nov-13

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// VDCeff                                                               //
//                                                                      //
// VDC hit efficiency calculation. Since the VDC records clusters       //
// of hits, the efficiency can be estimated quite well without          //
// tracking information. If in a group of three adjacent wires          //
// the two outer wires have a hit, check if the middle wire has a hit   //
// as well. The hit efficiency for that wire, then, is the probability  //
// that there is a hit. Of course, noise and overlapping clusters       //
// cause errors in this calculation, assumed to be small.               //
//                                                                      //
// This module reads a list of global variable names for VDC hit        //
// spectra (wire numbers) from the database. For each variable, it      //
// sets up a hit efficiency histogram which is updated every 500        //
// events (configurable, if desired).                                   //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include "VDCeff.h"
#include "THaVarList.h"
#include "THaGlobals.h"
#include "TObjArray.h"
#include "TH1F.h"
#include "TMath.h"
#include "TDirectory.h"
#include "TClass.h"

#include <stdexcept>
#include <memory>
#include <cassert>

using namespace std;

typedef vector<Short_t>::iterator Vsiter_t;
typedef vector<VDCeff::VDCvar_t>::iterator variter_t;

const TString nhit_suffix( "nhit" );
const TString eff_suffix(  "eff" );

//_____________________________________________________________________________
static void SafeDeleteHist( const TString& name, TH1F*& the_hist )
{
  // Verify that each histogram is still in memory. It usually isn't because
  // ROOT deletes it when the output file is closed.
  if( !the_hist ) return;
  TObject* obj = gDirectory->FindObject(name);
  if( obj && obj->IsA() && obj->IsA()->InheritsFrom(TH1::Class()) ) {
    assert( name == obj->GetName() );
    delete the_hist;
  }
  the_hist = 0; // Zero out now-invalid pointer, regardless of who deleted it
}

//_____________________________________________________________________________
VDCeff::VDCvar_t::~VDCvar_t()
{
  // VDCvar_t destructor. Delete histograms, if defined.

  SafeDeleteHist( histname + nhit_suffix, hist_eff );
  SafeDeleteHist( histname + eff_suffix,  hist_nhit );
}

//_____________________________________________________________________________
VDCeff::VDCeff( const char* name, const char* description )
  : THaPhysicsModule(name,description)
{
  // Normal constructor.

}

//_____________________________________________________________________________
VDCeff::~VDCeff()
{
  // Destructor

  RemoveVariables();
}

//_____________________________________________________________________________
void VDCeff::Clear( Option_t* opt )
{
  // Clear event-by-event data

  THaPhysicsModule::Clear(opt);

}

//_____________________________________________________________________________
Int_t VDCeff::Begin( THaRunBase* )
{
  // Start of analysis

  for( variter_t it = fVDCvar.begin(); it != fVDCvar.end(); ++it ) {
    VDCvar_t& thePlane = *it;
    assert( thePlane.nwire > 0 );
    thePlane.ncnt.assign( thePlane.nwire, 0 );
    thePlane.nhit.assign( thePlane.nwire, 0 );
    // Book histograms here, not in Init. The output file must be open for the
    // histogram to be saved. This is the case here, but not when Init runs.
    if( !thePlane.hist_nhit ) {
      TString name = thePlane.histname + nhit_suffix;
      TString title = "Num hits " + thePlane.histname;
      Int_t nmax = TMath::Nint( thePlane.nwire * fMaxOcc );
      thePlane.hist_nhit = new TH1F( name, title, nmax, -1, nmax-1 );
    }
    if( !thePlane.hist_eff ) {
      TString name = thePlane.histname + eff_suffix;
      TString title = thePlane.histname + " efficiency";
      thePlane.hist_nhit = new TH1F( name, title,
				     thePlane.nwire, 0, thePlane.nwire );
    }
  }

  fNevt = 0;

  return 0;
}

//_____________________________________________________________________________
Int_t VDCeff::End( THaRunBase* )
{
  // End of analysis

  WriteHist();
  return 0;
}

//_____________________________________________________________________________
Int_t VDCeff::DefineVariables( EMode mode )
{
  // Define/delete global variables.

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  // RVarDef vars[] = {
  //   { "A",  "Result A", "fResultA" },
  //   { "B",  "Result B", "fResultB" },
  //   { 0 }
  // };
  // return DefineVarsFromList( vars, mode );
  return kOK;
}

//_____________________________________________________________________________
THaAnalysisObject::EStatus VDCeff::Init( const TDatime& run_time )
{
  // Initialize VDCeff physics module

  const char* const here = __FUNCTION__;

  // Standard initialization. Calls ReadDatabase(), ReadRunDatabase(),
  // and DefineVariables() (see THaAnalysisObject::Init)
  if( THaPhysicsModule::Init( run_time ) != kOK )
    return fStatus;

  // Associate global variable pointers. This can and should be done
  // at every reinitialization (pointers in VDC class may have changed)
  for( variter_t it = fVDCvar.begin(); it != fVDCvar.end(); ++it ) {
    VDCvar_t& thePlane = *it;
    if( thePlane.name.IsNull() ) continue;  // no global variable name
    thePlane.pvar = gHaVars->Find( thePlane.name );
    if( !thePlane.pvar ) {
      Warning( Here(here), "Cannot find global VDC variable %s. Ignoring.",
	       thePlane.name.Data() );
    }
  }

//======= VDCEff stuff ==================

// struct HistDef {
//   const char* name;
//   const char* title;
//   Int_t       nbins;
//   Double_t    xmin;
//   Double_t    xmax;
// };

// static const HistDef histdefs[] = {
//   //FIXME: put the nhit spectra into the THaOutput Odef file
//   // they are simply histos of ${arm}.vdc.${plane}.nhit
//   { "Lu1nhit",  "Num Hits Left U1", 50, -1, 49 },
//   { "Lu2nhit",  "Num Hits Left U2", 50, -1, 49 },
//   { "Lv1nhit",  "Num Hits Left V1", 50, -1, 49 },
//   { "Lv2nhit",  "Num Hits Left V2", 50, -1, 49 },
//   { "Ru1nhit",  "Num Hits Right U1", 50, -1, 49 },
//   { "Ru2nhit",  "Num Hits Right U2", 50, -1, 49 },
//   { "Rv1nhit",  "Num Hits Right V1", 50, -1, 49 },
//   { "Rv2nhit",  "Num Hits Right V2", 50, -1, 49 },
//   // These are probably best left here
//   //FIXME: adjust nbins, xmax based on actual VDC configuration
//   { "Lu1eff",   "Left arm U1 efficiency", 400, 0,400 },
//   { "Lu2eff",   "Left arm U2 efficiency", 400, 0,400 },
//   { "Lv1eff",   "Left arm V1 efficiency", 400, 0,400 },
//   { "Lv2eff",   "Left arm V2 efficiency", 400, 0,400 },
//   { "Ru1eff",   "Right arm U1 efficiency", 400, 0,400 },
//   { "Ru2eff",   "Right arm U2 efficiency", 400, 0,400 },
//   { "Rv1eff",   "Right arm V1 efficiency", 400, 0,400 },
//   { "Rv2eff",   "Right arm V2 efficiency", 400, 0,400 },
//   //FIXME: why not put into THaOutput Odef file?
//   { "Lenroc12", "Event length in ROC12", 500, 0,5000 },
//   { "Lenroc16", "Event length in ROC16", 500, 0,5000 },
//   { 0 }
// };

// //_____________________________________________________________________________
//   void THaDecData::BookHist()
// {
//   // VDC efficiencies

//   const HistDef* h = histdefs;
//   while( h->name ) {
//     hist.push_back( new TH1F(h->name, h->title, h->nbins, h->xmin, h->xmax) );
//     ++h;
//   }
// }

// Init():
  // cnt1 = 0;
  // // Let VdcEff reassociate its global variable pointers upon re-init
  // if( fgVdcEffFirst == 0 )
  //   fgVdcEffFirst = 1;

  return fStatus;
}

//_____________________________________________________________________________
Int_t VDCeff::Process( const THaEvData& evdata )
{
  // Update VDC efficiency histograms with current event data

  // static const string VdcVars[] = {"L.vdc.u1.wire", "L.vdc.u2.wire",
  // 				   "L.vdc.v1.wire", "L.vdc.v2.wire",
  // 				   "R.vdc.u1.wire", "R.vdc.u2.wire",
  // 				   "R.vdc.v1.wire", "R.vdc.v2.wire"};

  const char* const here = __FUNCTION__;

  if( !IsOK() ) return -1;

  ++fNevt;
  bool cycle_event = ( (fNevt%fCycle) == 0 );

  for( variter_t it = fVDCvar.begin(); it != fVDCvar.end(); ++it ) {
    VDCvar_t& thePlane = *it;

    if( !thePlane.pvar ) continue;  // global variable wasn't found

    Int_t nwire = thePlane.nwire;
    fWire.clear();
    fHitWire.assign( nwire, false );

    Int_t nhit = thePlane.pvar->GetLen();
    thePlane.hist_nhit->Fill(nhit);
    if( nhit < 0 ) {
      Warning( Here(here), "nhit = %d < 0?", nhit );
      nhit = 0;
    } else if( nhit > nwire ) {
      Warning( Here(here), "nhit = %d > nwire = %d?", nhit, nwire );
      nhit = nwire;
    }

    for( Int_t i = 0; i < nhit; ++i ) {
      Int_t wire = TMath::Nint( thePlane.pvar->GetValue(i) );
      if( wire >= 0 && wire < nwire ) {
	fWire.push_back(wire);
	fHitWire[wire] = true;
      }
    }

    for( Vsiter_t iw = fWire.begin(); iw != fWire.end(); ++iw ) {
      Int_t wire = *iw;
      Int_t ngh2 = wire+2;
      if( ngh2 >= nwire ) continue;

      if( fHitWire[ngh2] ) {
	Int_t awire = wire+1;
	thePlane.ncnt[awire]++;
	if( fHitWire[awire] )
	  thePlane.nhit[awire]++;
      }
    }

    if( cycle_event ) {
      thePlane.hist_eff->Reset();
      for( Int_t i = 0; i < nwire; ++i ) {
	if( thePlane.ncnt[i] != 0 ) {
	  Double_t xeff = static_cast<Double_t>(thePlane.nhit[i]) /
	    static_cast<Double_t>(thePlane.ncnt[i]);
	  thePlane.hist_eff->Fill(i,xeff);
	}
      }
    }
  }

  // FIXME: repeated WriteHist seems to cause problems with splits files
  // (multiple cycles left in output)
  if( (cycle_event && fNevt < 4*fCycle) ||
      (fNevt % (10*fCycle) == 0) )
    WriteHist();

#ifdef WITH_DEBUG
  if( fDebug>1 && (fNevt%10) == 0 )
    Print();
#endif

  fDataValid = true;
  return 0;
}


/*
  const Int_t nwire = 400;
  //FIXME: really push 3.2kB on the stack every event?
  Int_t wire[nwire];
  Int_t hitwire[nwire];   // lookup to avoid O(N^3) algorithm // really??


  //FIXME: these static variables prevent multiple instances of this object!
  // use member variables
  static Int_t cnt = 0;  // Event counter
  static Double_t xcnt[8*nwire],eff[8*nwire];
  static THaVar* varp[8];
  if (fgVdcEffFirst>0) {
    if( fgVdcEffFirst>1) {
      cnt = 0;
      memset(eff,0,8*nwire*sizeof(eff[0]));
      memset(xcnt,0,8*nwire*sizeof(xcnt[0]));
    }
    for( Int_t i = 0; i<8; ++i ) {
      varp[i] = gHaVars->Find(VdcVars[i].c_str());
    }
    fgVdcEffFirst = 0;
  }

#ifdef WITH_DEBUG
  if (fDebug>4)
    cout << "\n *************** \n Vdc Effic "<<endl;
#endif

  for (Int_t ipl = 0; ipl < 8; ++ipl) {

    Int_t nhit = 0;
    THaVar* pvar = varp[ipl];
#ifdef WITH_DEBUG
     if (fDebug>4)
      cout << "plane "<<ipl<<"  "<<VdcVars[ipl]<<" $$$ "<<pvar<<endl;
#endif
     if (!pvar) continue;
     memset(wire,0,nwire*sizeof(wire[0]));
     memset(hitwire,0,nwire*sizeof(hitwire[0]));

     Int_t n = pvar->GetLen();
     nhit = n;
     hist[ipl]->Fill(nhit);
     if (n < 0) n = 0;
     if (n > nwire) n = nwire;
#ifdef WITH_DEBUG
     if (fDebug>4)
       cout << "nwire "<<n<<"  "<<nwire<<"  "<<nhit<<endl;
#endif

     for (Int_t i = 0; i < n; ++i) {
       wire[i] = (Int_t) pvar->GetValue(i);
       if (wire[i]>=0 && wire[i]<nwire)
	 hitwire[wire[i]]=1;
#ifdef WITH_DEBUG
       if (fDebug>4)
         cout << "wire "<<i<<"  "<<wire[i]<<endl;
#endif
     }

// The following does not assume that wire[] is ordered.
//FIXME: but we can order it
     for (Int_t i = 0; i < n; ++i) {
       // look for neighboring hit at +2 wires
       Int_t ngh2=wire[i]+2;
       if (wire[i]<0 || ngh2>=nwire) continue;

       if (hitwire[ngh2]) {
	 Int_t awire = wire[i]+1;
#ifdef WITH_DEBUG
	 if (fDebug>4)
	   cout << "wire eff "<<i<<"  "<<awire<<endl;
#endif
	 if (awire>=0 && awire<nwire) { //FIXME:  always true
	   xcnt[ipl*nwire+awire] = xcnt[ipl*nwire+awire] + 1;

	   if ( hitwire[awire] ) {
	     eff[ipl*nwire+awire] = eff[ipl*nwire+awire] + 1;
	   } else {
	     //FIXME: is this a joke?
	     eff[ipl*nwire+awire] = eff[ipl*nwire+awire] + 0;
	   }
	 }
       }
     }

     if ((cnt%500) == 0) {

       // FIXME: why reset?
       hist[ipl+8]->Reset();
       for (Int_t i = 0; i < nwire; ++i) {

         Double_t xeff = -1;
         if (xcnt[ipl*nwire+i] != 0) {
	   xeff = eff[ipl*nwire+i]/xcnt[ipl*nwire+i];
	 }
#ifdef WITH_DEBUG
         if (fDebug>4)
	   cout << "Efficiency "<<i<<"  "<<xcnt[ipl*nwire+i]<<"  "<<xeff<<endl;
#endif
         if (xeff > 0) hist[ipl+8]->Fill(i,xeff);
       }
     }

*/

//_____________________________________________________________________________
Int_t VDCeff::ReadDatabase( const TDatime& date )
{
  // Read database. Gets the VDC variables containing wire spectra.

  const char* const here = __FUNCTION__;
  const char* const separators = ", \t";
  const Int_t NPAR = 3;

  FILE* f = OpenFile( date );
  if( !f ) return kFileError;

  TString configstr;
  fCycle = 500;
  fMaxOcc = 0.25;

  Int_t status = kOK;
  try {
      const DBRequest request[] = {
      { "vdcvars",    &configstr,   kTString },
      { "cycle",      &fCycle,      kInt,     0, 1 },
      { "maxocc",     &fMaxOcc,     kDouble,  0, 1 },
      { 0 }
    };
    status = LoadDB( f, date, request, fPrefix );
  }
  catch(...) {
    fclose(f);
    throw;
  }
  fclose(f);
  if( status != kOK ) {
    return status;
  }

  if( configstr.Length() == 0 ) {
    Error( Here(here), "No VDC variables defined. Fix database." );
    return kInitError;
  };
  // Parse variables at set up definition structure for each
  auto_ptr<TObjArray> vdcvars( configstr.Tokenize(separators) );
  Int_t nparams = vdcvars->GetLast()+1;
  if( nparams == 0 ) {
    Error( Here(here), "No VDC variable names in vdcvars = %s. Fix database.",
	   configstr.Data() );
    return kInitError;
  }
  if( nparams % NPAR != 0 ) {
    Error( Here(here), "Incorrect number of parameters in vdcvars. "
	   "Have %d, but must be a multiple of %d. Fix database.",
	   nparams, NPAR );
    return kInitError;
  }
  fVDCvar.clear();
  const TObjArray* params = vdcvars.get();
  Int_t max_nwire = 0;
  for( Int_t ip = 0; ip < nparams; ip += NPAR ) {
    const TString& name     = GetObjArrayString(params,ip);
    const TString& histname = GetObjArrayString(params,ip+1);
    Int_t nwire             = GetObjArrayString(params,ip+2).Atoi();
    if( nwire <= 0 || nwire > kMaxShort ) {
      Error( Here(here), "Illegal number of wires = %d for VDC variable %s. "
	     "Fix database.", nwire, name.Data() );
      return kInitError;
    }
    if( fDebug>2 )
      Info( Here(here), "Defining VDC variable %s", name.Data() );

    fVDCvar.push_back( VDCvar_t(name, histname, nwire) );
    VDCvar_t& thePlane = fVDCvar.back();
    thePlane.ncnt.reserve( thePlane.nwire );
    thePlane.nhit.reserve( thePlane.nwire );
    max_nwire = TMath::Max(max_nwire,thePlane.nwire);
  }

  fWire.reserve( max_nwire*fMaxOcc );
  fHitWire.assign( max_nwire, 0 );

  return kOK;
}

//_____________________________________________________________________________
void VDCeff::WriteHist()
{
  // Write all defined histograms

  for( variter_t it = fVDCvar.begin(); it != fVDCvar.end(); ++it ) {
    VDCvar_t& thePlane = *it;
    if( thePlane.hist_nhit )
      thePlane.hist_nhit->Write();
    if( thePlane.hist_eff )
      thePlane.hist_eff->Write();
  }
}

//_____________________________________________________________________________
ClassImp(VDCeff)
