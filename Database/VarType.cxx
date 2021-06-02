//////////////////////////////////////////////////////////////////////////
//
// VarType
//
// Definitions and utility functions for Podd's "global variable" types
//
//////////////////////////////////////////////////////////////////////////

#include "VarType.h"
#include "TObject.h"
#include "TString.h"
#include "TVirtualMutex.h"
#include <typeinfo>
#include <cassert>
#include <vector>
#include <map>

using namespace std;

namespace Vars {

// Lookup map for use in constructors.
// Allows fast lookup of VarType from type_info.
//_____________________________________________________________________________
class ByTypeInfo {
public:
  bool operator() ( const type_info* a, const type_info* b ) const {
    assert(b);
    return a->before(*b);
  }
};

typedef map< const type_info*, VarType, ByTypeInfo > VarTypeMap_t;
static VarTypeMap_t var_type_map;
static TVirtualMutex* gVarTypeMapMutex = nullptr;

//_____________________________________________________________________________
// NB: Must match definition order in VarType.h
class VarTypeInfo_t {
public:
  VarType      type;
  const char*  enum_name;  // name of enumeration constant to use for this type
  const char*  cpp_name;   // C++ type as understood by compiler
  size_t       size;       // size of underlying (innermost) data elements
  const type_info* pinfo;  // Pointer to static std::type_info for this type
};
static const vector<VarTypeInfo_t> var_type_info = {
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

//_____________________________________________________________________________
static VarTypeMap_t& GetVarTypeMap()
{
  // Get reference to type_info cache map. Initializes map on first use.

  R__LOCKGUARD2(gVarTypeMapMutex);

  if( var_type_map.empty() ) {
    for( const auto& item : var_type_info ) {
      var_type_map[ item.pinfo ] = item.type;
    }
  }
  return var_type_map;
}

//_____________________________________________________________________________
VarType FindType( const type_info& tinfo )
{
  VarTypeMap_t& type_map = GetVarTypeMap();
  auto found = type_map.find( &tinfo );

  return ( found != type_map.end() ) ? found->second : kVarTypeEnd;
}

//_____________________________________________________________________________
void ClearCache()
{
  // Clear map used for caching type_info for faster constructor calls.
  // Calling this once after Analyzer::Init() will save a few kB of memory.

  R__LOCKGUARD2(gVarTypeMapMutex);
  var_type_map.clear();
}

// Access function into var_type_info[] table above
//_____________________________________________________________________________
const char* GetEnumName( VarType itype )
{
  // Return enumeration variable name of the given VarType

  assert( (size_t)itype < var_type_info.size() );
  assert( itype == var_type_info[itype].type );
  return var_type_info[itype].enum_name;
}

//_____________________________________________________________________________
const char* GetTypeName( VarType itype )
{
  // Return C++ name of the given VarType

  assert( (size_t)itype < var_type_info.size() );
  assert( itype == var_type_info[itype].type );
  return var_type_info[itype].cpp_name;
}

//_____________________________________________________________________________
size_t GetTypeSize( VarType itype )
{
  // Return size of the underlying (innermost) basic data type of each VarType.
  // Returns 0 for object types.

  assert( (size_t)itype < var_type_info.size() );
  assert( itype == var_type_info[itype].type );
  return var_type_info[itype].size;
}

} // namespace Vars

