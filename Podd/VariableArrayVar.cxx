//*-- Author :    Ole Hansen   15/2/2016

//////////////////////////////////////////////////////////////////////////
//
// VariableArrayVar
//
// A "global variable" referencing a variable size array of basic data.
//
//////////////////////////////////////////////////////////////////////////

#include "VariableArrayVar.h"
#include "THaVar.h"
#include <iostream>
#include <cstring>
#include <typeinfo>
#include <cassert>

using namespace std;

namespace Podd {

//_____________________________________________________________________________
VariableArrayVar::VariableArrayVar( THaVar* pvar, const void* addr,
				    VarType type, const Int_t* count )
  : Variable(pvar,addr,type), fCount(count)
{
  // Constructor

  assert( fCount );

  if( !VerifyNonArrayName(GetName()) ) {
    fValueP = 0;
    return;
  }
}

//_____________________________________________________________________________
Int_t VariableArrayVar::GetLen() const
{
  // Get number of elements of the variable

  if( !fCount )
    return 0;
  return *fCount;
}

//_____________________________________________________________________________
Int_t VariableArrayVar::GetNdim() const
{
  // Number of dimensions

  return 1;
}

//_____________________________________________________________________________
const Int_t* VariableArrayVar::GetDim() const
{
  // Return array of dimensions of the array. Scalers are always return a
  // pointer to 1 (as with array definition [1]).

  return fCount;
}

//_____________________________________________________________________________
Bool_t VariableArrayVar::HasSameSize( const Variable& rhs ) const
{
  // Compare the size counter of this variable to that of 'rhs'.

  if( typeid(*this) != typeid(rhs) )
    return kFALSE;

  const VariableArrayVar* other = dynamic_cast<const VariableArrayVar*>(&rhs);
  assert( other );
  if( !other )
    return kFALSE;

  return fCount == other->fCount;
}

//_____________________________________________________________________________
Bool_t VariableArrayVar::HasSizeVar() const
{
  return kTRUE;
}

//_____________________________________________________________________________
Bool_t VariableArrayVar::IsContiguous() const
{
  // Data are contiguous in memory

  return !IsPointerArray();
}

//_____________________________________________________________________________
Bool_t VariableArrayVar::IsPointerArray() const
{
  // Data are an array of pointers to data

  return fType>=kDouble2P && fType <= kObject2P;
}

//_____________________________________________________________________________
Bool_t VariableArrayVar::IsVarArray() const
{
  // Variable is a variable-size array

  return kTRUE;
}

//_____________________________________________________________________________

}// namespace Podd
