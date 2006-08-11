//*-- Author :    Vincent Sulkosky and R. Feuerbach,  Jan 2006
// Changed to an abstract base class, Ole Hansen, Aug 2006
////////////////////////////////////////////////////////////////////////
//
// THaHelicityDet
//
// Base class for a beam helicity detector.
//
////////////////////////////////////////////////////////////////////////


#include "THaHelicityDet.h"

using namespace std;

//_____________________________________________________________________________
THaHelicityDet::THaHelicityDet() : fHelicity(0) 
{
  // Constructor
}

//_____________________________________________________________________________
THaHelicityDet::THaHelicityDet( const char* name, const char* description ,
				THaApparatus* apparatus )
    : THaDetector( name, description, apparatus ), fHelicity(0) 
{
  // Constructor
}

//_____________________________________________________________________________
THaHelicityDet::~THaHelicityDet()
{
  // Destructor
}

ClassImp(THaHelicityDet)
