#ifndef HALLA_THaFilter
#define HALLA_THaFilter

// a class to handle event filtering
// Each filter has an associated global 'cut' variable
// to test against

class THaIORun;
class TList;
class THaCut;
class TString;
class TDatime;

#include "TString.h"
#include "THaPostProcess.h"

class THaFilter : public THaPostProcess {
 public:
  THaFilter( const char *cutname, const char* filename );
  virtual ~THaFilter() { CleanUp(); }
  
  Int_t Init(const TDatime&);
  Int_t Process( const THaRun* run, int force=0 );
  Int_t CleanUp();

 protected:
  TString fCutName;
  TString fFileName;
  THaIORun* fRun;
  int fRunStat;
  THaCut*  fCut;
  int fCutStat;
  
 public:
  ClassDef(THaFilter,0)
};

#endif

