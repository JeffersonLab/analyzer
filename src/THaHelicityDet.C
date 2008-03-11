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
//#include "THaDB.h"

using namespace std;

//_____________________________________________________________________________
THaHelicityDet::THaHelicityDet() : fHelicity(kUnknown), fSign(1)
{
  // Constructor
}

//_____________________________________________________________________________
THaHelicityDet::THaHelicityDet( const char* name, const char* description ,
				THaApparatus* apparatus )
  : THaDetector( name, description, apparatus ), fHelicity(kUnknown), fSign(1)
{
  // Constructor
}

//_____________________________________________________________________________
THaHelicityDet::~THaHelicityDet()
{
  // Destructor
}

//_____________________________________________________________________________
Int_t THaHelicityDet::DefineVariables( EMode mode )
{
  // Initialize global variables

  if( mode == kDefine && fIsSetup ) return kOK;
  fIsSetup = ( mode == kDefine );

  const RVarDef var[] = {
    { "helicity", "Beam helicity",               "fHelicity" },
    { 0 }
  };
  return DefineVarsFromList( var, mode );
}

//_____________________________________________________________________________
Int_t THaHelicityDet::ReadDatabase( const TDatime& date )
{
  // Read fSign

//   if( !gHaDB )
  return kInitError;

//   gHaDB->GetValue( GetPrefix(), "helicity_sign", fSign, date );

//   return kOK;
}

ClassImp(THaHelicityDet)
