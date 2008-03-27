//*-- Author :    Ole Hansen   03-May-2000


//////////////////////////////////////////////////////////////////////////
//
// THaCut -- Hall A analyzer cut (a.k.a. test)
// This is a slightly expanded version of a THaFormula
//
//////////////////////////////////////////////////////////////////////////

#include "THaCut.h"
#include "THaPrintOption.h"

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cstring>


using namespace std;

//_____________________________________________________________________________
THaCut::THaCut( const char* name, const char* expression, const char* block, 
		const THaVarList* vlst, const THaCutList* clst ) :
  THaFormula(), fLastResult(kFALSE), fBlockname(block), fNCalled(0), fNPassed(0)
{
  // Create a cut 'name' according to 'expression'. 
  // The cut may use global variables from the list 'vlst' and other,
  // previously defined cuts from 'clst'.
  //  
  // Unlike the behavior of THaFormula, THaCuts do NOT store themselves in 
  // ROOT's list of formulas. Otherwise existing cuts used in new cut expressions 
  // will get reparsed instead of queried. As a result, existing cuts can only
  // be used by other cuts, not by formulas.

  // Sadly, we have to duplicate the TFormula constructor code here because of
  // the call to Compile(), which in turn calls our virtual function 
  // DefinedVariable().

  SetName(name);
  SetList(vlst);
  SetCutList(clst);
  fRegister = kFALSE;

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
THaCut::THaCut( const THaCut& rhs ) :
  THaFormula(rhs), fLastResult(rhs.fLastResult), fBlockname(rhs.fBlockname),
  fNCalled(rhs.fNCalled), fNPassed(rhs.fNPassed)
{
  // Copy ctor
}

//_____________________________________________________________________________
THaCut& THaCut::operator=( const THaCut& rhs )
{
  if( this != &rhs ) {
    THaFormula::operator=(rhs);
    fLastResult = rhs.fLastResult;
    fBlockname  = rhs.fBlockname;
    fNCalled    = rhs.fNCalled;
    fNPassed    = rhs.fNPassed;
  }
  return *this;
}

//_____________________________________________________________________________
#if ROOT_VERSION_CODE >= ROOT_VERSION(4,0,0)
Int_t THaCut::DefinedVariable(TString& name, Int_t& action)
#else
Int_t THaCut::DefinedVariable(TString& name)
#endif
{
  // Check if 'name' is in the list of existing cuts. 
  // If so, store pointer to the cut for use by DefinedValue().
  // Otherwise, assume 'name' is a global variable and pass it on
  // to THaFormula::DefinedVariable().

  //  Return codes:
  //  >=0  serial number of variable or cut found
  //   -1  variable not found
  //   -2  error parsing variable name
  //   -3  error parsing variable name, error already printed

#if ROOT_VERSION_CODE >= ROOT_VERSION(4,0,0)
  action = kDefinedVariable;
#endif
  Int_t k = DefinedCut( name );
  if( k>=0 ) return k;
#if ROOT_VERSION_CODE >= ROOT_VERSION(4,0,0)
  return THaFormula::DefinedVariable( name, action );
#else
  return THaFormula::DefinedVariable( name );
#endif
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
void THaCut::SetBlockname( const Text_t* name )
{
  // Set name of block (=group of cuts) for this cut.
  // The block name is used only for informational purposes within the 
  // THaCut class.

  fBlockname = name;
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

