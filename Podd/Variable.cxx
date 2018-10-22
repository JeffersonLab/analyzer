//*-- Author :    Ole Hansen   16/02/2016


//////////////////////////////////////////////////////////////////////////
//
// Podd::Variable
//
//////////////////////////////////////////////////////////////////////////

#include "THaVar.h"
#include "THaArrayString.h"
#include "TError.h"
#include <cassert>
#include <iostream>
#include <cstring>

using namespace std;

#define kInvalid     THaVar::kInvalid
#define kInvalidInt  THaVar::kInvalidInt

namespace Podd {

//_____________________________________________________________________________
Variable::Variable( THaVar* pvar, const void* addr, VarType type )
  : fSelf(pvar), fValueP(addr), fType(type)
{
  // Base class constructor

  assert(fSelf);
  assert(fValueP);
}

//_____________________________________________________________________________
Variable::~Variable()
{
  // Destructor
}

//_____________________________________________________________________________
Bool_t Variable::VerifyNonArrayName( const char* name ) const
{
  // Return true if the variable string is NOT an array

  const char* const here = "Variable::VerifyNonArrayName";

  THaArrayString parsed_name(name);

  if( parsed_name.IsError() || parsed_name.IsArray() ) {
    fSelf->Error( here, "Illegal name for variable \"%s\". "
		  "Brackets and commans not allowed.", name );
    return false;
  }

  return true;
}

//_____________________________________________________________________________
const char* Variable::GetName() const
{
  return fSelf->TNamed::GetName();
}

//_____________________________________________________________________________
size_t Variable::GetTypeSize() const
{
  return THaVar::GetTypeSize( fType );
}

//_____________________________________________________________________________
const char* Variable::GetTypeName() const
{
  return THaVar::GetTypeName( fType );
}

//_____________________________________________________________________________
Int_t Variable::GetLen() const
{
  // Get number of elements of the variable

  return 1;
}

//_____________________________________________________________________________
Int_t Variable::GetNdim() const
{
  // Return number of dimensions of array. Scalars always have 0 dimensions.

  return 0;
}

//_____________________________________________________________________________
const Int_t* Variable::GetDim() const
{
  // Return array of dimensions of the array. Scalers return 0

  return 0;
}

//_____________________________________________________________________________
std::vector<Double_t> Variable::GetValues() const
{
  std::vector<Double_t> res;
  for(int i = 0 ; i < GetLen(); i++) {
    res.push_back(GetValue(i));
  }
  return res;
}
Double_t Variable::GetValue( Int_t i ) const
{
  // Retrieve current value of this global variable as double.
  // If the variable is an array/vector, return its i-th element.

  const char* const here = "GetValue()";

  if( i<0 || i>=GetLen() ) {
    fSelf->Error(here, "Index out of range, variable %s, index %d", GetName(), i );
    return kInvalid;
  }

  const void* loc = GetDataPointer(i);
  if( !loc )
    return kInvalid;

  switch( fType ) {
  case kDouble:
  case kDoubleP:
  case kDouble2P:
  case kDoubleV:
  case kDoubleM:
    return *static_cast<const Double_t*>(loc);
  case kFloat:
  case kFloatP:
  case kFloat2P:
  case kFloatV:
  case kFloatM:
    return *static_cast<const Float_t*>(loc);
  case kLong:
  case kLongP:
  case kLong2P:
    return *static_cast<const Long64_t*>(loc);
  case kULong:
  case kULongP:
  case kULong2P:
    return *static_cast<const ULong64_t*>(loc);
  case kInt:
  case kIntP:
  case kInt2P:
  case kIntV:
  case kIntM:
    return *static_cast<const Int_t*>(loc);
  case kUInt:
  case kUIntP:
  case kUInt2P:
  case kUIntV:
    return *static_cast<const UInt_t*>(loc);
  case kShort:
  case kShortP:
  case kShort2P:
    return *static_cast<const Short_t*>(loc);
  case kUShort:
  case kUShortP:
  case kUShort2P:
    return *static_cast<const UShort_t*>(loc);
  case kChar:
  case kCharP:
  case kChar2P:
    return *static_cast<const Char_t*>(loc);
  case kUChar:
  case kUCharP:
  case kUChar2P:
    return *static_cast<const UChar_t*>(loc);
  case kObject:
  case kObjectP:
  case kObject2P:
    fSelf->Error( here, "Cannot get value from composite object, variable %s",
		  GetName() );
    break;
  default:
    fSelf->Error( here, "Unsupported data type %s, variable %s",
		  THaVar::GetEnumName(fType), GetName() );
    break;
  }
  return kInvalid;
}

//_____________________________________________________________________________
Long64_t Variable::GetValueInt( Int_t i ) const
{
  // Retrieve current value of this global variable as integer.
  // If the variable is an array/vector, return its i-th element.

  const char* const here = "GetValueInt()";

  if( i<0 || i>=GetLen() ) {
    fSelf->Error(here, "Index out of range, variable %s, index %d", GetName(), i );
    return kInvalidInt;
  }

  if( IsFloat() ) {
    fSelf->Error(here, "Variable %s has floating-point type %s, "
		 "call GetValue() instead", GetName(), GetTypeName() );
    return kInvalidInt;
  }

  const void* loc = GetDataPointer(i);
  if( !loc )
    return kInvalidInt;

  switch( fType ) {
  case kLong:
  case kLongP:
  case kLong2P:
    return *static_cast<const Long64_t*>(loc);
  case kULong:
  case kULongP:
  case kULong2P:
    return *static_cast<const ULong64_t*>(loc);
  case kInt:
  case kIntP:
  case kInt2P:
  case kIntV:
  case kIntM:
    return *static_cast<const Int_t*>(loc);
  case kUInt:
  case kUIntP:
  case kUInt2P:
  case kUIntV:
    return *static_cast<const UInt_t*>(loc);
  case kShort:
  case kShortP:
  case kShort2P:
    return *static_cast<const Short_t*>(loc);
  case kUShort:
  case kUShortP:
  case kUShort2P:
    return *static_cast<const UShort_t*>(loc);
  case kChar:
  case kCharP:
  case kChar2P:
    return *static_cast<const Char_t*>(loc);
  case kUChar:
  case kUCharP:
  case kUChar2P:
    return *static_cast<const UChar_t*>(loc);
  case kObject:
  case kObjectP:
  case kObject2P:
    fSelf->Error( here, "Cannot get value from composite object, variable %s",
		  GetName() );
    break;
  default:
    fSelf->Error( here, "Unsupported data type %s, variable %s",
		  THaVar::GetEnumName(fType), GetName() );
    break;
  }

  return kInvalidInt;
}

//_____________________________________________________________________________
const void* Variable::GetDataPointer( Int_t i ) const
{
  // Get pointer to i-th element of the data in memory, including to data in
  // a non-contiguous pointer array

  const char* const here = "GetDataPointer()";

  assert( fValueP && IsBasic() );
  assert( sizeof(ULong_t) == sizeof(void*) );

  if( i<0 || i>=GetLen() ) {
    fSelf->Error(here, "Index out of range, variable %s, index %d", GetName(), i );
    return 0;
  }

  if( fType >= kDouble && fType <= kUChar ) {
    ULong_t loc = reinterpret_cast<ULong_t>(fValueP) + i*GetTypeSize();
    return reinterpret_cast<const void*>(loc);
  }

  if( fType >= kDoubleP && fType <= kUCharP ) {
    const void* ptr = *reinterpret_cast<void* const *>(fValueP);
    ULong_t loc = reinterpret_cast<ULong_t>(ptr) + i*GetTypeSize();
    return reinterpret_cast<const void*>(loc);
  }

  if( fType >= kDouble2P && fType <= kUChar2P ) {
    const void** const *ptr = reinterpret_cast<const void** const *>(fValueP);
    if( !*ptr )
      return 0;
    return (*ptr)[i];
  }

  return 0;
}

//_____________________________________________________________________________
size_t Variable::GetData( void* buf ) const
{
  // Copy the data into the provided buffer. The buffer must have been allocated
  // by the caller and must be able to hold at least GetLen()*GetTypeSize() bytes.
  // The return value is the number of bytes actually copied.

  Int_t nelem = GetLen();
  size_t type_size = GetTypeSize();
  size_t nbytes = nelem * type_size;
  if( nbytes == 0 || !fValueP )
    return 0;

  if( IsContiguous() ) {
    // Contiguous data can be copied in one fell swoop
    const void* src = GetDataPointer();
    if( !src )
      return 0;
    memcpy( buf, src, nbytes );
  }
  else {
    // Non-contiguous data (e.g. pointer array) must be copied element by element
    assert( sizeof(char) == 1 );
    nbytes = 0;
    for( Int_t i = 0; i<nelem; ++i ) {
      const void* src = GetDataPointer(i);
      if( !src )
	return nbytes;
      memcpy( static_cast<char*>(buf)+nbytes, src, type_size );
      nbytes += type_size;
    }
  }

  return nbytes;
}

//_____________________________________________________________________________
size_t Variable::GetData( void* buf, Int_t i ) const
{
  // Copy the data from the i-th element into the provided buffer. The buffer
  // must have been allocated by the caller and must be able to hold at least
  // GetTypeSize() bytes.
  // The return value is the number of bytes actually copied.

  const char* const here = "GetData()";

  size_t nbytes = GetTypeSize();
  if( nbytes == 0 || !fValueP )
    return 0;

  if( i<0 || i>=GetLen() ) {
    fSelf->Error(here, "Index out of range, variable %s, index %d", GetName(), i );
    return 0;
  }

  const void* src = GetDataPointer(i);
  if( !src )
    return 0;

  memcpy( buf, src, nbytes );

  return nbytes;
}

//_____________________________________________________________________________
Bool_t Variable::HasSameSize( const Variable& rhs ) const
{
  // Compare the size (=number of elements) of this variable to that of 'rhs'.
  // This is the basic version; derived classes will likely override

  Bool_t is_array = IsArray();
  if( is_array != rhs.IsArray())         // Must be same array/non-array
    return kFALSE;
  if( !is_array )                        // Scalars always agree
    return kTRUE;
  //FIXME: this isn't correct
  return (GetLen() == rhs.GetLen());     // Arrays must have same length
}

//_____________________________________________________________________________
Bool_t Variable::HasSizeVar() const
{
  return kFALSE;
}

//_____________________________________________________________________________
Int_t Variable::Index( const THaArrayString& elem ) const
{
  // Return linear index into this array variable corresponding
  // to the array element described by 'elem'.
  // Return -1 if subscript(s) out of bound(s) or -2 if incompatible arrays.

  if( elem.IsError() ) return -1;
  if( !elem.IsArray() ) return 0;

  Int_t ndim = GetNdim();
  if( ndim != elem.GetNdim() ) return -2;

  const Int_t *adim = GetDim();

  Int_t index = elem[0];
  for( Int_t i = 0; i<ndim; i++ ) {
    if( elem[i] >= adim[i] ) return -1;
    if( i>0 )
      index = index*adim[i] + elem[i];
  }
  if( index >= GetLen() ) return -1;
  return index;
}

//_____________________________________________________________________________
Bool_t Variable::IsArray() const
{
  // Data are an array (GetLen() may be != 1)

  return (GetNdim() > 0);
}

//_____________________________________________________________________________
Bool_t Variable::IsBasic() const
{
  // Data are basic (POD variable or array)

  return kTRUE;
}

//_____________________________________________________________________________
Bool_t Variable::IsContiguous() const
{
  // Data are contiguous in memory

  return kTRUE;
}

//_____________________________________________________________________________
Bool_t Variable::IsError() const
{
  // Variable is in error state/invalid (typically because error in constructor)

  return fValueP == 0;
}

//_____________________________________________________________________________
Bool_t Variable::IsFloat() const
{
  // Data are floating-point type

  return ( fType == kDouble   || fType == kFloat   ||
	   fType == kDoubleP  || fType == kFloatP  ||
	   fType == kDoubleV  || fType == kDoubleM ||
	   fType == kFloatV   || fType == kFloatM  ||
	   fType == kDouble2P || fType == kFloat2P );
}

//_____________________________________________________________________________
Bool_t Variable::IsPointerArray() const
{
  // Data are an array of pointers to data

  return kFALSE;
}

//_____________________________________________________________________________
Bool_t Variable::IsStreamable() const
{
  // Variable refers to an object that can be streamed via ROOT I/O

  return IsTObject();
}

//_____________________________________________________________________________
Bool_t Variable::IsTObject() const
{
  // Variable refers to an object that inherits from TObject

  return kFALSE;
}

//_____________________________________________________________________________
Bool_t Variable::IsVarArray() const
{
  // Variable is a variable-sized array

  return kFALSE;
}

//_____________________________________________________________________________
Bool_t Variable::IsVector() const
{
  // Variable is a std::vector or std::vector< std::vector >

  //TODO: support matrix types
  //  return fType >= kIntV && fType <= kDoubleM;
  return fType >= kIntV && fType <= kDoubleV;
}

//_____________________________________________________________________________
void Variable::Print(Option_t* option) const
{
  // Print a description of this variable. If option=="FULL" (default), also
  // print current data

  fSelf->TNamed::Print(option);

  if( strcmp(option, "FULL") ) return;

  cout << "(" << fSelf->GetTypeName() << ")  ";
  size_t len = GetLen();
  if( IsArray() )
    cout << "[" << len << "]";

  for( size_t i=0; i<len; i++ ) {
    cout << "  ";
    if( IsFloat() )
      cout << GetValue(i);
    else
      cout << GetValueInt(i);
  }
  cout << endl;

}

//_____________________________________________________________________________
void Variable::SetName( const char* name )
{
  // Set the name of the variable

  if( !VerifyNonArrayName(name) )
    return;

  fSelf->TNamed::SetName( name );
}

//_____________________________________________________________________________
void Variable::SetNameTitle( const char* name, const char* descript )
{
  // Set name and description of the variable

  if( !VerifyNonArrayName(name) )
    return;

  fSelf->TNamed::SetNameTitle( name, descript );
}

//_____________________________________________________________________________

}  // namespace Podd
