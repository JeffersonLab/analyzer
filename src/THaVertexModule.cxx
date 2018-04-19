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
#include "VarDef.h"

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
const RVarDef* THaVertexModule::GetRVarDef()
{
  // Return definition block of global variables for this object

  static const RVarDef vars[] = {
    { "x",  "vertex x-position", "fVertex.fX" },
    { "y",  "vertex y-position", "fVertex.fY" },
    { "z",  "vertex z-position", "fVertex.fZ" },
    { "ok", "Data valid (1=ok)", "fVertexOK" },
    { 0 }
  };
  return vars;
}


//_____________________________________________________________________________
ClassImp(THaVertexModule)
