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
ClassImp(THaTrackingModule)
