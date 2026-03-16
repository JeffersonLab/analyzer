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
#include <vector>

using namespace std;

//_____________________________________________________________________________
THaBeam::THaBeam( const char* name, const char* desc ) : 
  THaApparatus( name,desc ), fRunParam(nullptr)
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

  const vector<RVarDef> vars = {
    { .name = "x",     .desc = "reconstructed x-position at nom. interaction point", .def = "fPosition.fX" },
    { .name = "y",     .desc = "reconstructed y-position at nom. interaction point", .def = "fPosition.fY" },
    { .name = "z",     .desc = "reconstructed z-position at nom. interaction point", .def = "fPosition.fZ" },
    { .name = "dir.x", .desc = "reconstructed x-component of beam direction",        .def = "fDirection.fX" },
    { .name = "dir.y", .desc = "reconstructed y-component of beam direction",        .def = "fDirection.fY" },
    { .name = "dir.z", .desc = "reconstructed z-component of beam direction",        .def = "fDirection.fZ" },
  };

  if( Int_t ret = DefineVarsFromList(vars, mode) )
    return ret;

  // Define the variables for the beam info subobject
  return DefineVarsFromList(GetRVarDef(), mode);
}

//_____________________________________________________________________________
void THaBeam::Update()
{
  // Update the fBeamIfo data with the info from the current event

  if( THaRunParameters* rp = gHaRun->GetParameters() )
    fBeamIfo.Set( rp->GetBeamP(), fDirection, fPosition,
		  rp->GetBeamPol() );
  else
    fBeamIfo.Set( kBig, fDirection, fPosition, kBig );
}

//_____________________________________________________________________________
#if ROOT_VERSION_CODE < ROOT_VERSION(6,36,0)
ClassImp(THaBeam)
#endif


