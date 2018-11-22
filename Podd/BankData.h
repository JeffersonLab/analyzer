#ifndef ROOT_BankData
#define ROOT_BankData

//////////////////////////////////////////////////////////////////////////
//
// BankData
//
//////////////////////////////////////////////////////////////////////////

#include "THaPhysicsModule.h"
#include "TDatime.h"
#include <vector>

class BankLoc;

// FIXME: why is this a PhysicsModule?
class BankData : public THaPhysicsModule {

public:

  BankData( const char* name, const char* description);
  virtual ~BankData();

  virtual Int_t Process( const THaEvData& evdata );

protected:

  virtual Int_t DefineVariables( EMode mode = kDefine );
  virtual Int_t ReadDatabase( const TDatime& date );

private:

  Int_t fDebug;
  Int_t Nvars;
  Double_t *dvars;  // FIXME: make UInt_t once THaOutput supports integer branches
  UInt_t *vardata;

  std::vector<BankLoc*> banklocs;

  ClassDef(BankData,0)   // process bank data.
};

#endif
