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
    fValueP = 0;
    return;
  }
}

//_____________________________________________________________________________
Int_t VectorVar::GetLen() const
{
  // Get number of elements of the variable

  assert( fValueP && IsVector() );

  // To be safe, cast the pointer to the exact type of vector
  switch( fType ) {
  case kIntV: {
    const vector<int>& vec = *static_cast< const vector<int>* >(fValueP);
    return vec.size();
  }
  case kUIntV: {
    const vector<unsigned int>& vec =	*static_cast< const vector<unsigned int>* >(fValueP);
    return vec.size();
  }
  case kFloatV: {
    const vector<float>& vec = *static_cast< const vector<float>* >(fValueP);
    return vec.size();
  }
  case kDoubleV: {
    const vector<double>& vec = *static_cast< const vector<double>* >(fValueP);
    return vec.size();
  }
  //TODO: support matrix types
  default:
    assert(false); // should never happen, object misconstructed
    break;
  }
  return kInvalidInt;
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
    return 0;

  if( i<0 || i>=GetLen() ) {
    fSelf->Error(here, "Index out of range, variable %s, index %d", GetName(), i );
    return 0;
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
    return 0;
  }

  return 0;
}

//_____________________________________________________________________________
Bool_t VectorVar::HasSameSize( const Variable& ) const
{
  // Compare the size counter of this variable to that of 'rhs'.
  // As currently implemented, std::vector variables are always different
  // since their sizes are in principle independent

  return kFALSE;
}

//_____________________________________________________________________________
Bool_t VectorVar::IsBasic() const
{
  // Data are basic (POD variable or array)

  return kFALSE;
}

//_____________________________________________________________________________
Bool_t VectorVar::IsContiguous() const
{
  // Data are contiguous in memory

  return kTRUE;
}

//_____________________________________________________________________________
Bool_t VectorVar::IsPointerArray() const
{
  // Data are an array of pointers to data

  //TODO: support matrix types
  return kFALSE;
}

//_____________________________________________________________________________
Bool_t VectorVar::IsStreamable() const
{
  // Variable refers to an object that can be streamed via ROOT I/O

  return kTRUE;
}

//_____________________________________________________________________________
Bool_t VectorVar::IsVarArray() const
{
  // Variable is a variable-size array

  return kTRUE;
}

//_____________________________________________________________________________

}// namespace Podd
