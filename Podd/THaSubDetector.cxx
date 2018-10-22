//*-- Author :    Ole Hansen   15-June-01

//////////////////////////////////////////////////////////////////////////
//
// THaSubDetector
//
//////////////////////////////////////////////////////////////////////////

#include "THaSubDetector.h"
#include "THaDetector.h"
#include "TString.h"
#include "TError.h"

ClassImp(THaSubDetector)

//_____________________________________________________________________________
THaSubDetector::THaSubDetector( const char* name, const char* description,
				THaDetectorBase* parent )
  : THaDetectorBase(name,description), fParent(parent)
{
  // Normal constructor with name and description

  static const char* const here = "THaSubDetector";

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
THaApparatus* THaSubDetector::GetApparatus() const
{
  // Return parent apparatus (parent of parent detector)

  THaDetector* det = GetMainDetector();
  return det ? det->GetApparatus() : 0;
}

//_____________________________________________________________________________
const char* THaSubDetector::GetDBFileName() const
{
  // Get database file name root. By default, subdetectors use the same 
  // database file as their parent (sub)detector.

  THaDetectorBase* det = GetParent();
  return det ? det->GetDBFileName() : GetPrefix();
}

//_____________________________________________________________________________
THaDetector* THaSubDetector::GetMainDetector() const
{
  // Search up the chain of parent detectors until a THaDetector 
  // (not a THaSubDetector) is found. This is the "main detector"
  // containing this subdetector

  THaDetectorBase* parent = GetParent();
  while( parent && dynamic_cast<THaSubDetector*>(parent) )
    parent = static_cast<THaSubDetector*>(parent)->GetParent();
  return dynamic_cast<THaDetector*>(parent);
}

//_____________________________________________________________________________
void THaSubDetector::MakePrefix()
{
  // Set up name prefix for global variables. 
  //
  // Subdetector prefixes are of form "<parent_prefix><subdetector_name>.",
  // where <parent_prefix> is the prefix of the immediate parent, which may
  // be another subdetector. If a different behavior is needed, subdetectors
  // need to override MakePrefix().

  TString basename;
  THaDetectorBase *det = GetParent();
  if( det ) {
    basename = det->GetPrefix();
    basename.Chop();
  } else
    Warning( Here("MakePrefix()"), "No detector defined for this subdetector! "
	     "Fix your code!" );

  THaDetectorBase::MakePrefix( basename.Data() );
}

//_____________________________________________________________________________
void THaSubDetector::SetParent( THaDetectorBase* detector )
{
  // Associate this subdetector with the given detector or subdetector.
  // Only possible before initialization.

  static const char* const here = "SetParent()";

  if( IsInit() ) {
    Error( Here(here), "Cannot set detector. Object already initialized.");
    return;
  }
  if( !detector ) {
    Error( Here(here), "Cannot set detector to NULL. Detector not changed.");
    return;
  }    
  if( detector == this ) {
    Error( Here(here), "Cannot set detector to self. Detector not changed.");
    return;
  }    
  fParent = detector;
}

///////////////////////////////////////////////////////////////////////////////
