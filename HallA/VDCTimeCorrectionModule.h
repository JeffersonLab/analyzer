#ifndef HallA_VDCTimeCorrectionModule_h_
#define HallA_VDCTimeCorrectionModule_h_

//////////////////////////////////////////////////////////////////////////
//
// HallA::VDCTimeCorrectionModule
//
// Calculates a trigger time correction for the VDC
//
//////////////////////////////////////////////////////////////////////////

#include "InterStageModule.h"
#include "TString.h"
#include "THaVar.h"

class THaScintillator;

namespace HallA {

class VDCTimeCorrectionModule : public Podd::InterStageModule {
public:
  VDCTimeCorrectionModule( const char* name, const char* description,
                           Int_t stage );
  virtual ~VDCTimeCorrectionModule();

  Double_t        GetTimeOffset() const { return fEvtTime; }

  virtual void    Clear( Option_t* opt="" );
  virtual Int_t   Process( const THaEvData& ) = 0;

protected:
  // Configuration
  Double_t  fGlOffset;   // Fixed overall time offset (s)

  // Event-by-event data
  Double_t  fEvtTime;    // Time offset for this event (s)

protected:
  virtual Int_t DefineVariables( EMode mode = kDefine );
  virtual Int_t ReadDatabase( const TDatime& date );

  ClassDef(VDCTimeCorrectionModule, 0)   //Generic VDC time correction
};

} // namespace HallA

#endif

