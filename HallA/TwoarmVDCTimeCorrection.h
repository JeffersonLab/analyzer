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

#include "InterStageModule.h"
#include "TString.h"

class THaDetector;

namespace HallA {

class TwoarmVDCTimeCorrection : public Podd::InterStageModule {
public:
  TwoarmVDCTimeCorrection( const char* name, const char* description,
                           const char* det1, const char* det2 );


  virtual void  Clear( Option_t* opt="" );
  virtual Int_t Process( const THaEvData& data );

protected:
  TString        fName1;    // Name of detector 1
  TString        fName2;    // Name of detector 2
  THaDetector*   fDet1;     // Pointer to detector 1
  THaDetector*   fDet2;     // Pointer to detector 2

  virtual Int_t DefineVariables( EMode mode = kDefine );
  virtual Int_t ReadDatabase( const TDatime& date );

  ClassDef(TwoarmVDCTimeCorrection, 0)   //Two-arm VDC time correction
};

} // namespace HallA

#endif

