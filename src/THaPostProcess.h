#ifndef Podd_THaPostProcess_h_
#define Podd_THaPostProcess_h_

#include "TObject.h"

class THaRunBase;
class THaEvData;
class TDatime;
class TList;

class THaPostProcess : public TObject {
 public:
  THaPostProcess();
  virtual ~THaPostProcess();
  virtual Int_t Init(const TDatime& )=0;
  virtual Int_t Process( const THaEvData*, const THaRunBase*, Int_t code )=0;
  virtual Int_t Close()=0;

  enum { kUseReturnCode = BIT(23) };

protected:
  Int_t fIsInit;

  static TList* fgModules; // List of all current PostProcess modules

  ClassDef(THaPostProcess,0)
};

#endif
