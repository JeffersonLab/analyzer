//*-- Author :    Ole Hansen   17-Mar-03

//////////////////////////////////////////////////////////////////////////
//
// THaVertexModule
//
// Abstract base class for a "vertex" processing module, which is a 
// specialized physics module.
//
//////////////////////////////////////////////////////////////////////////

#include "THaVertexModule.h"

ClassImp(THaVertexModule)

//_____________________________________________________________________________
THaVertexModule::THaVertexModule( const char* name, 
				  const char* description ) :
  THaPhysicsModule(name,description)
{
  // Normal constructor.

}

//_____________________________________________________________________________
THaVertexModule::~THaVertexModule()
{
  // Destructor

}
