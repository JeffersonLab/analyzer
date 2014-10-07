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
void VDCeff::VDCvar_t::Reset( Option_t* )
{
  // Reset histograms and counters of this VDCvar

  if( nwire > 0 ) {
    ncnt.assign( nwire, 0 );
    nhit.assign( nwire, 0 );
  }
  if( hist_nhit ) hist_nhit->Reset();
  if( hist_eff  ) hist_eff->Reset();
}

//_____________________________________________________________________________
VDCeff::VDCeff( const char* name, const char* description )
  : THaPhysicsModule(name,description), fNevt(0)
{
  // VDCeff module constructor

}

//_____________________________________________________________________________
VDCeff::~VDCeff()
{
  // Destructor

  RemoveVariables();
}

//_____________________________________________________________________________
void VDCeff::Reset( Option_t* opt )
{
  // Clear event-by-event data

  Clear(opt);
  for( variter_t it = fVDCvar.begin(); it != fVDCvar.end(); ++it ) {
    it->Reset(opt);
  }
}

//_____________________________________________________________________________
Int_t VDCeff::Begin( THaRunBase* )
{
  // Start of analysis

  if( !IsOK() ) return -1;

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
      thePlane.hist_eff = new TH1F( name, title,
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
THaAnalysisObject::EStatus VDCeff::Init( const TDatime& run_time )
{
  // Initialize VDCeff physics module

  const char* const here = "Init";

  // Standard initialization. Calls ReadDatabase(), ReadRunDatabase(),
  // and DefineVariables() (see THaAnalysisObject::Init)
  if( THaPhysicsModule::Init( run_time ) != kOK )
    return fStatus;

  // Associate global variable pointers. This can and should be done
  // at every reinitialization (pointers in VDC class may have changed)
  for( variter_t it = fVDCvar.begin(); it != fVDCvar.end(); ++it ) {
    VDCvar_t& thePlane = *it;
    assert( !thePlane.name.IsNull() );
    thePlane.pvar = gHaVars->Find( thePlane.name );
    if( !thePlane.pvar ) {
      Warning( Here(here), "Cannot find global VDC variable %s. Ignoring.",
	       thePlane.name.Data() );
    }
  }

  return fStatus = kOK;
}

//_____________________________________________________________________________
Int_t VDCeff::Process( const THaEvData& evdata )
{
  // Update VDC efficiency histograms with current event data

  const char* const here = "Process";

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

//_____________________________________________________________________________
Int_t VDCeff::ReadDatabase( const TDatime& date )
{
  // Read database. Gets the VDC variables containing wire spectra.

  const char* const here = "ReadDatabase";
  const char* const separators = ", \t";
  const Int_t NPAR = 3;

  FILE* f = OpenFile( date );
  if( !f ) return kFileError;

  TString configstr;
  // Default values
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
    status = LoadDB( f, date, request );
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
  // Parse the vdcvars string and set up definition structure for each item
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
    if( name.IsNull() ) {
      Error( Here(here), "Missing global variable name at vdcvars[%d]. "
	     "Fix database.", ip );
      return kInitError;
    }
    if( histname.IsNull() ) {
      Error( Here(here), "Missing histogram name at vdcvars[%d]. "
	     "Fix database.", ip );
      return kInitError;
    }
    if( nwire <= 0 || nwire > kMaxShort ) {
      Error( Here(here), "Illegal number of wires = %d for VDC variable %s. "
	     "Fix database.", nwire, name.Data() );
      return kInitError;
    }
    // Check for duplicate name or histname
    for( variter_t it = fVDCvar.begin(); it != fVDCvar.end(); ++it ) {
      VDCvar_t& thePlane = *it;
      if( thePlane.name == name ) {
	Error( Here(here), "Duplicate global variable name %s. "
	       "Fix database.", name.Data() );
	return kInitError;
      }
      if( thePlane.histname == histname ) {
	Error( Here(here), "Duplicate histogram name %s. "
	       "Fix database.", histname.Data() );
	return kInitError;
      }
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
