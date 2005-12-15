//*-- Author :    Ole Hansen   15-Jun-01

//////////////////////////////////////////////////////////////////////////
//
// THaDetectorBase
//
// Abstract base class for a detector or subdetector.
//
// Derived classes must define all internal variables, a constructor 
// that registers any internal variables of interest in the global 
// physics variable list, and a Decode() method that fills the variables
// based on the information in the THaEvData structure.
//
//////////////////////////////////////////////////////////////////////////

#include "THaDetectorBase.h"
#include "THaDetMap.h"

ClassImp(THaDetectorBase)

//_____________________________________________________________________________
THaDetectorBase::THaDetectorBase( const char* name, 
				  const char* description ) :
  THaAnalysisObject(name,description), fNelem(0)
{
  // Normal constructor. Creates an empty detector map.

  fSize[0] = fSize[1] = fSize[2] = 0.0;
  fDetMap = new THaDetMap;
}

//_____________________________________________________________________________
THaDetectorBase::THaDetectorBase() : fDetMap(0) {
  // for ROOT I/O only
}

//_____________________________________________________________________________
THaDetectorBase::~THaDetectorBase()
{
  // Destructor
  if (fDetMap) delete fDetMap;
}

//_____________________________________________________________________________
void THaDetectorBase::PrintDetMap( Option_t* opt ) const
{
  // Print out detector map.

  fDetMap->Print( opt );
}


