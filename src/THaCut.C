//*-- Author :    Ole Hansen   03-May-2000

//////////////////////////////////////////////////////////////////////////
//
// THaCut -- Class for a cut (a.k.a. test)
//
// This is a slightly expanded version of a THaFormula that supports
// statistics counters and caches the result of the last evaluation.
//
//////////////////////////////////////////////////////////////////////////

#include "THaCut.h"
#include "THaPrintOption.h"
#include "TMath.h"

#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cstring>

using namespace std;

//_____________________________________________________________________________
THaCut::THaCut()
  : THaFormula(), fLastResult(kFALSE), fNCalled(0), fNPassed(0), fMode(kAND)
{
  // Default constructor
}

//_____________________________________________________________________________
THaCut::THaCut( const char* name, const char* expression, const char* block,
		const THaVarList* vlst, const THaCutList* clst )
  : THaFormula(), fLastResult(kFALSE), fBlockname(block), fNCalled(0),
    fNPassed(0), fMode(kAND)
{
  // Create a cut 'name' according to 'expression'.
  // The cut may use global variables from the list 'vlst' and other,
  // previously defined cuts from 'clst'.
  //
  // Unlike the behavior of THaFormula, THaCuts do NOT store themselves in
  // ROOT's list of functions. Otherwise existing cuts used in new cut
  // expressions would get reparsed instead of queried. This wouldn't
  // work properly with a non-default array evaluation mode (OR/XOR).

  SetList(vlst);
  SetCutList(clst);

  // Call common THaFormula::Init
  if( Init(name, expression) != 0 ||
      (fMode = ParsePrefix(fTitle)) == kModeErr ) {
    return;
  }

  // Do not register cuts in ROOT's list of functions
  SetBit(kNotGlobal);

  // This calls THaFormula::Compile(), which calls TFormula::Analyze(),
  // which then calls our own DefinedVariable()
  Compile();
}

//_____________________________________________________________________________
THaCut::THaCut( const THaCut& rhs ) :
  THaFormula(rhs), fLastResult(rhs.fLastResult), fBlockname(rhs.fBlockname),
  fNCalled(rhs.fNCalled), fNPassed(rhs.fNPassed), fMode(rhs.fMode)
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
    fMode       = rhs.fMode;
  }
  return *this;
}

//_____________________________________________________________________________
THaCut::~THaCut()
{
  // Destructor
}

//_____________________________________________________________________________
Int_t THaCut::DefinedVariable(TString& name, Int_t& action)
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

  action = kDefinedVariable;
  Int_t k = DefinedCut( name );
  if( k>=0 ) return k;
  return THaFormula::DefinedVariable( name, action );
}

//_____________________________________________________________________________
Bool_t THaCut::EvalElement( Int_t instance )
{
  return (TMath::Nint( THaFormula::EvalInstanceUnchecked(instance) ) != 0);
}

//_____________________________________________________________________________
Double_t THaCut::Eval()
{
  // Evaluate the cut and increment counters. The Double_t return value
  // is awkward, but results are usually retrieved via GetResult anyway.
  // Problems like this will go away if Eval() is templatized.

  ResetBit(kInvalid);
  fNCalled++;
  if( IsError() ) {
    fLastResult = false;
  }
  else {
    Int_t ndata = 1;
    if( TestBit(kArrayFormula) || TestBit(kFuncOfVarArray) )
      ndata = GetNdataUnchecked();
    if( ndata == 0 )
      SetBit(kInvalid);
    else {
      fLastResult = EvalElement(0);
      if( TestBit(kArrayFormula) && !IsInvalid() && ndata > 1 ) {
	switch( fMode ) {
	case kAND:
	  // All elements satisfy the test (==N)
	  for( Int_t i=1; fLastResult && i<ndata; ++i )
	    fLastResult = EvalElement(i);
	  break;
	case kOR:
	  // At least one element satisfies the test (>=1)
	  for( Int_t i=1; !fLastResult && i<ndata; ++i )
	    fLastResult = EvalElement(i);
	  break;
	case kXOR:
	  {
	    // Exactly one element satisfies the test (==1)
	    Int_t ntrue = fLastResult ? 1 : 0;
	    for( Int_t i=1; ntrue != 2 && i<ndata; ++i ) {
	      if( EvalElement(i) )
		++ntrue;
	    }
	    fLastResult = (ntrue == 1);
	  }
	  break;
	default:
	  fLastResult = false;
	  break;
	}
      }
    }
    if( IsInvalid() ) {
      fLastResult = false;
    }
    else if( fLastResult ) {
      fNPassed++;
    }
  }
  return fLastResult;
}

//_____________________________________________________________________________
THaCut::EvalMode THaCut::ParsePrefix( TString& expr )
{
  // Parse the given expression for a mode prefix (e.g. "OR:")
  // and return its value. If the expression does not have a mode prefix,
  // return the default kAND (=0).
  // The expression is expected to have all whitespace removed.
  // If the prefix was found, it is stripped from the expression.
  // If a mode prefix seems to be present, but is not found in
  // the defined modes, return kModeErr.

  const EvalMode kDefaultMode = kAND;
  const char* const here = "THaCut";

  struct ModeDef_t {
    const char* prefix;
    EvalMode    mode;
  };
  const ModeDef_t mode_defs[] = {
    { "AND", kAND }, { "ALL", kAND }, { "OR", kOR }, { "ANY", kOR },
    { "XOR", kXOR }, { "ONE", kXOR }, { "ONEOF", kXOR }, { 0, kModeErr }
  };

  Ssiz_t colon = expr.Index(":");
  if( colon == kNPOS )
    return kDefaultMode;

  const ModeDef_t* def = mode_defs;
  while( def->prefix ) {
    if( expr.BeginsWith(def->prefix) &&
	static_cast<Ssiz_t>(strlen(def->prefix)) == colon ) {
      expr.Remove(0,colon+1);
      break;
    }
    ++def;
  }
  EvalMode mode = def->mode;

  if( mode == kModeErr ) {
    TString prefix = expr(0,colon);
    Error(here, "Unknown prefix %s", prefix.Data() );
  } else if( expr.Length() == 0 ) {
    Error(here, "expression may not be empty");
    mode = kModeErr;
  }
  if( mode == kModeErr )
    SetBit(kError);
  return mode;
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
void THaCut::Reset()
{
  // Reset cut result and statistics counters

  ClearResult();
  fNCalled = fNPassed = 0;
}

//_____________________________________________________________________________
ClassImp(THaCut)
