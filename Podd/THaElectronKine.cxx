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
  // Standard constructor. 
}

//_____________________________________________________________________________
THaElectronKine::THaElectronKine( const char* name, const char* description,
				  const char* spectro, const char* beam,
				  Double_t target_mass ) :
  THaPrimaryKine(name,description,spectro,beam,target_mass)
{
  // Constructor with specification of optional beam apparatus
  SetMass( electron_mass );
}

//_____________________________________________________________________________
THaElectronKine::~THaElectronKine()
{
  // Destructor
}

  
  
