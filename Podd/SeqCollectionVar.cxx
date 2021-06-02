//*-- Author :    Ole Hansen   17/2/2016

//////////////////////////////////////////////////////////////////////////
//
// SeqCollectionVar
//
// A "global variable" referencing data in objects in a TSeqCollection
//
//////////////////////////////////////////////////////////////////////////

#include "SeqCollectionVar.h"
#include "THaVar.h"
#include "TError.h"
#include "TClass.h"
#include "TObjArray.h"
#include <cassert>

#define kInvalid     THaVar::kInvalid
#define kInvalidInt  THaVar::kInvalidInt

using namespace std;

namespace Podd {

//_____________________________________________________________________________
SeqCollectionVar::SeqCollectionVar( THaVar* pvar, const void* addr,
				    VarType type, Int_t offset )
  : Variable(pvar,addr,type), fOffset(offset), fDim(0)
{
  // Constructor
  assert( offset >= 0 );

  if( !VerifyNonArrayName(GetName()) ) {
    fValueP = nullptr;
    return;
  }
  // Currently supported data types
  if( !(fType >= kDouble && fType <= kUChar) &&
      !(fType >= kDoubleP && fType <= kUCharP) ) {
    Error( "SeqCollectionVar::SeqCollectionVar", "Variable %s: "
	   "Illegal data type = %s. Only basic types or pointers to basic "
	   "types allowed", pvar->GetName(), Vars::GetTypeName(fType) );
    fValueP = nullptr; // Make invalid
    return;
  }
}

//_____________________________________________________________________________
Int_t SeqCollectionVar::GetLen() const
{
  // Get number of elements of the variable

  assert( fValueP );

  const auto *const obj = static_cast<const TObject*>( fValueP );
  if( !obj || !obj->IsA()->InheritsFrom( TSeqCollection::Class() ))
    return kInvalidInt;

  const auto *const c = static_cast<const TSeqCollection*>( obj );

  // Get actual array size
  if( c->IsA()->InheritsFrom( TObjArray::Class() ))
    // TObjArray is a special TSeqCollection for which GetSize() reports the
    // current capacity, not number of elements, so we need to use GetLast():
    return static_cast<const TObjArray*>(c)->GetLast()+1;

  return c->GetSize();
}

//_____________________________________________________________________________
Int_t SeqCollectionVar::GetNdim() const
{
  // Number of dimensions

  return 1;
}

//_____________________________________________________________________________
const Int_t* SeqCollectionVar::GetDim() const
{
  // Return array of dimensions of the array

  fDim = GetLen();
  return &fDim;
}

//_____________________________________________________________________________
const void* SeqCollectionVar::GetDataPointer( Int_t i ) const
{
  // Get pointer to data in objects stored in a TSeqCollection (TClonesArray)

  const char* const here = "GetDataPointer()";

  static_assert( sizeof(ULong_t) == sizeof(void*) , "ULong_t must of of pointer size");
  assert( fValueP );

  Int_t len = GetLen();
  if( len == 0 || len == kInvalidInt )
    return nullptr;

  if( i<0 || i>=len ) {
    fSelf->Error( here, "Index out of range, variable %s, index %d",
		  GetName(), i );
    return nullptr;
  }

  void* obj = static_cast<const TSeqCollection*>(fValueP)->At(i);
  if( !obj ) {
    fSelf->Error( here, "Variable %s: Collection element %d does not exist. "
		 "Check detector code for bugs.", GetName(), i );
    return nullptr;
  }

  // Compute location using the offset.
  if( fOffset > 0 ) {
    ULong_t loc = reinterpret_cast<ULong_t>(obj) + fOffset;
    assert( loc ); // obj != 0 already assured
    if( fType >= kDoubleP && fType <= kUCharP )
      obj = *reinterpret_cast<void**>(loc);
    else
      obj = reinterpret_cast<void*>(loc);
  }
  return obj;
}

//_____________________________________________________________________________
Bool_t SeqCollectionVar::HasSameSize( const Variable& rhs ) const
{
  // Compare the size counter of this variable to that of 'rhs'.
  // Trivially, TSeqCollection variables have the same size if they
  // belong to the same TSeqCollection

  const auto *const other = dynamic_cast<const SeqCollectionVar*>(&rhs);
  if( !other )
    return false;

  return fValueP == other->fValueP;
}

//_____________________________________________________________________________
Bool_t SeqCollectionVar::IsBasic() const
{
  // Data are basic (POD variable or array)

  return false;
}

//_____________________________________________________________________________
Bool_t SeqCollectionVar::IsContiguous() const
{
  // Data are contiguous in memory

  return false;
}

//_____________________________________________________________________________
Bool_t SeqCollectionVar::IsPointerArray() const
{
  // Data are an array of pointers to data

  return false;
}

//_____________________________________________________________________________
Bool_t SeqCollectionVar::IsVarArray() const
{
  // Variable is a variable-size array

  return true;
}

//_____________________________________________________________________________

}// namespace Podd
