//*-- Author :    Ole Hansen   03/05/2000


//////////////////////////////////////////////////////////////////////////
//
// THaCut -- Hall A analyzer cut (a.k.a. test)
// This is a slightly expanded version of a THaFormula
//
//////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cstdlib>
#include <cstring>

#include "THaCut.h"
#include "THaPrintOption.h"
#include "THaCutList.h"
#include "THaVar.h"

using namespace std;

//_____________________________________________________________________________
THaCut::THaCut( const char* name, const char* expression, const char* block, 
		const THaVarList& lst, const THaCutList* cutlist ) :
  THaFormula(), fLastResult(kFALSE), fBlockname(block), fNCalled(0), fNPassed(0), 
  fCutList(cutlist), fIsCut(NULL)
{
  // Create a cut 'name' according to 'expression'. 
  // The cut may use global variables from the list 'lst' and other,
  // previously defined cuts from 'cutlist'.
  
  // We have to duplicate the TFormula constructor code here because of
  // the call to Compile().

  SetName(name);
  SetList(lst);

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
Int_t THaCut::Compile( const char* expression )
{
  // Parse the given expression, or, if empty, parse the title.
  // Return 0 on success, 1 if error in expression.
  //
  // Unlike the behavior of THaFormula, THaCuts do NOT store themselves in 
  // ROOT's list of formulas. Otherwise existing cuts used in new cut expressions 
  // will get reparsed instead of queried. As a result, existing cuts can only
  // be used by other cuts, not by formulas.

  delete [] fIsCut; 
  fIsCut = new Bool_t[kMAXCODES];
  memset( fIsCut, 0, kMAXCODES*sizeof(Bool_t));

  fRegister = kFALSE;  // Do not register this object in ROOT's list of functions
  return THaFormula::Compile( expression );
}

//_____________________________________________________________________________
THaCut::~THaCut()
{
  // Destructor

  delete [] fIsCut; fIsCut = 0;
}

//_____________________________________________________________________________
Double_t THaCut::DefinedValue( Int_t i )
{
  // Get value of i-th variable in the formula
  // If the i-th variable is another cut, return its last result
  // (calculated at the last evaluation).
  
  const THaVar* ptr = fCodes[i];
  if( !ptr ) return 0.0;
  if( fIsCut[i] )
    return reinterpret_cast<const THaCut*>(ptr)->GetResult();
  
  return ptr->GetValue( fIndex[i] );
}  

//_____________________________________________________________________________
Int_t THaCut::DefinedVariable(TString& name)
{
  // Check if 'name' is in the list of existing cuts. 
  // If so, store pointer to the cut for use by DefinedValue().
  // Otherwise, assume 'name' is a global variable and pass it on
  // to THaFormula::DefinedVariable().

  //  Return codes:
  //  >=0  serial number of variable or cut found
  //   -1  no list of variables defined
  //   -2  error parsing variable name
  //   -3  variable name not defined
  //   -4  array requested, but defined variable is no array
  //   -5  array index out of bounds
  //   -6  maximum number of variables exceeded

  fIsCut[ fNcodes ] = 0;

  // Other cut names are obviously only valid if there is a list of
  // existing cuts
  if( fCutList ) {
    const THaCut* pcut = fCutList->FindCut( name );
    if( pcut ) {
      if( fNcodes >= kMAXCODES ) return -6;
      // See if this cut already used earlier in this new cut
      for( Int_t i=0; i<fNcodes; i++ ) {
	if( fIsCut[i] && pcut == reinterpret_cast<const THaCut*>(fCodes[i]) )
	  return i;
      }
      Int_t code = fNcodes++;
      fIsCut[ code ] = 1;
      // okok, this is stretching it, but why not? (saves lots of space)
      fCodes[ code ] = reinterpret_cast<const THaVar*>(pcut);
      fNpar = 0;
      return code;
    }
  }
  // Either no cut list or cut not found -> check if it is a global variable
  return THaFormula::DefinedVariable( name );
}

//_____________________________________________________________________________
void THaCut::Print( Option_t* option ) const
{
  // Print this cut.
  // Options:
  //   "BRIEF" -- short (default)
  //   "FULL"  -- extended
  //   "LINE"  -- one-line (for listings)
  //   "STATS" -- one-line (listing of statistics only)

  THaPrintOption s(option);
  Int_t nn = max( s.GetValue(1), (Int_t)strlen(GetName()) );
  Int_t nt = max( s.GetValue(2), (Int_t)strlen(GetTitle()) );
  Int_t nb = max( s.GetValue(3), fBlockname.Length() );
  Int_t np = max( s.GetValue(4), (Int_t)strlen(s.GetOption(4)) );
    
  // Print data according to the requested format

  if ( s.IsLine() ) {

    cout.flags( ios::left );
    cout << setw(nn) << GetName() << "  " 
	 << setw(nt) << GetTitle() << "  ";
    if( !strcmp( s.GetOption(), kPRINTLINE )) {
      cout << setw(1)  << (bool)fLastResult << "  "
	   << setw(nb) << fBlockname << "  ";
    }
    cout << setw(9)  << fNCalled << "  "
	 << setw(np) << fNPassed << " ";
    cout << setprecision(3);
    if( fNCalled > 0 )
      cout << "(" << 100.0*((float)fNPassed)/((float)fNCalled) << "%)";
    else
      cout << "(0.0%)";

    cout << endl;

  } else {

    cout.flags( ios::right );
    THaFormula::Print( s.GetOption() );
    cout << "Curval: " << setw(9) << (bool)fLastResult << "  "
	 << "Block:  " << fBlockname << endl;
    cout << "Called: " << setw(9) << fNCalled << "  "
	 << "Passed: " << setw(9) << fNPassed;
    if( fNCalled > 0 )
      cout << setprecision(3)
	   <<"  (" << 100.0*((float)fNPassed)/((float)fNCalled) << "%)\n";
    else
      cout << "  (0.00%)\n";

  }
}

//_____________________________________________________________________________
void THaCut::SetName( const Text_t* name )
{
  // Set name of the cut
  // This method overrides the one provided by TNamed
  // so that the name can only be set if it is empty.
  // This avoids problems with hash values since THaCuts are
  // elements of a THashList.
  // Alternatively, we could rehash the associated table(s), but that 
  // would be much more involved.

  if( fName.Length() == 0 )
    TNamed::SetName( name );
}

//_____________________________________________________________________________
void THaCut::SetNameTitle( const Text_t* name, const Text_t* formula )
{
  // Set name and formula of the object, only if not yet initialized

  if( fName.Length() == 0 )
    TNamed::SetNameTitle( name, formula );
}

//_____________________________________________________________________________
ClassImp(THaCut)

