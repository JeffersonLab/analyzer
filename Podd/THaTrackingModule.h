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
  virtual ~THaTrackingModule() = default;
  
  THaTrackInfo*  GetTrackInfo() { return &fTrkIfo; }
  THaTrack*      GetTrack()     { return fTrk; }

  void TrkIfoClear();
  static const RVarDef* GetRVarDef();

protected:

  THaTrackInfo  fTrkIfo;          //  Track information
  THaTrack*     fTrk;             //! Pointer to associated track

  THaTrackingModule( const THaTrackingModule& ) = default;
  THaTrackingModule( THaTrackingModule&& ) = default;
  THaTrackingModule& operator=( const THaTrackingModule& ) = default;
  THaTrackingModule& operator=( THaTrackingModule&& ) = default;

  ClassDef(THaTrackingModule,2)   // ABC for a tracking module

};

#endif
