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
#include "THaString.h"
#include <vector>
#include <iostream>

using namespace std;

class THaVar;
class THaVarList;
class THaCut;
class THaCutList;
class THaVar;

class THaVform : public THaFormula {

public:

  THaVform() : THaFormula() {}
  THaVform( const char *type, const char* name, const char* formula,
      const THaVarList* vlst=gHaVars, const THaCutList* clst=gHaCuts );
  virtual  ~THaVform();
  THaVform& operator=(const THaVform& vform);

// Over-rides base class DefinedGlobalVariables
  Int_t DefinedGlobalVariable( const TString& variable );
// Self-explanatory printouts
  void  ShortPrint();
  void  LongPrint();
// Must initialize the variables.  They will go to the 'tree'
// if you subsequently call SetOutput();
  Int_t Init();
// Must reattach if pointers to global variables redone
  void ReAttach();
// To explain the error return status from Init() if it's nonzero.
  void ErrPrint(Int_t status);
// Must 'SetOutput' at initialization if output to appear in tree.
// Normally not desired for THaVforms that belong to THaVhist's
  Int_t SetOutput(TTree *tree);
// Must 'Process' once per event before processing the things
// that use this object.
  Int_t Process();
// To get the data (from index of array).  In the case of a 
// cut this will be a 0 or 1 (false or true).
  Double_t GetData(Int_t index = 0);
// This object is either a formula, a variable sized array, a cut, or
// an "eye" ("[I]" variable)
  Bool_t IsFormula() const;
  Bool_t IsVarray() const;
  Bool_t IsCut() const;
  Bool_t IsEye() const;
// Get the size (dimension) of this object 
  Int_t GetSize() { return fObjSize; };
// Get array elements from formula
  std::vector<THaString> GetArrays(); 
  
protected:

  Int_t fNvar, fObjSize;
  Double_t fData;
  THaString fType;
  enum FTy { kCut = 0, kForm, kUnknown };
  enum FAr { kScaler = 0, kAElem, kFAType, kVAType };
  enum FAp { kNoPrefix = 0, kAnd, kOr, kSum };
  enum FEr { kOK = 0, kIllVar, kIllTyp, kNoGlo,
             kIllMix, kArrZer, kArrSiz, kUnkPre, kIllPre};

  static const Int_t fgDebug  = 0;
  static const Int_t fgVFORM_HUGE = 10000;
  THaString fgAndStr, fgOrStr, fgSumStr;

  Int_t Clear();
  Int_t MakeFormula(Int_t flo, Int_t fhi);
  THaString StripPrefix(const char* formula);
  THaString StripBracket(THaString& var) const; 
  void  GetForm(Int_t size);
  void  Create(const THaVform& vf); 
  void  Uncreate();

  std::vector<THaString> fVarName;
  std::vector<Int_t> fVarStat;
  std::vector<THaFormula*> fFormula;
  std::vector<THaCut*> fCut;
  std::vector<THaString> fSarray;
  std::vector<THaString> fVectSform;
  THaString fStitle;
  THaVar   *fVarPtr;
  THaOdata *fOdata;
  Int_t fPrefix;

private:
  THaVform(const THaVform& vform) {}

  ClassDef(THaVform,0)  // Vector of formulas or a var-sized array
};


inline
Bool_t THaVform::IsEye() const {
  return fType.CmpNoCase("eye") == 0;
};

inline
Bool_t THaVform::IsCut() const {
  return fType.CmpNoCase("cut") == 0;
};

inline
Bool_t THaVform::IsFormula() const {
  return fType.CmpNoCase("formula") == 0;
};

inline
Bool_t THaVform::IsVarray() const {
  return fVarPtr != 0 && fType.CmpNoCase("vararray") == 0;
};




#endif


