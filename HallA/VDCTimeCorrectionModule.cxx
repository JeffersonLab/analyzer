//*-- Author :    Ole Hansen   21-Oct-19

//////////////////////////////////////////////////////////////////////////
//
// HallA::VDCTimeCorrectionModule
//
// Calculates a time correction for the VDC based on a generic formula.
//
//////////////////////////////////////////////////////////////////////////

#include "VDCTimeCorrectionModule.h"
#include "THaAnalyzer.h"

using namespace std;
using Podd::InterStageModule;

namespace HallA {

//_____________________________________________________________________________
VDCTimeCorrectionModule::VDCTimeCorrectionModule( const char* name,
                                                  const char* description,
                                                  Int_t stage )
  : InterStageModule(name, description, stage),
    fGlOffset(0.0), fEvtTime(0.0)
{
  // Constructor
}

//_____________________________________________________________________________
VDCTimeCorrectionModule::~VDCTimeCorrectionModule()
{
  // Destructor

  RemoveVariables();
}

//_____________________________________________________________________________
void VDCTimeCorrectionModule::Clear( Option_t* opt )
{
  InterStageModule::Clear(opt);
  fEvtTime = fGlOffset;
}

//_____________________________________________________________________________
Int_t VDCTimeCorrectionModule::DefineVariables( THaAnalysisObject::EMode mode )
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
Int_t VDCTimeCorrectionModule::ReadDatabase( const TDatime& date )
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
  Int_t err = LoadDB( file, date, config_request );
  fclose(file);
  if( err )
    return err;

  fIsInit = true;
  return kOK;
}

//_____________________________________________________________________________

} // namespace HallA

ClassImp(HallA::VDCTimeCorrectionModule)

