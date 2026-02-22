#ifndef Podd_THaFormula_h_
#define Podd_THaFormula_h_

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// THaFormula                                                           //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

#include "RVersion.h"
#include "v5/TFormula.h"
#include <vector>
#include <iostream>

class THaVarList;
class THaCutList;
class THaVar;


class THaFormula : public ROOT::v5::TFormula {

public:
  static const Option_t* const kPRINTFULL;
  static const Option_t* const kPRINTBRIEF;

  THaFormula();
  THaFormula( const char* name, const char* formula, Bool_t do_register = true,
              const THaVarList* vlst = nullptr, const THaCutList* clst = nullptr );
  THaFormula( const THaFormula& rhs );
  THaFormula& operator=( const THaFormula& rhs );
  ~THaFormula() override;

          Int_t       Compile( const char* expression="" ) override;
          char*       DefinedString( Int_t i ) override;
          Double_t    DefinedValue( Int_t i ) override;
          Int_t       DefinedVariable( TString& variable, Int_t& action ) override;
  virtual Int_t       DefinedCut( TString& variable );
  virtual Int_t       DefinedGlobalVariable( TString& variable );
  virtual Int_t       DefinedGlobalVariableExtraList( TString& variable, const THaVarList* extralist );
  virtual Int_t       DefinedSpecialFunction( TString& name );
  virtual Double_t    Eval();
  // Requires ROOT >= 4.02/04
          Double_t    Eval( Double_t /*x*/, Double_t /*y*/=0.0,
                            Double_t /*z*/=0.0, Double_t /*t*/=0.0 ) const override
  // need to hack this-pointer to be non-const - courtesy of ROOT team
  { return const_cast<THaFormula*>(this)->Eval(); }
  virtual Double_t    EvalInstance( Int_t instance );
  virtual Int_t       GetNdata()   const;
  virtual Bool_t      IsArray()    const { return TestBit(kArrayFormula); }
  virtual Bool_t      IsVarArray() const { return TestBit(kVarArray); }
          Bool_t      IsError()    const { return TestBit(kError); }
          Bool_t      IsInvalid()  const { return TestBit(kInvalid); }
          void        Print( Option_t* option="" ) const override;
          void        SetList( const THaVarList* lst )    { fVarList = lst; }
          void        SetCutList( const THaCutList* lst ) { fCutList = lst; }

protected:

  enum {
    kError          = BIT(0),  // Compile() failed
    kInvalid        = BIT(1),  // DefinedValue() encountered invalid data
    kVarArray       = BIT(2),  // Formula contains a variable-size array
    kArrayFormula   = BIT(3),  // Formula has multiple instances
    kFuncOfVarArray = BIT(4)   // Formula contains function of var-size array
  };

  enum EVariableType {  kVariable, kCut, kString, kArray,
			kFunction, kFormula, kVarFormula,
                        kCutScaler, kCutNCalled }; // These for hcana

  virtual Int_t       DefinedCutWithType( TString& variable, EVariableType type );
  class FVarDef_t {
  public:
    EVariableType type;                //Type of variable in the formula
    void*         obj;                 //Pointer to the respective object
    Int_t         index;               //Linear index into array, if fixed-size
    FVarDef_t( EVariableType t, void* p, Int_t i ) : type(t), obj(p), index(i) {}
    FVarDef_t( const FVarDef_t& rhs );
    FVarDef_t& operator=( const FVarDef_t& rhs );
    FVarDef_t( FVarDef_t&& rhs ) noexcept;
    FVarDef_t& operator=( FVarDef_t&& rhs ) noexcept;
    ~FVarDef_t();
  };
  std::vector<FVarDef_t> fVarDef;      //Global variables referenced in formula
  const THaVarList* fVarList;          //Pointer to list of variables
  const THaCutList* fCutList;          //Pointer to list of cuts
  Int_t             fInstance;         //Current instance to evaluate

          Double_t  EvalInstanceUnchecked( Int_t instance );
          Int_t     GetNdataUnchecked() const;
          Int_t     Init( const char* name, const char* expression );
          Bool_t    IsString( Int_t oper ) const override;
  virtual void      RegisterFormula( Bool_t add = true );

  ClassDefOverride(THaFormula,0)  //Formula defined on list of variables
};

//__________________inlines____________________________________________________
inline
Double_t THaFormula::EvalInstanceUnchecked( Int_t instance )
{
  fInstance = instance;
  if( fNoper == 1 && fVarDef.size() == 1 )
    return DefinedValue(0);
  else
    return EvalPar(nullptr);
}

#endif
