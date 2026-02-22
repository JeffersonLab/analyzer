#ifndef Podd_THaCut_h_
#define Podd_THaCut_h_

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
	  const THaVarList* vlst=nullptr, const THaCutList* clst=nullptr );

  using THaFormula::Eval;

  enum EvalMode { kModeErr = -1, kAND, kOR, kXOR };

          void         ClearResult()        { fLastResult = false; }
          Int_t        DefinedVariable( TString& variable, Int_t& action ) override;
          Double_t     Eval() override;
  // For backward compatibility
          Bool_t       EvalCut()            { Eval(); return fLastResult; }
          const char*  GetBlockname() const { return fBlockname.Data(); }
          EvalMode     GetMode()      const { return fMode; }
          UInt_t       GetNCalled()   const { return fNCalled; }
          UInt_t       GetNPassed()   const { return fNPassed; }
          Bool_t       GetResult()    const { return fLastResult; }
          Bool_t       IsArray()      const override { return false; }
          Bool_t       IsVarArray()   const override { return false; }
          void         Print( Option_t *opt="" ) const override;
  virtual void         Reset();
  virtual void         SetBlockname( const Text_t* name );
          void         SetName( const Text_t* name ) override;
          void         SetNameTitle( const Text_t* name, const Text_t* title ) override;

protected:
  Bool_t      fLastResult;  // Result of last evaluation of this formula
  TString     fBlockname;   // Name of block this cut belongs to
  UInt_t      fNCalled;     // Number of times this cut has been evaluated
  UInt_t      fNPassed;     // Number of times this cut was true when evaluated
  EvalMode    fMode;        // Evaluation mode of array expressions (AND/OR etc.)

  Bool_t      EvalElement( Int_t instance );
  EvalMode    ParsePrefix( TString& expr );

  ClassDefOverride(THaCut,0)   // A logical cut (a.k.a. test)
};

#endif
