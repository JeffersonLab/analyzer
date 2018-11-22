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
  TrigBitLoc( const char* nm, Int_t cra, Int_t slo, Int_t cha,
	      UInt_t num, UInt_t lo, UInt_t hi, UInt_t* loc )
    : CrateLocMulti(nm,cra,slo,cha), bitnum(num), cutlo(lo), cuthi(hi),
      bitloc(loc) { }
  TrigBitLoc() : bitnum(0), cutlo(0), cuthi(kMaxUInt), bitloc(0) {}
  virtual ~TrigBitLoc() {}

  virtual void    Load( const THaEvData& evt );
  virtual Int_t   Configure( const TObjArray* params, Int_t start = 0 );
  virtual Int_t   DefineVariables( EMode mode = THaAnalysisObject::kDefine );
  virtual Int_t   GetNparams() const      { return fgThisType->fNparams; }
  virtual const char* GetTypeKey() const  { return fgThisType->fDBkey; };
  virtual Int_t   OptionPtr( void* ptr );
  virtual void    Print( Option_t* opt="" ) const;

protected:
  UInt_t  bitnum;        // Bit number for this variable (0-31)
  UInt_t  cutlo, cuthi;  // TDC cut for detecting valid trigger bit data
  UInt_t* bitloc;        // External bitpattern variable to fill

private:
  static TypeIter_t fgThisType;

  ClassDef(TrigBitLoc,0)
};

#endif
