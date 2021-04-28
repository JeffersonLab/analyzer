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

class THaDetector;
class THaCut;

namespace HallA {

class TwoarmVDCTimeCorrection : public Podd::TimeCorrectionModule {
public:
  TwoarmVDCTimeCorrection( const char* name, const char* description,
                           const char* scint1, const char* scint2,
                           const char* cond="" );
  virtual ~TwoarmVDCTimeCorrection();

  virtual EStatus Init( const TDatime& run_time );
  virtual Int_t   Process( const THaEvData& );

protected:
  // Configuration
  class DetDef {
  public:
    explicit DetDef( const char* name ) :
      fName(name), fObj(nullptr),
      fNthit(nullptr), fTpad(nullptr), fRT(nullptr), fLT(nullptr),
      fNelem(0) {}
    TString      fName;  // Detector name
    THaDetector* fObj;   // Pointer to detector object
    THaVar       *fNthit, *fTpad, *fRT, *fLT;  // Required global variables
                                               // from this detector
    Int_t        fNelem; // Number of configured elements (pads)
  };
  DetDef  fDet[2];

  TString fTestBlockName;
  TString fCondExpr;
  THaCut* fCond;
  Bool_t  fDidInitDefs;

  Int_t InitDefs();
  void  MakeBlockName();
  virtual Int_t ReadDatabase( const TDatime& date );

  ClassDef(TwoarmVDCTimeCorrection, 0)   //Two-arm VDC time correction
};

} // namespace HallA

#endif

