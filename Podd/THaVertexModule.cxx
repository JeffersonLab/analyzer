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
#include "DataType.h"  // for kBig

using namespace std;

//_____________________________________________________________________________
THaVertexModule::THaVertexModule() : fVertexOK(false)
{
  // Normal constructor.

}

//_____________________________________________________________________________
void THaVertexModule::VertexClear()
{
  // Reset vertex object

  fVertex.SetXYZ(kBig,kBig,kBig);
  fVertexError.SetXYZ(1.0,1.0,1.0);
  fVertexOK = false;
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
    { nullptr }
  };
  return vars;
}


//_____________________________________________________________________________
ClassImp(THaVertexModule)
