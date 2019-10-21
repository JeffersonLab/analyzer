//*-- Author :    Ole Hansen   14-Feb-03

//////////////////////////////////////////////////////////////////////////
//
// InterStageModule
//
// Abstract base class for an "inter-stage" processing module.
//
// Similar to a THaPhysicsModule, but with processing after a given
// analysis stage (Decode, CoarseReconstruct, etc.) This can be used
// to combine data from different apparatuses early in the processing.
//
// Like physics modules, these modules live in the gHaPhysics list.
//
//////////////////////////////////////////////////////////////////////////

#include "InterStageModule.h"

namespace Podd {

//_____________________________________________________________________________
InterStageModule::InterStageModule( const char* name, const char* description,
                                    Int_t stage ) :
  THaAnalysisObject(name, description), fStage(stage), fDataValid(false)
{
  // Constructor
}

//_____________________________________________________________________________
void InterStageModule::Clear( Option_t* opt )
{
  THaAnalysisObject::Clear(opt);
  fDataValid = false;
}

//_____________________________________________________________________________
void InterStageModule::PrintInitError( const char* here )
{
  Error(Here(here), "Cannot set. Module already initialized.");
}

//_____________________________________________________________________________
Int_t InterStageModule::DefineVariables( THaAnalysisObject::EMode mode )
{
  // Define/delete global variables for this module

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  static const RVarDef vars[] = {
    { "good", "Data valid (1=ok)", "fDataValid" },
    { nullptr }
  };

  return DefineVarsFromList( vars, mode );

}
//_____________________________________________________________________________

} // namespace Podd

ClassImp(Podd::InterStageModule)
