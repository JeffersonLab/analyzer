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

  Int_t ret = InterStageModule::DefineVariables(mode);// exports fDataValid etc.
  if( ret )
    return ret;

  RVarDef vars[] = {
    { "evtime",  "Time offset for event", "fEvtTime" },
    { nullptr }
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
  DBRequest config_request[] = {
    { "glob_off", &fGlOffset, kDouble, 0, true },
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

} // namespace HallA

ClassImp(Podd::TimeCorrectionModule)

