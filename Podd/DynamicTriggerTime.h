#ifndef Podd_DynamicTriggerTime_h_
#define Podd_DynamicTriggerTime_h_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Podd::DynamicTriggerTime                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "THaTriggerTime.h"
#include "TString.h"
#include <set>

class THaFormula;
class THaCut;

namespace Podd {

class DynamicTriggerTime : public THaTriggerTime {
public:
  DynamicTriggerTime( const char* name, const char* description,
                      THaApparatus* app = nullptr );
  DynamicTriggerTime( const char* name, const char* description,
                      const char* expr, const char* cond = "",
                      THaApparatus* app = nullptr );

  virtual Int_t Decode( const THaEvData& evdata );
  virtual Int_t CoarseProcess( TClonesArray& tracks );
  virtual Int_t FineProcess( TClonesArray& tracks );
  virtual void  Clear( Option_t* opt = "" );

protected:
  virtual Int_t DefineVariables( EMode mode = kDefine );
  virtual Int_t ReadDatabase( const TDatime &date );

  struct TimeCorrDef {
    explicit TimeCorrDef( const char* expr, const char* cond = "",
                          UInt_t prio = 0 ) :
      fFormStr(expr), fCondStr(cond), fPrio(prio), fFromDB(true),
      fForm(nullptr), fCond(nullptr) {}
    ~TimeCorrDef();
    bool operator<(const TimeCorrDef& rhs) const { return fPrio < rhs.fPrio; }

    TString  fFormStr;         // Formula string
    TString  fCondStr;         // Condition string
    UInt_t   fPrio;            // Priority of this correction (0=highest)
    Bool_t   fFromDB;          // This definition comes from database, not constructor
    mutable THaFormula* fForm; // Formula for calculating time correction
    mutable THaCut*     fCond; // Condition to pass for this correction to apply (optional)
  };
  std::set<TimeCorrDef> fDefs;

  enum EEvalAt { kDecode, kCoarseProcess, kFineProcess };
  EEvalAt    fEvalAt;          // Processing stage after which this module runs
  TString    fTestBlockName;   // Name of test block for this module
  Bool_t     fDidFormInit;     // Formulas and tests have been initialized

private:
  Int_t InitDefs();
  void  MakeBlockName();
  Int_t CalcCorrection();

  ClassDef( DynamicTriggerTime, 0 )  // Trigger time correction based on formula(s)
};

} // namespace Podd

#endif  /*  Podd_DynamicTriggerTime_h_  */
