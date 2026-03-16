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
#include <vector>

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
const vector<RVarDef> THaVertexModule::GetRVarDef()
{
  // Return definition block of global variables for this object

  const vector<RVarDef> vars = {
    { .name = "x",  .desc = "vertex x-position", .def = "fVertex.fX" },
    { .name = "y",  .desc = "vertex y-position", .def = "fVertex.fY" },
    { .name = "z",  .desc = "vertex z-position", .def = "fVertex.fZ" },
    { .name = "ok", .desc = "Data valid (1=ok)", .def = "fVertexOK" },
  };
  return vars;
}


//_____________________________________________________________________________
#if ROOT_VERSION_CODE < ROOT_VERSION(6,36,0)
ClassImp(THaVertexModule)
#endif
