//*-- Author :    Ole Hansen   18-Oct-19

//////////////////////////////////////////////////////////////////////////
//
// HallA::TwoarmVDCTimeCorrection
//
// Calculates a time correction for the VDC based on a generic formula.
// Runs after Decode.
//
//////////////////////////////////////////////////////////////////////////

#include "TwoarmVDCTimeCorrection.h"
#include "THaScintillator.h"
#include "THaAnalyzer.h"
#include "THaVarList.h"
#include "TError.h"
#include <vector>

using namespace std;
using Podd::InterStageModule;

namespace HallA {

//_____________________________________________________________________________
TwoarmVDCTimeCorrection::TwoarmVDCTimeCorrection( const char* name,
                                                  const char* description,
                                                  const char* scint1,
                                                  const char* scint2 )
  : InterStageModule(name, description, THaAnalyzer::kDecode),
    fName1(scint1), fName2(scint2), fNpads1(0), fNpads2(0),
    fRT1(nullptr), fLT1(nullptr), fNhit1(nullptr), fTPad1(nullptr),
    fRT2(nullptr), fLT2(nullptr), fNhit2(nullptr), fTPad2(nullptr),
    fGlOffset(0.0), fEvtTime(0.0)
{
  // Constructor
}

//_____________________________________________________________________________
TwoarmVDCTimeCorrection::~TwoarmVDCTimeCorrection()
{
  // Destructor

  RemoveVariables();
}

//_____________________________________________________________________________
void TwoarmVDCTimeCorrection::Clear( Option_t* opt )
{
  InterStageModule::Clear(opt);
  fEvtTime = fGlOffset;
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

  // Find scintillators
  auto scint1 = dynamic_cast<THaScintillator*>
    ( FindModule(fName1.Data(), "THaScintillator"));
  if( !scint1 ) {
    fStatus = kInitError;
    return fStatus;
  }
  auto scint2 = dynamic_cast<THaScintillator*>
    ( FindModule(fName2.Data(), "THaScintillator"));
  if( !scint2 ) {
    fStatus = kInitError;
    return fStatus;
  }
  fNpads1 = scint1->GetNelem();
  fNpads2 = scint2->GetNelem();

  // Retrieve pointers to the global variables we need
  struct VarDef {
    TString  name;
    THaVar*& pvar;
  };
  vector<VarDef> vardef {
    {fName1+".rt_c",  fRT1},   {fName1+".lt_c", fLT1},
    {fName1+".nthit", fNhit1}, {fName1+".t_pads", fTPad1},
    {fName2+".rt_c",  fRT2},   {fName2+".lt_c", fLT2},
    {fName2+".nthit", fNhit2}, {fName2+".t_pads", fTPad2}
  };

  fIsInit = true;
  for( const auto& def : vardef ) {
    def.pvar = gHaVars->Find(def.name);
    if( !def.pvar ) {
      Error(Here(here), "Global variable %s not found. Module not initialized.",
            def.name.Data());
      fIsInit = false;
    }
  }
  if( !fIsInit )
    fStatus = kInitError;

  return fStatus;
}

//_____________________________________________________________________________
Int_t TwoarmVDCTimeCorrection::Process( const THaEvData& )
{
  // Calculate per-event time correction

  const char* const here = "Process";

  if( !fIsInit )
    return -1;

  //TODO: check event type
  Int_t nhit1 = fNhit1->GetValueInt();
  Int_t nhit2 = fNhit2->GetValueInt();
  if( nhit1 > 0 && nhit2 > 0 ) {
    // Both scintillators have at least one complete hit
    Int_t npad1 = fTPad1->GetValueInt();
    Int_t npad2 = fTPad2->GetValueInt();
    if( npad1 >= 0 && npad1 < fNpads1 && npad2 >= 0 && fNpads2 ) {
      //TODO: use fLT1/fLT2
      fEvtTime = fRT1->GetValue(npad1) - fRT2->GetValue(npad2);
      fDataValid = true;
    } else {
      Warning(Here(here), "Bad number of pads hit %s = %d, %s = %d",
              fTPad1->GetName(), npad1, fTPad2->GetName(), npad2 );
      return -2;
    }
  }
  fEvtTime += fGlOffset;

  return 0;
}

//_____________________________________________________________________________
Int_t TwoarmVDCTimeCorrection::DefineVariables( THaAnalysisObject::EMode mode )
{
  // Define/delete event-by-event global variables

  bool save_state = fIsSetup;
  InterStageModule::DefineVariables(mode);  // exports fDataValid etc.
  fIsSetup = save_state;

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  RVarDef vars[] = {
    { "evtime",  "Time offset for event", "fEvtTime" },
    { nullptr }
  };

  return DefineVarsFromList( vars, mode );
}

//_____________________________________________________________________________
Int_t TwoarmVDCTimeCorrection::ReadDatabase( const TDatime& date )
{
  // Read this detector's parameters from the database.

//  const char* const here = "ReadDatabase";

  FILE* file = OpenFile( date );
  if( !file )
    return kFileError;

  fIsInit = false;
  fGlOffset = 0.0;
  // Read configuration parameters
  DBRequest config_request[] = {
    { "glob_off", &fGlOffset, kDouble, 0, true },
    { nullptr }
  };
  Int_t err = LoadDB( file, date, config_request, fPrefix );
  fclose(file);
  if( err )
    return err;

  fIsInit = true;
  return kOK;
}

//_____________________________________________________________________________

} // namespace HallA

ClassImp(HallA::TwoarmVDCTimeCorrection)

