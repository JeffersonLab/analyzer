//*-- Author :    Ole Hansen    20/04/2000

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

using namespace std;

const Option_t* const THaFormula::kPRINTFULL  = "FULL";
const Option_t* const THaFormula::kPRINTBRIEF = "BRIEF";

static const Double_t kBig = 1e38; // Error value

//_____________________________________________________________________________
//
//     A THaFormula defines cuts and histogram expressions
//     for the Hall A analyzer
//
//  A THaFormula can contain any arithmetic expression including
//  standard operators and mathematical functions separated by operators.
//  Examples of valid expression:
//          "x<y && sqrt(z)>3.2"
//


//_____________________________________________________________________________
THaFormula::THaFormula( const char* name, const char* expression, 
			const THaVarList* vlst, const THaCutList* clst )
  : TFormula(), fVarDef(NULL), fVarList(vlst), fCutList(clst),
    fError(kFALSE), fRegister(kTRUE)
{
  // Create a formula 'expression' with name 'name' and symbolic variables 
  // from the list 'lst'.
  
  // We have to duplicate the TFormula constructor code here because of
  // the call to Compile(). Compile() only works if fVarList is set. 

  SetName(name);

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
  TFormula(rhs), fNcodes(rhs.fNcodes), fVarList(rhs.fVarList),
  fCutList(rhs.fCutList), fError(rhs.fError), fRegister(rhs.fRegister)
{
  // Copy ctor
  fVarDef = new FVarDef_t[ kMAXCODES ];
  memcpy( fVarDef, rhs.fVarDef, kMAXCODES*sizeof(FVarDef_t));
}

//_____________________________________________________________________________
THaFormula& THaFormula::operator=( const THaFormula& rhs )
{
  if( this != &rhs ) {
    TFormula::operator=(rhs);
    fNcodes = rhs.fNcodes;
    fVarList = rhs.fVarList;
    fCutList = rhs.fCutList;
    fError = rhs.fError;
    fRegister = rhs.fRegister;
    delete fVarDef;
    fVarDef = new FVarDef_t[ kMAXCODES ];
    memcpy( fVarDef, rhs.fVarDef, kMAXCODES*sizeof(FVarDef_t));
  }
  return *this;
}

//_____________________________________________________________________________
Int_t THaFormula::Compile( const char* expression )
{
  // Parse the given expression, or, if empty, parse the title.
  // Return 0 on success, 1 if error in expression.

  fNcodes = 0;
  fNval   = 0;
  fAlreadyFound.ResetAllBits();
  delete [] fVarDef;
  fVarDef = new FVarDef_t[ kMAXCODES ];
  memset( fVarDef, 0, kMAXCODES*sizeof(FVarDef_t));

  Int_t status = TFormula::Compile( expression );

  //*-*- Store formula in linked list of formula in ROOT

  fError = (status != 0);
  if( fError ) {
    if( fRegister) gROOT->GetListOfFunctions()->Remove(this);
  } else {
#ifdef WITH_DEBUG
    R__ASSERT( fNval+fNstring == fNcodes );
    R__ASSERT( fNstring >= 0 && fNval >= 0 );
#endif
    if( fRegister ) {
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
      fNval = fNstring = fNcodes;
  }
  return status;
}

//_____________________________________________________________________________
THaFormula::~THaFormula()
{
  // Destructor

  delete [] fVarDef; fVarDef = 0;
}

//_____________________________________________________________________________
char* THaFormula::DefinedString( Int_t i )
{
  // Get pointer to the i-th string variable. If the variable is not
  // a string, return pointer to an empty string.

#ifdef WITH_DEBUG
  R__ASSERT( i>=0 && i<fNcodes );
#endif
  FVarDef_t* def = fVarDef+i;
  if( def->type == kString ) {
    const THaVar* pvar = reinterpret_cast<const THaVar*>( def->code );
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
  
#ifdef WITH_DEBUG
  R__ASSERT( i>=0 && i<fNcodes );
#endif
  FVarDef_t* def = fVarDef+i;
  const void* ptr = def->code;
  if( !ptr ) return kBig;
  switch( def->type ) {
  case kVariable:
  case kString:
    return reinterpret_cast<const THaVar*>(ptr)->GetValue( def->index );
    break;
  case kCut:
    return reinterpret_cast<const THaCut*>(ptr)->GetResult();
    break;
  default:
    return kBig;
  }
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
    FVarDef_t* def = fVarDef+k;
    // Interpret Char_t* variables as strings
    const THaVar* pvar = reinterpret_cast<const THaVar*>( def->code );
    if( pvar->GetType() == kCharP ) {
#if ROOT_VERSION_CODE >= ROOT_VERSION(4,0,0)
      action = kDefinedString;
#endif
      // String definitions must be in the same array as variable definitions
      // because TFormula may revert a kDefinedString to a kDefinedVariable.
      def->type = kString;
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
      if( fNcodes >= kMAXCODES ) return -1;
      // See if this cut already used earlier in this new cut
      FVarDef_t* def = fVarDef;
      for( Int_t i=0; i<fNcodes; i++, def++ ) {
	if( def->type == kCut && pcut == def->code )
	  return i;
      }
      def->type = kCut;
      def->code  = pcut;
      def->index = 0;
      fNpar = 0;
      return fNcodes++;
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
  if( !fVarList || fNcodes >= kMAXCODES )
    return -2;

  // Parse name for array syntax
  THaArrayString var(name);
  if( var.IsError() ) return -1;

  // Find the variable with this name
  const THaVar* obj = fVarList->Find( var.GetName() );
  if( !obj ) 
    return -1;

  // Error if array requested but the corresponding variable is not an array
  if( var.IsArray() && !obj->IsArray() )
    return -2;

  // Subscript(s) within bounds?
  Int_t index = 0;
  if( var.IsArray() 
      && (index = obj->Index( var )) <0 ) return -2;
		    
  // Check if this variable already used in this formula
  FVarDef_t* def = fVarDef;
  for( Int_t i=0; i<fNcodes; i++, def++ ) {
    if( obj == def->code && index == def->index )
      return i;
  }
  // If this is a new variable, add it to the list
  def->type = kVariable;
  def->code = obj;
  def->index = index;

  // No parameters ever for a THaFormula
  fNpar = 0;

  return fNcodes++;
}

//_____________________________________________________________________________
Double_t THaFormula::Eval( void )
{
  // Evaluate this formula

  if( fError )  return kBig;
  if (fNoper == 1 && fNcodes == 1 )  return DefinedValue(0);
  
  return EvalPar( 0 );
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

