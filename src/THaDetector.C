//*-- Author :    Ole Hansen   15-May-00

//////////////////////////////////////////////////////////////////////////
//
// THaDetector
//
//////////////////////////////////////////////////////////////////////////

#include "THaDetector.h"
#include "THaApparatus.h"

#include <cstring>

ClassImp(THaDetector)

//_____________________________________________________________________________
THaDetector::THaDetector( const char* name, const char* description,
			  THaApparatus* apparatus )
  : THaDetectorBase(name,description), fApparatus(apparatus)
{
  // Constructor

  if( !name || strlen(name) == 0 ) {
    Error( "THaDetector()", "Must construct detector with valid name! "
	   "Object construction failed." );
    MakeZombie();
    return;
  }
}

//_____________________________________________________________________________
THaDetector::~THaDetector()
{
  // Destructor
}

//_____________________________________________________________________________
void THaDetector::SetApparatus( THaApparatus* apparatus )
{
  // Associate this detector with the given apparatus.
  // Only possible before initialization.

  if( IsInit() ) {
    Warning( Here("SetApparatus()"), "Cannot set apparatus. "
	     "Object already initialized.");
    return;
  }
  fApparatus = apparatus;
}

//_____________________________________________________________________________
void THaDetector::MakePrefix()
{
  // Set up name prefix for global variables. Internal function called by
  // constructor.

  const char* basename = NULL;
  if( fApparatus )
    basename = fApparatus->GetName();
  THaDetectorBase::MakePrefix( basename );

}
  
