//*-- Author :    Ole Hansen   18 April 2001

//////////////////////////////////////////////////////////////////////////
//
// THaParticleInfo
//
// Properties of a particle
//
//////////////////////////////////////////////////////////////////////////

#include "THaParticleInfo.h"
#include <iostream>

using namespace std;

//_____________________________________________________________________________
THaParticleInfo::THaParticleInfo() : fMass(0), fCharge(0)
{
  // Default constructor
}

//_____________________________________________________________________________
THaParticleInfo::THaParticleInfo( const THaParticleInfo& rhs ) : TNamed(rhs)
{
  // Copy ctor

  fMass    = rhs.fMass;
  fCharge  = rhs.fCharge;
}

//_____________________________________________________________________________
THaParticleInfo& THaParticleInfo::operator=(const THaParticleInfo& rhs )
{
  // Assignment operator

  if( this != &rhs ) {
    TNamed::operator=(rhs);
    fMass    = rhs.fMass;
    fCharge  = rhs.fCharge;
  }
  return *this;
}

//_____________________________________________________________________________
void THaParticleInfo::Print( Option_t* opt ) const
{
  // Print particle info

  TNamed::Print( opt );
  cout << "Mass:   " << fMass << endl;
  cout << "Charge: " << fCharge << endl;
}


//_____________________________________________________________________________

ClassImp(THaParticleInfo)
