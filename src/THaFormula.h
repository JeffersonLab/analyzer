#ifndef ROOT_THaFormula
#define ROOT_THaFormula

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// THaFormula                                                           //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include "TFormula.h"
#include "THaGlobals.h"
#include "RVersion.h"
#include <vector>

class THaVarList;
class THaCutList;
class THaVar;

class THaFormula : public TFormula {

public:
  static const Option_t* const kPRINTFULL;
  static const Option_t* const kPRINTBRIEF;

  THaFormula();
  THaFormula( const char* name, const char* formula,
	      const THaVarList* vlst=gHaVars, const THaCutList* clst=gHaCuts );
  THaFormula( const THaFormula& rhs );
  THaFormula& operator=( const THaFormula& rhs );
  virtual ~THaFormula();

  virtual Int_t       Compile( const char* expression="" );
  virtual char*       DefinedString( Int_t i );
  virtual Double_t    DefinedValue( Int_t i );
  // Requires ROOT >= 4.00/00
  virtual Int_t       DefinedVariable( TString& variable, Int_t& action );
  virtual Int_t       DefinedCut( const TString& variable );
  virtual Int_t       DefinedGlobalVariable( const TString& variable );
  virtual Double_t    Eval();
  // Requires ROOT >= 4.02/04
  virtual Double_t    Eval( Double_t /*x*/, Double_t /*y*/=0.0,
			    Double_t /*z*/=0.0, Double_t /*t*/=0.0 ) const
  // need to hack this-pointer to be non-const - courtesy of ROOT team
  { return const_cast<THaFormula*>(this)->Eval(); }
  virtual Double_t    EvalInstance( Int_t instance );
  virtual Int_t       GetNdata();
          Bool_t      IsArray()   const { return TestBit(kArrayFormula); }
          Bool_t      IsError()   const { return TestBit(kError); }
          Bool_t      IsInvalid() const { return TestBit(kInvalid); }
  virtual void        Print( Option_t* option="" ) const;
          void        SetList( const THaVarList* lst )    { fVarList = lst; }
          void        SetCutList( const THaCutList* lst ) { fCutList = lst; }

protected:

  enum {
    kError        = BIT(21),   // Compile() failed
    kInvalid      = BIT(22),   // DefinedValue() encountered invalid data
    kArrayFormula = BIT(23)    // Formula has multiple instances
  };

  enum EVariableType {  kVariable, kCut, kString, kArray };

  struct FVarDef_t {
    EVariableType type;                //Type of variable in the formula
    const void*   code;                //Pointer to the variable
    Int_t         index;               //Linear index into array, if fixed-size
    FVarDef_t( EVariableType t, const void* c, Int_t i )
      : type(t), code(c), index(i) {}
  };
  std::vector<FVarDef_t> fVarDef;      //Global variables referenced in formula
  const THaVarList* fVarList;          //Pointer to list of variables
  const THaCutList* fCutList;          //Pointer to list of cuts
  Int_t             fInstance;         //Current instance to evaluate

          Int_t  Init( const char* name, const char* expression );
  virtual Bool_t IsString( Int_t oper ) const;

  ClassDef(THaFormula,0)  //Formula defined on list of variables
};

#endif


