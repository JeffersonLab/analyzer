#ifndef ROOT_THaFormula
#define ROOT_THaFormula

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// THaFormula                                                           //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include "TFormula.h"
#include "THaGlobals.h"

class THaVarList;
class THaVar;

class THaFormula : public TFormula {

public:
  static const Option_t* const kPRINTFULL;
  static const Option_t* const kPRINTBRIEF;

  THaFormula() : TFormula(), fCodes(NULL), fIndex(NULL),
		 fNcodes(0), fVarList(NULL), fError(kFALSE), fRegister(kTRUE) {}
  THaFormula( const char* name, const char* formula, 
	      const THaVarList& lst = *gHaVars );
  virtual             ~THaFormula();
  virtual Int_t       Compile( const char* expression="" );
  virtual Double_t    DefinedValue( Int_t i );
  virtual Int_t       DefinedVariable( TString& variable );
  virtual Double_t    Eval();
  virtual Double_t    Eval( Double_t x, Double_t y=0.0, Double_t z=0.0 ) 
                          { return Eval(); }
          Bool_t      IsError() const { return fError; }
  virtual void        Print( Option_t* option="" ) const; // *MENU*
          void        SetList( const THaVarList& lst )  { fVarList = &lst; }

protected:
  enum { kMAXCODES = 100 };            //Max. number of global variables per formula

  const THaVar**    fCodes;            //Global objects referenced in formula
  Int_t*            fIndex;            //Linear index into array variable (0=scalar)
  Int_t             fNcodes;           //Number of global variables referenced in formula
  const THaVarList* fVarList;          //Pointer to list of variables
  Bool_t            fError;            //Flag indicating error in expression
  Bool_t            fRegister;         //If true, register this formula in ROOT's global list

  ClassDef(THaFormula,0)  //Formula defined on list of variables
};

#endif


