//*-- Author :    Ole Hansen   23-May-03

//////////////////////////////////////////////////////////////////////////
//
// THaTrackingModule
//
// Base class for a "track" processing module, which is a  
// specialized physics module.
//
//////////////////////////////////////////////////////////////////////////

#include "THaTrackingModule.h"
#include "VarDef.h"
#include <vector>

using namespace std;

//_____________________________________________________________________________
THaTrackingModule::THaTrackingModule() :  fTrk(nullptr)
{
  // Normal constructor.

}

//_____________________________________________________________________________
void THaTrackingModule::TrkIfoClear()
{
  fTrkIfo.Clear();
  fTrk = nullptr;
}

//_____________________________________________________________________________
const vector<RVarDef> THaTrackingModule::GetRVarDef()
{
  // Return definition block of global variables for the fTrkIfo object

  const vector<RVarDef> vars = {
    { .name = "x",        .desc = "Target x coordinate",            .def = "fTrkIfo.fX"      },
    { .name = "y",        .desc = "Target y coordinate",            .def = "fTrkIfo.fY"      },
    { .name = "th",       .desc = "Tangent of target theta angle",  .def = "fTrkIfo.fTheta"  },
    { .name = "ph",       .desc = "Tangent of target phi angle",    .def = "fTrkIfo.fPhi"    },
    { .name = "dp",       .desc = "Target delta",                   .def = "fTrkIfo.fDp"     },
    { .name = "p",        .desc = "Lab momentum (GeV)",             .def = "fTrkIfo.fP"      },
    { .name = "px",       .desc = "Lab momentum x (GeV)",           .def = "fTrkIfo.GetPx()" },
    { .name = "py",       .desc = "Lab momentum y (GeV)",           .def = "fTrkIfo.GetPy()" },
    { .name = "pz",       .desc = "Lab momentum z (GeV)",           .def = "fTrkIfo.GetPz()" },
    { .name = "ok",       .desc = "Data valid status flag (1=ok)",  .def = "fTrkIfo.fOK"     },
  };

  return vars;
}

//_____________________________________________________________________________
#if ROOT_VERSION_CODE < ROOT_VERSION(6,36,0)
ClassImp(THaTrackingModule)
#endif
