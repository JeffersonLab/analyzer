//*-- Author :    Ole Hansen   27-Apr-04

//////////////////////////////////////////////////////////////////////////
//
// THaBeamModule
//
// Base class for a "beam" processing module. This is a  
// physics module that provides a generic information about
// beam properties.
//
//////////////////////////////////////////////////////////////////////////

#include "THaBeamModule.h"
#include "VarDef.h"

using namespace std;

//_____________________________________________________________________________
THaBeamModule::THaBeamModule()
{
  // Normal constructor.

}

//_____________________________________________________________________________
THaBeamModule::~THaBeamModule()
{
  // Destructor

}

//_____________________________________________________________________________
const RVarDef* THaBeamModule::GetRVarDef()
{
  // Get definition block of global variables for the fBeamIfo object

  static const RVarDef vars[] = {
    { "e",    "Beam energy",                    "fBeamIfo.GetE()"},
    { "p",    "Lab momentum (GeV)",             "fBeamIfo.GetP()"},
    { "px",   "Lab momentum x (GeV)",           "fBeamIfo.GetPx()"},
    { "py",   "Lab momentum y (GeV)",           "fBeamIfo.GetPy()"},
    { "pz",   "Lab momentum z (GeV)",           "fBeamIfo.GetPz()"},
    { "xpos", "x position (m)",                 "fBeamIfo.GetX()"},
    { "ypos", "y position (m)",                 "fBeamIfo.GetY()"},
    { "zpos", "z position (m)",                 "fBeamIfo.GetZ()"},
    { "th",   "Tangent theta angle",            "fBeamIfo.GetTheta()"},
    { "ph",   "Tangent phi angle",              "fBeamIfo.GetPhi()"},    
    { "pol",  "Beam polarization",              "fBeamIfo.fPol"},
    { "ok",   "Data valid status flag (1=ok)",  "fBeamIfo.fOK"},
    { 0 }
  };

  return vars;
}


//_____________________________________________________________________________
ClassImp(THaBeamModule)
