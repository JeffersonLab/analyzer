#ifndef HALLA_THaFilter
#define HALLA_THaFilter

#include "THaPostProcess.h"
#include "TString.h"

class THaCodaFile;
class THaCut;
class TString;
class TDatime;
class THaRunBase;

class THaFilter : public THaPostProcess {
 public:
  THaFilter( const char *cutname, const char* filename );
  virtual ~THaFilter();
  
  virtual Int_t Init(const TDatime&);
  virtual Int_t Process( const THaEvData*, const THaRunBase*, Int_t code );
  virtual Int_t Close();

 protected:
  TString   fCutName;
  TString   fFileName;
  THaCodaFile* fCodaOut;
  THaCut*   fCut;
  
 public:
  ClassDef(THaFilter,0)
};

#endif

