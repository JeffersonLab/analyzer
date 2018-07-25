//*-- Author :    Ole Hansen   14/2/2018

//////////////////////////////////////////////////////////////////////////
//
// VectorObjVar
//
// A "global variable" referencing data in objects held in a std::vector
//
//////////////////////////////////////////////////////////////////////////

#include "VectorObjVar.h"
#include "THaVar.h"
#include "TError.h"
#include <cassert>
#include <vector>

#define kInvalid     THaVar::kInvalid
#define kInvalidInt  THaVar::kInvalidInt

using namespace std;

typedef vector<void*> VecType;

namespace Podd {

//_____________________________________________________________________________
VectorObjVar::VectorObjVar( THaVar* pvar, const void* addr, VarType type,
			    Int_t elem_size, Int_t offset )
  : Variable(pvar,addr,type), SeqCollectionVar(pvar,addr,type,offset),
    fElemSize(elem_size)
{
  // Constructor
  assert( fElemSize >= 0 );
}

//_____________________________________________________________________________
Int_t VectorObjVar::GetLen() const
{
  // Get number of elements of the variable

  assert( fValueP );

  const VecType* pvec = reinterpret_cast<const VecType*>( fValueP );
  if( !pvec )
    return kInvalidInt;

  VecType::size_type n = pvec->size();
  if( n > static_cast<VecType::size_type>(kMaxInt) )
    return kInvalidInt;
  Int_t len = static_cast<Int_t>(n);
#ifndef STDVECTOR_SIZE_INDEPENDENT_OF_TYPE
  // If the vector contains the objects themselves, may need to correct for the
  // actual object size (could be implementation-dependent)
  if( fElemSize > 0 ) {
    assert( len*sizeof(VecType::value_type) % fElemSize == 0 );
    len = (len*sizeof(VecType::value_type))/fElemSize;
  }
#endif
  return len;
}

//_____________________________________________________________________________
const void* VectorObjVar::GetDataPointer( Int_t i ) const
{
  // Get pointer to data in objects stored in a std::vector

  const char* const here = "GetDataPointer()";

  assert( sizeof(ULong_t) == sizeof(void*) );
  assert( fValueP );

  Int_t len = GetLen();
  if( len == 0 || len == kInvalidInt )
    return 0;

  if( i<0 || i>=len ) {
    fSelf->Error( here, "Index out of range, variable %s, index %d",
		  GetName(), i );
    return 0;
  }

  const VecType& vec = *reinterpret_cast<const VecType*>(fValueP);
  void* obj;
  if( fElemSize == 0 ) {
    obj = vec[i];
    if( !obj ) {
      fSelf->Error( here, "Null pointer in vector, variable %s, index %d. "
		    "Check detector code for bugs.", GetName(), i );
      return 0;
    }
  } else {
    obj = reinterpret_cast<void*>((ULong_t)&vec[0] + i*fElemSize);
    assert(obj); // else corrupted std::vector
  }

  // Now obj points to the object. Compute data location using the offset, if needed
  if( fOffset > 0 ) {
    ULong_t loc = reinterpret_cast<ULong_t>(obj) + fOffset;
    assert( loc ); // obj != already assured
    if( fType >= kDoubleP && fType <= kUCharP )
      obj = *reinterpret_cast<void**>(loc);
    else
      obj = reinterpret_cast<void*>(loc);
  }
  return obj;
}

//_____________________________________________________________________________
Bool_t VectorObjVar::HasSameSize( const Variable& rhs ) const
{
  // Compare the size counter of this variable to that of 'rhs'.
  // Trivially, VectorObjVar variables have the same size if they
  // belong to the same std::vector

  const VectorObjVar* other = dynamic_cast<const VectorObjVar*>(&rhs);
  if( !other )
    return kFALSE;

  return fValueP == other->fValueP;
}

//_____________________________________________________________________________

}// namespace Podd
