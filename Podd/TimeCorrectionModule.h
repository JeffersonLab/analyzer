#ifndef Podd_TimeCorrectionModule_h_
#define Podd_TimeCorrectionModule_h_

//////////////////////////////////////////////////////////////////////////
//
// HallA::TimeCorrectionModule
//
// Calculates a trigger time correction for the VDC
//
//////////////////////////////////////////////////////////////////////////

#include "InterStageModule.h"
#include "TString.h"
#include "THaVar.h"

class THaScintillator;

namespace Podd {

class TimeCorrectionModule : public InterStageModule {
public:
  TimeCorrectionModule( const char* name, const char* description,
                        Int_t stage );
  ~TimeCorrectionModule() override;

  void            Clear( Option_t* opt="" ) override;

  Double_t        TimeOffset() const { return fEvtTime; }

protected:
  // Configuration
  Double_t  fGlOffset;   // Fixed overall time offset (s)

  // Event-by-event data
  Double_t  fEvtTime;    // Time offset for this event (s)

  Int_t DefineVariables( EMode mode = kDefine ) override;
  Int_t ReadDatabase( const TDatime& date ) override;

  ClassDefOverride(TimeCorrectionModule, 0)   //Generic VDC time correction
};

} // namespace HallA

#endif

