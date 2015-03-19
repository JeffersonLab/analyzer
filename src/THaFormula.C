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
#include "TVirtualMutex.h"
#include "TMath.h"

#include <iostream>
#include <cstring>
#include <cassert>
#include <algorithm>
#include <numeric>

using namespace std;

const Option_t* const THaFormula::kPRINTFULL  = "FULL";
const Option_t* const THaFormula::kPRINTBRIEF = "BRIEF";

static const Double_t kBig = 1e38; // Error value

enum EFuncCode { kLength, kSum, kMean, kStdDev, kMax, kMin,
		 kGeoMean, kMedian, kIteration, kNumSetBits };

#define ALL(c) (c).begin(), (c).end()

//_____________________________________________________________________________
static inline Int_t NumberOfSetBits( UInt_t v )
{
  // Count number of bits set in 32-bit integer. From
  // http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel

  v = v - ((v >> 1) & 0x55555555);
  v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
  return (((v + (v >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

//_____________________________________________________________________________
static inline Int_t NumberOfSetBits( ULong64_t v )
{
  // Count number of bits in 64-bit integer

  const ULong64_t mask32 = (1LL<<32)-1;
  return NumberOfSetBits( static_cast<UInt_t>(mask32 & v) ) +
    NumberOfSetBits( static_cast<UInt_t>(mask32 & (v>>32)) );
}

//_____________________________________________________________________________
THaFormula::THaFormula() : TFormula(), fVarList(0), fCutList(0), fInstance(0)
{
  // Default constructor

  Init( 0, 0 );
}

//_____________________________________________________________________________
THaFormula::THaFormula( const char* name, const char* expression,
			Bool_t do_register,
			const THaVarList* vlst, const THaCutList* clst )
  : TFormula(), fVarList(vlst), fCutList(clst), fInstance(0)
{
  // Create a formula 'expression' with name 'name' and symbolic variables
  // from the list 'lst'.

  // We have to duplicate the TFormula constructor code here because of
  // the call to Compile(). Not only do we have our own Compile(), but
  // also out own DefinedVariable() etc. virtual functions. A disturbing
  // design error of the ROOT base class indeed.

  if( Init( name, expression ) != 0 ) {
    RegisterFormula(false);
    return;
  }

  SetBit(kNotGlobal,!do_register);

  Compile();   // This calls our own Compile()

  if( do_register )
    RegisterFormula();
}

//_____________________________________________________________________________
Int_t THaFormula::Init( const char* name, const char* expression )
{
  // Common initialization called from the constructors

  const char* const here = "THaFormula";

  ResetBit(kNotGlobal);
  ResetBit(kError);
  ResetBit(kInvalid);
  ResetBit(kVarArray);
  ResetBit(kArrayFormula);
  ResetBit(kFuncOfVarArray);

  if( !name )
    return -1;
  SetName(name);

  if( fName.IsWhitespace() ) {
    Error(here, "name may not be empty");
    SetBit(kError);
  }

  // Eliminate blanks in expression and convert "**" to "^"
  TString chaine(expression);
  chaine.ReplaceAll(" ","");
  if( chaine.Length() == 0 ) {
    Error(here, "expression may not be empty");
    SetBit(kError);
    return -1;
  }
  chaine.ReplaceAll("**","^");

  Bool_t gausNorm = kFALSE, landauNorm = kFALSE, linear = kFALSE;

  //special case for functions for linear fitting
  if (chaine.Contains("++"))
    linear = kTRUE;
  // special case for normalized gaus
  if (chaine.Contains("gausn")) {
    gausNorm = kTRUE;
    chaine.ReplaceAll("gausn","gaus");
  }
  // special case for normalized landau
  if (chaine.Contains("landaun")) {
    landauNorm = kTRUE;
    chaine.ReplaceAll("landaun","landau");
  }
  SetTitle(chaine.Data());

  if( linear )
    SetBit(kLinear);

  if( gausNorm || landauNorm )
    SetBit(kNormalized);

  return 0;
}

//_____________________________________________________________________________
THaFormula::THaFormula( const THaFormula& rhs ) :
  TFormula(rhs), fVarList(rhs.fVarList), fCutList(rhs.fCutList), fInstance(0)
{
  // Copy ctor
}

//_____________________________________________________________________________
THaFormula::~THaFormula()
{
  // Destructor

  // Delete any subformulas we may have created
  for( vector<FVarDef_t>::size_type i=0; i<fVarDef.size(); ++i ) {
    FVarDef_t& def = fVarDef[i];
    if( def.type == kFormula ) {
      delete static_cast<THaFormula*>(def.obj);
      def.obj = 0;
    }
  }
}

//_____________________________________________________________________________
THaFormula& THaFormula::operator=( const THaFormula& rhs )
{
  if( this != &rhs ) {
    TFormula::operator=(rhs);
    fVarList = rhs.fVarList;
    fCutList = rhs.fCutList;
    fInstance = 0;
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


  SetBit(kError, (status != 0));
  if( !IsError() ) {
    assert( fNval+fNstring == (Int_t)fVarDef.size() );
    assert( fNstring >= 0 && fNval >= 0 );
    // If the formula is good, then fix the variable counters that TFormula
    // may have messed with when reverting lone kDefinedString variables to
    // kDefinedVariable. If there is a mix of strings and values, we force
    // the variable counters to the full number of defined variables, so
    // that the loops in EvalPar calculate all the values. This is inefficient,
    // but the best we can do with the implementation of TFormula.
    if( fNstring > 0 && fNval > 0 )
      fNval = fNstring = fVarDef.size();
  }
  return status;
}

//_____________________________________________________________________________
char* THaFormula::DefinedString( Int_t i )
{
  // Get pointer to the i-th string variable. If the variable is not
  // a string, return pointer to an empty string.

  assert( i>=0 && i<(Int_t)fVarDef.size() );
  FVarDef_t& def = fVarDef[i];
  if( def.type == kString ) {
    const THaVar* pvar = static_cast<const THaVar*>( def.obj );
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

  typedef vector<Double_t>::size_type vsiz_t;
  typedef vector<Double_t>::iterator  viter_t;

  assert( i>=0 && i<(Int_t)fVarDef.size() );

  if( IsInvalid() )
    return 1.0;

  FVarDef_t& def = fVarDef[i];
  switch( def.type ) {
  case kVariable:
  case kString:
  case kArray:
    {
      const THaVar* var = static_cast<const THaVar*>(def.obj);
      assert(var);
      Int_t index = (def.type == kArray) ? fInstance : def.index;
      assert(index >= 0);
      if( index >= var->GetLen() ) {
	SetBit(kInvalid);
	return 1.0; // safer than kBig to prevent overflow
      }
      return var->GetValue( index );
    }
    break;
  case kCut:
    {
      const THaCut* cut = static_cast<const THaCut*>(def.obj);
      assert(cut);
      return cut->GetResult();
    }
    break;
  case kFunction:
    {
      EFuncCode code = static_cast<EFuncCode>(def.index);
      switch( code ) {
      case kIteration:
	return fInstance;
      default:
	assert(false); // not reached
	break;
      }
    }
    break;
  case kFormula:
  case kVarFormula:
    {
      EFuncCode code = static_cast<EFuncCode>(def.index);
      THaFormula* func = static_cast<THaFormula*>(def.obj);
      assert(func);

      vsiz_t ndata = func->GetNdata();
      if( code == kLength )
	return ndata;

      if( ndata == 0 ) {
	//FIXME: needs thought
	SetBit(kInvalid);
	return 1.0;
      }
      Double_t y;
      if( code == kNumSetBits ) {
	// Number of set bits is intended for unsigned int-type expressions
	y = func->EvalInstance(fInstance);
	if( y > kMaxULong64 || y < kMinLong64 ) {
	  return 0;
	}
	return NumberOfSetBits( static_cast<ULong64_t>(y) );
      }

      vector<Double_t> values;
      values.reserve(ndata);
      for( vsiz_t i = 0; i < ndata; ++i ) {
	values.push_back( func->EvalInstance(i) );
      }
      if( func->IsInvalid() ) {
	SetBit(kInvalid);
	return 1.0;
      }
      switch( code ) {
      case kSum:
	y = accumulate( ALL(values), static_cast<Double_t>(0.0) );
	break;
      case kMean:
	y = TMath::Mean( ndata, &values[0] );
	break;
      case kStdDev:
	y = TMath::RMS( ndata, &values[0] );
	break;
      case kMax:
	y = *max_element( ALL(values) );
	break;
      case kMin:
	y = *min_element( ALL(values) );
	break;
      case kGeoMean:
	y = TMath::GeomMean( ndata, &values[0] );
	break;
      case kMedian:
	y = TMath::Median( ndata, &values[0] );
	break;
      default:
	assert(false); // not reached
	break;
      }
      return y;
    }
    break;
  }
  assert(false); // not reached
  return kBig;
}

//_____________________________________________________________________________
Int_t THaFormula::DefinedVariable( TString& name, Int_t& action )
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

  action = kDefinedVariable;

  Int_t k = DefinedSpecialFunction( name );
  if( k != -1 )
    return k;

  k = DefinedGlobalVariable( name );
  if( k>=0 ) {
    FVarDef_t& def = fVarDef[k];
    const THaVar* pvar = static_cast<const THaVar*>( def.obj );
    assert(pvar);
    // Interpret Char_t* variables as strings
    if( pvar->GetType() == kCharP ) {
      action = kDefinedString;
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
  if( k != -1 )
    return k;

  return DefinedCut( name );
}

//_____________________________________________________________________________
Int_t THaFormula::DefinedCut( TString& name )
{
  // Check if 'name' is a known cut. If so, enter it in the local list of
  // variables used in this formula.

  // Cut names are obviously only valid if there is a list of existing cuts
  if( fCutList ) {
    THaCut* pcut = fCutList->FindCut( name );
    if( pcut ) {
      // See if this cut already used earlier in the expression
      for( vector<FVarDef_t>::size_type i=0; i<fVarDef.size(); ++i ) {
	FVarDef_t& def = fVarDef[i];
	if( def.type == kCut && pcut == def.obj )
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
Int_t THaFormula::DefinedGlobalVariable( TString& name )
{
  // Check if 'name' is a known global variable. If so, enter it in the
  // local list of variables used in this formula.

  // No global variable list?
  if( !fVarList )
    return -1;

  // Parse name for array syntax
  THaArrayString parsed_name(name);
  if( parsed_name.IsError() ) return -1;

  // Find the variable with this name
  THaVar* var = fVarList->Find( parsed_name.GetName() );
  if( !var )
    return -1;

  EVariableType type = kVariable;
  Int_t index = 0;
  if( parsed_name.IsArray() ) {
    // Requesting an array element
    if( !var->IsArray() )
      // Element requested but the corresponding variable is not an array
      return -2;
    if( var->IsVarArray() ) {
      // Variable-size arrays are always 1-d
      assert( var->GetNdim() == 1 );
      if( parsed_name.GetNdim() != 1 )
	return -2;
      // For variable-size arrays, we allow requests for any element and
      // check at evaluation time whether the element actually exists
      index = parsed_name[0];
    } else {
      // Fixed-size arrays: check if subscript(s) within bounds?
      //TODO: allow fixing some dimensions
      index = var->Index( parsed_name );
      if( index < 0 )
	return -2;
    }
  } else if( var->IsArray() ) {
    // Asking for an entire array
    type = kArray;
    SetBit(kArrayFormula);
    if( var->IsVarArray() )
      SetBit(kVarArray);
  }

  // Check if this variable already used in this formula
  for( vector<FVarDef_t>::size_type i=0; i<fVarDef.size(); ++i ) {
    FVarDef_t& def = fVarDef[i];
    if( var == def.obj && index == def.index ) {
      assert( type == def.type );
      return i;
    }
  }
  // If this is a new variable, add it to the list
  fVarDef.push_back( FVarDef_t(type,var,index) );

  // No parameters ever for a THaFormula
  fNpar = 0;

  return fVarDef.size()-1;
}

//_____________________________________________________________________________
Int_t THaFormula::DefinedSpecialFunction( TString& name )
{
  // Check if 'name' is a predefined special function

  struct FuncDef_t {
    const char*    func;
    const char*    form;
    EVariableType  type;
    EFuncCode      code;
  };
  const FuncDef_t func_defs[] = {
    { "Length$(",     "length$Form",  kFormula,    kLength },
    { "Sum$(",        "sum$Form",     kFormula,    kSum },
    { "Mean$(",       "mean$Form",    kFormula,    kMean },
    { "StdDev$(",     "stddev$Form",  kFormula,    kStdDev },
    { "Max$(",        "max$Form",     kFormula,    kMax },
    { "Min$(",        "min$Form",     kFormula,    kMin },
    { "GeoMean$(",    "geoMean$Form", kFormula,    kGeoMean },
    { "Median$(",     "median$Form",  kFormula,    kMedian },
    { "Iteration$",   0,              kFunction,   kIteration },
    { "NumSetBits$(", "numbits$Form", kVarFormula, kNumSetBits },
    { 0 }
  };
  const FuncDef_t* def = func_defs;
  while( def->func ) {
    if( def->form && name.BeginsWith(def->func) && name.EndsWith(")") ) {
      // Make a subformula for the argument, but don't register it with ROOT
      TString subform = name( strlen(def->func), name.Length() );
      subform.Chop();
      THaFormula* func = new THaFormula( def->form, subform, false,
					 fVarList, fCutList );
      if( func->IsError() ) {
	delete func;
	return -3;
      }
      if( func->IsArray() ) {
	if( func->IsVarArray() ) {
	  // Make a note that the argument may be empty at evaluation time,
	  // even if the formula is not an array per se
	  SetBit( kFuncOfVarArray );
	} else if( func->GetNdata() == 0 ) {
	  // Empty fixed-size array is an invalid argument
	  return -2;
	}
      }
      fVarDef.push_back( FVarDef_t(def->type, func, def->code) );
      // Expand the function argument in case it is a recursive definition
      name = def->func; name += func->GetExpFormula(); name += ")";
      // Treat the kFormula-type functions as scalars, even though they might
      // cause this formula to have GetNdata() == 0 if they operate on an
      // empty array. kVarFormula however passes the array type right through.
      if( def->type == kVarFormula ) {
	SetBit( kArrayFormula, func->IsArray() );
	SetBit( kVarArray, func->IsVarArray() );
      }
      return fVarDef.size()-1;
    }
    else if( !def->form && name == def->func ) {
      fVarDef.push_back( FVarDef_t(def->type, 0, def->code) );
      return fVarDef.size()-1;
    }
    ++def;
  }
  return -1;
}


//_____________________________________________________________________________
Double_t THaFormula::Eval()
{
  // Evaluate this formula

  return EvalInstance(0);
}

//_____________________________________________________________________________
Double_t THaFormula::EvalInstance( Int_t instance )
{
  // Evaluate this formula

  if( IsError() )
    return kBig;
  if( instance < 0 || (!IsArray() && instance != 0) ) {
    SetBit(kInvalid);
    return kBig;
  }

  ResetBit(kInvalid);
  Double_t y = EvalInstanceUnchecked( instance );

  if( IsInvalid() )
    return kBig;

  return y;
}

//_____________________________________________________________________________
#if ROOT_VERSION_CODE >= ROOT_VERSION(5,15,9) && \
    ROOT_VERSION_CODE <  ROOT_VERSION(5,26,0)
TString THaFormula::GetExpFormula( Option_t* ) const
{
  // Override ROOT's buggy TFormula::GetExpFormula because it does not handle
  // kDefinedVariable and kDefinedString opcodes.
  // This bug became apparent with the change to TFormula::Analyze on Mar 9, 2007:
  // http://root.cern.ch/gitweb?p=root.git;a=commit;h=dd7d9db6417e006e575518b209a9d11f85726684
  // (approximately ROOT 5.15/09)
  // and was fixed on Aug 4, 2009 during the ROOT 5.26 development cycle:
  // http://root.cern.ch/gitweb?p=root.git;a=commit;h=3c6b681704ba8a8d61c407fabb6be14722925d0b
  // The workaround here is a quick fix that reverts to the old behavior before
  // ROOT 5.15/09, i.e. does not allow more than one level of recursion for
  // defining forumulas of formulas, but that's good enough for our purposes.

  return fTitle;
}
#endif

//_____________________________________________________________________________
Int_t THaFormula::GetNdataUnchecked() const
{
  // Return minimum of sizes of all referenced arrays

  Int_t ndata = TestBit(kArrayFormula) ? kMaxInt : 1; // 1 = kFuncOfVarArray
  for( vector<FVarDef_t>::size_type i = 0; ndata > 0 && i < fVarDef.size();
       ++i ) {
    const FVarDef_t& def = fVarDef[i];
    switch( def.type ) {
    case kArray:
      {
	const THaVar* pvar = static_cast<const THaVar*>(def.obj);
	assert( pvar );
	assert( pvar->IsArray() );
	assert( pvar->GetNdim() > 0 );
	ndata = TMath::Min( ndata, pvar->GetLen() );
      }
      break;
    case kFormula:
    case kVarFormula:
      {
	THaFormula* func = static_cast<THaFormula*>(def.obj);
	assert( func );
	Int_t nfunc = func->GetNdata();
	if( def.type == kFormula && nfunc == 0 )
	  ndata = 0;
	else if( def.type == kVarFormula )
	  ndata = TMath::Min( ndata, nfunc );
      }
      break;
    default:
      break;
    }
  }
  return ndata;
}

//_____________________________________________________________________________
Int_t THaFormula::GetNdata() const
{
  // Get number of available instances of this formula

  if( !IsArray() && !TestBit(kFuncOfVarArray) )
    return 1;

  return GetNdataUnchecked();
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
    TNamed::Print(option);
}

//_____________________________________________________________________________
void THaFormula::RegisterFormula( Bool_t add )
{
  // Store/remove this formula in/from ROOT's list of functions

  if( gROOT ) {
    R__LOCKGUARD2(gROOTMutex);
    const char* name = GetName();
    TCollection* flist = gROOT->GetListOfFunctions();
    TObject* old = flist->FindObject(name);
    if( !add || IsError() ) {
      if( old == this )
	flist->Remove(old);
    } else {
      if( old )
	flist->Remove(old);
      if( !TestBit(kNotGlobal) ) {
	flist->Add(this);
      }
    }
  }
}

//_____________________________________________________________________________

ClassImp(THaFormula)
