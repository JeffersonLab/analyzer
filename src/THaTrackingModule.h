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
  THaTrackingModule(); // needed public for ROOT I/O
  virtual ~THaTrackingModule();
  
  THaTrackInfo*  GetTrackInfo()  { return &fTrkIfo; }
  THaTrack*      GetTrack()      { return static_cast<THaTrack*>(fTrk.GetObject()); }

  void TrkIfoClear();

protected:

  THaTrackInfo  fTrkIfo;          // Track information
  TRef          fTrk;             // Pointer to associated track

  ClassDef(THaTrackingModule,1)   // ABC for a tracking module

};

#endif
