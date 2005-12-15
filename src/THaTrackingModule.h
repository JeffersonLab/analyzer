#ifndef ROOT_THaTrackingModule
#define ROOT_THaTrackingModule

//////////////////////////////////////////////////////////////////////////
//
// THaTrackingModule
//
//////////////////////////////////////////////////////////////////////////

#include "THaTrackInfo.h"
#include "THaTrack.h"
#include <TRef.h>

class THaTrackingModule {
  
public:
  virtual ~THaTrackingModule();
  
  THaTrackInfo*  GetTrackInfo()  { return &fTrkIfo; }
  THaTrack*      GetTrack()      { return static_cast<THaTrack*>(fTrk.GetObject()); }

  void TrkIfoClear();

protected:

  THaTrackInfo  fTrkIfo;          // Track information
  TRef          fTrk;             // Pointer to associated track

  THaTrackingModule();

  ClassDef(THaTrackingModule,1)   // ABC for a tracking module

};

#endif
