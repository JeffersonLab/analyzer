//*-- Author :    Ole Hansen   27-Apr-04

//////////////////////////////////////////////////////////////////////////
//
// THaBeamModule
//
// Base class for a "beam" processing module. This is a physics module
// that provides generic information about beam properties.
//
//////////////////////////////////////////////////////////////////////////

#include "THaBeamModule.h"
#include "VarDef.h"
#include <vector>

using namespace std;

//_____________________________________________________________________________
const vector<RVarDef> THaBeamModule::GetRVarDef()
{
  // Get definition block of global variables for the fBeamIfo object

  const vector<RVarDef> vars = {
    { .name = "e",    .desc = "Beam energy",                    .def = "fBeamIfo.GetE()"},
    { .name = "p",    .desc = "Lab momentum (GeV)",             .def = "fBeamIfo.GetP()"},
    { .name = "px",   .desc = "Lab momentum x (GeV)",           .def = "fBeamIfo.GetPx()"},
    { .name = "py",   .desc = "Lab momentum y (GeV)",           .def = "fBeamIfo.GetPy()"},
    { .name = "pz",   .desc = "Lab momentum z (GeV)",           .def = "fBeamIfo.GetPz()"},
    { .name = "xpos", .desc = "x position (m)",                 .def = "fBeamIfo.GetX()"},
    { .name = "ypos", .desc = "y position (m)",                 .def = "fBeamIfo.GetY()"},
    { .name = "zpos", .desc = "z position (m)",                 .def = "fBeamIfo.GetZ()"},
    { .name = "th",   .desc = "Tangent theta angle",            .def = "fBeamIfo.GetTheta()"},
    { .name = "ph",   .desc = "Tangent phi angle",              .def = "fBeamIfo.GetPhi()"},
    { .name = "pol",  .desc = "Beam polarization",              .def = "fBeamIfo.fPol"},
    { .name = "ok",   .desc = "Data valid status flag (1=ok)",  .def = "fBeamIfo.fOK"},
  };

  return vars;
}

//_____________________________________________________________________________
ClassImp(THaBeamModule)
