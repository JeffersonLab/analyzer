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

ClassImp(THaBeam)


//_____________________________________________________________________________
THaBeam::THaBeam( const char* name, const char* desc ) : 
  THaApparatus( name,desc )
{
  // Constructor.
  // Protected. Can only be called by derived classes.

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
    
  return DefineVarsFromList( vars, mode );

}
