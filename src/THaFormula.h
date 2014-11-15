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

  THaFormula();// : TFormula(), fNcodes(0), fVarList(0), fCutList(0) {}
  THaFormula( const char* name, const char* formula,
	      const THaVarList* vlst=gHaVars, const THaCutList* clst=gHaCuts );
  THaFormula( const THaFormula& rhs );
  THaFormula& operator=( const THaFormula& rhs );
  virtual             ~THaFormula();
  virtual Int_t       Compile( const char* expression="" );
  virtual char*       DefinedString( Int_t i );
  virtual Double_t    DefinedValue( Int_t i );
#if ROOT_VERSION_CODE >= 262144 // 4.00/00
  virtual Int_t       DefinedVariable( TString& variable, Int_t& action );
#else
  virtual Int_t       DefinedVariable( TString& variable );
#endif
  virtual Int_t       DefinedCut( const TString& variable );
  virtual Int_t       DefinedGlobalVariable( const TString& variable );
  virtual Double_t    Eval();
#if ROOT_VERSION_CODE > 262660 // 4.02/04  Dumb rootcint chokes on ROOT_VERSION macro
  // The ROOT team strikes again - this one is really BAD
  virtual Double_t    Eval( Double_t /*x*/, Double_t /*y*/=0.0,
			    Double_t /*z*/=0.0, Double_t /*t*/=0.0 ) const
    // hack this-pointer to be non-const - courtesy of ROOT team
  { return const_cast<THaFormula*>(this)->Eval(); }
#else
#if ROOT_VERSION_CODE >= 197632 // 3.04/00
  virtual Double_t    Eval( Double_t /*x*/, Double_t /*y*/=0.0,
			    Double_t /*z*/=0.0, Double_t /*t*/=0.0 )
#else
    virtual Double_t    Eval( Double_t /*x*/, Double_t /*y*/=0.0,
			      Double_t /*z*/=0.0 )
#endif
  { return Eval(); }
#endif
          Bool_t      IsArray()   const { return TestBit(kArrayFormula); }
          Bool_t      IsError()   const { return TestBit(kError); }
          Bool_t      IsInvalid() const { return TestBit(kInvalid); }
#if ROOT_VERSION_CODE >= 197895 // 3.05/07
#if ROOT_VERSION_CODE >= 331776 // 5.16/00
  virtual TString     GetExpFormula( Option_t* opt="" ) const;
#else
  virtual TString     GetExpFormula() const;
#endif
#endif
  virtual void        Print( Option_t* option="" ) const; // *MENU*
          void        SetList( const THaVarList* lst )    { fVarList = lst; }
          void        SetCutList( const THaCutList* lst ) { fCutList = lst; }

protected:

  enum EVariableType {  kVariable, kCut, kString };
  enum {
    kError        = BIT(21),   // Compile() failed
    kInvalid      = BIT(22),   // DefinedValue() encountered invalid data
    kArrayFormula = BIT(23)    // Formula has multiple instances
  };

  // struct FVarDef_t;
  // friend struct FVarDef_t;
  struct FVarDef_t {
    EVariableType type;                //Type of variable in the formula
    const void*   code;                //Pointer to the variable
    Int_t         index;               //Linear index into array variable (0=scalar)
    FVarDef_t( EVariableType t, const void* c, Int_t i )
      : type(t), code(c), index(i) {}
  };
  std::vector<FVarDef_t> fVarDef;      //Global variables referenced in formula
  const THaVarList* fVarList;          //Pointer to list of variables
  const THaCutList* fCutList;          //Pointer to list of cuts

  virtual Bool_t IsString( Int_t oper ) const;

  ClassDef(THaFormula,0)  //Formula defined on list of variables
};

#endif


