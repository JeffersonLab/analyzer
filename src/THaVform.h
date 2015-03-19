#ifndef ROOT_THaVform
#define ROOT_THaVform

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// THaVform                                                             //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include "THaFormula.h"
#include "THaOutput.h"
#include "TTree.h"
#include <string>
#include <vector>
#include <iostream>

class THaVar;
class THaVarList;
class THaCut;
class THaCutList;
class THaVar;

class THaVform : public THaFormula {

public:

  THaVform() : THaFormula(), fType(kUnknown), fVarPtr(0), fOdata(0) {}
  THaVform( const char *type, const char* name, const char* formula,
      const THaVarList* vlst=gHaVars, const THaCutList* clst=gHaCuts );
  virtual  ~THaVform();
  THaVform(const THaVform& vform);
  THaVform& operator=(const THaVform& vform);

// Over-rides base class DefinedGlobalVariables
  Int_t DefinedGlobalVariable( TString& variable );
// Self-explanatory printouts
  void  ShortPrint() const;
  void  LongPrint() const;
// Must initialize the variables.  They will go to the 'tree'
// if you subsequently call SetOutput();
  Int_t Init();
// Must reattach if pointers to global variables redone
  void ReAttach();
// To explain the error return status from Init() if it's nonzero.
  void ErrPrint(Int_t status) const;
// Must 'SetOutput' at initialization if output to appear in tree.
// Normally not desired for THaVforms that belong to THaVhist's
  Int_t SetOutput(TTree *tree);
// Must 'Process' once per event before processing the things
// that use this object.
  Int_t Process();
// To get the data (from index of array).  In the case of a
// cut this will be a 0 or 1 (false or true).
  Double_t GetData(Int_t index = 0) const;
// This object is either a formula, a variable sized array, a cut, or
// an "eye" ("[I]" variable)
  Bool_t IsFormula() const { return (fType == kForm); }
  Bool_t IsVarray() const  { return (fVarPtr != NULL && fType == kVarArray); }
  Bool_t IsCut() const     { return (fType == kCut); }
  Bool_t IsEye() const     { return (fType == kEye); }
// Get the size (dimension) of this object
  Int_t GetSize() const { return fObjSize; };
// Get names of variable that are used by this formula.
  std::vector<std::string> GetVars() const;

protected:

  enum FTy { kCut = 0, kForm, kEye, kVarArray, kUnknown };
  enum FAr { kScaler = 0, kAElem, kFAType, kVAType };
  enum FAp { kNoPrefix = 0, kAnd, kOr, kSum };
  enum FEr { kOK = 0, kIllVar, kIllTyp, kNoGlo,
             kIllMix, kArrZer, kArrSiz, kUnkPre, kIllPre};
  Int_t fNvar, fObjSize;
  Double_t fData;
  FTy fType;

  static const Int_t fgDebug  = 0;
  static const Int_t fgVFORM_HUGE = 10000;
  std::string fgAndStr, fgOrStr, fgSumStr;

  Int_t MakeFormula(Int_t flo, Int_t fhi);
  std::string StripPrefix(const char* formula);
  std::string StripBracket(const std::string& var) const;
  void  GetForm(Int_t size);
  void  Create(const THaVform& vf);
  void  Uncreate();

  std::vector<std::string> fVarName;
  std::vector<Int_t> fVarStat;
  std::vector<THaFormula*> fFormula;
  std::vector<THaCut*> fCut;
  std::vector<std::string> fSarray;
  std::vector<std::string> fVectSform;
  std::string   fStitle;
  THaVar   *fVarPtr;
  THaOdata *fOdata;
  Int_t fPrefix;

private:

  ClassDef(THaVform,0)  // Vector of formulas or a var-sized array
};


inline
Double_t THaVform::GetData(Int_t index) const {
  if (IsEye())
    return (Double_t)index;
  if (fOdata == 0)
    return fData;
  return (fObjSize > 1) ? fOdata->Get(index) : fOdata->Get();
}

#endif


