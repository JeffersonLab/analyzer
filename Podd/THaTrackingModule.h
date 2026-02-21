#ifndef Podd_THaTrackingModule_h_
#define Podd_THaTrackingModule_h_

//////////////////////////////////////////////////////////////////////////
//
// THaTrackingModule
//
//////////////////////////////////////////////////////////////////////////

#include "THaTrackInfo.h"

class THaTrack;
struct RVarDef;

class THaTrackingModule {
  
public:
  THaTrackingModule(); // needed public for ROOT I/O

  THaTrackInfo*  GetTrackInfo() { return &fTrkIfo; }
  THaTrack*      GetTrack()     { return fTrk; }

  void TrkIfoClear();
  static const std::vector<RVarDef> GetRVarDef();

protected:

  THaTrackInfo  fTrkIfo;          //  Track information
  THaTrack*     fTrk;             //! Pointer to associated track

  ClassDefNV(THaTrackingModule,2)   // ABC for a tracking module

};

#endif
