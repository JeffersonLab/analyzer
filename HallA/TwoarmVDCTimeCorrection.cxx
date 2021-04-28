//*-- Author :    Ole Hansen   18-Oct-19

//////////////////////////////////////////////////////////////////////////
//
// HallA::TwoarmVDCTimeCorrection
//
// Calculates a trigger time correction for the VDC from the TDC
// difference between two scintillators.
// Runs after Decode.
//
//////////////////////////////////////////////////////////////////////////

#include "TwoarmVDCTimeCorrection.h"
#include "THaDetector.h"
#include "THaAnalyzer.h"
#include "THaVarList.h"
#include "THaCut.h"
#include "THaGlobals.h"
#include "TError.h"
#include <vector>

using namespace std;
using Podd::TimeCorrectionModule;

namespace HallA {

//_____________________________________________________________________________
TwoarmVDCTimeCorrection::TwoarmVDCTimeCorrection( const char* name,
  const char* description, const char* scint1, const char* scint2,
  const char* cond )
  : TimeCorrectionModule(name, description, THaAnalyzer::kDecode),
    fDet{DetDef(scint1), DetDef(scint2)}, fCondExpr(cond), fCond(nullptr),
    fDidInitDefs(false)
{
  // Constructor
}

//_____________________________________________________________________________
TwoarmVDCTimeCorrection::~TwoarmVDCTimeCorrection()
{
  // Destructor

  delete fCond;
}

//_____________________________________________________________________________
THaAnalysisObject::EStatus
TwoarmVDCTimeCorrection::Init( const TDatime& run_time )
{
  // Initialize the module.
  // Locate the relevant variables of the two scintillator detectors specified
  // in the constructor and store pointers to them

  const char* const here = "Init";

  // Standard initialization. Calls this object's ReadDatabase()
  // and DefineVariables().
  if( InterStageModule::Init( run_time ) != kOK )
    return fStatus;

  // Make the name of our block of tests. We need fPrefix to be set
  MakeBlockName();

  // Find reference detectors
  // In principle we should check that these are THaScintillators, but Tritium/nnL
  // use their own TriFadcScin class, which inherits from THaNonTrackingDetector.
  // In fact, generic THaDetectors are fine for this purpose. As long as they
  // export the required variables which have the expected contents (see below),
  // there's no need to be more restrictive.
  fStatus = kOK;
  for( auto& detdef : fDet ) {
    auto* obj = dynamic_cast<THaDetector*>
    ( FindModule(detdef.fName.Data(), "THaDetector"));
    if( !obj ) {
      fStatus = kInitError;
      // Keep going to get reports on all failures
      continue;
    }
    detdef.fObj = obj;
    detdef.fNelem = obj->GetNelem();
    // Retrieve pointers to the global variables we need
    class VarDef {
    public:
      TString  name;
      THaVar*& pvar;
    };
    vector<VarDef> vardefs {
      { detdef.fName + ".nthit",  detdef.fNthit },
      { detdef.fName + ".t_pads", detdef.fTpad },
      { detdef.fName + ".rt_c",   detdef.fRT },
      { detdef.fName + ".lt_c",   detdef.fLT }
    };
    for( const auto& vardef : vardefs ) {
      vardef.pvar = gHaVars->Find(vardef.name); // sets the relevant THaVar
                                           // pointer in the current detdef
      if( !vardef.pvar ) {
        Error(Here(here), "Global variable %s not found. "
                          "Module not initialized.", vardef.name.Data() );
        fStatus = kInitError;
        // Keep going to get reports on all failures
      }
    }
  }
  fIsInit = (fStatus == kOK);

  return fStatus;
}

//_____________________________________________________________________________
Int_t TwoarmVDCTimeCorrection::Process( const THaEvData& )
{
  // Calculate per-event time correction

  const char* const here = "Process";

  if( !fIsInit )
    return -1;

  if( !fDidInitDefs ) {
    InitDefs();
    fDidInitDefs = true;
  }

  fEvtTime = fGlOffset;
  if( fCond && !fCond->EvalCut() )
    return 0;

  const DetDef& det1 = fDet[0], det2 = fDet[1];
  if( det1.fNthit->GetValueInt() > 0 && det2.fNthit->GetValueInt() > 0 ) {
    // Both scintillators have at least one complete hit, i.e. the TDCs on both
    // sides of a paddle fired
    //FIXME: taking index = 0 when fNthit > 1 not necessarily correct!
    auto thePad1 = static_cast<Int_t>( det1.fTpad->GetValueInt(0) );
    auto thePad2 = static_cast<Int_t>( det2.fTpad->GetValueInt(0) );
    if( thePad1 >= 0 && thePad1 < det1.fNelem &&
        thePad2 >= 0 && thePad2 < det2.fNelem ) {
      //TODO: use fLT too
      fEvtTime += det1.fRT->GetValue(thePad1) - det2.fRT->GetValue(thePad2);
      fDataValid = true;
    } else {
      // If we get here, the detector lied to us: if fNthit > 0, then all
      // fTpad[i] should always be valid pad numbers
      Warning(Here(here), "Bad pad number %s = %d, %s = %d. "
                          "Should never happen. Call expert.",
              det1.fTpad->GetName(), thePad1, det2.fTpad->GetName(), thePad2);
      return -2;
    }
  }

  return 0;
}

//_____________________________________________________________________________
Int_t TwoarmVDCTimeCorrection::ReadDatabase( const TDatime& date )
{
  // Read this detector's parameters from the database.
  // 'date' contains the date/time of the run being analyzed.

  Int_t ret = TimeCorrectionModule::ReadDatabase(date);
  if( ret != kOK )
    return ret;

  FILE* file = OpenFile(date);
  if( !file ) {
    // Missing file OK -- all keys are optional
    fIsInit = true;
    return kOK;
  }

  // Read configuration parameters
  fIsInit = false;
  DBRequest config_request[] = {
    { "condition", &fCondExpr, kTString, 0, true },
    { nullptr }
  };
  Int_t err = LoadDB( file, date, config_request );
  fclose(file);
  if( err )
    return err;

  fIsInit = true;
  return kOK;
}

//_____________________________________________________________________________
Int_t TwoarmVDCTimeCorrection::InitDefs()
{
  // Initialize tests based on the string expressions read from the database.
  // Return number of successful definitions.

  Int_t ndef = 0;

  // Delete existing test, if any, to allow reinitialization
  delete fCond;
  fCond = nullptr;

  // Parse test expression, if any
  if( !fCondExpr.IsNull() ) {
    fCond = new THaCut( "cond", fCondExpr, fTestBlockName );
    if( fCond->IsZombie() or fCond->IsError() ) {
      delete fCond;
      fCond = nullptr;
    }
    ++ndef;
  }

  return ndef;
}

//____________________________________________________________________________
void TwoarmVDCTimeCorrection::MakeBlockName()
{
  // Create the test block name

  if( fPrefix && *fPrefix ) {
    fTestBlockName = fPrefix;
    fTestBlockName.Chop(); // remove trailing "."
  } else
    fTestBlockName = fName;
  fTestBlockName.Append("_Tests");
}

//_____________________________________________________________________________

} // namespace HallA

ClassImp(HallA::TwoarmVDCTimeCorrection)

