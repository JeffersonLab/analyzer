//*-- Author :    Ole Hansen   17-Mar-03

//////////////////////////////////////////////////////////////////////////
//
// THaVertexModule
//
// Base class for a "vertex" processing module, which is a 
// specialized physics module.
//
//////////////////////////////////////////////////////////////////////////

#include "THaVertexModule.h"

using namespace std;

//_____________________________________________________________________________
THaVertexModule::THaVertexModule() : fVertexOK(kFALSE)
{
  // Normal constructor.

}

//_____________________________________________________________________________
THaVertexModule::~THaVertexModule()
{
  // Destructor

}

//_____________________________________________________________________________
void THaVertexModule::VertexClear()
{
  // Reset vertex object

  const Double_t kBig = 1e38;

  fVertex.SetXYZ(kBig,kBig,kBig);
  fVertexError.SetXYZ(1.0,1.0,1.0);
  fVertexOK = kFALSE;
}

//_____________________________________________________________________________
ClassImp(THaVertexModule)
