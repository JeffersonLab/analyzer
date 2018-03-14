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
#include <cassert>
#include <typeinfo>
#include <string>
#include <map>

using namespace std;

const Long64_t THaVar::kInvalidInt = kMaxLong64;
const Double_t THaVar::kInvalid    = 1e38;

static const char* const here = "THaVar";

// NB: Must match definition order in VarDef.h
static struct VarTypeInfo_t {
  VarType      type;
  const char*  enum_name;  // name of enumeration constant to use for this type
  const char*  cpp_name;   // C++ type as understood by compiler
  size_t       size;       // size of underlying (innermost) data elements
  const type_info* pinfo;  // Pointer to static std::type_info for this type
}
  var_type_info[] = {
    { kDouble,   "kDouble",   "Double_t",    sizeof(Double_t),  &typeid(Double_t) },
    { kFloat,    "kFloat",    "Float_t",     sizeof(Float_t),   &typeid(Float_t) },
    { kLong,     "kLong",     "Long64_t",    sizeof(Long64_t),  &typeid(Long64_t) },
    { kULong,    "kULong",    "ULong64_t",   sizeof(ULong64_t), &typeid(ULong64_t) },
    { kInt,      "kInt",      "Int_t",       sizeof(Int_t),     &typeid(Int_t) },
    { kUInt,     "kUInt",     "UInt_t",      sizeof(UInt_t),    &typeid(UInt_t) },
    { kShort,    "kShort",    "Short_t",     sizeof(Short_t),   &typeid(Short_t) },
    { kUShort,   "kUShort",   "UShort_t",    sizeof(UShort_t),  &typeid(UShort_t) },
    { kChar,     "kChar",     "Char_t",      sizeof(Char_t),    &typeid(Char_t) },
    { kUChar,    "kUChar",    "UChar_t",     sizeof(UChar_t),   &typeid(UChar_t) },
    { kObject,   "kObject",   "TObject",     0,                 &typeid(TObject) },
    { kTString,  "kTString",  "TString",     sizeof(char),      &typeid(TString) },
    { kString,   "kString",   "string",      sizeof(char),      &typeid(std::string) },
    { kIntV,     "kIntV",     "vector<int>",              sizeof(int),         &typeid(std::vector<int>) },
    { kUIntV,    "kUIntV",    "vector<unsigned int>",     sizeof(unsigned int),&typeid(std::vector<unsigned int>) },
    { kFloatV,   "kFloatV",   "vector<float>",            sizeof(float),       &typeid(std::vector<float>) },
    { kDoubleV,  "kDoubleV",  "vector<double>",           sizeof(double),      &typeid(std::vector<double>) },
    { kObjectV,  "kObjectV",  "vector<TObject>",          0,    &typeid(std::vector<TObject>) },
    { kObjectPV, "kObjectPV", "vector<TObject*>",         0,    &typeid(std::vector<TObject*>) },
    { kIntM,     "kIntM",     "vector< vector<int> >",    sizeof(int),         &typeid(std::vector<std::vector<int> >) },
    { kFloatM,   "kFloatM",   "vector< vector<float> >",  sizeof(float),       &typeid(std::vector<std::vector<float> >) },
    { kDoubleM,  "kDoubleM",  "vector< vector<double> >", sizeof(double),      &typeid(std::vector<std::vector<double> >) },
    { kDoubleP,  "kDoubleP",  "Double_t*",   sizeof(Double_t),  &typeid(Double_t*) },
    { kFloatP,   "kFloatP",   "Float_t*",    sizeof(Float_t),   &typeid(Float_t*) },
    { kLongP,    "kLongP",    "Long64_t*",   sizeof(Long64_t),  &typeid(Long64_t*) },
    { kULongP,   "kULongP",   "ULong64_t*",  sizeof(ULong64_t), &typeid(ULong64_t*) },
    { kIntP,     "kIntP",     "Int_t*",      sizeof(Int_t),     &typeid(Int_t*) },
    { kUIntP,    "kUIntP",    "UInt_t*",     sizeof(UInt_t),    &typeid(UInt_t*) },
    { kShortP,   "kShortP",   "Short_t*",    sizeof(Short_t),   &typeid(Short_t*) },
    { kUShortP,  "kUShortP",  "UShort_t*",   sizeof(UShort_t),  &typeid(UShort_t*) },
    { kCharP,    "kCharP",    "Char_t*",     sizeof(Char_t),    &typeid(Char_t*) },
    { kUCharP,   "kUCharP",   "UChar_t*",    sizeof(UChar_t),   &typeid(UChar_t*) },
    { kObjectP,  "kObjectP",  "TObject*",    0,                 &typeid(TObject*) },
    { kDouble2P, "kDouble2P", "Double_t**",  sizeof(Double_t),  &typeid(Double_t**) },
    { kFloat2P,  "kFloat2P",  "Float_t**",   sizeof(Float_t),   &typeid(Float_t**) },
    { kLong2P,   "kLong2P",   "Long64_t**",  sizeof(Long64_t),  &typeid(Long64_t**) },
    { kULong2P,  "kULong2P",  "ULong64_t**", sizeof(ULong64_t), &typeid(ULong64_t**) },
    { kInt2P,    "kInt2P",    "Int_t**",     sizeof(Int_t),     &typeid(Int_t**) },
    { kUInt2P,   "kUInt2P",   "UInt_t**",    sizeof(UInt_t),    &typeid(UInt_t**) },
    { kShort2P,  "kShort2P",  "Short_t**",   sizeof(Short_t),   &typeid(Short_t**) },
    { kUShort2P, "kUShort2P", "UShort_t**",  sizeof(UShort_t),  &typeid(UShort_t**) },
    { kChar2P,   "kChar2P",   "Char_t**",    sizeof(Char_t),    &typeid(Char_t**) },
    { kUChar2P,  "kUChar2P",  "UChar_t**",   sizeof(UChar_t),   &typeid(UChar_t**) },
    { kObject2P, "kObject2P", "TObject**",   0,                 &typeid(TObject**)}
  };

// Static access function into var_type_info[] table above
//_____________________________________________________________________________
const char* THaVar::GetEnumName( VarType itype )
{
  // Return enumeration variable name of the given VarType

  assert( (size_t)itype < sizeof(var_type_info)/sizeof(VarTypeInfo_t) );
  assert( itype == var_type_info[itype].type );
  return var_type_info[itype].enum_name;
}

//_____________________________________________________________________________
const char* THaVar::GetTypeName( VarType itype )
{
  // Return C++ name of the given VarType

  assert( (size_t)itype < sizeof(var_type_info)/sizeof(VarTypeInfo_t) );
  assert( itype == var_type_info[itype].type );
  return var_type_info[itype].cpp_name;
}

//_____________________________________________________________________________
size_t THaVar::GetTypeSize( VarType itype )
{
  // Return size of the underlying (innermost) basic data type of each VarType.
  // Returns 0 for object types.

  assert( (size_t)itype < sizeof(var_type_info)/sizeof(VarTypeInfo_t) );
  assert( itype == var_type_info[itype].type );
  return var_type_info[itype].size;
}

// Lookup map for use in constructors.
// Allows fast lookup of VarType from type_info.
//_____________________________________________________________________________
struct ByTypeInfo
{
  bool operator() ( const type_info* a, const type_info* b ) const {
    assert(b);
    return a->before(*b);
  }
};

typedef map< const type_info*, VarType, ByTypeInfo > VarTypeMap_t;
static VarTypeMap_t var_type_map;

//_____________________________________________________________________________
static VarTypeMap_t& GetVarTypeMap()
{
  // Get reference to type_info cache map. Initializes map on first use.

  if( var_type_map.empty() ) {
    for( size_t i=0; i<sizeof(var_type_info)/sizeof(VarTypeInfo_t); ++i ) {
      var_type_map[ var_type_info[i].pinfo ] = var_type_info[i].type;
    }
  }
  return var_type_map;
}

//_____________________________________________________________________________
void THaVar::ClearCache()
{
  // Clear map used for caching type_info for faster constructor calls.
  // Calling this once after Analyzer::Init() will save a few kB of memory.

  var_type_map.clear();
}

//_____________________________________________________________________________
inline
static VarType FindType( const type_info& tinfo )
{
  VarTypeMap_t& type_map = GetVarTypeMap();
  VarTypeMap_t::const_iterator found = type_map.find( &tinfo );

  return ( found != type_map.end() ) ? found->second : kVarTypeEnd;
}

//_____________________________________________________________________________
template <typename T>
THaVar::THaVar( const char* name, const char* descript, T& var,
		const Int_t* count )
  : TNamed(name,descript), fImpl(0)
{
  // Constructor for basic types and fixed and variable-size arrays

  VarType type = FindType( typeid(T) );
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
  : TNamed(name,descript), fImpl(0)
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
  : TNamed(name,descript), fImpl(0)
{
  // Generic constructor for std::vector<TObject(*)>
  // (used by THaVarList::DefineByType)

  if( type > kByte ) {
    Error( here, "Variable %s: Illegal type %s for data in std::vector "
	   "of objects", name, GetTypeName(type) );
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
  // THaVar::Index( THaArraySring& ) above because the string has
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
