//*-- Author :    Ole Hansen   25-Feb-03

//////////////////////////////////////////////////////////////////////////
//
// THaElectronKine
//
// Special case of THaPrimaryKine, with an electron as the 
// primary particle.  Exists for convenience and backwards-compatibility.
//
//////////////////////////////////////////////////////////////////////////

#include "THaElectronKine.h"

ClassImp(THaElectronKine)

static const Double_t electron_mass = 0.511e-3;

//_____________________________________________________________________________
THaElectronKine::THaElectronKine( const char* name, const char* description,
				const char* spectro, Double_t target_mass ) :
  THaPrimaryKine(name,description,spectro,electron_mass,target_mass)
{
  // Normal constructor. 
}

//_____________________________________________________________________________
THaElectronKine::~THaElectronKine()
{
  // Destructor
}

  
  
