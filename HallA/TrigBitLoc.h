#ifndef Podd_TrigBitLoc_h_
#define Podd_TrigBitLoc_h_

//////////////////////////////////////////////////////////////////////////
//
// TrigBitLoc
//
// Plugin for DecData to process Hall A trigger bits
//
//////////////////////////////////////////////////////////////////////////

#include "BdataLoc.h"

//___________________________________________________________________________
class TrigBitLoc : public CrateLocMulti {
public:
  // (crate,slot,channel) multihit TDC for Hall A-style trigger bits
  TrigBitLoc( const char* nm, UInt_t cra, UInt_t slo, UInt_t cha,
              UInt_t num, UInt_t lo, UInt_t hi, UInt_t* loc )
    : CrateLocMulti(nm,cra,slo,cha), bitnum(num), cutlo(lo), cuthi(hi),
      bitloc(loc) { }
  TrigBitLoc() : bitnum(0), cutlo(0), cuthi(kMaxUInt), bitloc(nullptr) {}

  void    Load( const THaEvData& evt ) override;
  Int_t   Configure( const TObjArray* params, Int_t start = 0 ) override;
  Int_t   DefineVariables( EMode mode = THaAnalysisObject::kDefine ) override;
  Int_t   GetNparams() const override        { return fgThisType->fNparams; }
  const char* GetTypeKey() const override    { return fgThisType->fDBkey; };
  Int_t   OptionPtr( void* ptr ) override;
  void    Print( Option_t* opt="" ) const override;

protected:
  UInt_t  bitnum;        // Bit number for this variable (0-31)
  UInt_t  cutlo, cuthi;  // TDC cut for detecting valid trigger bit data
  UInt_t* bitloc;        // External bitpattern variable to fill

private:
  static TypeIter_t fgThisType;

  ClassDefOverride(TrigBitLoc,0)
};

#endif
