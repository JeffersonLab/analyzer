#ifndef HALLA_THaPostProcess
#define HALLA_THaPostProcess

//
// abstract class to optionally handle the raw buffer of
// EVERY EVENT (regardless of type) that comes through
// the pipeline/

#include "TObject.h"

class TList;
class THaRun;
class TDatime;

class THaPostProcess : public TObject {
 public:
  THaPostProcess();
  virtual ~THaPostProcess();
  virtual Int_t Init(const TDatime& )=0;
  virtual Int_t Process( const THaRun* run, int force=0 )=0;
  virtual Int_t CleanUp()=0;
 protected:
  Int_t fIsInit;

  ClassDef(THaPostProcess,0)
};

extern TList* gHaPostProcess;

#endif
