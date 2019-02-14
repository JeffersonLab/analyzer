//*-- Author :    Ole Hansen 25-Mar-2003

//////////////////////////////////////////////////////////////////////////
//
// THaBeam
//
// Abstract apparatus class that provides position and direction of
// a particle beam, usually event by event.
// 
//////////////////////////////////////////////////////////////////////////

#include "THaBeam.h"
#include "VarDef.h"
#include "THaRunBase.h"
#include "THaRunParameters.h"
#include "THaGlobals.h"

//_____________________________________________________________________________
THaBeam::THaBeam( const char* name, const char* desc ) : 
  THaApparatus( name,desc ), fRunParam(0)
{
  // Constructor.
  // Protected. Can only be called by derived classes.

  fBeamIfo.SetBeam( this );
}


//_____________________________________________________________________________
THaBeam::~THaBeam()
{
  // Destructor

  RemoveVariables();
}

//_____________________________________________________________________________
THaAnalysisObject::EStatus THaBeam::Init( const TDatime& run_time )
{
  // Init method for a beam apparatus. Calls the standard THaApparatus
  // initialization and, in addition, finds pointer to the current 
  // run parameters.

  if( !gHaRun || !gHaRun->IsInit() ) {
    Error( Here("Init"), "Current run not initialized. "
	   "Failed to initialize beam apparatus %s (\"%s\"). ",
	   GetName(), GetTitle() );
    return fStatus = kInitError;
  }
  fRunParam = gHaRun->GetParameters();
  if( !fRunParam ) {
    Error( Here("Init"), "Current run has no parameters?!? "
	   "Failed to initialize beam apparatus %s (\"%s\"). ",
	   GetName(), GetTitle() );
    return fStatus = kInitError;
  }

  return THaApparatus::Init(run_time);
}
  
//_____________________________________________________________________________
Int_t THaBeam::DefineVariables( EMode mode )
{
  // Initialize global variables and lookup table for decoder

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  RVarDef vars[] = {
    { "x", "reconstructed x-position at nom. interaction point", "fPosition.fX"},
    { "y", "reconstructed y-position at nom. interaction point", "fPosition.fY"},
    { "z", "reconstructed z-position at nom. interaction point", "fPosition.fZ"},
    { "dir.x", "reconstructed x-component of beam direction", "fDirection.fX"},
    { "dir.y", "reconstructed y-component of beam direction", "fDirection.fY"},
    { "dir.z", "reconstructed z-component of beam direction", "fDirection.fZ"},
    { 0 }
  };
    
  DefineVarsFromList( vars, mode );

  // Define the variables for the beam info subobject
  DefineVarsFromList( GetRVarDef(), mode );

  return 0;
}

//_____________________________________________________________________________
void THaBeam::Update()
{
  // Update the fBeamIfo data with the info from the current event

  THaRunParameters* rp = gHaRun->GetParameters();
  if( rp )
    fBeamIfo.Set( rp->GetBeamP(), fDirection, fPosition,
		  rp->GetBeamPol() );
  else
    fBeamIfo.Set( kBig, fDirection, fPosition, kBig );
}

//_____________________________________________________________________________
ClassImp(THaBeam)


