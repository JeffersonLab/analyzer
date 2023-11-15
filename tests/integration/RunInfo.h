#ifndef Podd_IntegratonTests_RunInfo_h_
#define Podd_IntegratonTests_RunInfo_h_

// RunInfo struct stored in ref.root, containing reference values for selected
// items from THaRunBase and THaRunParameters

#include "TDatime.h"

struct RunInfo {
  UInt_t    num;
  TDatime   date;
  UInt_t    nevt;
  Int_t     vers;
  Double_t  beamE;
  Double_t  beamM;
  Int_t     ps[12];
};

#endif
