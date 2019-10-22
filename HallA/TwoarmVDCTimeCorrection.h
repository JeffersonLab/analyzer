#ifndef HallA_TwoarmVDCTimeCorrection_h_
#define HallA_TwoarmVDCTimeCorrection_h_

//////////////////////////////////////////////////////////////////////////
//
// HallA::TwoarmVDCTimeCorrection
//
// Calculates a time correction for the VDC from the time difference
// between two scintillators.
// Runs after Decode.
//
//////////////////////////////////////////////////////////////////////////

#include "TimeCorrectionModule.h"
#include "TString.h"
#include "THaVar.h"

class THaScintillator;

namespace HallA {

class TwoarmVDCTimeCorrection : public Podd::TimeCorrectionModule {
public:
  TwoarmVDCTimeCorrection( const char* name, const char* description,
                           const char* scint1, const char* scint2 );

  virtual EStatus Init( const TDatime& run_time );
  virtual Int_t   Process( const THaEvData& );

protected:
  // Configuration
  TString fName1;      // Name of scintillator 1
  TString fName2;      // Name of scintillator 2
  Int_t   fNpads1;     // Number of pads in scintillator 1
  Int_t   fNpads2;     // Number of pads in scintillator 2
  THaVar *fRT1, *fLT1, *fNhit1, *fTPad1, *fRT2, *fLT2, *fNhit2, *fTPad2;

  ClassDef(TwoarmVDCTimeCorrection, 0)   //Two-arm VDC time correction
};

} // namespace HallA

#endif

