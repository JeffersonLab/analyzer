//*-- Author :    Ole Hansen   15-June-01

//////////////////////////////////////////////////////////////////////////
//
// THaSubDetector
//
//////////////////////////////////////////////////////////////////////////

#include "THaSubDetector.h"
#include "TError.h"

#include <string.h>

ClassImp(THaSubDetector)

//_____________________________________________________________________________
THaSubDetector::THaSubDetector( const char* name, const char* description,
				THaDetectorBase* detector )
  : THaDetectorBase(name,description), fDetector(detector)
{
  // Normal constructor with name and description

  static const char* const here = "THaSubDetector()";

  if( !name || strlen(name) == 0 ) {
    Error( here, "Must construct subdetector with valid name! "
	   "Object construction failed." );
    MakeZombie();
    return;
  }
  if( fDetector )
    MakePrefix();
  else
    Warning( Here(here), "No detector defined for this subdetector! "
	     "Fix your code!" );
}

//_____________________________________________________________________________
THaSubDetector::~THaSubDetector()
{
  // Destructor

}

//_____________________________________________________________________________
void THaSubDetector::SetDetector( THaDetectorBase* detector )
{
  // Associate this subdetector with the given detector or subdetector.
  // Only possible before initialization.

  static const char* const here = "SetDetector()";

  if( IsInit() ) {
    Warning( Here(here), "Cannot set detector. Object already initialized.");
    return;
  }
  if( !detector ) {
    Warning( Here(here), "Cannot set detector to NULL. Detector not changed.");
    return;
  }    
  if( detector == this ) {
    Warning( Here(here), "Cannot set detector to self. Detector not changed.");
    return;
  }    
  fDetector = detector;
  MakePrefix();
}

//_____________________________________________________________________________
void THaSubDetector::MakePrefix()
{
  // Set up name prefix for global variables. 
  // Internal function called by constructors of derived classes.

  const char* basename = NULL;
  if( fDetector )
    basename = fDetector->GetName();
  THaDetectorBase::MakePrefix( basename );

}
