#ifndef ROOT_BankData
#define ROOT_BankData

//////////////////////////////////////////////////////////////////////////
//
// BankData
//
//////////////////////////////////////////////////////////////////////////

#include "THaPhysicsModule.h"
#include "THaEvData.h"
#include "TTree.h"
#include "TNamed.h"
#include "THaGlobals.h"
#include "TDatime.h"
#include "VarDef.h"
#include <vector>
#include <string>
#include <cstdio>
#include <stdarg.h>

class THaTrackingModule;

class BankLoc { // Utility class used by BankData
 public:
   BankLoc(std::string svar, Int_t iroc, Int_t ibank, Int_t ioff, Int_t inum)
     : svarname(svar), roc(iroc), bank(ibank), offset(ioff), numwords(inum) {}
   ~BankLoc();
   std::string svarname;
   Int_t roc,bank,offset,numwords;
};

class BankData : public THaPhysicsModule {

public:

  BankData( const char* name, const char* description);
  virtual ~BankData();

  virtual Int_t Process( const THaEvData& );

protected:

  virtual Int_t DefineVariables( EMode mode = kDefine );
  virtual Int_t ReadDatabase( const TDatime& date );

private:

  Int_t fDebug;
  Int_t Nvars;
  Double_t *dvars;
  UInt_t *vardata;

  std::vector<BankLoc*> banklocs;

  ClassDef(BankData,0)   // process bank data.
};

#endif
