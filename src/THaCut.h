#ifndef ROOT_THaCut
#define ROOT_THaCut

//////////////////////////////////////////////////////////////////////////
//
// THaCut
//
//////////////////////////////////////////////////////////////////////////

#include "THaFormula.h"

class THaCut : public THaFormula {

public:
  THaCut();
  THaCut( const char* name, const char* expression, const char* block,
	  const THaVarList* vlst = gHaVars, const THaCutList* clst = gHaCuts );
  THaCut( const THaCut& rhs );
  THaCut& operator=( const THaCut& rhs );
  virtual ~THaCut();

  using THaFormula::Eval;

  enum EvalMode { kModeErr = -1, kAND, kOR, kXOR };

          void         ClearResult()        { fLastResult = kFALSE; }
  // Requires ROOT >= 4.00/00
  virtual Int_t        DefinedVariable( TString& variable, Int_t& action );
  virtual Double_t     Eval();
  // For backward compatibility
          Bool_t       EvalCut()            { Eval(); return fLastResult; }
          const char*  GetBlockname() const { return fBlockname.Data(); }
          EvalMode     GetMode()      const { return fMode; }
          UInt_t       GetNCalled()   const { return fNCalled; }
          UInt_t       GetNPassed()   const { return fNPassed; }
          Bool_t       GetResult()    const { return fLastResult; }
  virtual Bool_t       IsArray()      const { return kFALSE; }
  virtual Bool_t       IsVarArray()   const { return kFALSE; }
  virtual void         Print( Option_t *opt="" ) const;
  virtual void         Reset();
  virtual void         SetBlockname( const Text_t* name );
  virtual void         SetName( const Text_t* name );
  virtual void         SetNameTitle( const Text_t* name, const Text_t* title );

protected:
  Bool_t      fLastResult;  // Result of last evaluation of this formula
  TString     fBlockname;   // Name of block this cut belongs to
  UInt_t      fNCalled;     // Number of times this cut has been evaluated
  UInt_t      fNPassed;     // Number of times this cut was true when evaluated
  EvalMode    fMode;        // Evaluation mode of array expressions (AND/OR etc)

  Bool_t      EvalElement( Int_t instance );
  EvalMode    ParsePrefix( TString& expr );

  ClassDef(THaCut,0)   // A logical cut (a.k.a. test)
};

#endif
