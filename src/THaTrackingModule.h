#ifndef ROOT_THaTrackingModule
#define ROOT_THaTrackingModule

//////////////////////////////////////////////////////////////////////////
//
// THaTrackingModule
//
//////////////////////////////////////////////////////////////////////////

#include "THaTrackInfo.h"

class THaTrackingModule {
  
public:
  virtual ~THaTrackingModule();
  
  THaTrackInfo*  GetTrackInfo()  { return &fTrkIfo; }
  THaTrack*      GetTrack()      { return fTrk; }

  void TrkIfoClear();

protected:

  THaTrackInfo  fTrkIfo;          // Track information
  THaTrack*     fTrk;             // Pointer to associated track

  THaTrackingModule();

  ClassDef(THaTrackingModule,0)   //ABC for a vertex module

};

#endif
