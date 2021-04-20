//*-- Author :    Ole Hansen   15/2/2016

//////////////////////////////////////////////////////////////////////////
//
// VectorVar
//
// A "global variable" referencing a std::vector of basic data
//
//////////////////////////////////////////////////////////////////////////

#include "VectorVar.h"
#include "THaVar.h"
#include "TError.h"
#include <vector>
#include <cassert>
#include <stdexcept>

#define kInvalid     THaVar::kInvalid
#define kInvalidInt  THaVar::kInvalidInt

using namespace std;

namespace Podd {

//_____________________________________________________________________________
VectorVar::VectorVar( THaVar* pvar, const void* addr, VarType type )
  : Variable(pvar,addr,type), fDim(0)
{
  // Constructor

  if( !VerifyNonArrayName(GetName()) ) {
    fValueP = nullptr;
    return;
  }
}

//_____________________________________________________________________________
Int_t VectorVar::GetLen() const
{
  // Get number of elements of the variable

  assert( fValueP && IsVector() );

  // To be safe, cast the pointer to the exact type of vector
  vector<int>::size_type siz = kInvalidInt;
  switch( fType ) {
  case kIntV: {
    const vector<int>& vec = *static_cast< const vector<int>* >(fValueP);
    siz = vec.size();
    break;
  }
  case kUIntV: {
    const vector<unsigned int>& vec = *static_cast< const vector<unsigned int>* >(fValueP);
    siz = vec.size();
    break;
  }
  case kFloatV: {
    const vector<float>& vec = *static_cast< const vector<float>* >(fValueP);
    siz = vec.size();
    break;
  }
  case kDoubleV: {
    const vector<double>& vec = *static_cast< const vector<double>* >(fValueP);
    siz = vec.size();
    break;
  }
  //TODO: support matrix types
  default:
    assert(false); // should never happen, object ill-constructed
    break;
  }
  if( siz > kMaxInt )
    throw overflow_error("Podd::VectorVar array size overflow");
  return static_cast<Int_t>(siz);
}

//_____________________________________________________________________________
Int_t VectorVar::GetNdim() const
{
  // Number of dimensions

  return 1;
}

//_____________________________________________________________________________
const Int_t* VectorVar::GetDim() const
{
  // Return array of dimensions of the array

  fDim = GetLen();
  return &fDim;
}

//_____________________________________________________________________________
const void* VectorVar::GetDataPointer( Int_t i ) const
{
  // Get pointer to data in memory, including to data in a non-contiguous
  // pointer array and data in objects stored in a TSeqCollection (TClonesArray)

  const char* const here = "GetDataPointer()";

  assert( fValueP && IsVector() );

  if( GetLen() == 0 )
    return nullptr;

  if( i<0 || i>=GetLen() ) {
    fSelf->Error(here, "Index out of range, variable %s, index %d", GetName(), i );
    return nullptr;
  }

  switch( fType ) {
  case kIntV: {
    const vector<int>& vec = *static_cast<const vector<int>* >(fValueP);
    return &vec[i];
  }
  case kUIntV: {
    const vector<unsigned int>& vec = *static_cast<const vector<unsigned int>* >(fValueP);
    return &vec[i];
  }
  case kFloatV: {
    const vector<float>& vec = *static_cast<const vector<float>* >(fValueP);
    return &vec[i];
  }
  case kDoubleV: {
    const vector<double>& vec = *static_cast<const vector<double>* >(fValueP);
    return &vec[i];
  }
  //TODO: support matrix types?
  default:
    break;
  }

  return nullptr;
}

//_____________________________________________________________________________
Bool_t VectorVar::HasSameSize( const Variable& ) const
{
  // Compare the size counter of this variable to that of 'rhs'.
  // As currently implemented, std::vector variables are always different
  // since their sizes are in principle independent

  return false;
}

//_____________________________________________________________________________
Bool_t VectorVar::IsBasic() const
{
  // Data are basic (POD variable or array)

  return false;
}

//_____________________________________________________________________________
Bool_t VectorVar::IsContiguous() const
{
  // Data are contiguous in memory

  return true;
}

//_____________________________________________________________________________
Bool_t VectorVar::IsPointerArray() const
{
  // Data are an array of pointers to data

  //TODO: support matrix types
  return false;
}

//_____________________________________________________________________________
Bool_t VectorVar::IsStreamable() const
{
  // Variable refers to an object that can be streamed via ROOT I/O

  return true;
}

//_____________________________________________________________________________
Bool_t VectorVar::IsVarArray() const
{
  // Variable is a variable-size array

  return true;
}

//_____________________________________________________________________________

}// namespace Podd
