//*-- Author :    Ole Hansen   14-Feb-03

//////////////////////////////////////////////////////////////////////////
//
// THaPhysicsModule
//
// Abstract base class for a "physics" processing module.
//
// Physics processing takes place after detector and apparatus
// processing, i.e. after basic tracking. Physics modules 
// combine data from different apparatuses.
//
// Examples:
//  - Vertex calculations (e.g. "reaction_z")
//  - Vertex corrections  (e.g. extended target corrections)
//  - Target energy loss calculations (requires beam position and tracks)
//  - single-arm and coincidence kinematics corrections.
//
// These routines typically require some "global" database information
// such as spectrometer angles, central momentum, target parameters, etc.
// Some database functions are provided.
//
//////////////////////////////////////////////////////////////////////////

#include "THaPhysicsModule.h"

using namespace std;

//_____________________________________________________________________________
THaPhysicsModule::THaPhysicsModule( const char* name, 
				    const char* description ) :
  THaAnalysisObject(name,description), fMultiTrk(false), fDataValid(false)
{
  // Constructor
}

//_____________________________________________________________________________
THaPhysicsModule::~THaPhysicsModule()
{
  // Destructor
}

//_____________________________________________________________________________
void THaPhysicsModule::PrintInitError( const char* here )
{
  Error( Here(here), "Cannot set. Module already initialized." );
}

//_____________________________________________________________________________

ClassImp(THaPhysicsModule)
