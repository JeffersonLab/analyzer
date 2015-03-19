#ifndef HALLA_THaFilter
#define HALLA_THaFilter

#include "THaPostProcess.h"
#include "TString.h"
#include "Decoder.h"

class THaCut;
class TString;
class TDatime;
class THaRunBase;

class THaFilter : public THaPostProcess {
 public:
  THaFilter( const char *cutexpr, const char* filename );
  virtual ~THaFilter();

  virtual Int_t Init(const TDatime&);
  virtual Int_t Process( const THaEvData*, const THaRunBase*, Int_t code );
  virtual Int_t Close();

  THaCut* GetCut() const { return fCut; }

 protected:
  TString   fCutExpr;    // Definition of cut to use for filtering events
  TString   fFileName;   // Name of CODA output file
  Decoder::THaCodaFile* fCodaOut; // The CODA output file
  THaCut*   fCut;        // Pointer to cut used for filtering

 public:
  ClassDef(THaFilter,0)
};

#endif

