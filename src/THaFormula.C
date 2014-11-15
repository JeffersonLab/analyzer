//*-- Author :    Ole Hansen    20/04/2000

//////////////////////////////////////////////////////////////////////////
//
// THaFormula
//
// A formula that is aware of the analyzer's global variables, similar
// to TFormula;s parameters. Unlike TFormula, an arbitrary number of
// global variables may be referenced in a THaFormula.
//
// THaFormulas containing arrays are arrays themselves. Each element
// (instance) of such an array formula may be evaluated separately.
//
//////////////////////////////////////////////////////////////////////////

#include "THaFormula.h"
#include "THaArrayString.h"
#include "THaVarList.h"
#include "THaCutList.h"
#include "THaCut.h"
#include "TROOT.h"
#include "TError.h"

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cassert>

using namespace std;

const Option_t* const THaFormula::kPRINTFULL  = "FULL";
const Option_t* const THaFormula::kPRINTBRIEF = "BRIEF";

static const Double_t kBig = 1e38; // Error value

//_____________________________________________________________________________
THaFormula::THaFormula() : TFormula(), fVarList(0), fCutList(0)
{
  // Default constructor

  ResetBit(kNotGlobal);
  ResetBit(kError);
  ResetBit(kInvalid);
  ResetBit(kArrayFormula);
}

//_____________________________________________________________________________
THaFormula::THaFormula( const char* name, const char* expression,
			const THaVarList* vlst, const THaCutList* clst )
  : TFormula(), fVarList(vlst), fCutList(clst)
{
  // Create a formula 'expression' with name 'name' and symbolic variables
  // from the list 'lst'.

  // We have to duplicate the TFormula constructor code here because of
  // the call to Compile(). Not only do we have our own Compile(), but
  // also out own DefinedVariable() etc. virtual functions. A disturbing
  // design error of the ROOT base class indeed.

  SetName(name);

  ResetBit(kNotGlobal);
  ResetBit(kError);
  ResetBit(kInvalid);
  ResetBit(kArrayFormula);

  //eliminate blanks in expression
  Int_t nch = strlen(expression);
  char *expr = new char[nch+1];
  Int_t j = 0;
  for (Int_t i=0;i<nch;i++) {
     if (expression[i] == ' ') continue;
     if (i > 0 && (expression[i] == '*') && (expression[i-1] == '*')) {
        expr[j-1] = '^';
        continue;
     }
     expr[j] = expression[i]; j++;
   }
  expr[j] = 0;
  if (j) SetTitle(expr);
  delete [] expr;

  Compile();   // This calls our own Compile()
}

//_____________________________________________________________________________
THaFormula::THaFormula( const THaFormula& rhs ) :
  TFormula(rhs), fVarList(rhs.fVarList), fCutList(rhs.fCutList)
{
  // Copy ctor
}

//_____________________________________________________________________________
THaFormula& THaFormula::operator=( const THaFormula& rhs )
{
  if( this != &rhs ) {
    TFormula::operator=(rhs);
    fVarList = rhs.fVarList;
    fCutList = rhs.fCutList;
  }
  return *this;
}

//_____________________________________________________________________________
Int_t THaFormula::Compile( const char* expression )
{
  // Parse the given expression, or, if empty, parse the title.
  // Return 0 on success, 1 if error in expression.

  fNval = 0;
  fAlreadyFound.ResetAllBits(); // Seems to be missing in ROOT
  fVarDef.clear();
  ResetBit(kArrayFormula);

  Int_t status = TFormula::Compile( expression );

  //*-*- Store formula in linked list of formula in ROOT

  SetBit( kError, (status != 0));
  if( status != 0 ) {
    if( !TestBit(kNotGlobal) ) gROOT->GetListOfFunctions()->Remove(this);
  } else {
    assert( fNval+fNstring == (Int_t)fVarDef.size() );
    assert( fNstring >= 0 && fNval >= 0 );
    if( !TestBit(kNotGlobal) ) {
      TObject* old = gROOT->GetListOfFunctions()->FindObject(GetName());
      if (old) gROOT->GetListOfFunctions()->Remove(old);
      gROOT->GetListOfFunctions()->Add(this);
    }
    // If the formula is good, then fix the variable counters that TFormula
    // may have messed with when reverting lone kDefinedString variables to
    // kDefinedVariable. If there is a mix of strings and values,
    // we force the variable counters to the full number of defined variables,
    // so that the loops in EvalPar calculate all the values. This inefficient,
    // but the best we can do with the implementation of TFormula.
    if( fNstring > 0 && fNval > 0 )
      fNval = fNstring = fVarDef.size();
  }
  return status;
}

//_____________________________________________________________________________
THaFormula::~THaFormula()
{
  // Destructor

}

//_____________________________________________________________________________
char* THaFormula::DefinedString( Int_t i )
{
  // Get pointer to the i-th string variable. If the variable is not
  // a string, return pointer to an empty string.

  assert( i>=0 && i<(Int_t)fVarDef.size() );
  FVarDef_t& def = fVarDef[i];
  if( def.type == kString ) {
    const THaVar* pvar = static_cast<const THaVar*>( def.code );
    char** ppc = (char**)pvar->GetValuePointer(); //truly gruesome cast
    return *ppc;
  }
  return (char*)"";
}

//_____________________________________________________________________________
Bool_t THaFormula::IsString(Int_t oper) const
{
  Int_t action = GetAction(oper);
  return ( action == kStringConst || action == kDefinedString );
}

//_____________________________________________________________________________
Double_t THaFormula::DefinedValue( Int_t i )
{
  // Get value of i-th variable in the formula
  // If the i-th variable is a cut, return its last result
  // (calculated at the last evaluation).
  // If the variable is a string, return value of its character value

  assert( i>=0 && i<(Int_t)fVarDef.size() );

  if( IsInvalid() )
    return 1.0;

  FVarDef_t& def = fVarDef[i];
  switch( def.type ) {
  case kVariable:
  case kString:
    {
      const THaVar* var = static_cast<const THaVar*>(def.code);
      assert(var);
      if( def.index >= var->GetLen() ) {
	SetBit(kInvalid);
	return 1.0; // safer than kBig to prevent overflow
      }
      return var->GetValue( def.index );
    }
    break;
  case kCut:
    {
      const THaCut* cut = static_cast<const THaCut*>(def.code);
      assert(cut);
      return cut->GetResult();
      break;
    }
  }
  assert(false); // not reached
  return kBig;
}

//_____________________________________________________________________________
#if ROOT_VERSION_CODE >= ROOT_VERSION(4,0,0)
Int_t THaFormula::DefinedVariable(TString& name, Int_t& action)
#else
Int_t THaFormula::DefinedVariable(TString& name)
#endif
{
  // Check if name is in the list of global objects

  //  This member function redefines the function in TFormula.
  //  A THaFormula may contain more than one variable.
  //  For each variable referenced, a pointer to the corresponding
  //  global object is stored
  //
  //  Return codes:
  //  >=0  serial number of variable or cut found
  //   -1  variable not found
  //   -2  error parsing variable name
  //   -3  error parsing variable name, error already printed

#if ROOT_VERSION_CODE >= ROOT_VERSION(4,0,0)
  action = kDefinedVariable;
#endif
  Int_t k = DefinedGlobalVariable( name );
  if( k>=0 ) {
    FVarDef_t& def = fVarDef[k];
    // Interpret Char_t* variables as strings
    const THaVar* pvar = static_cast<const THaVar*>( def.code );
    assert(pvar);
    if( pvar->GetType() == kCharP ) {
#if ROOT_VERSION_CODE >= ROOT_VERSION(4,0,0)
      action = kDefinedString;
#endif
      // String definitions must be in the same array as variable definitions
      // because TFormula may revert a kDefinedString to a kDefinedVariable.
      def.type = kString;
      // TFormula::Analyze will increment fNstring even if the string
      // was already found. Fix this here.
      if( k < kMAXFOUND && !fAlreadyFound.TestBitNumber(k) )
	fAlreadyFound.SetBitNumber(k);
      else
	--fNstring;
    }
    return k;
  }
  return DefinedCut( name );
}

//_____________________________________________________________________________
Int_t THaFormula::DefinedCut( const TString& name )
{
  // Check if 'name' is a known cut. If so, enter it in the local list of
  // variables used in this formula.

  // Cut names are obviously only valid if there is a list of existing cuts
  if( fCutList ) {
    const THaCut* pcut = fCutList->FindCut( name );
    if( pcut ) {
      // See if this cut already used earlier in this new cut
      for( vector<FVarDef_t>::size_type i=0; i<fVarDef.size(); ++i ) {
	FVarDef_t& def = fVarDef[i];
	if( def.type == kCut && pcut == def.code )
	  return i;
      }
      fVarDef.push_back( FVarDef_t(kCut,pcut,0) );
      fNpar = 0;
      return fVarDef.size()-1;
    }
  }
  return -1;
}

//_____________________________________________________________________________
Int_t THaFormula::DefinedGlobalVariable( const TString& name )
{
  // Check if 'name' is a known global variable. If so, enter it in the
  // local list of variables used in this formula.

  // No list of variables or too many variables in this formula?
  if( !fVarList )
    return -2;

  // Parse name for array syntax
  THaArrayString parsed_name(name);
  if( parsed_name.IsError() ) return -1;

  // Find the variable with this name
  const THaVar* var = fVarList->Find( parsed_name.GetName() );
  if( !var )
    return -1;

  Int_t index = 0;
  if( parsed_name.IsArray() ) {
    // Error if array requested but the corresponding variable is not an array
    if( !var->IsArray() )
      return -2;
    if( !var->IsVarArray() ) {
      // Fixed-size arrays: check if subscript(s) within bounds?
      index = var->Index( parsed_name );
      if( index < 0 )
	return -2;
    }
    else {
      // Variable-size arrays are always 1-d
      assert( var->GetNdim() == 1 );
      if( parsed_name.GetNdim() != 1 )
	return -1;
      // For variable-size arrays, we allow requests for any element and
      // check at evaluation time whether the element actually exists
      index = parsed_name[0];
    }
  } else if( var->IsArray() ) {
    // Asking for an entire array -> array formula
    SetBit(kArrayFormula);
    // TODO: support instances
  }

  // Check if this variable already used in this formula
  for( vector<FVarDef_t>::size_type i=0; i<fVarDef.size(); ++i ) {
    FVarDef_t& def = fVarDef[i];
    if( var == def.code && index == def.index )
      return i;
  }
  // If this is a new variable, add it to the list
  fVarDef.push_back( FVarDef_t(kVariable,var,index) );

  // No parameters ever for a THaFormula
  fNpar = 0;

  return fVarDef.size()-1;
}

//_____________________________________________________________________________
Double_t THaFormula::Eval( void )
{
  // Evaluate this formula

  ResetBit(kInvalid);
  Double_t y;

  if( IsError() )  return kBig;
  if (fNoper == 1 && fVarDef.size() == 1 )
    y = DefinedValue(0);
  else
    y = EvalPar(0);

  if( IsInvalid() )
    return kBig;

  return y;
}

//_____________________________________________________________________________
#if ROOT_VERSION_CODE >= ROOT_VERSION(3,5,7)
#if ROOT_VERSION_CODE >= ROOT_VERSION(5,16,0)
TString THaFormula::GetExpFormula( Option_t* ) const
#else
TString THaFormula::GetExpFormula() const
#endif
{
  // Override ROOT's TFormula::GetExpFormula because it does not handle
  // kDefinedVariable and kDefinedString opcodes.
  // For our purposes, the un-expanded expression is good enough

  return fTitle;
}
#endif
//_____________________________________________________________________________
void THaFormula::Print( Option_t* option ) const
{
  // Print this formula to stdout
  // Options:
  //   "BRIEF" -- short, one line
  //   "FULL"  -- full, multiple lines

  if( !strcmp( option, kPRINTFULL ))
    TFormula::Print( option );
  else
    TNamed::Print(option);
}

//_____________________________________________________________________________

ClassImp(THaFormula)

