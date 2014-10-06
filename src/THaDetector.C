//*-- Author :    Ole Hansen   15-May-00

//////////////////////////////////////////////////////////////////////////
//
// THaDetector
//
//////////////////////////////////////////////////////////////////////////

#include "THaDetector.h"
#include "THaApparatus.h"

ClassImp(THaDetector)

//_____________________________________________________________________________
THaDetector::THaDetector( const char* name, const char* description,
			  THaApparatus* apparatus )
  : THaDetectorBase(name,description), fApparatus(apparatus)
{
  // Constructor

  if( !name || !*name ) {
    Error( "THaDetector()", "Must construct detector with valid name! "
	   "Object construction failed." );
    MakeZombie();
    return;
  }
}

//_____________________________________________________________________________
THaDetector::THaDetector( ) : fApparatus(0) {
  // for ROOT I/O only
}

//_____________________________________________________________________________
THaDetector::~THaDetector()
{
  // Destructor
}

//_____________________________________________________________________________
THaApparatus* THaDetector::GetApparatus() const
{
  return static_cast<THaApparatus*>(fApparatus.GetObject());
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
  // Set up name prefix for global variables. Internal function called
  // during initialization.

  THaApparatus *app = GetApparatus();
  const char* basename = (app != 0) ? app->GetName() : 0;
  THaDetectorBase::MakePrefix( basename );

}

