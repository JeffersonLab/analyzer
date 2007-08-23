//*-- Author :    Ole Hansen   15-June-01

//////////////////////////////////////////////////////////////////////////
//
// THaSubDetector
//
//////////////////////////////////////////////////////////////////////////

#include "THaSubDetector.h"
#include "THaDetector.h"
#include "TString.h"
//#include "TError.h"

ClassImp(THaSubDetector)

//_____________________________________________________________________________
THaSubDetector::THaSubDetector( const char* name, const char* description,
				THaDetectorBase* detector )
  : THaDetectorBase(name,description), fDetector(detector)
{
  // Normal constructor with name and description

  static const char* const here = "THaSubDetector()";

  if( !name || !*name ) {
    Error( Here(here), "Must construct subdetector with valid name! "
	   "Object construction failed." );
    MakeZombie();
    return;
  }
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
}

//_____________________________________________________________________________
void THaSubDetector::MakePrefix()
{
  // Set up name prefix for global variables. 
  // Internal function called by constructors of derived classes.
  // Subdetector prefixes are of form "<detector_prefix><subdetector_name>.",
  // e.g. R.vdc.u1.

  TString basename;
  THaDetectorBase *det = GetDetector();
  if( det ) {
    basename = det->GetPrefix();
    basename.Chop();
  } else
    Warning( Here("MakePrefix()"), "No detector defined for this subdetector! "
	     "Fix your code!" );

  THaDetectorBase::MakePrefix( basename.Data() );
}

//_____________________________________________________________________________
THaApparatus* THaSubDetector::GetApparatus() const
{
  // Return parent apparatus (parent of parent detector)

  THaDetectorBase* parent = GetDetector();
  while( parent && dynamic_cast<THaSubDetector*>(parent) )
    parent = static_cast<THaSubDetector*>(parent)->GetDetector();
  THaDetector* det = dynamic_cast<THaDetector*>(parent);
  if( det == NULL )
    return NULL;
  return det->GetApparatus();
}
