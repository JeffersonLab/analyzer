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
#include <memory>

class BankLoc;

// FIXME: why is this a PhysicsModule?
class BankData : public THaPhysicsModule {

public:

  BankData( const char* name, const char* description);
  ~BankData() override;

  Int_t Process( const THaEvData& evdata ) override;

protected:
  Int_t DefineVariables( EMode mode = kDefine ) override;
  Int_t ReadDatabase( const TDatime& date ) override;

private:
  Int_t Nvars;
  Double_t *dvars;  // FIXME: make UInt_t once THaOutput supports integer branches
  UInt_t *vardata;

  std::vector<std::unique_ptr<BankLoc>> banklocs;

  ClassDefOverride(BankData,0)   // process bank data.
};

#endif
