//*-- Author :    Ole Hansen   26/04/2000


//////////////////////////////////////////////////////////////////////////
//
// THaVar
//
// A "global variable".  This is essentially a read-only pointer to the
// actual data, along with support for data types and arrays.  It can
// be used to retrieve data from anywhere in memory, e.g. analysis results
// of some detector object.
//
// THaVar objects are managed by the THaVarList.
//
// The standard way to define a variable is via the constructor, e.g.
//
//    Double_t x = 1.5;
//    THaVar v("x","variable x",x);
//
// Constructors and Set() determine the data type automatically.
// Supported types are
//    Double_t, Float_t, Long64_t, ULong64_t, Int_t, UInt_t,
//    Short_t, UShort_t, Char_t, UChar_t
//
// Arrays can be defined as follows:
//
//    Double_t* a = new Double_t[10];
//    THaVar* va = new THaVar("a[10]","array of doubles",a);
//
// One-dimensional variable-size arrays are also supported. The actual
// size must be contained in a variable of type Int_t.
// Example:
//
//    Double_t* a = new Double_t[20];
//    Int_t size;
//    GetValues( a, size );  //Fills a and sets size to number of data.
//    THaVar* va = new THaVar("a","var size array",a,&size);
//
// Any array size given in the name string is ignored (e.g. "a[20]"
// would have the same effect as "a"). The data are always interpreted
// as a one-dimensional array.
//
// Data are retrieved using GetValue(), which always returns Double_t.
// If access to the raw data is needed, one can use GetValuePointer()
// (with the appropriate caution).
//
//////////////////////////////////////////////////////////////////////////

#include "THaVar.h"
#include "THaArrayString.h"
#include "FixedArrayVar.h"
#include "VariableArrayVar.h"
#include "VectorVar.h"
#include "SeqCollectionMethodVar.h"
#include "VectorObjMethodVar.h"
#include "TError.h"
#include "DataType.h"  // for kBig

using namespace std;

const Long64_t THaVar::kInvalidInt = kMaxLong64;
const Double_t THaVar::kInvalid    = kBig;

static const char* const here = "THaVar";

//_____________________________________________________________________________
template <typename T>
THaVar::THaVar( const char* name, const char* descript, T& var,
		const Int_t* count )
  : TNamed(name,descript), fImpl(nullptr)
{
  // Constructor for basic types and fixed and variable-size arrays

  VarType type = Vars::FindType( typeid(T) );
  if( type >= kIntV && type <= kDoubleV ) {
    if( count )
      Warning( here, "Ignoring size counter for std::vector variable %s", name );
    fImpl = new Podd::VectorVar( this, &var, type );
  }
  else if( type != kVarTypeEnd ) {
    if( count )
      fImpl = new Podd::VariableArrayVar( this, &var, type, count );
    else if( fName.Index("[") != kNPOS )
      fImpl = new Podd::FixedArrayVar( this, &var, type );
    else
      fImpl = new Podd::Variable( this, &var, type );

    if( fImpl->IsError() )
      MakeZombie();
  }
  else {
    Error( here, "Unsupported data type for variable %s", name );
    MakeZombie();
  }
}

//_____________________________________________________________________________
THaVar::THaVar( const char* name, const char* descript, const void* obj,
	VarType type, Int_t offset, TMethodCall* method, const Int_t* count )
  : TNamed(name,descript), fImpl(nullptr)
{
  // Generic constructor (used by THaVarList::DefineByType)

  if( type == kObject  || type == kObjectP  || type == kObject2P ||
      type == kObjectV || type == kObjectPV ) {
    Error( here, "Variable %s: Object types not (yet) supported", name );
    MakeZombie();
    return;
  }
  else if( type >= kIntM && type <= kDoubleM ) {
    Error( here, "Variable %s: Matrix types not (yet) supported", name );
    MakeZombie();
    return;
  }
  else if( type >= kIntV && type <= kDoubleV ) {
    if( count ) {
      Warning( here, "Ignoring size counter for std::vector variable %s. "
	       "Fix code or call expert", name );
    }
    fImpl = new Podd::VectorVar( this, obj, type );
  }
  else if( count ) {
    if( offset >= 0 || method ) {
      Error( here, "Variable %s: Inconsistent arguments. Cannot specify "
	     "both count and offset/method", name );
      MakeZombie();
      return;
    }
    fImpl = new Podd::VariableArrayVar( this, obj, type, count );
  }
  else if( method || offset >= 0 ) {
    if( method && offset >= 0 ) {
      if( offset > 0 ) {
	Warning( here, "Variable %s: Offset > 0 ignored for method call on "
		 "object in collection. Fix code or call expert", name );
      }
      fImpl = new Podd::SeqCollectionMethodVar( this, obj, type, method );
    } else if( !method )
      fImpl = new Podd::SeqCollectionVar( this, obj, type, offset );
    else
      fImpl = new Podd::MethodVar( this, obj, type, method );
  }
  else if( fName.Index("[") != kNPOS )
    fImpl = new Podd::FixedArrayVar( this, obj, type );
  else
    fImpl = new Podd::Variable( this, obj, type );

  if( fImpl->IsError() )
    MakeZombie();
}

//_____________________________________________________________________________
THaVar::THaVar( const char* name, const char* descript, const void* obj,
	VarType type, Int_t elem_size, Int_t offset, TMethodCall* method )
  : TNamed(name,descript), fImpl(nullptr)
{
  // Generic constructor for std::vector<TObject(*)>
  // (used by THaVarList::DefineByType)

  if( type > kByte ) {
    Error(here, "Variable %s: Illegal type %s for data in std::vector "
	   "of objects", name, Vars::GetTypeName(type) );
    MakeZombie();
    return;
  }
  else if( elem_size < 0 || offset < 0 ) {
    Error( here, "Variable %s: Illegal parameters elem_size = %d, "
	   "offset = %d. Must be >= 0", name, elem_size, offset );
    MakeZombie();
    return;
  }
  if( method ) {
    if( offset > 0 ) {
      Warning( here, "Variable %s: Offset > 0 ignored for method call on "
	       "object", name );
    }
    fImpl = new Podd::VectorObjMethodVar( this, obj, type, elem_size, method );
  } else {
    fImpl = new Podd::VectorObjVar( this, obj, type, elem_size, offset );
  }

  if( fImpl->IsError() )
    MakeZombie();
}

//_____________________________________________________________________________
THaVar::~THaVar()
{
  delete fImpl;
}

//_____________________________________________________________________________
Int_t THaVar::Index( const char* s ) const
{
  // Return linear index into this array variable corresponding
  // to the array element described by the string 's'.
  // 's' must be either a single integer subscript (for a 1-d array)
  // or a comma-separated list of subscripts (for multi-dimensional arrays).
  //
  // NOTE: This method is less efficient than
  // THaVar::Index( THaArrayString& ) above because the string has
  // to be parsed first.
  //
  // Return -1 if subscript(s) out of bound(s) or -2 if incompatible arrays.

  if( !s || !*s )
    return 0;

  TString str = "t[";
  str.Append(s);
  str.Append(']');

  return Index( THaArrayString(str.Data()) );
}

//_____________________________________________________________________________
// Explicit template instantiations for supported types

template THaVar::THaVar( const char*, const char*, Double_t&, const Int_t* );
template THaVar::THaVar( const char*, const char*, Float_t&, const Int_t* );
template THaVar::THaVar( const char*, const char*, Long64_t&, const Int_t* );
template THaVar::THaVar( const char*, const char*, ULong64_t&, const Int_t* );
template THaVar::THaVar( const char*, const char*, Int_t&, const Int_t* );
template THaVar::THaVar( const char*, const char*, UInt_t&, const Int_t* );
template THaVar::THaVar( const char*, const char*, Short_t&, const Int_t* );
template THaVar::THaVar( const char*, const char*, UShort_t&, const Int_t* );
template THaVar::THaVar( const char*, const char*, Char_t&, const Int_t* );
template THaVar::THaVar( const char*, const char*, UChar_t&, const Int_t* );

template THaVar::THaVar( const char*, const char*, Double_t*&, const Int_t* );
template THaVar::THaVar( const char*, const char*, Float_t*&, const Int_t* );
template THaVar::THaVar( const char*, const char*, Long64_t*&, const Int_t* );
template THaVar::THaVar( const char*, const char*, ULong64_t*&, const Int_t* );
template THaVar::THaVar( const char*, const char*, Int_t*&, const Int_t* );
template THaVar::THaVar( const char*, const char*, UInt_t*&, const Int_t* );
template THaVar::THaVar( const char*, const char*, Short_t*&, const Int_t* );
template THaVar::THaVar( const char*, const char*, UShort_t*&, const Int_t* );
template THaVar::THaVar( const char*, const char*, Char_t*&, const Int_t* );
template THaVar::THaVar( const char*, const char*, UChar_t*&, const Int_t* );

template THaVar::THaVar( const char*, const char*, Double_t**&, const Int_t* );
template THaVar::THaVar( const char*, const char*, Float_t**&, const Int_t* );
template THaVar::THaVar( const char*, const char*, Long64_t**&, const Int_t* );
template THaVar::THaVar( const char*, const char*, ULong64_t**&, const Int_t* );
template THaVar::THaVar( const char*, const char*, Int_t**&, const Int_t* );
template THaVar::THaVar( const char*, const char*, UInt_t**&, const Int_t* );
template THaVar::THaVar( const char*, const char*, Short_t**&, const Int_t* );
template THaVar::THaVar( const char*, const char*, UShort_t**&, const Int_t* );
template THaVar::THaVar( const char*, const char*, Char_t**&, const Int_t* );
template THaVar::THaVar( const char*, const char*, UChar_t**&, const Int_t* );

template THaVar::THaVar( const char*, const char*, vector<int>&, const Int_t* );
template THaVar::THaVar( const char*, const char*, vector<unsigned int>&, const Int_t* );
template THaVar::THaVar( const char*, const char*, vector<double>&, const Int_t* );
template THaVar::THaVar( const char*, const char*, vector<float>&, const Int_t* );

//_____________________________________________________________________________
ClassImp(THaVar)
