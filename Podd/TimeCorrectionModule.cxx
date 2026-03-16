//*-- Author :    Ole Hansen   18-Oct-19

//////////////////////////////////////////////////////////////////////////
//
// HallA::TimeCorrectionModule
//
// Calculates a time correction for the VDC based on a generic formula.
//
//////////////////////////////////////////////////////////////////////////

#include "TimeCorrectionModule.h"
#include "THaAnalyzer.h"
#include <vector>

using namespace std;

namespace Podd {

//_____________________________________________________________________________
TimeCorrectionModule::TimeCorrectionModule( const char* name,
                                            const char* description,
                                            Int_t stage )
  : InterStageModule(name, description, stage),
    fGlOffset(0.0), fEvtTime(0.0)
{
  // Constructor
}

//_____________________________________________________________________________
TimeCorrectionModule::~TimeCorrectionModule()
{
  // Destructor

  RemoveVariables();
}

//_____________________________________________________________________________
void TimeCorrectionModule::Clear( Option_t* opt )
{
  InterStageModule::Clear(opt);
  fEvtTime = fGlOffset;
}

//_____________________________________________________________________________
Int_t TimeCorrectionModule::DefineVariables( THaAnalysisObject::EMode mode )
{
  // Define/delete event-by-event global variables

  if( Int_t ret = InterStageModule::DefineVariables(mode) ) // exports fDataValid etc.
    return ret;

  const vector<RVarDef> vars = {
    { .name = "evtime",  .desc = "Time offset for event", .def = "fEvtTime" },
  };

  return DefineVarsFromList( vars, mode );
}

//_____________________________________________________________________________
Int_t TimeCorrectionModule::ReadDatabase( const TDatime& date )
{
  // Read this detector's parameters from the database.

//  const char* const here = "ReadDatabase";

  FILE* file = OpenFile( date );

  fGlOffset = 0.0;
  if( !file ) {
    // Missing file OK -- all keys are optional
    fIsInit = true;
    return kOK;
  }

  // Read configuration parameters
  fIsInit = false;
  const vector<DBRequest> config_request = {
    { .name = "glob_off", .var = &fGlOffset, .optional = true },
  };
  Int_t err = LoadDB( file, date, config_request );
  (void)fclose(file);
  if( err )
    return err;

  fIsInit = true;
  return kOK;
}

//_____________________________________________________________________________

} // namespace HallA

#if ROOT_VERSION_CODE < ROOT_VERSION(6,36,0)
ClassImp(Podd::TimeCorrectionModule)
#endif

