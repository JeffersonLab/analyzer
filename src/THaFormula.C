//*-- Author :    Ole Hansen    20/04/2000

#include "THaFormula.h"
#include "THaArrayString.h"
#include "THaVarList.h"
#include "THaCutList.h"
#include "THaCut.h"
#include "TROOT.h"

#include <iostream>
#include <cstring>
#include <cstdlib>

using namespace std;

const Option_t* const THaFormula::kPRINTFULL  = "FULL";
const Option_t* const THaFormula::kPRINTBRIEF = "BRIEF";

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
Int_t THaFormula::Compile( const char* expression )
{
  // Parse the given expression, or, if empty, parse the title.
  // Return 0 on success, 1 if error in expression.

  fNcodes = 0;
  fNval   = 0;
  delete [] fVarDef;
  fVarDef = new FVarDef_t[ kMAXCODES ];
  memset( fVarDef, 0, kMAXCODES*sizeof(FVarDef_t));

  Int_t status = TFormula::Compile( expression );

  //*-*- Store formula in linked list of formula in ROOT

  if( status ) {
    fError = kTRUE;
    if( fRegister) gROOT->GetListOfFunctions()->Remove(this);
  } else {
    fError = kFALSE;
    if( fRegister ) {
      TObject* old = gROOT->GetListOfFunctions()->FindObject(GetName());
      if (old) gROOT->GetListOfFunctions()->Remove(old);
      gROOT->GetListOfFunctions()->Add(this);
    }
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
Double_t THaFormula::DefinedValue( Int_t i )
{
  // Get value of i-th variable in the formula
  // If the i-th variable is a cut, return its last result
  // (calculated at the last evaluation).
  
  FVarDef_t* def = fVarDef+i;
  const void* ptr = def->code;
  if( !ptr ) return 0.0;
  switch( def->type ) {
  case kVariable:
    return reinterpret_cast<const THaVar*>(ptr)->GetValue( def->index );
    break;
  case kCut:
    return reinterpret_cast<const THaCut*>(ptr)->GetResult();
    break;
  default:
    return 0.0;
  }
  return 0.0;
}  

//_____________________________________________________________________________
Int_t THaFormula::DefinedVariable(TString& name)
{
  // Check if name is in the list of global objects

  //  This member function redefines the function in TFormula.
  //  A THaFormula may contain more than one variable.
  //  For each variable referenced, a pointer to the corresponding
  //  global object is stored 
  //
  //  Return codes:
  //  >=0  serial number of variable found
  //   -1  no list of variables defined
  //   -2  error parsing variable name
  //   -3  variable name not defined
  //   -4  array requested, but defined variable is no array
  //   -5  array index out of bounds
  //   -6  maximum number of variables exceeded

  Int_t k = DefinedGlobalVariable( name );
  if( k>=0 ) return k;
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
      if( fNcodes >= kMAXCODES ) return -6;
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
  return -3;
}

//_____________________________________________________________________________
Int_t THaFormula::DefinedGlobalVariable( const TString& name )
{
  // Check if 'name' is a known global variable. If so, enter it in the
  // local list of variables used in this formula.

  // No list of variables or too many variables in this formula?
  if( !fVarList ) 
    return -1;
  if( fNcodes >= kMAXCODES )
    return -6;

  // Parse name for array syntax
  THaArrayString var;
  if( var.IsError() ) return -2;

  // Find the variable with this name
  const THaVar* obj = fVarList->Find( var.GetName() );
  if( !obj ) 
    return -3;

  // Error if array requested but the corresponding variable is not an array
  if( var.IsArray() && !obj->IsArray() )
    return -4;

  // Subscript(s) within bounds?
  Int_t index = 0;
  if( var.IsArray() 
      && (index = obj->Index( var )) <0 ) return -5;
		    
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

  if( fError )  return 0.0;
  if (fNoper == 1 && fNcodes == 1 )  return DefinedValue(0);
  
  return EvalPar( 0 );
}
  
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
    printf("%-20s : %s\n",GetName(),GetTitle());
}

//_____________________________________________________________________________

ClassImp(THaFormula)

