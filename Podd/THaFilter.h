#ifndef Podd_THaFilter_h_
#define Podd_THaFilter_h_

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
  ~THaFilter() override;

  Int_t Init(const TDatime&) override;
  Int_t Process( const THaEvData*, const THaRunBase*, Int_t code ) override;
  Int_t Close() override;

  THaCut* GetCut() const { return fCut; }

 protected:
  TString   fCutExpr;    // Definition of cut to use for filtering events
  TString   fFileName;   // Name of CODA output file
  Decoder::THaCodaFile* fCodaOut; // The CODA output file
  THaCut*   fCut;        // Pointer to cut used for filtering

 public:
  ClassDefOverride(THaFilter,0)
};

#endif

