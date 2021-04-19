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
  virtual ~TimeCorrectionModule();

  Double_t        TimeOffset() const { return fEvtTime; }

  virtual void    Clear( Option_t* opt="" );
  virtual Int_t   Process( const THaEvData& ) = 0;

protected:
  // Configuration
  Double_t  fGlOffset;   // Fixed overall time offset (s)

  // Event-by-event data
  Double_t  fEvtTime;    // Time offset for this event (s)

  virtual Int_t DefineVariables( EMode mode = kDefine );
  virtual Int_t ReadDatabase( const TDatime& date );

  ClassDef(TimeCorrectionModule, 0)   //Generic VDC time correction
};

} // namespace HallA

#endif

