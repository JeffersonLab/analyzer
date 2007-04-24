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

using namespace std;

//_____________________________________________________________________________
THaTrackingModule::THaTrackingModule() :  fTrk(NULL)
{
  // Normal constructor.

}

//_____________________________________________________________________________
THaTrackingModule::~THaTrackingModule()
{
  // Destructor

}

//_____________________________________________________________________________
void THaTrackingModule::TrkIfoClear()
{
  fTrkIfo.Clear();
  fTrk = NULL;
}

//_____________________________________________________________________________
const RVarDef* THaTrackingModule::GetRVarDef()
{
  // Return definition block of global variables for the fTrkIfo object

  static const RVarDef vars[] = {
    { "x",        "Target x coordinate",            "fTrkIfo.fX"},
    { "y",        "Target y coordinate",            "fTrkIfo.fY"},
    { "th",       "Tangent of target theta angle",  "fTrkIfo.fTheta"},
    { "ph",       "Tangent of target phi angle",    "fTrkIfo.fPhi"},    
    { "dp",       "Target delta",                   "fTrkIfo.fDp"},
    { "p",        "Lab momentum (GeV)",             "fTrkIfo.fP"},
    { "px",       "Lab momentum x (GeV)",           "fTrkIfo.GetPx()"},
    { "py",       "Lab momentum y (GeV)",           "fTrkIfo.GetPy()"},
    { "pz",       "Lab momentum z (GeV)",           "fTrkIfo.GetPz()"},
    { "ok",       "Data valid status flag (1=ok)",  "fTrkIfo.fOK"},
    { 0 }
  };

  return vars;
}

//_____________________________________________________________________________
ClassImp(THaTrackingModule)
